cmake_minimum_required(VERSION 3.21)
project(aidc_sysml_parser_runtime VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(AIDC_BUILD_RUNTIME "Build the SysML parser runtime" ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

add_subdirectory(third_party/antlr4-runtime-4.13.2)
add_subdirectory(engine)

enable_testing()
add_executable(aidc-p1-embedded-sources-tests tests/P1EmbeddedSourcesTests.cpp)
target_link_libraries(aidc-p1-embedded-sources-tests PRIVATE
    aidc_sysml_model_layer
    antlr4_static
    Qt6::Test
)
add_test(NAME aidc-p1-embedded-sources-tests COMMAND aidc-p1-embedded-sources-tests)

add_executable(aidc-p2-embedded-parser-tests tests/P2EmbeddedParserTests.cpp)
target_link_libraries(aidc-p2-embedded-parser-tests PRIVATE
    aidc_sysml_parser
    Qt6::Test
)
add_test(NAME aidc-p2-embedded-parser-tests COMMAND aidc-p2-embedded-parser-tests)

find_package(Python3 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME aidc-p1-import-integrity
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_p1_import.py"
    )
    add_test(
        NAME aidc-p2-import-integrity
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_p2_import.py"
    )
    add_test(
        NAME aidc-embedded-source-boundaries
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_embedded_source_boundaries.py"
    )
    add_test(
        NAME aidc-p4-self-contained
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_p4_self_contained.py"
    )
endif()

if(AIDC_BUILD_RUNTIME)
find_package(Qt6 REQUIRED COMPONENTS Gui Network Concurrent)

add_library(aidc_parser_runtime_core STATIC
    src/ModelSnapshotAdapter.h
    src/ModelSnapshotAdapter.cpp
    src/StandardLibraryIndex.h
    src/StandardLibraryIndex.cpp
    src/AnalysisEngine.h
    src/AnalysisEngine.cpp
)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/standard-library/2026-08/sysml.library" AIDC_SYSML_LIBRARY_ROOT)
target_compile_definitions(aidc_parser_runtime_core PRIVATE
    ANTLR4CPP_STATIC
    AIDC_DEFAULT_SYSML_LIBRARY_ROOT="${AIDC_SYSML_LIBRARY_ROOT}"
)
target_include_directories(aidc_parser_runtime_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
string(SUBSTRING "${AIDC_SYSML_ENGINE_SOURCE_DIGEST}" 0 16 AIDC_SYSML_ENGINE_DIGEST_SHORT)
target_compile_definitions(aidc_parser_runtime_core PUBLIC
    AIDC_PARSER_MODE="embedded-antlr"
    AIDC_PARSER_BASELINE="aidc-engine/${AIDC_SYSML_ENGINE_VERSION}+sha256:${AIDC_SYSML_ENGINE_DIGEST_SHORT}"
)
target_link_libraries(aidc_parser_runtime_core PUBLIC
    aidc_sysml_parser
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

if(Python3_Interpreter_FOUND)
    add_test(
        NAME aidc-p2-embedded-http-smoke
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/p2_embedded_http_smoke.py"
            --runtime "$<TARGET_FILE:aidc-sysml-parser-runtime>"
    )
endif()

add_executable(aidc-parser-runtime-tests tests/RuntimeTests.cpp)
target_link_libraries(aidc-parser-runtime-tests PRIVATE
    aidc_parser_runtime_core
    Qt6::Test
)
add_test(NAME aidc-parser-runtime-tests COMMAND aidc-parser-runtime-tests)

install(TARGETS aidc-sysml-parser-runtime RUNTIME DESTINATION bin)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/contracts/model-snapshot.schema.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/contracts/analysis-snapshot.schema.json"
    DESTINATION schema
)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/contracts/parser-contract.md"
    DESTINATION doc
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/run-runtime.ps1" DESTINATION .)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/standard-library/2026-08/sysml.library"
    DESTINATION library
)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/standard-library/2026-08/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/standard-library/2026-08/README.md"
    DESTINATION library
)
endif()
