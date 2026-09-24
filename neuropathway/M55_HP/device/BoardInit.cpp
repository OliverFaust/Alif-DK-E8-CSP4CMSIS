/*
 * Board init for the ExecuTorch smoke test on DK-E8/RTSS-HP.
 *
 * Trimmed from object_detection_e8/M55_HP/device/BoardInit.cpp (same
 * confirmed-working board bring-up sequence: stdout_init/NpuInit/
 * CpuCacheEnable) -- the camera-specific pieces (I3CPinsInit,
 * CameraPinsInit) are dropped entirely since this smoke test has no
 * camera and no sensor input at all yet (Step 2's test vectors replace
 * live input for this stage).
 *
 * Real bug found on first hardware run: object_detection_e8's own
 * CameraClocksEnable() -- which this file originally dropped as
 * "camera-specific" -- also did the ONE thing that turned out to matter
 * for the NPU: an SE run-config update. But it never actually gated in
 * CLKEN_NPU either (only MIPI/100MHz/HFOSC clocks), so simply restoring
 * that function wouldn't have fixed this. Diagnostic bracketing
 * (printf before/after NpuInit()) on real hardware showed execution
 * hangs inside NpuInit()/ethosu_init() specifically -- never reaching
 * the print after it, despite stdout already working. Checked
 * se_services/include/aipm.h's run_profile_t: ip_clock_gating has an
 * explicit NPU_HP_MASK (IP_CLOCK_NPU_HP) -- and services_lib_api.h's
 * clock_enable_t has CLKEN_NPU/CLKEN_SRAM0 as separate, simpler
 * single-clock enables via SERVICES_clocks_enable_clock(), same API
 * object_detection_e8 already uses for CLKEN_CLK_100M/CLKEN_HFOSC. The
 * NPU and SRAM0 (where our ExecuTorch arena lives, see
 * ACTIVATION_BUF_ATTRIBUTE in main.cpp) are both clock-gated off by
 * default and neither this file's original version nor
 * object_detection_e8's ever enabled them -- object_detection_e8's own
 * copy of this code has never actually been hardware-tested either
 * (build-only reference), so this gap was latent there too, not
 * something this port introduced.
 */

#include "BoardInit.hpp"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

#include "RTE_Components.h"
#include "RTE_Device.h"
#include CMSIS_device_header
#include "Driver_Common.h"
#include "ethosu_driver.h"
#include "retarget_init.h"
#include "retarget_stdout.h"
#include "se_services_port.h"
#include <stdio.h>

static struct ethosu_driver npuDriver;

/* Statically-named weak-symbol override, NOT NVIC_SetVector() -- the vector
 * table lives in read-only MRAM (never relocated to RAM) and was never
 * meant to be patched at runtime. NVIC_SetVector() attempts exactly that
 * (a write into the IRQ 55 slot at MRAM+0x11c) and MemManage-faults
 * (DACCVIOL) doing it -- confirmed via J-Link/GDB on real hardware:
 * CFSR=0x82, MMFAR=0x8020011c, which is exactly MRAM_BASE + vector-table
 * offset for NPU_HP_IRQ_IRQn (55). Matches Alif's own Ensemble pack
 * reference (Boards/DevKit-e8/Layers/M55_HP/ethos_setup.c), which defines
 * NPU_HP_IRQHandler this same way rather than calling NVIC_SetVector(). */
void NPU_HP_IRQHandler(void)
{
    ethosu_irq_handler(&npuDriver);
}

#if defined(__cplusplus)
}
#endif // defined(__cplusplus)

/* Enable the SE clock gates for NPU_HP and SRAM0 -- both are gated off by
 * default. Without this, ethosu_init() hangs/faults touching NPU_HP_BASE
 * registers on an unclocked block (confirmed on real hardware: execution
 * never returns from ethosu_init() without this). */
static uint32_t NpuSramClocksEnable(void)
{
    uint32_t error_code = SERVICES_REQ_SUCCESS;
    uint32_t service_error_code;

    se_services_port_init();

    error_code = SERVICES_clocks_enable_clock(se_services_s_handle, CLKEN_NPU, true, &service_error_code);
    if (error_code != SERVICES_REQ_SUCCESS) {
        printf("SE: NPU clock enable = %u\n", error_code);
        return error_code;
    }

    error_code = SERVICES_clocks_enable_clock(se_services_s_handle, CLKEN_SRAM0, true, &service_error_code);
    if (error_code != SERVICES_REQ_SUCCESS) {
        printf("SE: SRAM0 clock enable = %u\n", error_code);
        return error_code;
    }

    return SERVICES_REQ_SUCCESS;
}

/* NPU_HP_BASE/NPU_HP_IRQ_IRQn confirmed against Ensemble's own
 * ethos_setup.c for M55_HP+U55 (same as object_detection_e8 -- see that
 * project's BoardInit.cpp header comment). */
bool NpuInit()
{
    if (NpuSramClocksEnable() != SERVICES_REQ_SUCCESS) {
        return false;
    }
    printf("Board init: NPU/SRAM0 clocks enabled, calling ethosu_init()...\r\n");

    void * const npuBaseAddr = reinterpret_cast<void*>(NPU_HP_BASE);

    if (ethosu_init(&npuDriver, npuBaseAddr,
                    0, /* Cache memory pointer (not applicable for U55) */
                    0, /* Cache memory size */
                    1, /* Secure */
                    1) /* Privileged */ ) {
        printf("Failed to initialize Arm Ethos-U driver\n");
        return false;
    }

    /* No NVIC_SetVector() -- NPU_HP_IRQHandler above is a link-time weak-
     * symbol override, already in the vector table by the time this runs. */
    NVIC_EnableIRQ(NPU_HP_IRQ_IRQn);

    return true;
}

static void CpuCacheEnable(void)
{
    SCB_EnableICache();
    SCB_EnableDCache();
}

void BoardInit(void)
{
#if defined(RTE_CMSIS_Compiler_STDOUT_Custom)
    if (stdout_init() != ARM_DRIVER_OK) {
        printf("stdout_init failed\n");
        return;
    }
#endif /* defined(RTE_CMSIS_Compiler_STDOUT_Custom) */

    /* Diagnostic bracket: NpuInit()/ethosu_init() is the one piece of this
     * bring-up sequence that's genuinely new and never hardware-tested in
     * this project (object_detection_e8's own copy of this code was never
     * flashed and run in this session either -- it was only ever a build
     * reference). If output stops appearing between these two prints, the
     * hang is in NpuInit(), not stdout. */
    printf("Board init: stdout OK, calling NpuInit()...\r\n");

#if defined(ETHOSU55)
    /* ETHOSU55 is our own project-level define (csolution.yml target-types),
     * not the ethos-u-core-driver pack's internal ETHOSU_ARCH macro --
     * object_detection_e8's TFLM/MLEK component chain happens to also define
     * ETHOSU_ARCH via its "NPU Support:Ethos-U Driver&Generic U55" component,
     * but this project pulls its ethos-u-core-driver transitively through
     * ExecuTorch's "Backend EthosU" component instead, and that path wasn't
     * confirmed to define ETHOSU_ARCH the same way -- so gate on the define
     * we set ourselves rather than assume. */
    if (!NpuInit()) {
        return;
    }
#endif

    CpuCacheEnable();

    printf("Board init: completed\n");
    return;
}
