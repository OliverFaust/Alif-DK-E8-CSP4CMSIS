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
 * (silent) default. It runs in Inference's thread (inside ExecuTorch's
 * calls), and Inference does not print: the hook counts the messages and
 * keeps the first one's level and source line, and Inference reports them
 * (RuntimeLog once, then the count with every Result). */
static volatile uint32_t s_et_log_count = 0;
static volatile uint32_t s_et_first_line = 0;
static volatile char     s_et_first_level = 0;

extern "C" void et_pal_emit_log_message(
    et_timestamp_t timestamp, et_pal_log_level_t level, const char* filename,
    const char* function, size_t line, const char* message, size_t length)
{
    (void)timestamp; (void)filename; (void)function; (void)message; (void)length;
    if (s_et_log_count == 0) {
        s_et_first_line  = (uint32_t)line;
        s_et_first_level = (char)level;
    }
    s_et_log_count = s_et_log_count + 1;
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

Inference::Inference(Chanin<window_t> in, Chanout<report_t> report)
    : m_window_in(in), m_report_out(report) {}

void Inference::report(report_t r)
{
    if (!m_log_reported && s_et_log_count > 0) {
        report_t log = {};
        log.kind   = ReportKind::RuntimeLog;
        log.source = ReportSource::Inference;
        log.code   = (int16_t)s_et_first_line;
        log.index  = r.index;
        log.aux    = (uint32_t)s_et_first_level;
        m_report_out.write(log);
        m_log_reported = true;
    }
    r.source = ReportSource::Inference;
    r.count  = s_et_log_count;
    m_report_out.write(r);
}

static report_t failed(int16_t step, uint32_t error)
{
    report_t r = {};
    r.kind = ReportKind::Failed;
    r.code = step;
    r.aux  = error;
    return r;
}

/* Inference does not print: it reports to the Reporter (one Result per
 * window, Started/Failed at start-up). */
void Inference::run()
{
    /* --- One-time model/tensor setup, exactly as executorch_batch_test's
     * main() did before its 100-example loop. --- */
    executorch::runtime::runtime_init();

    static BufferDataLoader loader(har_net_pte, har_net_pte_len);
    Result<Program> program_result = Program::load(&loader);
    if (!program_result.ok()) {
        report(failed(1, (uint32_t)program_result.error()));     /* Program::load */
        return;
    }
    Program program = std::move(program_result.get());

    Result<const char*> method_name_result = program.get_method_name(0);
    if (!method_name_result.ok()) {
        report(failed(2, (uint32_t)method_name_result.error())); /* get_method_name */
        return;
    }
    const char* method_name = *method_name_result;

    Result<MethodMeta> method_meta_result = program.method_meta(method_name);
    if (!method_meta_result.ok()) {
        report(failed(3, (uint32_t)method_meta_result.error())); /* method_meta */
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
            report(failed(4, (uint32_t)buffer_size));               /* planned buffer: bytes */
            return;
        }
        planned_buffers.push_back(buffer);
        planned_spans.push_back({planned_buffers.back(), buffer_size});
    }
    HierarchicalAllocator planned_memory({planned_spans.data(), planned_spans.size()});
    MemoryManager memory_manager(&method_allocator, &planned_memory, &temp_allocator);

    Result<Method> method_result = program.load_method(method_name, &memory_manager, nullptr);
    if (!method_result.ok()) {
        report(failed(5, (uint32_t)method_result.error()));      /* load_method */
        return;
    }
    Method method = std::move(method_result.get());
    {
        report_t r = {};
        r.kind = ReportKind::Started;
        r.aux  = METHOD_ALLOCATION_POOL_SIZE;   /* model ready; method pool in SRAM0 */
        report(r);
    }

    /* --- Per-window loop -- receives a window_t from Sensor in place of
     * executorch_batch_test's batch_test_vectors[i], everything after
     * that point is the same proven call sequence. --- */
    while (true) {
        window_t w;
        m_window_in.read(w);

        /* Handoff check: signature of what Inference actually received
         * through the channel, computed on w.data BEFORE any copy --
         * reported together with Sensor's own signature of the same
         * window (w.sensor_sum, computed before the channel write). */
        report_t r = {};
        r.kind      = ReportKind::Result;
        r.index     = w.index;
        r.buffer    = w.buffer_index;
        r.value[0]  = w.sensor_sum;
        {
            float sum = 0.0f;
            for (int i = 0; i < NUM_CHANNELS * WINDOW_SAMPLES; i++) {
                sum += w.data[i];
            }
            r.value[1] = sum;
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
            r.code = -1; r.aux = (uint32_t)err;        /* get_inputs */
            report(r);
            continue;
        }

        Result<TensorInfo> tensor_meta = method_meta.input_tensor_meta(0);
        if (!tensor_meta.ok()) {
            r.code = -2; r.aux = (uint32_t)tensor_meta.error();   /* input_tensor_meta */
            report(r);
            continue;
        }
        if (tensor_meta->nbytes() != sizeof(input_window)) {
            r.code = -3; r.aux = (uint32_t)tensor_meta->nbytes(); /* input size mismatch */
            report(r);
            continue;
        }

        Tensor input_tensor = input_evalues[0].toTensor();
        std::memcpy(input_tensor.mutable_data_ptr<int8_t>(), input_window,
                    sizeof(input_window));

        Error status = method.execute();
        if (status != Error::Ok) {
            r.code = -4; r.aux = (uint32_t)status;     /* execute */
            report(r);
            continue;
        }

        std::vector<EValue> outputs(method.outputs_size());
        status = method.get_outputs(outputs.data(), outputs.size());
        if (status != Error::Ok) {
            r.code = -5; r.aux = (uint32_t)status;     /* get_outputs */
            report(r);
            continue;
        }
        if (!outputs[0].isTensor()) {
            r.code = -6;                               /* output[0] is not a tensor */
            report(r);
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

        r.code     = (int16_t)predicted;
        r.value[2] = logits[0];
        r.value[3] = (numel > 1) ? logits[1] : 0.0f;
        report(r);
    }
}
