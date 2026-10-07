/*
 * Builds and runs the Sensor -> Inference -> Reporter network. Structure
 * matches csp4cmsis_neuropathway's csp4cmsis_spn.cpp (MainApp_Task,
 * static osThreadNew() bootstrap) -- only the process types and channel
 * payload types differ (window_t/report_t as defined in common_types.h).
 *
 * The console has one owner: the Reporter is the only process that prints
 * while the network runs (it also prints the periodic stack report).
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

/* MainApp_Task isn't a CSProcess, so its stack isn't sized via
 * CSProcessStatic<N> -- given explicitly here, same as
 * csp4cmsis_neuropathway. */
#define MAIN_APP_STACK_WORDS 1024

// MainApp_Task's priority, one step above the CSP network it spawns
// (Run()'s default CSP_DEFAULT_NETWORK_PRIORITY, osPriorityLow) --
// identical reasoning and identical resolved value to
// csp4cmsis_alt_test/application.cpp's own CSP_MAIN_APP_PRIORITY: the old
// native-FreeRTOS formula here was the exact same tskIDLE_PRIORITY+3
// (vs. the network's tskIDLE_PRIORITY+2), and this task's own lifecycle
// is functionally the same shape -- spawn the network non-blocking. Being
// above the network, MainApp prints its line and ends before any process
// runs, so it never competes with the Reporter for the console.
constexpr osPriority_t CSP_MAIN_APP_PRIORITY = osPriorityLow1;

void MainApp_Task(void* params)
{
    (void)params;
    osDelay(500);

    static Channel<window_t> window_chan; // unbuffered
    static Channel<report_t> report_chan; // unbuffered; writers: Sensor, Inference

    static Sensor    sensor(window_chan.writer(), report_chan.writer());
    static Inference inference(window_chan.reader(), report_chan.writer());
    static CSProcess* const reported[] = { &sensor, &inference };   // for the stack report
    static Reporter  reporter(report_chan.reader(), reported, 2);

    Run(InParallel(sensor, inference, reporter), ExecutionMode::StaticNetwork);

    printf("*** MainApp_Task: network started. Terminating. ***\r\n");
    osThreadExit();
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
