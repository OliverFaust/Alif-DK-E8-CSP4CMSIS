# components.cmake

# component ARM::CMSIS-Compiler:CORE@1.2.1
add_library(ARM_CMSIS-Compiler_CORE_1_2_1 OBJECT
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-Compiler/2.3.0/source/core/gcc/retarget_syscalls.c"
)
target_include_directories(ARM_CMSIS-Compiler_CORE_1_2_1 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(ARM_CMSIS-Compiler_CORE_1_2_1 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(ARM_CMSIS-Compiler_CORE_1_2_1 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(ARM_CMSIS-Compiler_CORE_1_2_1 PUBLIC
  ${CONTEXT}_ABSTRACTIONS
)

# component ARM::CMSIS-Compiler:STDOUT:Custom@1.1.0
add_library(ARM_CMSIS-Compiler_STDOUT_Custom_1_1_0 INTERFACE)
target_include_directories(ARM_CMSIS-Compiler_STDOUT_Custom_1_1_0 INTERFACE
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-Compiler/2.3.0/include"
)
target_compile_definitions(ARM_CMSIS-Compiler_STDOUT_Custom_1_1_0 INTERFACE
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_link_libraries(ARM_CMSIS-Compiler_STDOUT_Custom_1_1_0 INTERFACE
  ${CONTEXT}_ABSTRACTIONS
)

# component ARM::CMSIS:CORE@6.2.0
add_library(ARM_CMSIS_CORE_6_2_0 INTERFACE)
target_include_directories(ARM_CMSIS_CORE_6_2_0 INTERFACE
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${CMSIS_PACK_ROOT}/ARM/CMSIS/6.3.0/CMSIS/Core/Include"
)
target_compile_definitions(ARM_CMSIS_CORE_6_2_0 INTERFACE
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_link_libraries(ARM_CMSIS_CORE_6_2_0 INTERFACE
  ${CONTEXT}_ABSTRACTIONS
)

# component ARM::CMSIS:OS Tick:SysTick@1.0.5
add_library(ARM_CMSIS_OS_Tick_SysTick_1_0_5 OBJECT
  "${CMSIS_PACK_ROOT}/ARM/CMSIS/6.3.0/CMSIS/RTOS2/Source/os_systick.c"
)
target_include_directories(ARM_CMSIS_OS_Tick_SysTick_1_0_5 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${CMSIS_PACK_ROOT}/ARM/CMSIS/6.3.0/CMSIS/RTOS2/Include"
)
target_compile_definitions(ARM_CMSIS_OS_Tick_SysTick_1_0_5 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(ARM_CMSIS_OS_Tick_SysTick_1_0_5 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(ARM_CMSIS_OS_Tick_SysTick_1_0_5 PUBLIC
  ${CONTEXT}_ABSTRACTIONS
)

# component ARM::CMSIS:RTOS2:Keil RTX5&Source@5.9.1
add_library(ARM_CMSIS_RTOS2_Keil_RTX5_Source_5_9_1 OBJECT
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/GCC/irq_armv8mml.S"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/rtx_delay.c"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/rtx_evflags.c"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/rtx_evr.c"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/rtx_kernel.c"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/rtx_lib.c"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/rtx_memory.c"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/rtx_mempool.c"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/rtx_msgqueue.c"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/rtx_mutex.c"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/rtx_semaphore.c"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/rtx_system.c"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/rtx_thread.c"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/rtx_timer.c"
  "${SOLUTION_ROOT}/M55_HP/RTE/CMSIS/RTX_Config.c"
)
target_include_directories(ARM_CMSIS_RTOS2_Keil_RTX5_Source_5_9_1 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${SOLUTION_ROOT}/M55_HP/RTE/CMSIS"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Include"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS/6.3.0/CMSIS/RTOS2/Include"
)
target_compile_definitions(ARM_CMSIS_RTOS2_Keil_RTX5_Source_5_9_1 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(ARM_CMSIS_RTOS2_Keil_RTX5_Source_5_9_1 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(ARM_CMSIS_RTOS2_Keil_RTX5_Source_5_9_1 PUBLIC
  ${CONTEXT}_ABSTRACTIONS
)
set_source_files_properties("${CMSIS_PACK_ROOT}/ARM/CMSIS-RTX/5.9.1/Source/GCC/irq_armv8mml.S" PROPERTIES
  COMPILE_DEFINITIONS "RTSS_HP;_RTE_"
)

# component AlifSemiconductor::BSP:Board Config@2.2.0
add_library(AlifSemiconductor_BSP_Board_Config_2_2_0 OBJECT
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/libs/board_config/board_config.c"
)
target_include_directories(AlifSemiconductor_BSP_Board_Config_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/libs/board_config"
)
target_compile_definitions(AlifSemiconductor_BSP_Board_Config_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(AlifSemiconductor_BSP_Board_Config_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(AlifSemiconductor_BSP_Board_Config_2_2_0 PUBLIC
  ${CONTEXT}_ABSTRACTIONS
)

# component AlifSemiconductor::BSP:DevKit Config&DevKit-e8@2.2.0
add_library(AlifSemiconductor_BSP_DevKit_Config_DevKit-e8_2_2_0 INTERFACE)
target_include_directories(AlifSemiconductor_BSP_DevKit_Config_DevKit-e8_2_2_0 INTERFACE
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${SOLUTION_ROOT}/M55_HP/RTE/BSP/AE822FA0E5597LS0_M55_HP"
)
target_compile_definitions(AlifSemiconductor_BSP_DevKit_Config_DevKit-e8_2_2_0 INTERFACE
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_link_libraries(AlifSemiconductor_BSP_DevKit_Config_DevKit-e8_2_2_0 INTERFACE
  ${CONTEXT}_ABSTRACTIONS
)

# component AlifSemiconductor::CMSIS Driver:USART@2.2.0
add_library(AlifSemiconductor_CMSIS_Driver_USART_2_2_0 OBJECT
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Alif_CMSIS/Source/Driver_USART.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/drivers/source/uart.c"
)
target_include_directories(AlifSemiconductor_CMSIS_Driver_USART_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Alif_CMSIS/Include"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Alif_CMSIS/Source"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/drivers/include"
  "${CMSIS_PACK_ROOT}/ARM/CMSIS/6.3.0/CMSIS/Driver/Include"
)
target_compile_definitions(AlifSemiconductor_CMSIS_Driver_USART_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(AlifSemiconductor_CMSIS_Driver_USART_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(AlifSemiconductor_CMSIS_Driver_USART_2_2_0 PUBLIC
  ${CONTEXT}_ABSTRACTIONS
)

# component AlifSemiconductor::Device:SOC Peripherals:GPIO@2.2.0
add_library(AlifSemiconductor_Device_SOC_Peripherals_GPIO_2_2_0 OBJECT
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Alif_CMSIS/Source/Driver_IO_Private.h"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Alif_CMSIS/Source/Driver_IO.c"
)
target_include_directories(AlifSemiconductor_Device_SOC_Peripherals_GPIO_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Alif_CMSIS/Include"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/drivers/include"
)
target_compile_definitions(AlifSemiconductor_Device_SOC_Peripherals_GPIO_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(AlifSemiconductor_Device_SOC_Peripherals_GPIO_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(AlifSemiconductor_Device_SOC_Peripherals_GPIO_2_2_0 PUBLIC
  ${CONTEXT}_ABSTRACTIONS
)

# component AlifSemiconductor::Device:SOC Peripherals:MHU@1.110.0
add_library(AlifSemiconductor_Device_SOC_Peripherals_MHU_1_110_0 OBJECT
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/drivers/source/mhu_driver.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/drivers/source/mhu_receiver.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/drivers/source/mhu_sender.c"
)
target_include_directories(AlifSemiconductor_Device_SOC_Peripherals_MHU_1_110_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/drivers/include"
)
target_compile_definitions(AlifSemiconductor_Device_SOC_Peripherals_MHU_1_110_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(AlifSemiconductor_Device_SOC_Peripherals_MHU_1_110_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(AlifSemiconductor_Device_SOC_Peripherals_MHU_1_110_0 PUBLIC
  ${CONTEXT}_ABSTRACTIONS
)

# component AlifSemiconductor::Device:SOC Peripherals:PINCONF@2.2.0
add_library(AlifSemiconductor_Device_SOC_Peripherals_PINCONF_2_2_0 OBJECT
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/drivers/source/pinconf.c"
)
target_include_directories(AlifSemiconductor_Device_SOC_Peripherals_PINCONF_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/drivers/include"
)
target_compile_definitions(AlifSemiconductor_Device_SOC_Peripherals_PINCONF_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(AlifSemiconductor_Device_SOC_Peripherals_PINCONF_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(AlifSemiconductor_Device_SOC_Peripherals_PINCONF_2_2_0 PUBLIC
  ${CONTEXT}_ABSTRACTIONS
)

# component AlifSemiconductor::Device:Startup@2.2.0
add_library(AlifSemiconductor_Device_Startup_2_2_0 OBJECT
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/core/common/source/cache.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/core/common/source/mpu.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/core/common/source/pm.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/core/common/source/sau_tcm_ns_setup.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/core/common/source/startup.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/core/common/source/system.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/core/common/source/tgu.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/core/common/source/vectors.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/system/source/sys_clocks.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/system/source/sys_utils.c"
)
target_include_directories(AlifSemiconductor_Device_Startup_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${SOLUTION_ROOT}/M55_HP/RTE/Device/AE822FA0E5597LS0_M55_HP"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/core/common/include"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/soc/AE822FA0E5597/include"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/soc/AE822FA0E5597/include/rtss_hp"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/Device/system/include"
)
target_compile_definitions(AlifSemiconductor_Device_Startup_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(AlifSemiconductor_Device_Startup_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(AlifSemiconductor_Device_Startup_2_2_0 PUBLIC
  ${CONTEXT}_ABSTRACTIONS
)

# component AlifSemiconductor::Services:Retarget IO:STDOUT@2.2.0
add_library(AlifSemiconductor_Services_Retarget_IO_STDOUT_2_2_0 OBJECT
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/libs/retarget_io/source/stdout_USART.c"
)
target_include_directories(AlifSemiconductor_Services_Retarget_IO_STDOUT_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${SOLUTION_ROOT}/M55_HP/RTE/Services/AE822FA0E5597LS0_M55_HP"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/libs/retarget_io/include"
)
target_compile_definitions(AlifSemiconductor_Services_Retarget_IO_STDOUT_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(AlifSemiconductor_Services_Retarget_IO_STDOUT_2_2_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(AlifSemiconductor_Services_Retarget_IO_STDOUT_2_2_0 PUBLIC
  ${CONTEXT}_ABSTRACTIONS
)

# component AlifSemiconductor::Services:Secure Enclave:Initialization Helper&Source@1.110.0
add_library(AlifSemiconductor_Services_Secure_Enclave_Initialization_Helper_Source_1_110_0 OBJECT
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/port/clock_runtime.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/port/se_services_port.c"
)
target_include_directories(AlifSemiconductor_Services_Secure_Enclave_Initialization_Helper_Source_1_110_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/port/include"
)
target_compile_definitions(AlifSemiconductor_Services_Secure_Enclave_Initialization_Helper_Source_1_110_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(AlifSemiconductor_Services_Secure_Enclave_Initialization_Helper_Source_1_110_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(AlifSemiconductor_Services_Secure_Enclave_Initialization_Helper_Source_1_110_0 PUBLIC
  ${CONTEXT}_ABSTRACTIONS
)

# component AlifSemiconductor::Services:Secure Enclave:core&Source@1.110.0
add_library(AlifSemiconductor_Services_Secure_Enclave_core_Source_1_110_0 OBJECT
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/source/services_host_application.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/source/services_host_boot.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/source/services_host_clocks.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/source/services_host_cryptocell.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/source/services_host_error.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/source/services_host_extsys0.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/source/services_host_handler.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/source/services_host_maintenance.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/source/services_host_padcontrol.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/source/services_host_pinmux.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/source/services_host_power.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/source/services_host_system.c"
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/source/services_host_update.c"
)
target_include_directories(AlifSemiconductor_Services_Secure_Enclave_core_Source_1_110_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${CMSIS_PACK_ROOT}/AlifSemiconductor/Ensemble/2.2.0/se_services/include"
)
target_compile_definitions(AlifSemiconductor_Services_Secure_Enclave_core_Source_1_110_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(AlifSemiconductor_Services_Secure_Enclave_core_Source_1_110_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(AlifSemiconductor_Services_Secure_Enclave_core_Source_1_110_0 PUBLIC
  ${CONTEXT}_ABSTRACTIONS
)

# component OliverFaust::CSP4CMSIS:Core@1.0.0
add_library(OliverFaust_CSP4CMSIS_Core_1_0_0 OBJECT
  "${CMSIS_PACK_ROOT}/OliverFaust/CSP4CMSIS/1.0.0/csp4cmsis/src/alt_channel_sync.cpp"
  "${CMSIS_PACK_ROOT}/OliverFaust/CSP4CMSIS/1.0.0/csp4cmsis/src/alternative.cpp"
  "${CMSIS_PACK_ROOT}/OliverFaust/CSP4CMSIS/1.0.0/csp4cmsis/src/barrier.cpp"
  "${CMSIS_PACK_ROOT}/OliverFaust/CSP4CMSIS/1.0.0/csp4cmsis/src/buffered_channel.cpp"
  "${CMSIS_PACK_ROOT}/OliverFaust/CSP4CMSIS/1.0.0/csp4cmsis/src/channel_sync.cpp"
  "${CMSIS_PACK_ROOT}/OliverFaust/CSP4CMSIS/1.0.0/csp4cmsis/src/csp_wrapper.cpp"
  "${CMSIS_PACK_ROOT}/OliverFaust/CSP4CMSIS/1.0.0/csp4cmsis/src/glue.cpp"
  "${CMSIS_PACK_ROOT}/OliverFaust/CSP4CMSIS/1.0.0/csp4cmsis/src/sync_channel.cpp"
)
target_include_directories(OliverFaust_CSP4CMSIS_Core_1_0_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
  "${CMSIS_PACK_ROOT}/OliverFaust/CSP4CMSIS/1.0.0/csp4cmsis/inc"
  "${CMSIS_PACK_ROOT}/OliverFaust/CSP4CMSIS/1.0.0/csp4cmsis/inc/csp"
)
target_compile_definitions(OliverFaust_CSP4CMSIS_Core_1_0_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(OliverFaust_CSP4CMSIS_Core_1_0_0 PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(OliverFaust_CSP4CMSIS_Core_1_0_0 PUBLIC
  ${CONTEXT}_ABSTRACTIONS
)
