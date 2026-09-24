// --- heap_glue.cpp ---
//
// Global operator new/delete override, routed through FreeRTOS's
// pvPortMalloc/vPortFree. Lives here -- neuropathway's own application
// code -- rather than in csp4cmsis's shared glue.cpp, where an equivalent
// override used to live: csp4cmsis itself and csp4cmsis_alt_test have
// zero live allocation need (confirmed by investigation), so a shared-
// library-wide heap override was the wrong layer for it. This project
// does need one: inference_process.cpp's std::vector<EValue> outputs(...)
// allocates once per inference cycle (a hot path, not one-time startup).
//
// Not newlib-nano's default malloc: confirmed unsafe on this exact
// toolchain/port (ARM_CM55_NTZ, GCC) -- configUSE_NEWLIB_REENTRANT
// defaults to 0 (unset anywhere in this project's FreeRTOSConfig.h), and
// no __malloc_lock/__malloc_unlock implementation exists anywhere in the
// installed ARM::CMSIS-FreeRTOS pack for this port (only Xtensa and
// AVR32_UC3 have one) -- so newlib's malloc-lock hooks resolve to no-op
// stubs, meaning no locking happens at all. neuropathway runs three CSP
// processes concurrently (Sensor/Inference/Console); FreeRTOS's own
// heap_4.c (pvPortMalloc/vPortFree) is a documented, genuinely
// thread-safe implementation (internal critical-section protection) and
// is already linked (RTOS&FreeRTOS:Heap&Heap_4), so no new dependency is
// needed -- just this override, relocated from the library to here.

#include <cstddef>

extern "C" {
    // These functions must be linked from your FreeRTOS port
    extern void* pvPortMalloc(size_t xSize);
    extern void vPortFree(void* pv);
}

void* operator new(size_t size) {
    return pvPortMalloc(size);
}

void operator delete(void* ptr) noexcept {
    vPortFree(ptr);
}
