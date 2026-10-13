#include "BasicViewWidget.h"
#include "PartBasicItem.h"
#include "BasicScene.h"
#include "../../../parser/src/annotation/AnnotationParser.h"
#include "../../../ui/mid_windows/architecture/BaseGraphicMetadata.h"
#include "../../../ui/mid_windows/architecture/PartBasicViewMetadata.h"
#include "../../../core/model/AttributeDefinition.h"
#include "../../../core/model/PortDefinition.h"
#include "../../../core/model/PortUsage.h"
#include "../../../core/model/FeatureTyping.h"
#include "../../../core/model/InterfaceDefinition.h"
#include "../../../core/model/ItemDefinition.h"
#include "../../../core/model/Annotation.h"
#include "../../../core/model/Feature.h"
#include "../../../core/model/Type.h"
#include "../../../core/model/Namespace.h"
#include "../../../core/model/DataTypeDefinition.h"
#include <QWheelEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QtMath>
#include <QDebug>

SYSML_NAMESPACE_BEGIN

BasicViewWidget::BasicViewWidget(QWidget* parent)
    : QGraphicsView(parent)
    , m_part(nullptr)
    , m_scene(nullptr)
{
    // 设置视图属性
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    
    // 不设置 CustomContextMenu，让场景和图形项处理右键事件
    // setContextMenuPolicy(Qt::CustomContextMenu);
    
    // 设置场景
    setupScene();
}

BasicViewWidget::~BasicViewWidget() {
    // Scene会自动清理，但可以显式删除
    if (m_scene) {
        delete m_scene;
        m_scene = nullptr;
    }
}

void BasicViewWidget::setupScene() {
    // 创建场景
    m_scene = new BasicScene(this);
    setScene(m_scene);
    
    // 设置场景属性
    m_scene->setSceneRect(0, 0, 1000, 1000);
}

void BasicViewWidget::setPart(Element* part) {
    if (m_part == part) {
        return;
    }
    
    // 断开旧的信号连接
    if (m_part) {
        disconnect(m_part, &Element::elementChanged, this, &BasicViewWidget::refresh);
        disconnect(m_part, &Element::childAdded, this, &BasicViewWidget::refresh);
        disconnect(m_part, &Element::childRemoved, this, &BasicViewWidget::refresh);
    }
    
    m_part = part;
    
    // 连接新的信号
    if (m_part) {
        connect(m_part, &Element::elementChanged, this, &BasicViewWidget::refresh);
        connect(m_part, &Element::childAdded, this, &BasicViewWidget::refresh);
        connect(m_part, &Element::childRemoved, this, &BasicViewWidget::refresh);
    }
    
    // 清除场景中的现有项
    m_scene->clear();
    
    // 渲染part
    if (m_part) {
        renderPart(m_part);
        centerOnPart();
    }
}

void BasicViewWidget::renderPart(Element* part) {
    if (!part) {
        return;
    }
    
    // 获取GraphicMetadata（用于基础视图显示配置）
    BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(part);
    PartBasicViewMetadata* basicViewMeta = qobject_cast<PartBasicViewMetadata*>(metadata);
    
    // 创建PartBasicItem
    PartBasicItem* partItem = new PartBasicItem(part, basicViewMeta);
    
    // 设置BasicViewWidget（用于编辑功能）
    partItem->setBasicViewWidget(this);
    
    // 设置位置（居中）
    QPointF position = QPointF(400, 300);
    partItem->setPos(position);
    
    // 添加到场景
    m_scene->addItem(partItem);
}

void BasicViewWidget::refresh() {
    if (!m_part || !m_scene) {
        return;
    }
    
    // 查找场景中现有的 PartBasicItem
    QList<QGraphicsItem*> items = m_scene->items();
    PartBasicItem* partItem = nullptr;
    for (QGraphicsItem* item : items) {
        PartBasicItem* candidate = qgraphicsitem_cast<PartBasicItem*>(item);
        if (candidate && candidate->part() == m_part) {
            partItem = candidate;
            break;
        }
    }
    
    if (partItem) {
        // 如果找到了现有的 PartBasicItem，调用其 updateDisplay 方法更新显示
        partItem->updateDisplay();
    } else {
        // 如果没有找到，重新渲染（清除场景并重新创建）
        m_scene->clear();
        renderPart(m_part);
    }
    
    // 居中显示
    centerOnPart();
}

