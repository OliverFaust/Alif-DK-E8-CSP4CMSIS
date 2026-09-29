/*
 * CSP4CMSIS regression suite on the DK-E8 (RTSS-HP) -- board bring-up.
 *
 * Board/RTOS bring-up as in csp4cmsis_alt_test/M55_HP/main.cpp (stdout over
 * the board UART, SystemCoreClockUpdate, CMSIS-RTOS2 start). Then prints a
 * log header (library commit, toolchain, clock, caches, RTOS, priority
 * threshold) and hands over to bc_tests.cpp (csp_app_main_init()), which
 * prints RESULT/SUMMARY lines in the same format as the FVP logs.
 */

#include <cstdio>
#include <RTE_Components.h>
#include CMSIS_device_header

#include "cmsis_os2.h"
#if defined(RTE_CMSIS_RTOS2_FreeRTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif
#include "Driver_USART.h"
#if defined(RTE_CMSIS_Compiler_STDOUT)
#include "retarget_init.h"
#include "retarget_stdout.h"
#endif

// Written by hwtest.sh for every build: which CSP4CMSIS tree is under test.
#if __has_include("lib_version.h")
#include "lib_version.h"
#else
#error "lib_version.h missing: build with hwtest.sh (it records the library commit)"
#endif

extern "C" void csp_app_main_init(void);   // bc_tests.cpp

#if defined(__ARMCC_VERSION)
#define HWTEST_TOOLCHAIN "Arm Compiler " __VERSION__
#else
#define HWTEST_TOOLCHAIN "GCC " __VERSION__
#endif
#define HWTEST_STR2(x) #x
#define HWTEST_STR(x)  HWTEST_STR2(x)

#if defined(RTE_CMSIS_RTOS2_FreeRTOS)
// configKERNEL_PROVIDED_STATIC_MEMORY is 0 in this project's FreeRTOSConfig.h,
// so the application supplies the Idle/Timer task memory (as alt_test did).
static StackType_t  IdleStack[configMINIMAL_STACK_SIZE];
static StaticTask_t IdleTcb;
static StackType_t  TimerStack[configTIMER_TASK_STACK_DEPTH];
static StaticTask_t TimerTcb;

extern "C" void vApplicationGetIdleTaskMemory(StaticTask_t **tcb, StackType_t **stack,
                                               configSTACK_DEPTH_TYPE *size)
{
    *tcb = &IdleTcb; *stack = IdleStack; *size = configMINIMAL_STACK_SIZE;
}

extern "C" void vApplicationGetTimerTaskMemory(StaticTask_t **tcb, StackType_t **stack,
                                                configSTACK_DEPTH_TYPE *size)
{
    *tcb = &TimerTcb; *stack = TimerStack; *size = configTIMER_TASK_STACK_DEPTH;
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    printf("!! FreeRTOS stack overflow in %s\r\nSUMMARY: aborted by stack overflow\r\n\x04", name);
    for (;;) {
    }
}

extern "C" void vApplicationMallocFailedHook(void)
{
    printf("!! FreeRTOS malloc failed\r\n");
}
#endif

int main(void)
{
#if defined(RTE_CMSIS_Compiler_STDOUT_Custom)
    if (stdout_init() != ARM_DRIVER_OK) {
        for (;;) {
        }
    }
#endif
    SystemCoreClockUpdate();

    // Cycle counter for the latency measurements.
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;

    osKernelInitialize();
    char rtos_id[48] = "?";
    osVersion_t ver = {};
    osKernelGetInfo(&ver, rtos_id, sizeof(rtos_id));

    printf("\r\n=== HWTEST header ===\r\n");
    printf("board: Alif DevKit-E8, AE822FA0E5597LS0, core M55_HP; SystemCoreClock=%lu Hz\r\n",
           (unsigned long)SystemCoreClock);
    printf("caches: I=%s D=%s\r\n", (SCB->CCR & SCB_CCR_IC_Msk) ? "on" : "off",
           (SCB->CCR & SCB_CCR_DC_Msk) ? "on" : "off");
    printf("library: %s commit %s%s\r\n", HWTEST_LIB_NAME, HWTEST_LIB_COMMIT, HWTEST_LIB_DIRTY ? " (dirty)" : "");
    printf("build type: %s; toolchain: %s (%s)\r\n", HWTEST_BUILD_TYPE, HWTEST_TOOLCHAIN_ID, HWTEST_TOOLCHAIN);
    printf("rtos: %s (api %lu, kernel %lu); tick %lu Hz\r\n", rtos_id, (unsigned long)ver.api,
           (unsigned long)ver.kernel, (unsigned long)osKernelGetTickFreq());
    printf("CSP4CMSIS_MAX_SYSCALL_INTERRUPT_PRIORITY=%s (__NVIC_PRIO_BITS=%d); SWI IRQ %d at priority %s\r\n",
           HWTEST_STR(CSP4CMSIS_MAX_SYSCALL_INTERRUPT_PRIORITY), __NVIC_PRIO_BITS, (int)BC_SWI_IRQn,
           HWTEST_STR(BC_SWI_PRIO));
    printf("sweep: search hi=%s, range kb-%s..kb+%s\r\n", HWTEST_STR(BC_SEARCH_HI), HWTEST_STR(BC_SWEEP_BELOW),
           HWTEST_STR(BC_SWEEP_ABOVE));

    csp_app_main_init();
    if (osKernelGetState() == osKernelReady) {
        osKernelStart();
    }
    printf("ERROR: kernel did not start\r\n");
    for (;;) {
    }
}
