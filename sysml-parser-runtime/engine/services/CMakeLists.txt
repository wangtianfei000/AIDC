add_library(aidc_sysml_services STATIC
    ModelSnapshotAdapter.h
    ModelSnapshotAdapter.cpp
    SysMLKernelService.h
    SysMLKernelService.cpp
)
target_compile_definitions(aidc_sysml_services PRIVATE ANTLR4CPP_STATIC)
target_include_directories(aidc_sysml_services PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
)
target_link_libraries(aidc_sysml_services PUBLIC
    aidc_sysml_parser
    aidc_sysml_kernel
    Qt6::Core
)
