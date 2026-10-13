#ifndef GRAPHICSITEMTYPES_H
#define GRAPHICSITEMTYPES_H

#include <QGraphicsItem>

SYSML_NAMESPACE_BEGIN

/**
 * @brief GraphicsItemTypes 图形视图中的所有图形项类型枚举
 * 
 * 统一管理所有继承自 QGraphicsItem 的图形项类型
 * 使用枚举确保类型值的唯一性和规范性
 */
namespace GraphicsItemTypes {
    enum Type {
        // 架构视图 (Architecture View) 图形项类型
        PortGraphicsItem = QGraphicsItem::UserType + 1,    // 架构视图中的端口图形项
        PartGraphicsItem = QGraphicsItem::UserType + 2,     // 架构视图中的Part图形项
        ConnectionGraphicsItem = QGraphicsItem::UserType + 3, // 架构视图中的连接线图形项
        
        // 基础视图 (Basic View) 图形项类型
        PortBasicItem = QGraphicsItem::UserType + 10,       // 基础视图中的端口图形项
        PartBasicItem = QGraphicsItem::UserType + 11,      // 基础视图中的Part图形项
        
        // 其他辅助图形项类型（如果需要）
        // ResizeHandleItem = QGraphicsItem::UserType + 20,  // 调整大小手柄（如果需要）
        // PortLabelItem = QGraphicsItem::UserType + 21,     // 端口标签项（如果需要）
    };
}

SYSML_NAMESPACE_END

#endif // GRAPHICSITEMTYPES_H
