/* TEST BRANCH: heap-free build (FreeRTOS-NoHeap).
 * CMSIS-FreeRTOS 11.3.0 cmsis_os2.c references pvPortMalloc()/vPortFree()
 * without checking configSUPPORT_DYNAMIC_ALLOCATION (osThreadEnumerate(),
 * osMemoryPool*()), so a build without a heap implementation needs these
 * symbols. They are traps: every call is counted and reported, and the
 * allocation fails. bc_tests.cpp prints the count (T19). */
#include <stddef.h>
#include <stdio.h>

volatile unsigned int noheap_alloc_calls = 0;

void *pvPortMalloc(size_t size) {
    noheap_alloc_calls++;
    printf("!! pvPortMalloc(%u) called in the heap-free build\r\n", (unsigned int)size);
    return NULL;
}

void vPortFree(void *p) {
    (void)p;
    noheap_alloc_calls++;
    printf("!! vPortFree() called in the heap-free build\r\n");
}
