cmake_minimum_required(VERSION 3.21)
project(reference_parser_probe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEFINED REFERENCE_ROOT OR NOT DEFINED REFERENCE_BUILD)
    message(FATAL_ERROR "REFERENCE_ROOT and REFERENCE_BUILD are required")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Test)

add_executable(reference_parser_probe main.cpp)
target_compile_definitions(reference_parser_probe PRIVATE ANTLR4CPP_STATIC)
target_include_directories(reference_parser_probe PRIVATE
    "${REFERENCE_ROOT}"
    "${REFERENCE_ROOT}/parser/src"
    "${REFERENCE_ROOT}/third_party/antlr4/runtime/src"
)

set(REFERENCE_LIB_DIR "${REFERENCE_BUILD}/lib")
target_link_libraries(reference_parser_probe PRIVATE
    "${REFERENCE_LIB_DIR}/libsysml_parser.a"
    "${REFERENCE_LIB_DIR}/libsysml_architecture_metadata.a"
    "${REFERENCE_LIB_DIR}/libsysml_core.a"
    "${REFERENCE_LIB_DIR}/libsysml_infrastructure.a"
    "${REFERENCE_LIB_DIR}/libsysml_model.a"
    "${REFERENCE_LIB_DIR}/libsysml_common.a"
    "${REFERENCE_LIB_DIR}/libantlr4-runtime-static.a"
    "${REFERENCE_LIB_DIR}/libsysml_parser.a"
    "${REFERENCE_LIB_DIR}/libsysml_architecture_metadata.a"
    "${REFERENCE_LIB_DIR}/libsysml_core.a"
    "${REFERENCE_LIB_DIR}/libsysml_infrastructure.a"
    "${REFERENCE_LIB_DIR}/libsysml_model.a"
    "${REFERENCE_LIB_DIR}/libsysml_common.a"
    Qt6::Gui
    Qt6::Core
)

# Compile the reference repository's parser test without changing its source tree.
add_executable(reference_parser_unit
    "${REFERENCE_ROOT}/parser/tests/test_parser.cpp"
)
target_compile_definitions(reference_parser_unit PRIVATE ANTLR4CPP_STATIC)
target_include_directories(reference_parser_unit PRIVATE
    "${REFERENCE_ROOT}"
    "${REFERENCE_ROOT}/parser/src"
    "${REFERENCE_ROOT}/third_party/antlr4/runtime/src"
)
target_link_libraries(reference_parser_unit PRIVATE
    "${REFERENCE_LIB_DIR}/libsysml_parser.a"
    "${REFERENCE_LIB_DIR}/libsysml_architecture_metadata.a"
    "${REFERENCE_LIB_DIR}/libsysml_core.a"
    "${REFERENCE_LIB_DIR}/libsysml_infrastructure.a"
    "${REFERENCE_LIB_DIR}/libsysml_model.a"
    "${REFERENCE_LIB_DIR}/libsysml_common.a"
    "${REFERENCE_LIB_DIR}/libantlr4-runtime-static.a"
    "${REFERENCE_LIB_DIR}/libsysml_parser.a"
    "${REFERENCE_LIB_DIR}/libsysml_architecture_metadata.a"
    "${REFERENCE_LIB_DIR}/libsysml_core.a"
    "${REFERENCE_LIB_DIR}/libsysml_infrastructure.a"
    "${REFERENCE_LIB_DIR}/libsysml_model.a"
    "${REFERENCE_LIB_DIR}/libsysml_common.a"
    Qt6::Test
    Qt6::Gui
    Qt6::Core
)
