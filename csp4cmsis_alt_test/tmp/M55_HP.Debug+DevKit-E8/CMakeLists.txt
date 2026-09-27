cmake_minimum_required(VERSION 3.27)

# Roots
include("../roots.cmake")

set(CONTEXT M55_HP.Debug+DevKit-E8)
set(TARGET ${CONTEXT})
set(DNAME AE822FA0E5597LS0)
set(PNAME M55_HP)
set(DPACK AlifSemiconductor::Ensemble@2.2.0)
set(DPACK_DIR "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0")
set(OUT_DIR "${SOLUTION_ROOT}/out/M55_HP/DevKit-E8/Debug")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COMPILE_COMMANDS ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json)
set(COMPILE_COMMANDS ${OUT_DIR}/compile_commands.json)
set(COMPILE_MACROS_CXX ${OUT_DIR}/compile_macros_cxx.h)
set(COMPILE_MACROS_C ${OUT_DIR}/compile_macros_c.h)
set(BIN_FILE "M55_HP.bin")
set(HEX_FILE "M55_HP.hex")
set(LD_MAP_FILE "M55_HP.elf.map")
set(LD_SCRIPT "${SOLUTION_ROOT}/M55_HP/RTE/Device/AE822FA0E5597LS0_M55_HP/linker_gnu_mram.ld.src")
set(LD_SCRIPT_PP "${CMAKE_CURRENT_BINARY_DIR}/linker_gnu_mram.ld")

# Processor Options
set(CPU Cortex-M55)
set(FPU DP_FPU)
set(DSP DSP)
set(MVE FP_MVE)
set(BYTE_ORDER Little-endian)

# Preprocessor options
set(CPP_OPTIONS_CXX "-xc++" "-std=gnu++17" "-fno-exceptions" "-fno-rtti" "-masm-syntax-unified" "-fomit-frame-pointer" "-fdata-sections")
set(CPP_OPTIONS_C "-xc" "-std=gnu11" "-masm-syntax-unified" "-fomit-frame-pointer" "-fdata-sections")

# Toolchain config map
set(COMPILER GCC)
include("toolchain.cmake")

# Setup project
project(${CONTEXT} LANGUAGES CXX C ASM)

# Enable color diagnostics
set(CMAKE_COLOR_DIAGNOSTICS ON)

# Compilation database
add_custom_target(database DEPENDS ${COMPILE_COMMANDS} ${COMPILE_MACROS_CXX} ${COMPILE_MACROS_C})
add_custom_command(OUTPUT ${COMPILE_COMMANDS}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_COMPILE_COMMANDS}" "${COMPILE_COMMANDS}"
  DEPENDS "${CMAKE_COMPILE_COMMANDS}"
)
add_custom_command(OUTPUT ${COMPILE_MACROS_CXX} ${COMPILE_MACROS_C}
  COMMAND ${CPP} ${CPP_OPTIONS_CXX} ${CPP_DUMP_MACROS} "${COMPILE_MACROS_CXX}"
  COMMAND ${CPP} ${CPP_OPTIONS_C} ${CPP_DUMP_MACROS} "${COMPILE_MACROS_C}"
)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})

