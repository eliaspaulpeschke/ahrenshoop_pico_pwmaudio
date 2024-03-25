# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/unheil/pico/pico-sdk")

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(audiopwm C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_library(adsr INTERFACE)
target_sources(adsr INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/adsr.c
        )
target_include_directories(adsr INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        )
target_link_libraries(adsr INTERFACE
        pico_stdlib
        )

add_library(filter INTERFACE)
target_sources(filter INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/filter.c
        )
target_include_directories(filter INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        )
target_link_libraries(filter INTERFACE
        pico_stdlib
        )

add_executable(audiopwm audiopwm.c )
pico_generate_pio_header(audiopwm ${CMAKE_CURRENT_LIST_DIR}/audio_pwm.pio)
target_sources(audiopwm PRIVATE audiopwm.c)


pico_set_program_name(audiopwm "audiopwm")
pico_set_program_version(audiopwm "0.1")

pico_enable_stdio_uart(audiopwm 1)
pico_enable_stdio_usb(audiopwm 1)

# Add the standard library to the build
target_link_libraries(audiopwm
        pico_stdlib
        adsr filter)

# Add the standard include files to the build
target_include_directories(audiopwm PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(audiopwm 
        hardware_dma
        hardware_pio
        hardware_adc
        )

pico_add_extra_outputs(audiopwm)

