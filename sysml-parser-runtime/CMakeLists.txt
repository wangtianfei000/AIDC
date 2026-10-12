cmake_minimum_required(VERSION 3.21)
project(aidc_sysml_parser_runtime VERSION 0.8.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(NOT DEFINED REFERENCE_ROOT OR NOT DEFINED REFERENCE_BUILD)
    message(FATAL_ERROR "REFERENCE_ROOT and REFERENCE_BUILD are required")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Concurrent Test)

set(REFERENCE_LIB_DIR "${REFERENCE_BUILD}/lib")
set(REFERENCE_LIBS
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
)

add_library(aidc_parser_runtime_core STATIC
    src/ModelSnapshotAdapter.h
    src/ModelSnapshotAdapter.cpp
    src/StandardLibraryIndex.h
    src/StandardLibraryIndex.cpp
    src/AnalysisEngine.h
    src/AnalysisEngine.cpp
)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../sysml-standard-library/2026-08/sysml.library" AIDC_SYSML_LIBRARY_ROOT)
target_compile_definitions(aidc_parser_runtime_core PRIVATE
    ANTLR4CPP_STATIC
    AIDC_DEFAULT_SYSML_LIBRARY_ROOT="${AIDC_SYSML_LIBRARY_ROOT}"
)
target_include_directories(aidc_parser_runtime_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(aidc_parser_runtime_core PRIVATE
    "${REFERENCE_ROOT}"
    "${REFERENCE_ROOT}/parser/src"
    "${REFERENCE_ROOT}/third_party/antlr4/runtime/src"
)
target_link_libraries(aidc_parser_runtime_core PUBLIC
    ${REFERENCE_LIBS}
    Qt6::Gui
    Qt6::Core
)

add_executable(aidc-sysml-parser-runtime
    src/main.cpp
    src/RuntimeServer.h
    src/RuntimeServer.cpp
)
target_link_libraries(aidc-sysml-parser-runtime PRIVATE
    aidc_parser_runtime_core
    Qt6::Network
    Qt6::Concurrent
)

enable_testing()
add_executable(aidc-parser-runtime-tests tests/RuntimeTests.cpp)
target_link_libraries(aidc-parser-runtime-tests PRIVATE
    aidc_parser_runtime_core
    Qt6::Test
)
add_test(NAME aidc-parser-runtime-tests COMMAND aidc-parser-runtime-tests)

install(TARGETS aidc-sysml-parser-runtime RUNTIME DESTINATION bin)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../reference-parser-p0/model-snapshot.schema.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/../reference-parser-p0/analysis-snapshot.schema.json"
    DESTINATION schema
)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/../reference-parser-p0/parser-contract.md"
    DESTINATION doc
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/run-runtime.ps1" DESTINATION .)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../sysml-standard-library/2026-08/sysml.library"
    DESTINATION library
)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../sysml-standard-library/2026-08/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/../sysml-standard-library/2026-08/README.md"
    DESTINATION library
)