void BasicViewWidget::centerOnPart() {
    if (!m_scene || m_scene->items().isEmpty()) {
        return;
    }
    
    // 找到PartBasicItem并居中显示
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem* item : items) {
        PartBasicItem* partItem = qgraphicsitem_cast<PartBasicItem*>(item);
        if (partItem) {
            centerOn(partItem);
            return;
        }
    }
}

void BasicViewWidget::wheelEvent(QWheelEvent* event) {
    // 缩放功能
    if (event->modifiers() & Qt::ControlModifier) {
        // Ctrl + 滚轮：缩放
        double scaleFactor = qPow(1.2, event->angleDelta().y() / 240.0);
        scale(scaleFactor, scaleFactor);
        event->accept();
    } else {
        // 普通滚轮：滚动
        QGraphicsView::wheelEvent(event);
    }
}

void BasicViewWidget::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    
    // 调整大小时居中显示part
    centerOnPart();
}

Element* BasicViewWidget::createAttribute(const QString& name, const QString& type, const QString& value) {
    if (!m_part) {
        return nullptr;
    }
    
    // 检查名称是否重复
    QList<Element*> attributes = m_part->getAttributes();
    for (Element* attr : attributes) {
        if (attr->declaredName() == name) {
            qWarning() << "[BasicViewWidget] Attribute already exists:" << name;
            return nullptr;
        }
    }
    
    // 在 part def 中应该创建 Feature 类型的属性，而不是 AttributeDefinition
    // Feature 会被序列化为 "attribute name : Type = value;" 格式
    Feature* attr = new Feature(m_part);
    attr->setDeclaredName(name);
    
    // 设置简化属性标志为 false，这样序列化时会输出 "attribute" 关键字
    attr->setIsSimplifiedAttribute(false);
    
    // 设置类型（如果有）
    if (!type.isEmpty()) {
        // 首先尝试查找类型对象
        Type* typeObj = findTypeByName(type);
        if (typeObj) {
            // 创建 FeatureTyping 关系
            FeatureTyping* featureTyping = new FeatureTyping(m_part);
            featureTyping->setTypedFeature(attr);
            featureTyping->setType(typeObj);
            m_part->addOwnedRelationship(featureTyping);
        } else {
            // 如果找不到类型对象，创建 FeatureTyping 并只保存类型名称
            FeatureTyping* featureTyping = new FeatureTyping(m_part);
            featureTyping->setTypedFeature(attr);
            featureTyping->setTypeName(type);
            m_part->addOwnedRelationship(featureTyping);
        }
    }
    
    // 设置值（如果需要）
    if (!value.isEmpty()) {
        attr->setValueExpression(value);
    }
    
    // 添加到 part
    m_part->addOwnedElement(attr);
    
    // 触发变更通知
    emit m_part->elementChanged();
    
    return attr;
}

bool BasicViewWidget::deleteAttribute(Element* attribute) {
    if (!m_part || !attribute) {
        return false;
    }
    
    // 检查是否属于当前 part
    if (attribute->owner() != m_part) {
        return false;
    }
    
    // 从 part 中移除
    m_part->removeOwnedElement(attribute);
    
    // 删除元素
    attribute->deleteLater();
    
    // 触发变更通知
    emit m_part->elementChanged();
    
    return true;
}

