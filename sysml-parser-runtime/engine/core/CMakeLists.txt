file(GLOB AIDC_SYSML_COMMON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/*.h"
)
file(GLOB AIDC_SYSML_MODEL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/model/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/model/*.h"
)
file(GLOB AIDC_SYSML_INFRASTRUCTURE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/*.h"
)

add_library(aidc_sysml_common STATIC ${AIDC_SYSML_COMMON_SOURCES})
target_link_libraries(aidc_sysml_common PUBLIC Qt6::Core)
target_include_directories(aidc_sysml_common PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)

add_library(aidc_sysml_model STATIC ${AIDC_SYSML_MODEL_SOURCES})
target_link_libraries(aidc_sysml_model PUBLIC aidc_sysml_common Qt6::Core)
target_include_directories(aidc_sysml_model PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)

add_library(aidc_sysml_infrastructure STATIC ${AIDC_SYSML_INFRASTRUCTURE_SOURCES})
target_link_libraries(aidc_sysml_infrastructure PUBLIC aidc_sysml_model aidc_sysml_common Qt6::Core)
target_include_directories(aidc_sysml_infrastructure PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)

add_library(aidc_sysml_model_layer INTERFACE)
target_link_libraries(aidc_sysml_model_layer INTERFACE
    aidc_sysml_infrastructure
    aidc_sysml_model
    aidc_sysml_common
)
