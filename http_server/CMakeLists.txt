cmake_minimum_required(VERSION 3.16)

# Set toolchain before project()
set(CMAKE_TOOLCHAIN_FILE cmake/arm-none-eabi-toolchain.cmake)

project(stm32h753zi-baremetal C ASM)

# MCU specific settings
set(MCU_FAMILY STM32H7)
set(MCU_MODEL STM32H753ZI)

# Compiler flags
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -O2
    -fdata-sections
    -ffunction-sections
    -fno-common
    -fstack-usage
)

# Debug build flags
set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# Include directories
include_directories(inc)

# Source files
set(SOURCES
    src/main.c
    src/startup.s
    src/uart.c
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Linker script
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/stm32h753zi.ld)

# Linker flags - FIXED VERSION
target_link_options(${PROJECT_NAME} PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${PROJECT_NAME}.map
)

# Create hex and bin files
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Building hex and bin files"
)

# Flash target
find_program(OPENOCD openocd)
if(OPENOCD)
    add_custom_target(flash
        COMMAND ${OPENOCD} -f ${CMAKE_SOURCE_DIR}/openocd.cfg 
                          -c "program ${PROJECT_NAME}.bin 0x08000000 verify reset exit"
        DEPENDS ${PROJECT_NAME}
        COMMENT "Flashing target hardware"
    )
endif()

# Debug target
add_custom_target(debug
    COMMAND ${OPENOCD} -f ${CMAKE_SOURCE_DIR}/openocd.cfg
    COMMENT "Starting OpenOCD for debugging"
)

# Clean extra files
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
    "${PROJECT_NAME}.hex;${PROJECT_NAME}.bin;${PROJECT_NAME}.map"
)
