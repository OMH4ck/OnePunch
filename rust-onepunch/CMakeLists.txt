cmake_minimum_required(VERSION 3.14)

project(RustOnePunch)

# Set the directory where Cargo will build the Rust library
set(RUST_TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/target)

# Determine the target directory based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(RUST_LIB_DIR ${RUST_TARGET_DIR}/debug)
else()
    set(RUST_LIB_DIR ${RUST_TARGET_DIR}/release)
    set(CARGO_BUILD_FLAGS "--release")
endif()

# Name of the static library produced by Cargo
set(RUST_LIB_NAME rust_onepunch)
set(RUST_STATIC_LIB ${RUST_LIB_DIR}/lib${RUST_LIB_NAME}.a)

# Custom target to build the Rust library
add_custom_target(
    rust_onepunch_build
    COMMAND cargo build ${CARGO_BUILD_FLAGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Rust library"
)

# Custom command to ensure the static library is built
add_custom_command(
    OUTPUT ${RUST_STATIC_LIB}
    COMMAND cargo build ${CARGO_BUILD_FLAGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Compiling Rust static library"
)

# Create an IMPORTED library target
add_library(rust_onepunch_static STATIC IMPORTED GLOBAL)
add_dependencies(rust_onepunch_static rust_onepunch_build)

# Set the properties of the imported library
set_target_properties(rust_onepunch_static PROPERTIES
    IMPORTED_LOCATION ${RUST_STATIC_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
)

# Make sure the library file is actually there when linking
add_dependencies(rust_onepunch_static rust_onepunch_build)

# Ensure the bindings.h header is available
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/bindings.h
    DEPENDS ${RUST_STATIC_LIB}
    COMMENT "Rust bindings header is ready"
)

add_custom_target(rust_bindings DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bindings.h)
add_dependencies(rust_onepunch_static rust_bindings)