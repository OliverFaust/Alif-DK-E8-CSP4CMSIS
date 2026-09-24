/*
 * neuropathway main.cpp -- FreeRTOS bootstrap for the Sensor -> Inference
 * -> Console CSP network (RTSS-HP, DK-E8). Structurally identical to
 * csp4cmsis_neuropathway's main.cpp (SE/stdout/board init,
 * SystemCoreClockUpdate, hand off to RunProcessingChainTest(), start the
 * scheduler) -- BoardInit() here is executorch_batch_test's proven
 * NPU/cache bring-up (zero SERVICES_set_run_cfg() calls -- see
 * Neuropathway.csolution.yml's header comment), not
 * csp4cmsis_neuropathway's camera-pinmux BoardInit().
 */

#include <cstdio>
#include <RTE_Components.h>
#include CMSIS_device_header

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "Driver_USART.h" /* For ARM_DRIVER_OK. */
#if defined(RTE_CMSIS_Compiler_STDOUT)
#include "retarget_init.h"
#include "retarget_stdout.h"
#endif /* RTE_CMSIS_Compiler_STDOUT */

#include "BoardInit.hpp"
#include "csp/csp4cmsis_spn.h"

extern "C" {
#include "app_utils.h"
}

/*----------------------------------------------------------------------------
 * Static memory for the FreeRTOS kernel's own Idle/Timer tasks.
 * Required whenever configSUPPORT_STATIC_ALLOCATION is enabled.
 *--------------------------------------------------------------------------*/
#define IDLE_TASK_STACK_SIZE           configMINIMAL_STACK_SIZE
#define TIMER_SERVICE_TASK_STACK_SIZE  configTIMER_TASK_STACK_DEPTH

// alignas(8): ARM AAPCS requires 8-byte SP alignment at public interfaces
// -- found via the RTX5 validation pass (csp4cmsis_alt_test), which
// caught csp4cmsis's own CSProcessStatic<N> stack buffers violating this
// (FreeRTOS's own port never checks it, RTX5 does). This project stays
// on the FreeRTOS backend, but these are the same kind of raw
// xTaskCreateStatic()-style stack buffer (here, indirectly, via
// vApplicationGetIdleTaskMemory/vApplicationGetTimerTaskMemory), so the
// same correctness requirement applies regardless of whether this
// backend currently enforces it.
alignas(8) static StackType_t  IdleStack[IDLE_TASK_STACK_SIZE];
static StaticTask_t IdleTcb;
alignas(8) static StackType_t  TimerStack[TIMER_SERVICE_TASK_STACK_SIZE];
static StaticTask_t TimerTcb;

extern "C" void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                               StackType_t  **ppxIdleTaskStackBuffer,
                                               configSTACK_DEPTH_TYPE *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &IdleTcb;
    *ppxIdleTaskStackBuffer = IdleStack;
    *pulIdleTaskStackSize   = IDLE_TASK_STACK_SIZE;
}

extern "C" void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                                StackType_t  **ppxTimerTaskStackBuffer,
                                                configSTACK_DEPTH_TYPE *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer   = &TimerTcb;
    *ppxTimerTaskStackBuffer = TimerStack;
    *pulTimerTaskStackSize   = TIMER_SERVICE_TASK_STACK_SIZE;
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    ARG_UNUSED(pxTask);
    ARG_UNUSED(pcTaskName);
    ASSERT_HANG_LOOP
}

/* configUSE_MALLOC_FAILED_HOOK requires this -- deliberately no printf/heap
 * use inside, since this can fire from inside pvPortMalloc() before the
 * heap is known-good (same rationale as every prior csp4cmsis stage). */
extern "C" void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

int main(void)
{
    /* No stdout_init() here -- BoardInit() (executorch_batch_test's,
     * below) already calls it; a second call here would just be a
     * redundant re-init of the same USART driver. */
    SystemCoreClockUpdate();

    BoardInit();

    printf("\r\n=== BUILD MARKER: neuropathway-pingpong-0001 ===\r\n");
    printf("\r\nneuropathway: Sensor -> Inference -> Console (RTSS-HP, DK-E8) "
           "starting\r\n");

    RunProcessingChainTest();

    vTaskStartScheduler();

    /* Only reached if the scheduler could not start. */
    printf("ERROR: vTaskStartScheduler() returned -- scheduler did not start!\r\n");
    for (;;) {
    }
}
