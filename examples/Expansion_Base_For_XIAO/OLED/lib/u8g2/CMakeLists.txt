cmake_minimum_required(VERSION 3.13)

set(COMPONENT_ADD_INCLUDEDIRS csrc)
file(GLOB COMPONENT_SRCS csrc/*.c)
file(GLOB COMPONENT_HDRS csrc/*.h)

if(COMMAND idf_component_register)
    idf_component_register(SRCS "${COMPONENT_SRCS}" INCLUDE_DIRS csrc)
    return()
endif()

project(u8g2)

if(COMMAND register_component)
    set(COMPONENT_NAME ".")
    register_component()
else()
    add_library(u8g2 ${COMPONENT_SRCS})
    target_include_directories(u8g2 PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/csrc>" "$<INSTALL_INTERFACE:include>")
     target_compile_options(u8g2 PRIVATE
        -mcpu=cortex-m4       # Specify Cortex-M4 CPU
        -mthumb               # Use Thumb instruction set
        -mfloat-abi=hard      # Use hard-float ABI (hardware floating point)
        -mfpu=fpv4-sp-d16     # Specify FPU (Floating Point Unit)
        -fno-builtin          # Disable some built-in functions
        -fno-strict-aliasing  # Disable strict aliasing rules
        -fno-common           # Place uninitialized globals in BSS
    )
endif()

install(TARGETS u8g2
        EXPORT u8g2-targets
        ARCHIVE DESTINATION lib)

install(FILES ${COMPONENT_HDRS}
        DESTINATION include/u8g2)

install(EXPORT u8g2-targets
        DESTINATION lib/cmake/u8g2)

install(FILES ${u8g2_SOURCE_DIR}/pkg/u8g2-config.cmake
        DESTINATION lib/cmake/u8g2)
