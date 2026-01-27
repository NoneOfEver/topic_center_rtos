#此文件从模板自动生成! 请勿更改!
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.22)

# specify cross-compilers and tools
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER  arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# project settings
project(Dust_InfantryRobotChassis C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Uncomment for hardware floating point
add_compile_definitions(ARM_MATH_CM4;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)
add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)

#Uncomment for software floating point
#add_compile_options(-mfloat-abi=soft)

add_compile_options(-mcpu=cortex-m7 -mthumb -mthumb-interwork)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

# 优化方案 2: 禁用 C++ 异常和 RTTI (节省 ~10-15KB)
# 使用 static_pointer_cast 替代 dynamic_pointer_cast 以支持 -fno-rtti
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

# 优化方案 4: 启用链接时优化 LTO (节省 ~10-20KB)
# 注意: 对于大型项目，LTO 可能导致链接错误，可以先禁用测试其他优化效果
# add_compile_options(-flto)
# add_link_options(-flto)

# uncomment to mitigate c++17 absolute addresses warnings
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

# Enable assembler files preprocessing
add_compile_options($<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-Og -g)
endif ()

include_directories(
        Board/dm-h723/
        Board/dm-h723/Inc 
        ThirdParty/STM32H7xx_HAL_Driver/Inc 
        ThirdParty/STM32H7xx_HAL_Driver/Inc/Legacy 
        ThirdParty/FreeRTOS/Source/include 
        ThirdParty/FreeRTOS/Source/CMSIS_RTOS_V2 
        ThirdParty/FreeRTOS/Source/portable/GCC/ARM_CM4F 
        ThirdParty/STM32_USB_Device_Library/Core/Inc 
        ThirdParty/STM32_USB_Device_Library/Class/CDC/Inc 
        ThirdParty/ARM/DSP/Inc
        ThirdParty/CMSIS/Device/ST/STM32H7xx/Include 
        ThirdParty/CMSIS/Include 
)

include_directories(
        ThirdParty/SEGGER/Config
        ThirdParty/SEGGER/RTT
        topic_center
        uorb_lite
)

add_definitions(
    -DDEBUG 
    -DUSE_PWR_LDO_SUPPLY 
    -DUSE_HAL_DRIVER 
    -DSTM32H723xx
)

file(
    GLOB_RECURSE SOURCES 
    "Board/dm-h723/*.*" 
    "ThirdParty/*.*" 
    "topic_center/*.*"
    "main.cpp"
    "worker_task_example.cpp"
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Board/dm-h723/STM32H723VGTX_FLASH.ld)

add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
add_link_options(-mcpu=cortex-m7 -mthumb -mthumb-interwork)

# 优化方案 3: 使用 newlib-nano (节省 ~20-30KB)
add_link_options(--specs=nano.specs)

# 优化方案 4: 启用链接时优化 LTO（暂时禁用，避免链接错误）
# add_link_options(-flto)

add_link_options(-T ${LINKER_SCRIPT})

link_directories(${CMAKE_SOURCE_DIR}/ThirdParty/ARM/DSP/Lib)

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})

target_link_libraries(${PROJECT_NAME}.elf libarm_cortexM7lfsp_math.a)

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE} Building ${BIN_FILE}")