Element* BasicViewWidget::createPort(const QString& name, const QString& position, const QString& type) {
    if (!m_part) {
        return nullptr;
    }
    
    // 检查名称是否重复
    QList<Element*> ports = m_part->getPorts();
    for (Element* port : ports) {
        if (port->declaredName() == name) {
            qWarning() << "[BasicViewWidget] Port already exists:" << name;
            return nullptr;
        }
    }
    
    // 创建 PortUsage（在 part def 中应该使用 PortUsage，而不是 PortDefinition）
    PortUsage* port = new PortUsage(m_part);
    port->setDeclaredName(name);
    
    // 设置端口类型（如果有）
    if (!type.isEmpty()) {
        // 首先尝试查找类型对象
        Type* typeObj = findTypeByName(type);
        if (typeObj) {
            // PortUsage 继承自 Type，需要通过 FeatureTyping 关系设置类型
            // 但 PortUsage 本身不是 Feature，所以我们需要通过其他方式
            // 查看序列化代码，PortUsage 使用 typeName() 或通过 findUsageType 查找类型
            // 这里我们直接设置 typeName，序列化器会处理
            port->setTypeName(type);
            // TODO: 如果需要建立类型关系，可能需要其他方式
        } else {
            // 如果找不到类型对象，只保存类型名称（用于序列化）
            port->setTypeName(type);
        }
    }
    
    // 添加到 part
    m_part->addOwnedElement(port);
    
    // 添加图形元数据注解（设置 portPosition）
    if (!position.isEmpty()) {
        Annotation* annotation = new Annotation(port);
        annotation->setAnnotationName("GraphicMetadata");
        annotation->setMetadataAssignment("portPosition", position);
        port->addAnnotation(annotation);
    }
    
    // 触发变更通知
    emit m_part->elementChanged();
    
    return port;
}

bool BasicViewWidget::deletePort(Element* port) {
    if (!m_part || !port) {
        return false;
    }
    
    // 检查是否属于当前 part
    if (port->owner() != m_part) {
        return false;
    }
    
    // 从 part 中移除
    m_part->removeOwnedElement(port);
    
    // 删除元素
    port->deleteLater();
    
    // 触发变更通知
    emit m_part->elementChanged();
    
    return true;
}

Element* BasicViewWidget::createItem(const QString& name, const QString& type) {
    Q_UNUSED(type);  // TODO: 将来实现类型设置
    if (!m_part) {
        return nullptr;
    }
    
    // 检查名称是否重复
    QList<Element*> items = m_part->getItems();
    for (Element* item : items) {
        if (item->declaredName() == name) {
            qWarning() << "[BasicViewWidget] Item already exists:" << name;
            return nullptr;
        }
    }
    
    // 创建 ItemDefinition
    ItemDefinition* item = new ItemDefinition(m_part);
    item->setDeclaredName(name);
    
    // 添加到 part
    m_part->addOwnedElement(item);
    
    // 触发变更通知
    emit m_part->elementChanged();
    
    return item;
}

bool BasicViewWidget::deleteItem(Element* item) {
    if (!m_part || !item) {
        return false;
    }
    
    // 检查是否属于当前 part
    if (item->owner() != m_part) {
        return false;
    }
    
    // 从 part 中移除
    m_part->removeOwnedElement(item);
    
    // 删除元素
    item->deleteLater();
    
    // 触发变更通知
    emit m_part->elementChanged();
    
    return true;
}

QStringList BasicViewWidget::getAvailablePortTypes() const {
    QStringList types;
    
    if (!m_part) {
        return types;
    }
    
    // 从当前 part 的命名空间开始，向上查找所有接口定义
    Namespace* ns = m_part->owningNamespace();
    while (ns) {
        Element* nsElement = dynamic_cast<Element*>(ns);
        if (nsElement) {
            // 查找所有子元素中的 InterfaceDefinition
            for (Element* child : nsElement->ownedElements()) {
                if (child->elementType() == "InterfaceDefinition") {
                    QString name = child->declaredName();
                    if (!name.isEmpty() && !types.contains(name)) {
                        types << name;
                    }
                }
            }
        }
        
        // 向上查找父命名空间
        if (nsElement) {
            ns = nsElement->owningNamespace();
        } else {
            break;
        }
    }
    
    // 排序并返回
    types.sort();
    return types;
}

Type* BasicViewWidget::findTypeByName(const QString& typeName) const {
    if (!m_part || typeName.isEmpty()) {
        return nullptr;
    }
    
    // 从当前 part 的命名空间开始，向上查找类型
    Namespace* ns = m_part->owningNamespace();
    while (ns) {
        Element* nsElement = dynamic_cast<Element*>(ns);
        if (nsElement) {
            // 查找所有子元素中的类型
            for (Element* child : nsElement->ownedElements()) {
                Type* type = qobject_cast<Type*>(child);
                if (type && type->declaredName() == typeName) {
                    return type;
                }
            }
        }
        
        // 向上查找父命名空间
        if (nsElement) {
            ns = nsElement->owningNamespace();
        } else {
            break;
        }
    }
    
    return nullptr;
}

SYSML_NAMESPACE_END
