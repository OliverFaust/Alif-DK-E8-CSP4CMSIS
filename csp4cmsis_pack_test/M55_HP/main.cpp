/*
 * CSP4CMSIS Alternation smoke-test entry point (RTSS-HP, DK-E8) -- stage 2
 * of the neuropathway port. Same board/RTOS bring-up as freertos_blinky's
 * main.c (SE/stdout init, SystemCoreClockUpdate, start the scheduler), but
 * instead of a blink task it hands off to application.cpp's
 * RunProcessingChainTest(), which spawns the csp4cmsis Alternation network
 * (see application.cpp for the network itself: two Senders racing into a
 * Receiver via Alternative).
 *
 * Backend-conditional (CSP4CMSIS_RTOS2_BACKEND_FREERTOS vs.
 * CSP4CMSIS_RTOS2_BACKEND_RTX5, same macros csp_rtos_static.h selects on)
 * since this file's own board-bringup code -- not csp4cmsis itself -- has
 * genuine FreeRTOS-kernel-bootstrap-specific pieces with no portable
 * equivalent (see the block below). Discovered during the RTX5 validation
 * pass: csp4cmsis's own library source needed zero changes to build
 * against RTX5; this file is not library code, and did.
 */

#include <cstdio>
#include <RTE_Components.h>
#include CMSIS_device_header

#include "cmsis_os2.h"
#if defined(CSP4CMSIS_RTOS2_BACKEND_FREERTOS)
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#endif
#include "Driver_USART.h" /* For ARM_DRIVER_OK (blinky pulled this in via
                            * Driver_IO.h instead, which this project drops
                            * since it doesn't touch GPIO). */
#if defined(RTE_CMSIS_Compiler_STDOUT)
#include "retarget_init.h"
#include "retarget_stdout.h"
#endif /* RTE_CMSIS_Compiler_STDOUT */

extern "C" {
#include "app_utils.h"
}

/* Defined in application.cpp -- spawns the csp4cmsis Alternation network
 * (MainApp_Task, itself created via osThreadNew) and returns. */
void RunProcessingChainTest(void);

#if defined(CSP4CMSIS_RTOS2_BACKEND_FREERTOS)
/*----------------------------------------------------------------------------
 * Static memory for the FreeRTOS kernel's own Idle/Timer tasks.
 * Required whenever configSUPPORT_STATIC_ALLOCATION is enabled.
 *
 * FreeRTOS-backend-specific, not a portability gap needing translation:
 * this is FreeRTOS's own hook mechanism for the app to supply the
 * *kernel's* own housekeeping-thread memory (configKERNEL_PROVIDED_
 * STATIC_MEMORY=0). RTX5 (and CMSIS-RTOS2 backends generally) has no
 * equivalent application-supplied-hook concept at all -- it manages its
 * own Idle thread's memory entirely internally via its own config
 * mechanism (RTX_Config.h's OS_IDLE_THREAD_STACK_SIZE / OS_DYNAMIC_
 * MEM_SIZE). Confirmed by reading RTX5's actual source, not assumed.
 *--------------------------------------------------------------------------*/
#define IDLE_TASK_STACK_SIZE           configMINIMAL_STACK_SIZE
#define TIMER_SERVICE_TASK_STACK_SIZE  configTIMER_TASK_STACK_DEPTH

static StackType_t  IdleStack[IDLE_TASK_STACK_SIZE];
static StaticTask_t IdleTcb;
static StackType_t  TimerStack[TIMER_SERVICE_TASK_STACK_SIZE];
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

/* vApplicationStackOverflowHook/vApplicationMallocFailedHook: FreeRTOS-
 * specific hook *names* the FreeRTOS kernel calls by convention, not a
 * rename target. RTX5 uses a completely different mechanism
 * (osRtxErrorNotify(), __WEAK and already given a working default
 * implementation in the RTX5 component's own generated RTX_Config.c --
 * confirmed by reading it, not assumed -- so RTX5 needs no equivalent
 * hook supplied here at all). */
extern "C" void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    ARG_UNUSED(pxTask);
    ARG_UNUSED(pcTaskName);
    ASSERT_HANG_LOOP
}

/*
 * configUSE_MALLOC_FAILED_HOOK requires this. Deliberately no printf (or
 * any other call that might itself need the heap, e.g. via newlib's
 * lazily-initialized stdio buffers) -- this can fire from inside
 * pvPortMalloc() before the heap is known-good, per task 4's requirement.
 * Spin with interrupts disabled so a debugger/JTAG halt can still find it;
 * toggling a bit here (e.g. an LED) would need a GPIO already initialized,
 * which we don't guarantee at this point either, so this stays a bare loop.
 */
extern "C" void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}
#endif // CSP4CMSIS_RTOS2_BACKEND_FREERTOS

int main(void)
{
#if defined(RTE_CMSIS_Compiler_STDOUT_Custom)
    if (stdout_init() != ARM_DRIVER_OK) {
        WAIT_FOREVER_LOOP
    }
#endif

    SystemCoreClockUpdate();

    printf("\r\ncsp4cmsis Alternation smoke test (RTSS-HP, DK-E8) starting\r\n");
#if defined(CSP4CMSIS_RTOS2_BACKEND_FREERTOS)
    printf("Free heap before MainApp_Task creation: %u bytes\r\n",
           (unsigned)xPortGetFreeHeapSize());
#endif

    RunProcessingChainTest();
    printf("RunProcessingChainTest() returned, starting scheduler...\r\n");

    /* osKernelInitialize()/osKernelStart() are genuinely portable across
     * CMSIS-RTOS2 backends -- confirmed the FreeRTOS adapter's own
     * osKernelStart() (cmsis_os2.c) internally calls vTaskStartScheduler()
     * itself, so this single call sequence replaces the old direct
     * vTaskStartScheduler() call for both backends, not just RTX5. No
     * backend branching needed here. */
    osKernelInitialize();
    if (osKernelGetState() == osKernelReady) {
        osKernelStart();
    }

    /* Only reached if the kernel could not start. */
    printf("ERROR: kernel did not start!\r\n");
    for (;;) {
    }
}
