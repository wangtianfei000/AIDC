set(AIDC_SYSML_GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated")

set(AIDC_SYSML_GENERATED_SOURCES
    generated/SysMLLexer.cpp
    generated/SysMLParser.cpp
    generated/SysMLBaseVisitor.cpp
    generated/SysMLVisitor.cpp
)

add_library(aidc_sysml_parser STATIC
    src/parser/SysMLParser.h
    src/parser/SysMLParser.cpp
    src/visitor/ModelBuilderVisitor.h
    src/visitor/ModelBuilderVisitor.cpp
    ${AIDC_SYSML_GENERATED_SOURCES}
)
target_compile_definitions(aidc_sysml_parser PRIVATE ANTLR4CPP_STATIC)
target_include_directories(aidc_sysml_parser
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
    PRIVATE "${AIDC_SYSML_GENERATED_DIR}"
)
target_link_libraries(aidc_sysml_parser PUBLIC
    aidc_sysml_model_layer
    antlr4_static
    Qt6::Core
)

# Text serialization is kept separate from the runtime parsing path.
add_library(aidc_sysml_serializer STATIC
    src/serializer/SysMLSerializer.h
    src/serializer/SysMLSerializer.cpp
)
target_include_directories(aidc_sysml_serializer
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
)
target_link_libraries(aidc_sysml_serializer PUBLIC
    aidc_sysml_model_layer
    Qt6::Core
)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../P2-IMPORT.json" AIDC_SYSML_P2_IMPORT_JSON)
string(JSON AIDC_SYSML_ENGINE_VERSION GET "${AIDC_SYSML_P2_IMPORT_JSON}" engineVersion)
string(JSON AIDC_SYSML_ENGINE_SOURCE_DIGEST GET "${AIDC_SYSML_P2_IMPORT_JSON}" sourceDigest)
set(AIDC_SYSML_ENGINE_VERSION "${AIDC_SYSML_ENGINE_VERSION}" PARENT_SCOPE)
set(AIDC_SYSML_ENGINE_SOURCE_DIGEST "${AIDC_SYSML_ENGINE_SOURCE_DIGEST}" PARENT_SCOPE)