# Setup context
add_executable(${CONTEXT})
set_target_properties(${CONTEXT} PROPERTIES PREFIX "" SUFFIX ".elf" OUTPUT_NAME "M55_HP")
set_target_properties(${CONTEXT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
add_library(${CONTEXT}_GLOBAL INTERFACE)

# Includes
target_include_directories(${CONTEXT} PUBLIC
  "${SOLUTION_ROOT}/M55_HP/RTE/_Debug_DevKit-E8"
  "${SOLUTION_ROOT}/M55_HP/RTE/BSP/AE822FA0E5597LS0_M55_HP"
  "${SOLUTION_ROOT}/M55_HP/RTE/CMSIS"
  "${SOLUTION_ROOT}/M55_HP/RTE/Device/AE822FA0E5597LS0_M55_HP"
  "${SOLUTION_ROOT}/M55_HP/RTE/Services/AE822FA0E5597LS0_M55_HP"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-Compiler/2.3.0/include"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Include"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS/6.3.0/CMSIS/Core/Include"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS/6.3.0/CMSIS/Driver/Include"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS/6.3.0/CMSIS/RTOS2/Include"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Alif_CMSIS/Include"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Alif_CMSIS/Source"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/core/common/include"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/soc/AE822FA0E5597/include"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/soc/AE822FA0E5597/include/rtss_hp"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/system/include"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/drivers/include"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/libs/board_config"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/libs/retarget_io/include"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/include"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/port/include"
  "${CMSIS_PACK_ROOT}/OliverFaust/CSP4CMSIS/1.0.0/csp4cmsis/inc"
  "${CMSIS_PACK_ROOT}/OliverFaust/CSP4CMSIS/1.0.0/csp4cmsis/inc/csp"
  "${SOLUTION_ROOT}/M55_HP"
)

# Defines
target_compile_definitions(${CONTEXT} PUBLIC
  $<$<COMPILE_LANGUAGE:C,CXX>:
    LIB_CSP4CMSIS
    CSP4CMSIS_MAX_SYSCALL_INTERRUPT_PRIORITY=128
    CSP4CMSIS_STATIC_ALLOCATION
    CSP4CMSIS_RTOS2_BACKEND_RTX5
    RTSS_HP
    _RTE_
  >
)

# Compile options
target_compile_options(${CONTEXT} PUBLIC
  $<$<COMPILE_LANGUAGE:ASM>:
    "SHELL:${ASM_CPU}"
    "SHELL:${ASM_FLAGS}"
    "SHELL:${ASM_BYTE_ORDER}"
  >
  $<$<COMPILE_LANGUAGE:C>:
    "SHELL:${CC_CPU}"
    "SHELL:${CC_FLAGS}"
    "SHELL:${CC_BYTE_ORDER}"
    "SHELL:-std=gnu11"
    "SHELL:-masm-syntax-unified"
    "SHELL:-fomit-frame-pointer"
    "SHELL:-fdata-sections"
  >
  $<$<COMPILE_LANGUAGE:CXX>:
    "SHELL:${CXX_CPU}"
    "SHELL:${CXX_FLAGS}"
    "SHELL:${CXX_BYTE_ORDER}"
    "SHELL:-std=gnu++17"
    "SHELL:-fno-exceptions"
    "SHELL:-fno-rtti"
    "SHELL:-masm-syntax-unified"
    "SHELL:-fomit-frame-pointer"
    "SHELL:-fdata-sections"
  >
)

# Compile Options Abstractions
add_library(${CONTEXT}_ABSTRACTIONS INTERFACE)
cbuild_set_options_flags(CXX "none" "on" "" "" CXX_OPTIONS_FLAGS_${CONTEXT})
cbuild_set_options_flags(CC "none" "on" "" "" CC_OPTIONS_FLAGS_${CONTEXT})
cbuild_set_options_flags(ASM "none" "on" "" "" ASM_OPTIONS_FLAGS_${CONTEXT})
target_compile_options(${CONTEXT}_ABSTRACTIONS INTERFACE
  $<$<COMPILE_LANGUAGE:CXX>:
    "SHELL:${CXX_OPTIONS_FLAGS_${CONTEXT}}"
  >
  $<$<COMPILE_LANGUAGE:C>:
    "SHELL:${CC_OPTIONS_FLAGS_${CONTEXT}}"
  >
  $<$<COMPILE_LANGUAGE:ASM>:
    "SHELL:${ASM_OPTIONS_FLAGS_${CONTEXT}}"
  >
)

# Add groups and components
include("groups.cmake")
include("components.cmake")

target_link_libraries(${CONTEXT} PUBLIC
  Group_Source_Files
  Group_Application
  ARM_CMSIS-Compiler_CORE_1_2_1
  ARM_CMSIS-Compiler_STDOUT_Custom_1_1_0
  ARM_CMSIS_CORE_6_2_0
  ARM_CMSIS_OS_Tick_SysTick_1_0_5
  ARM_CMSIS_RTOS2_Keil_RTX5_Source_5_9_1
  AlifSemiconductor_BSP_Board_Config_2_2_0
  AlifSemiconductor_BSP_DevKit_Config_DevKit-e8_2_2_0
  AlifSemiconductor_CMSIS_Driver_USART_2_2_0
  AlifSemiconductor_Device_SOC_Peripherals_GPIO_2_2_0
  AlifSemiconductor_Device_SOC_Peripherals_MHU_1_110_0
  AlifSemiconductor_Device_SOC_Peripherals_PINCONF_2_2_0
  AlifSemiconductor_Device_Startup_2_2_0
  AlifSemiconductor_Services_Retarget_IO_STDOUT_2_2_0
  AlifSemiconductor_Services_Secure_Enclave_Initialization_Helper_Source_1_110_0
  AlifSemiconductor_Services_Secure_Enclave_core_Source_1_110_0
  OliverFaust_CSP4CMSIS_Core_1_0_0
)

# Linker options
target_link_options(${CONTEXT} PUBLIC
  "SHELL:${LD_CPU}"
  "SHELL:${_LS}\"${LD_SCRIPT_PP}\""
  "SHELL:${LD_MAP}"
  "SHELL:--specs=nano.specs"
  "SHELL:--specs=nosys.specs"
  "SHELL:-Wl,-print-memory-usage"
  "SHELL:-Wl,--gc-sections"
  "SHELL:-Wl,--no-warn-rwx-segments"
)
set(LD_DEPS ${LD_SCRIPT})
set_target_properties(${CONTEXT} PROPERTIES LINK_DEPENDS "${LD_DEPS}")

# Linker script pre-processing
add_custom_command(TARGET ${CONTEXT} PRE_LINK COMMAND ${CPP} ARGS ${CPP_ARGS_LD_SCRIPT} BYPRODUCTS ${LD_SCRIPT_PP})

# Bin Conversion
add_custom_command(TARGET ${CONTEXT} POST_BUILD COMMAND ${CMAKE_OBJCOPY} ${ELF2BIN})

# Hex Conversion
add_custom_command(TARGET ${CONTEXT} POST_BUILD COMMAND ${CMAKE_OBJCOPY} ${ELF2HEX})
