/*
 * Builds and runs the Sensor -> Inference -> Reporter network. Structure
 * matches csp4cmsis_neuropathway's csp4cmsis_spn.cpp (MainApp_Task,
 * static osThreadNew() bootstrap) -- only the process types and channel
 * payload types differ (window_t/report_t as defined in common_types.h).
 *
 * The console has one owner: the Reporter is the only process that prints
 * while the network runs. MainApp stays alive and sends the stack report
 * (one Stack report per thread every CSP_STACK_REPORT_INTERVAL_MS) over its
 * own writer end of the report channel; it never prints.
 *
 * Migrated from native FreeRTOS (xTaskCreateStatic, raw TaskHandle_t/
 * StackType_t/StaticTask_t/UBaseType_t) to CMSIS-RTOS2, mirroring
 * csp4cmsis_alt_test/application.cpp's own migration exactly.
 */
#include "csp4cmsis_spn.h"

#include "csp/csp4cmsis.h"
#include "sensor_process.h"
#include "inference_process.h"
#include "reporter_process.h"

#include <cstdio>

using namespace csp;

/* MainApp sends the stack report on its own timer, whatever the network is
 * doing, so the report continues when the pipeline stalls. A live
 * worst-observed-so-far reading. */
#ifndef CSP_STACK_REPORT_INTERVAL_MS
#define CSP_STACK_REPORT_INTERVAL_MS (3000)
#endif

/* MainApp_Task isn't a CSProcess, so its stack isn't sized via
 * CSProcessStatic<N> -- given explicitly here, same as
 * csp4cmsis_neuropathway. Measured on the board: 480 bytes used (it never
 * prints), so 256 words (1 KB). */
#define MAIN_APP_STACK_WORDS 256

// MainApp_Task's priority, one step above the CSP network it spawns
// (Run()'s default CSP_DEFAULT_NETWORK_PRIORITY, osPriorityLow) --
// identical reasoning and identical resolved value to
// csp4cmsis_alt_test/application.cpp's own CSP_MAIN_APP_PRIORITY: the old
// native-FreeRTOS formula here was the exact same tskIDLE_PRIORITY+3
// (vs. the network's tskIDLE_PRIORITY+2), and this task's own lifecycle
// is functionally the same shape -- spawn the network non-blocking. Being
// above the network, MainApp's stack report is not held up by a busy network
// process; it then waits on the report channel while the Reporter prints.
constexpr osPriority_t CSP_MAIN_APP_PRIORITY = osPriorityLow1;

static report_t stackReport(ReportSource source, uint32_t allocated_bytes, uint32_t hwm_words)
{
    report_t r = {};
    r.kind   = ReportKind::Stack;
    r.source = source;
    r.count  = allocated_bytes;
    r.index  = hwm_words;                       /* UINT32_MAX (CSP_STACK_HWM_UNAVAILABLE): unavailable */
    if (hwm_words != CSP_STACK_HWM_UNAVAILABLE) {
        uint32_t unused_bytes = hwm_words * sizeof(internal::csp_stack_word_t);
        r.aux = (unused_bytes <= allocated_bytes) ? allocated_bytes - unused_bytes : 0;
    }
    return r;
}

void MainApp_Task(void* params)
{
    (void)params;
    osDelay(500);

    static Channel<window_t> window_chan; // unbuffered
    static Channel<report_t> report_chan; // unbuffered; writers: Sensor, Inference, MainApp

    static Sensor    sensor(window_chan.writer(), report_chan.writer());
    static Inference inference(window_chan.reader(), report_chan.writer());
    static Reporter  reporter(report_chan.reader());

    /* Kept as a named object: the stack report walks it with forEachProcess(). */
    auto network = InParallel(sensor, inference, reporter);
    Run(network, ExecutionMode::StaticNetwork);

    /* MainApp is an ordinary CMSIS-RTOS2 thread with its own writer end: a
     * rendezvous may be used by any thread (it blocks on thread flag 0 of the
     * calling thread, which MainApp uses for nothing else). */
    Chanout<report_t> report_out = report_chan.writer();
    static constexpr ReportSource kSources[] = {
        ReportSource::Sensor, ReportSource::Inference, ReportSource::Reporter };  /* InParallel order */

    while (true) {
        SleepFor(Milliseconds(CSP_STACK_REPORT_INTERVAL_MS));

        size_t i = 0;
        network.forEachProcess([&](CSProcess& p) {
            report_out << stackReport(kSources[i++],
                                      p.stackWords() * sizeof(internal::csp_stack_word_t),
                                      p.stackHighWaterMarkWords());
        });
        report_out << stackReport(ReportSource::Main,
                                  MAIN_APP_STACK_WORDS * sizeof(internal::csp_stack_word_t),
                                  osThreadGetStackSpace(osThreadGetId()) / sizeof(internal::csp_stack_word_t));
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
    if (handle == NULL) {
        printf("FATAL ERROR: Failed to create MainApp_Task "
               "(osThreadNew returned NULL -- check stack/TCB buffers).\r\n");
    }
}
