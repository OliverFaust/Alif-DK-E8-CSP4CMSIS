/*
 * Builds and runs the Sensor -> Inference -> Console network. Structure
 * matches csp4cmsis_neuropathway's csp4cmsis_spn.cpp (MainApp_Task,
 * static osThreadNew() bootstrap, periodic per-process stack-usage report)
 * -- only the process types and channel payload types differ (window_t/
 * result_t as defined in common_types.h, not that project's frame_t/
 * detection-list shapes).
 *
 * Migrated from native FreeRTOS (xTaskCreateStatic, raw TaskHandle_t/
 * StackType_t/StaticTask_t/UBaseType_t) to CMSIS-RTOS2, mirroring
 * csp4cmsis_alt_test/application.cpp's own migration exactly.
 */
#include "csp4cmsis_spn.h"

#include "csp/csp4cmsis.h"
#include "sensor_process.h"
#include "inference_process.h"
#include "console_process.h"

#include <cstdio>

using namespace csp;

#ifndef CSP_STACK_REPORT_INTERVAL_MS
#define CSP_STACK_REPORT_INTERVAL_MS (10000)
#endif

/* MainApp_Task isn't a CSProcess, so its stack isn't sized via
 * CSProcessStatic<N> -- given explicitly here, same as
 * csp4cmsis_neuropathway. */
#define MAIN_APP_STACK_WORDS 1024

// MainApp_Task's priority, one step above the CSP network it spawns
// (csp4cmsis's own CSP_LEGACY_PARALLEL_PRIORITY, osPriorityLow) --
// identical reasoning and identical resolved value to
// csp4cmsis_alt_test/application.cpp's own CSP_MAIN_APP_PRIORITY: the old
// native-FreeRTOS formula here was the exact same tskIDLE_PRIORITY+3
// (vs. the network's tskIDLE_PRIORITY+2), and this task's own lifecycle
// is functionally the same shape -- spawn the network non-blocking, then
// do nothing but periodic (10s) diagnostic stack-usage reporting forever,
// with zero further interaction with Sensor/Inference/Console (no shared
// channel, no signal, just p.stackHighWaterMarkWords()'s passive kernel-
// bookkeeping read). No functional requirement was found here either
// that demands this specific gap over, say, equal priority to the
// network -- preserved on the same evidence-of-intent grounds as
// application.cpp's version, not a proven functional need.
constexpr osPriority_t CSP_MAIN_APP_PRIORITY = osPriorityLow1;

static osThreadId_t s_main_app_task_handle = NULL;

void MainApp_Task(void* params)
{
    (void)params;
    osDelay(500);

    static Channel<window_t> window_chan; // unbuffered
    static Channel<result_t> result_chan; // unbuffered

    static Sensor    sensor(window_chan.writer());
    static Inference inference(window_chan.reader(), result_chan.writer());
    static Console   console(result_chan.reader());

    auto network = InParallel(sensor, inference, console);
    Run(network, ExecutionMode::StaticNetwork);

    printf("*** MainApp_Task: Run() returned, entering report loop ***\r\n");

    while (true) {
        osDelay(CSP_STACK_REPORT_INTERVAL_MS);

        if (s_main_app_task_handle != NULL) {
            // Mirrors csp4cmsis's own stackHighWaterMarkWords()
            // (process.h) exactly: osThreadGetStackSpace() returns
            // bytes, divided by the same portable word-size type the
            // library itself uses.
            uint32_t hwm = osThreadGetStackSpace(s_main_app_task_handle)
                           / sizeof(internal::csp_stack_word_t);
            size_t unused_bytes = hwm * sizeof(internal::csp_stack_word_t);
            printf("CSP_Main: %u bytes unused headroom (%u words HWM, of %u allocated)\r\n",
                   (unsigned)unused_bytes, (unsigned)hwm, (unsigned)MAIN_APP_STACK_WORDS);
        }

        network.forEachProcess([](CSProcess& p) {
            size_t allocated_words = p.stackWords();
            size_t allocated_bytes = allocated_words * sizeof(internal::csp_stack_word_t);

            uint32_t hwm = p.stackHighWaterMarkWords();
            if (hwm == CSP_STACK_HWM_UNAVAILABLE) {
                printf("%s: allocated = %u words (%u bytes), HWM unavailable\r\n",
                       p.name(), (unsigned)allocated_words, (unsigned)allocated_bytes);
            } else {
                size_t unused_bytes = hwm * sizeof(internal::csp_stack_word_t);
                size_t used_bytes = (unused_bytes <= allocated_bytes)
                                        ? allocated_bytes - unused_bytes
                                        : 0;
                printf("%s: %u/%u bytes used (%u bytes unused headroom, %u words HWM)\r\n",
                       p.name(), (unsigned)used_bytes, (unsigned)allocated_bytes,
                       (unsigned)unused_bytes, (unsigned)hwm);
            }
        });
    }
}

// alignas(8): ARM AAPCS requires 8-byte SP alignment at public interfaces
// -- see process.h's CSProcessStatic<N>::m_stack comment (found via the
// RTX5 validation pass: a plain csp_stack_word_t/uint32_t array only
// guarantees natural 4-byte alignment, which RTX5's osThreadNew()
// validates and rejects outright; the FreeRTOS adapter never checks this
// at all, so it's a real, silent bug on this backend too, just never
// triggered). This project stays on the FreeRTOS backend, but the same
// buffer-declaration pattern applies here, so the same fix does too.
alignas(8) static internal::csp_stack_word_t s_main_app_stack[MAIN_APP_STACK_WORDS];
#if defined(CSP4CMSIS_STATIC_ALLOCATION)
static internal::csp_static_thread_storage_t s_main_app_tcb;
#endif

extern "C" void RunProcessingChainTest(void)
{
    osThreadAttr_t attr = {};
    attr.name       = "CSP_Main";
    attr.stack_mem  = s_main_app_stack;
    attr.stack_size = MAIN_APP_STACK_WORDS * sizeof(internal::csp_stack_word_t);
#if defined(CSP4CMSIS_STATIC_ALLOCATION)
    attr.cb_mem     = &s_main_app_tcb;
    attr.cb_size    = sizeof(internal::csp_static_thread_storage_t);
#endif
    attr.priority   = CSP_MAIN_APP_PRIORITY;

    osThreadId_t handle = osThreadNew(MainApp_Task, NULL, &attr);

    s_main_app_task_handle = handle; // NULL on failure -- report loop guards for that

    if (handle == NULL) {
        printf("FATAL ERROR: Failed to create MainApp_Task "
               "(osThreadNew returned NULL -- check stack/TCB buffers).\r\n");
    }
}
