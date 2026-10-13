# Architecture view metadata library
add_library(sysml_architecture_metadata STATIC
    BaseGraphicMetadata.h
    BaseGraphicMetadata.cpp
    PartBasicViewMetadata.h
    PartBasicViewMetadata.cpp
    PartGraphicMetadata.h
    PartGraphicMetadata.cpp
    PortGraphicMetadata.h
    PortGraphicMetadata.cpp
    ConnectionGraphicMetadata.h
    ConnectionGraphicMetadata.cpp
    BasicScene.h
    BasicScene.cpp
    ElidedTextItem.h
    ElidedTextItem.cpp
    PartNameItem.h
    PartNameItem.cpp
    DocItem.h
    DocItem.cpp
    AttributeItem.h
    AttributeItem.cpp
    ItemItem.h
    ItemItem.cpp
    AttributeEditDialog.h
    AttributeEditDialog.cpp
    PortEditDialog.h
    PortEditDialog.cpp
    ItemEditDialog.h
    ItemEditDialog.cpp
    PortLabelItem.h
    PortLabelItem.cpp
    PortBasicItem.h
    PortBasicItem.cpp
    PartBasicItem.h
    PartBasicItem.cpp
    BasicViewWidget.h
    BasicViewWidget.cpp
    ArchitectureScene.h
    ArchitectureScene.cpp
    PartGraphicsItem.h
    PartGraphicsItem.cpp
    PortGraphicsItem.h
    PortGraphicsItem.cpp
    ConnectionGraphicsItem.h
    ConnectionGraphicsItem.cpp
    GraphicViewWidget.h
    GraphicViewWidget.cpp
)

target_link_libraries(sysml_architecture_metadata
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    sysml_core
    sysml_parser  # 需要 AnnotationParser，使用 PRIVATE 避免传播依赖
    # 注意：虽然 sysml_parser 也链接了 sysml_architecture_metadata，但这是静态库，
    # 链接顺序会确保所有符号都能正确解析，不会造成真正的循环依赖问题
)

# 安装头文件
install(FILES
    BaseGraphicMetadata.h
    PartBasicViewMetadata.h
    PartGraphicMetadata.h
    PortGraphicMetadata.h
    ConnectionGraphicMetadata.h
    ElidedTextItem.h
    PartNameItem.h
    DocItem.h
    AttributeItem.h
    ItemItem.h
    PortLabelItem.h
    BasicScene.h
    PortBasicItem.h
    PartBasicItem.h
    BasicViewWidget.h
    ArchitectureScene.h
    PartGraphicsItem.h
    PortGraphicsItem.h
    ConnectionGraphicsItem.h
    GraphicViewWidget.h
    DESTINATION include/sysml/ui/mid_windows/architecture
)
