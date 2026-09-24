/*
 * Inference process -- ExecuTorch/NPU call chain exactly as proven in
 * executorch_batch_test/M55_HP/main.cpp (100/100 correct across live
 * hardware runs), now split so the one-time Program::load()/
 * load_method() setup runs once at the top of run() and the per-window
 * get_inputs()/memcpy/execute()/get_outputs() sequence runs in the
 * per-window loop, matching csp4cmsis_neuropathway's Inference::run()
 * shape (one-time model init, then per-frame.read() loop).
 */
#include "inference_process.h"
#include "sensor_process.h" /* sensor_release_buffer() -- ping-pong handoff */

#include "BufAttributes.hpp" /* ACTIVATION_BUF_ATTRIBUTE -> SRAM0, NPU-visible */
#include "har_net_pte.h"

#include <executorch/extension/data_loader/buffer_data_loader.h>
#include <executorch/runtime/core/evalue.h>
#include <executorch/runtime/core/hierarchical_allocator.h>
#include <executorch/runtime/core/memory_allocator.h>
#include <executorch/runtime/executor/method.h>
#include <executorch/runtime/executor/program.h>
#include <executorch/runtime/platform/log.h>
#include <executorch/runtime/platform/platform.h>
#include <executorch/runtime/platform/runtime.h>

#include <cstdio>
#include <cstring>
#include <cinttypes>
#include <vector>

using namespace csp;

using executorch::extension::BufferDataLoader;
using executorch::runtime::Error;
using executorch::runtime::EValue;
using executorch::runtime::HierarchicalAllocator;
using executorch::runtime::MemoryAllocator;
using executorch::runtime::MemoryManager;
using executorch::runtime::Method;
using executorch::runtime::MethodMeta;
using executorch::runtime::Program;
using executorch::runtime::Result;
using executorch::runtime::Span;
using executorch::runtime::TensorInfo;
using executorch::aten::Tensor;

/* Strong override of the "Platform Bare-Metal" component's weak
 * (silent) default -- makes ET_LOG/ET_CHECK_MSG failures actually
 * visible over UART4, same as every prior ExecuTorch stage. */
extern "C" void et_pal_emit_log_message(
    et_timestamp_t timestamp, et_pal_log_level_t level, const char* filename,
    const char* function, size_t line, const char* message, size_t length)
{
    (void)timestamp; (void)length;
    printf("[executorch:%c %s:%u %s()] %s\r\n", (char)level, filename,
           (unsigned)line, function, message);
}

/* Same pool sizes/placement as executorch_batch_test (proven, 100/100
 * correct) -- NOT icm42670_inference_test_mpu_fix's 128KB/32KB, which
 * was never actually required (see executorch_batch_test's own header
 * comment on this). */
#define METHOD_ALLOCATION_POOL_SIZE (32 * 1024)
#define TEMP_ALLOCATION_POOL_SIZE   (8 * 1024)

namespace {
unsigned char method_allocation_pool[METHOD_ALLOCATION_POOL_SIZE]
    ACTIVATION_BUF_ATTRIBUTE;
unsigned char temp_allocation_pool[TEMP_ALLOCATION_POOL_SIZE]
    ACTIVATION_BUF_ATTRIBUTE;

/* Local copy of the window data -- get_inputs()'s returned Tensor is
 * memcpy'd into directly (see below), same pattern
 * executorch_batch_test used per-example; kept as a named intermediate
 * only for the size check against tensor_meta->nbytes(). */
float input_window[NUM_CHANNELS * WINDOW_SAMPLES];
} // namespace

Inference::Inference(Chanin<window_t> in, Chanout<result_t> out)
    : m_window_in(in), m_result_out(out) {}

