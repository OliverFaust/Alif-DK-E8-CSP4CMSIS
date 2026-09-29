/* TEST BRANCH: heap-free build (FreeRTOS-NoHeap), forced include.
 * CMSIS-FreeRTOS 11.3.0 clib_os.c (Arm C library locks, AC6 only) falls back
 * to the dynamic xSemaphoreCreateMutex() without checking
 * configSUPPORT_DYNAMIC_ALLOCATION. Make that fallback yield NULL, so only the
 * adapter's static mutex pool (OS_MUTEX_CLIB_NUM = 5) is used. */
#ifndef NOHEAP_SHIM_H
#define NOHEAP_SHIM_H
#define xSemaphoreCreateMutex() ((void *)0)
#endif
