cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# -Og 提供适度优化且不影响调试，-g3 提供包含宏定义的完整调试信息
set(CMAKE_C_FLAGS_DEBUG "-Og -g3" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g3" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-Os -g0" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0" CACHE STRING "" FORCE)

project(FOC_Bootloader)
message("Build type: " ${CMAKE_BUILD_TYPE})

enable_language(C ASM)

add_executable(${CMAKE_PROJECT_NAME})

set(BOOTLOADER_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${BOOTLOADER_ROOT}/Core/Src/main.c
    ${BOOTLOADER_ROOT}/Core/Src/bootloader.c
    ${BOOTLOADER_ROOT}/Core/Src/stm32f1xx_it.c
    ${PROJECT_ROOT}/Core/Src/system_stm32f1xx.c
    ${PROJECT_ROOT}/Core/Src/sysmem.c
    ${PROJECT_ROOT}/Core/Src/syscalls.c
    ${PROJECT_ROOT}/startup_stm32f103xb.s
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pcd.c
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pcd_ex.c
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_usb.c
    ${PROJECT_ROOT}/USB_DEVICE/App/usb_device.c
    ${PROJECT_ROOT}/USB_DEVICE/App/usbd_desc.c
    ${PROJECT_ROOT}/USB_DEVICE/App/usbd_dfu_if.c
    ${PROJECT_ROOT}/USB_DEVICE/Target/usbd_conf.c
    ${PROJECT_ROOT}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c
    ${PROJECT_ROOT}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
    ${PROJECT_ROOT}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c
    ${PROJECT_ROOT}/Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Src/usbd_dfu.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${BOOTLOADER_ROOT}/Core/Inc
    ${PROJECT_ROOT}/Core/Inc
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Inc
    ${PROJECT_ROOT}/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy
    ${PROJECT_ROOT}/Drivers/CMSIS/Device/ST/STM32F1xx/Include
    ${PROJECT_ROOT}/Drivers/CMSIS/Include
    ${PROJECT_ROOT}/USB_DEVICE/App
    ${PROJECT_ROOT}/USB_DEVICE/Target
    ${PROJECT_ROOT}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    ${PROJECT_ROOT}/Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    STM32F103xB
    HSE_VALUE=8000000U
    USER_VECT_TAB_ADDRESS
    VECT_TAB_OFFSET=0x00000000U
)

set(CMAKE_EXE_LINKER_FLAGS
    "${TARGET_FLAGS} -T \"${BOOTLOADER_ROOT}/STM32F103XX_BOOT.ld\" --specs=nano.specs -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections -Wl,--print-memory-usage"
)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-Os -g3>
        $<$<CONFIG:Release>:-Os -g0>
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE m)
