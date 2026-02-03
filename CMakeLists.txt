cmake_minimum_required(VERSION 3.18)

# Set toolchain file BEFORE project()
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/arm-none-eabi-gcc.cmake)


project(BlinkLED C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)

# Compiler flags
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fdata-sections -ffunction-sections")

# Add after your compiler flags
#set(CMAKE_C_FLAGS_DEBUG "-g3 -O0 -gdwarf-2")

set(CMAKE_C_FLAGS_DEBUG "-g3 -O0 -gdwarf-2 -Wall -fdata-sections -ffunction-sections")
set(CMAKE_C_FLAGS_RELEASE "-Os -Wall -fdata-sections -ffunction-sections")

# Debug/Release flags
set(CMAKE_C_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_C_FLAGS_RELEASE "-Os")

# Linker flags - you'll need to specify your linker script
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc -lm -lnosys")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

# Add sources
add_executable(${PROJECT_NAME}.elf
    src/main_app.c
    src/tasks/blink_led.c
    src/freertos_static.c

    CubeMX/Core/Src/main.c 
    CubeMX/Core/Src/stm32f4xx_it.c
    CubeMX/Core/Src/stm32f4xx_hal_msp.c
    CubeMX/Core/Src/system_stm32f4xx.c
    CubeMX/Core/Src/stm32f4xx_hal_timebase_tim.c
    

    CubeMX/startup_stm32f407xx.s
    # HAL Driver sources
    CubeMX/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    CubeMX/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    CubeMX/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    CubeMX/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    CubeMX/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
    CubeMX/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    CubeMX/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    CubeMX/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
    CubeMX/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
    

    # FreeRTOS kernel
    CubeMX/Middlewares/Third_Party/FreeRTOS/Source/tasks.c
    CubeMX/Middlewares/Third_Party/FreeRTOS/Source/queue.c
    CubeMX/Middlewares/Third_Party/FreeRTOS/Source/list.c
    CubeMX/Middlewares/Third_Party/FreeRTOS/Source/timers.c
    CubeMX/Middlewares/Third_Party/FreeRTOS/Source/croutine.c
    CubeMX/Middlewares/Third_Party/FreeRTOS/Source/event_groups.c
    CubeMX/Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c
    
    # FreeRTOS portable layer (Cortex-M4 with FPU)
    CubeMX/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c
    
    # FreeRTOS heap implementation (heap_4.c)
    CubeMX/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c

    CubeMX/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c
)

# Add this line after the add_executable command
target_link_options(${PROJECT_NAME}.elf PRIVATE
    -T${CMAKE_SOURCE_DIR}/CubeMX/STM32F407XX_FLASH.ld  
)

# Include directories
target_include_directories(${PROJECT_NAME}.elf PRIVATE
    CubeMX/Drivers/CMSIS/Include
    CubeMX/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    CubeMX/Core/Inc
    CubeMX/Drivers/STM32F4xx_HAL_Driver/Inc

    CubeMX/Middlewares/Third_Party/FreeRTOS/Source/include
    CubeMX/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
    CubeMX/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F

    
    include
)

# Defines
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    STM32F407xx
    USE_HAL_DRIVER
)

# Create hex and bin files
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Building ${PROJECT_NAME}.hex and ${PROJECT_NAME}.bin"
)

