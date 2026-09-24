/*
 * Workaround for a newlib/gcc-arm-none-eabi packaging defect: this
 * toolchain's <inttypes.h> gates PRId64/PRIu64/PRIx64/etc behind
 * `#if __int64_t_defined`, but that macro never ends up set for this
 * target's multilib (cortex-m55, hard-float) even with <stdint.h>
 * included first -- reproduced with a minimal standalone .c file, so this
 * isn't specific to our project's include order.
 *
 * ARM::ethos-u-core-driver@1.26.5's ethosu_driver.c/ethosu_pmu.c use
 * PRIx64/PRIu64 without defining __STDC_FORMAT_MACROS or working around
 * this themselves, so the build needs these defined from somewhere. This
 * header is force-included ahead of every C translation unit (see
 * M55_HP.cproject.yml's `misc: C:` block) instead of editing the pack's
 * source directly.
 *
 * long long is 64-bit on this target (ILP32 EABI), matching what
 * inttypes.h's own __PRI64(x) would have produced (__INT64 == "ll").
 */
#ifndef OBJECT_DETECTION_E8_INTTYPES_WORKAROUND_H
#define OBJECT_DETECTION_E8_INTTYPES_WORKAROUND_H

#include <inttypes.h>

#ifndef PRId64
#define PRId64 "lld"
#endif
#ifndef PRIi64
#define PRIi64 "lli"
#endif
#ifndef PRIo64
#define PRIo64 "llo"
#endif
#ifndef PRIu64
#define PRIu64 "llu"
#endif
#ifndef PRIx64
#define PRIx64 "llx"
#endif
#ifndef PRIX64
#define PRIX64 "llX"
#endif

#endif /* OBJECT_DETECTION_E8_INTTYPES_WORKAROUND_H */
