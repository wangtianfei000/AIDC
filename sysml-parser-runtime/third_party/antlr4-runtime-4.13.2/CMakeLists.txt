file(GLOB_RECURSE AIDC_ANTLR4_RUNTIME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/src/*.h"
)

add_library(antlr4_static STATIC ${AIDC_ANTLR4_RUNTIME_SOURCES})
target_compile_definitions(antlr4_static PUBLIC ANTLR4CPP_STATIC)
target_include_directories(antlr4_static PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime/src>"
)
set_target_properties(antlr4_static PROPERTIES OUTPUT_NAME "antlr4-runtime-static")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(antlr4_static PRIVATE -Wno-overloaded-virtual -Wno-multichar)
elseif(MSVC)
    target_compile_options(antlr4_static PRIVATE /wd4251)
endif()