void Inference::run()
{
    /* --- One-time model/tensor setup, exactly as executorch_batch_test's
     * main() did before its 100-example loop. --- */
    executorch::runtime::runtime_init();

    static BufferDataLoader loader(har_net_pte, har_net_pte_len);
    Result<Program> program_result = Program::load(&loader);
    if (!program_result.ok()) {
        printf("Inference: Program::load failed: 0x%x\r\n",
               (unsigned)program_result.error());
        return;
    }
    Program program = std::move(program_result.get());

    Result<const char*> method_name_result = program.get_method_name(0);
    if (!method_name_result.ok()) {
        printf("Inference: get_method_name failed: 0x%x\r\n",
               (unsigned)method_name_result.error());
        return;
    }
    const char* method_name = *method_name_result;

    Result<MethodMeta> method_meta_result = program.method_meta(method_name);
    if (!method_meta_result.ok()) {
        printf("Inference: method_meta failed: 0x%x\r\n",
               (unsigned)method_meta_result.error());
        return;
    }
    MethodMeta method_meta = method_meta_result.get();

    MemoryAllocator method_allocator(METHOD_ALLOCATION_POOL_SIZE, method_allocation_pool);
    MemoryAllocator temp_allocator(TEMP_ALLOCATION_POOL_SIZE, temp_allocation_pool);

    size_t num_memory_planned_buffers = method_meta.num_memory_planned_buffers();
    std::vector<uint8_t*> planned_buffers;
    std::vector<Span<uint8_t>> planned_spans;
    for (size_t id = 0; id < num_memory_planned_buffers; ++id) {
        size_t buffer_size = static_cast<size_t>(
            method_meta.memory_planned_buffer_size(id).get());
        uint8_t* buffer = reinterpret_cast<uint8_t*>(
            method_allocator.allocate(buffer_size, 16UL));
        if (buffer == nullptr) {
            printf("Inference: Failed to allocate planned buffer %u (%u bytes)\r\n",
                   (unsigned)id, (unsigned)buffer_size);
            return;
        }
        planned_buffers.push_back(buffer);
        planned_spans.push_back({planned_buffers.back(), buffer_size});
    }
    HierarchicalAllocator planned_memory({planned_spans.data(), planned_spans.size()});
    MemoryManager memory_manager(&method_allocator, &planned_memory, &temp_allocator);

    Result<Method> method_result = program.load_method(method_name, &memory_manager, nullptr);
    if (!method_result.ok()) {
        printf("Inference: load_method failed: 0x%x\r\n",
               (unsigned)method_result.error());
        return;
    }
    Method method = std::move(method_result.get());
    printf("Inference: model ready (method_allocation_pool[SRAM0]=%u bytes, "
           "temp_allocation_pool[SRAM0]=%u bytes), waiting for windows\r\n",
           METHOD_ALLOCATION_POOL_SIZE, TEMP_ALLOCATION_POOL_SIZE);

    /* --- Per-window loop -- receives a window_t from Sensor in place of
     * executorch_batch_test's batch_test_vectors[i], everything after
     * that point is the same proven call sequence. --- */
    while (true) {
        window_t w;
        m_window_in.read(w);

        /* DIAGNOSTIC (handoff staleness investigation): signature of
         * what Inference actually received through the channel,
         * computed on w.data BEFORE any copy/quantization -- compare
         * against Sensor's own signature of the same window (printed
         * before the channel write) to localize whether the bug is in
         * the handoff itself or upstream/downstream of it. */
        {
            float sum = 0.0f;
            for (int i = 0; i < NUM_CHANNELS * WINDOW_SAMPLES; i++) {
                sum += w.data[i];
            }
            printf("[Infer   sig w=%4" PRIu32 " buf=%" PRIu32 "] acc_x[0]=%.6f acc_x[127]=%.6f sum=%.6f\r\n",
                   w.index, w.buffer_index, (double)w.data[0], (double)w.data[WINDOW_SAMPLES - 1], (double)sum);
        }

        std::memcpy(input_window, w.data, sizeof(input_window));

        /* Release the ping-pong buffer slot immediately after our own
         * copy out of it -- everything from here on operates on
         * input_window (our private copy), so Sensor is now free to
         * reuse w.buffer_index for a future window. Minimizes how long
         * Sensor might stall waiting for this slot. */
        sensor_release_buffer(w.buffer_index);

        size_t num_inputs = method_meta.num_inputs();
        EValue input_evalues[1];
        Error err = method.get_inputs(input_evalues, num_inputs);
        if (err != Error::Ok) {
            printf("Inference: get_inputs failed: 0x%x (window %" PRIu32 ")\r\n",
                   (unsigned)err, w.index);
            continue;
        }

        Result<TensorInfo> tensor_meta = method_meta.input_tensor_meta(0);
        if (!tensor_meta.ok()) {
            printf("Inference: input_tensor_meta failed: 0x%x\r\n",
                   (unsigned)tensor_meta.error());
            continue;
        }
        if (tensor_meta->nbytes() != sizeof(input_window)) {
            printf("Inference: input size mismatch: tensor wants %zu bytes, "
                   "have %zu\r\n", tensor_meta->nbytes(), sizeof(input_window));
            continue;
        }

        Tensor input_tensor = input_evalues[0].toTensor();
        std::memcpy(input_tensor.mutable_data_ptr<int8_t>(), input_window,
                    sizeof(input_window));

        Error status = method.execute();
        if (status != Error::Ok) {
            printf("Inference: execute() failed: 0x%x (window %" PRIu32 ")\r\n",
                   (unsigned)status, w.index);
            continue;
        }

        std::vector<EValue> outputs(method.outputs_size());
        status = method.get_outputs(outputs.data(), outputs.size());
        if (status != Error::Ok) {
            printf("Inference: get_outputs failed: 0x%x (window %" PRIu32 ")\r\n",
                   (unsigned)status, w.index);
            continue;
        }
        if (!outputs[0].isTensor()) {
            printf("Inference: output[0] is not a tensor (window %" PRIu32 ")\r\n",
                   w.index);
            continue;
        }
        Tensor out = outputs[0].toTensor();
        const float* logits = out.const_data_ptr<float>();
        int numel = out.numel();

        int predicted = 0;
        float best = logits[0];
        for (int i = 1; i < numel; i++) {
            if (logits[i] > best) {
                best = logits[i];
                predicted = i;
            }
        }

        result_t res;
        res.index     = w.index;
        res.predicted = predicted;
        res.logits[0] = logits[0];
        res.logits[1] = (numel > 1) ? logits[1] : 0.0f;

        m_result_out.write(res);
    }
}
