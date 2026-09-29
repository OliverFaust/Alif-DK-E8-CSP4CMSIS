# Third-party licences

The repository's own code is MIT-licensed (`LICENSE`). The files below are **not** covered by that
licence: they come from Alif Semiconductor or Arm (mostly as CMSIS-Pack configuration files that
CMSIS-Toolbox copies into each project's `RTE/` directory) and stay under their own licences. Every
file keeps its original header. The licence texts are in `LICENSES/`.

`*.base@<version>` files are the unmodified pack originals that CMSIS-Toolbox keeps next to each
configuration file, for merging updates; they are under the same licence as the file they belong to.

## Alif Semiconductor Software License Agreement

Licence text: [`LICENSES/Alif-Software-License-Agreement.txt`](LICENSES/Alif-Software-License-Agreement.txt)
(Rev. 10/2022), a verbatim copy of `License.txt` at the root of the pack
`AlifSemiconductor::Ensemble` 2.2.0 (`AlifSemiconductor.Ensemble.2.2.0.pack`, from
github.com/alifsemi/alif_ensemble-cmsis-dfp, release v2.2.0). Copyright Alif Semiconductor. The files
below come from that pack and its project templates. **These files may only be used with and executed on Alif Semiconductor devices**
(agreement, condition 4).

- `csp4cmsis_alt_test/.alif/`: `M55_HE_mram_cfg.json`, `M55_HP_HE_mram_cfg.json`, `M55_HP_mram_cfg.json`
- `csp4cmsis_alt_test/M55_HP/RTE/BSP/AE822FA0E5597LS0_M55_HP/`: `board_defs.h`, `board_defs.h.base@2.2.0`, `gpios.h`, `gpios.h.base@2.2.0`, `pins.h`, `pins.h.base@2.2.0`
- `csp4cmsis_alt_test/M55_HP/RTE/Device/AE822FA0E5597LS0_M55_HP/`: `RTE_Device.h`, `RTE_Device.h.base@2.2.0`, `app_mem_regions.h`, `app_mem_regions.h.base@3.0.0`, `core_config.h`, `core_config.h.base@2.1.0`
- `csp4cmsis_alt_test/M55_HP/RTE/Services/AE822FA0E5597LS0_M55_HP/`: `retarget_config.h`, `retarget_config.h.base@2.1.0`
- `csp4cmsis_alt_test/M55_HP/`: `app_utils.h`
- `csp4cmsis_pack_test/.alif/`: `M55_HE_mram_cfg.json`, `M55_HP_HE_mram_cfg.json`, `M55_HP_mram_cfg.json`
- `csp4cmsis_pack_test/M55_HP/RTE/BSP/AE822FA0E5597LS0_M55_HP/`: `board_defs.h`, `board_defs.h.base@2.2.0`, `gpios.h`, `gpios.h.base@2.2.0`, `pins.h`, `pins.h.base@2.2.0`
- `csp4cmsis_pack_test/M55_HP/RTE/Device/AE822FA0E5597LS0_M55_HP/`: `RTE_Device.h`, `RTE_Device.h.base@2.2.0`, `app_mem_regions.h`, `app_mem_regions.h.base@3.0.0`, `core_config.h`, `core_config.h.base@2.1.0`
- `csp4cmsis_pack_test/M55_HP/RTE/Services/AE822FA0E5597LS0_M55_HP/`: `retarget_config.h`, `retarget_config.h.base@2.1.0`
- `csp4cmsis_pack_test/M55_HP/`: `app_utils.h`
- `neuropathway/M55_HP/RTE/BSP/AE822FA0E5597LS0_M55_HP/`: `board_defs.h`, `board_defs.h.base@2.2.0`, `gpios.h`, `gpios.h.base@2.2.0`, `pins.h`, `pins.h.base@2.2.0`
- `neuropathway/M55_HP/RTE/Device/AE822FA0E5597LS0_M55_HP/`: `RTE_Device.h`, `RTE_Device.h.base@2.2.0`, `app_mem_regions.h`, `app_mem_regions.h.base@3.0.0`, `core_config.h`, `core_config.h.base@2.1.0`
- `neuropathway/M55_HP/RTE/Services/AE822FA0E5597LS0_M55_HP/`: `retarget_config.h`, `retarget_config.h.base@2.1.0`
- `neuropathway/M55_HP/`: `app_utils.h`

The `.alif/*_mram_cfg.json` files are identical to the pack's templates
(`Alif_CMSIS/Template/Zephyr/.alif/`); they have no header of their own.

## Alif Semiconductor Software License Agreement and Apache-2.0 (Arm files ported by Alif)

These files were written by Arm under Apache-2.0 and ported and modified by Alif Semiconductor; both
headers are in each file, and both licences apply.

- `csp4cmsis_alt_test/M55_HP/RTE/Device/AE822FA0E5597LS0_M55_HP/`: `app_tz.h`, `app_tz.h.base@2.1.0`, `linker_gnu_mram.ld.src`, `linker_gnu_mram.ld.src.base@2.2.0`, `linker_gnu_tcm.ld.src`, `linker_gnu_tcm.ld.src.base@2.1.0`
- `csp4cmsis_pack_test/M55_HP/RTE/Device/AE822FA0E5597LS0_M55_HP/`: `app_tz.h`, `app_tz.h.base@2.1.0`, `linker_gnu_mram.ld.src`, `linker_gnu_mram.ld.src.base@2.2.0`, `linker_gnu_tcm.ld.src`, `linker_gnu_tcm.ld.src.base@2.1.0`
- `neuropathway/M55_HP/RTE/Device/AE822FA0E5597LS0_M55_HP/`: `app_tz.h`, `app_tz.h.base@2.1.0`, `linker_gnu_mram.ld.src`, `linker_gnu_mram.ld.src.base@2.2.0`, `linker_gnu_tcm.ld.src`, `linker_gnu_tcm.ld.src.base@2.1.0`

## Apache License 2.0 (Arm)

Licence text: [`LICENSES/Apache-2.0.txt`](LICENSES/Apache-2.0.txt). Copyright Arm Limited and/or its
affiliates.

- `csp4cmsis_alt_test/M55_HP/RTE/CMSIS/`: `RTX_Config.c`, `RTX_Config.c.base@5.2.0`, `RTX_Config.h`, `RTX_Config.h.base@5.6.1`
- `csp4cmsis_alt_test/M55_HP/RTE/RTOS/`: `FreeRTOSConfig.h`, `FreeRTOSConfig.h.base@10.7.1`
- `csp4cmsis_pack_test/M55_HP/RTE/RTOS/`: `FreeRTOSConfig.h`, `FreeRTOSConfig.h.base@10.7.1`
- `neuropathway/M55_HP/RTE/RTOS/`: `FreeRTOSConfig.h`, `FreeRTOSConfig.h.base@10.7.1`
- `neuropathway/M55_HP/device/`: `BoardInit.hpp`
- `neuropathway/M55_HP/model/`: `BufAttributes.hpp`, `ethosu_mem_config.h`


- **Derived, original notice not present:** `neuropathway/M55_HP/device/BoardInit.cpp` is adapted
  (through the local `object_detection_e8` project) from `device/alif-ensemble/src/BoardInit.cpp` in
  [Arm-Examples/mlek-cmsis-pack-examples](https://github.com/Arm-Examples/mlek-cmsis-pack-examples),
  Copyright Arm Limited and/or its affiliates, Apache-2.0. The copy here has no licence header; this
  entry records its origin and licence.

RTX_Config and FreeRTOSConfig come from the ARM::CMSIS-RTX and ARM::CMSIS-FreeRTOS packs;
`BoardInit.hpp`, `BufAttributes.hpp` and `ethosu_mem_config.h` from Arm's ML embedded evaluation kit
examples.

## Not third-party code

- Generated by CMSIS-Toolbox: `RTE/_*/RTE_Components.h`, `RTE/_*/Pre_Include_Global.h`, `*.cbuild-*.yml`.
- Fetched at setup time, not in the repository: the ExecuTorch pack (BSD 3-Clause, Meta Platforms), see
  `scripts/fetch_executorch_pack.sh`; Alif SETOOLS (see the README).
