#include "Element.h"
#include "Namespace.h"
#include "Relationship.h"
#include "Type.h"
#include "PartDefinition.h"
#include "PartUsage.h"
#include "PortDefinition.h"
#include "PortUsage.h"
#include "ConnectionDefinition.h"
#include "ConnectionUsage.h"
#include "AttributeDefinition.h"
#include "Feature.h"
#include "ItemDefinition.h"
#include "ItemUsage.h"
#include "DocStatement.h"
#include "Annotation.h"
#include "../common/Global.h"
#include <QVariant>

SYSML_NAMESPACE_BEGIN

Element::Element(QObject* parent)
    : QObject(parent)
{
}

QString Element::declaredName() const
{
    return m_declaredName;
}

void Element::setDeclaredName(const QString& name)
{
    if (m_declaredName != name) {
        m_declaredName = name;
        emit declaredNameChanged(name);
        emit nameChanged(this->name());
        emit qualifiedNameChanged(qualifiedName());
        notifyChanged();
    }
}

QString Element::name() const
{
    return m_declaredName;
}

QString Element::qualifiedName() const
{
    if (m_declaredName.isEmpty()) {
        return QString();
    }
    
    QStringList parts;
    const Element* current = this;
    
    while (current) {
        if (!current->declaredName().isEmpty()) {
            parts.prepend(current->declaredName());
        }
        current = current->owner();
    }
    
    return parts.join("::");
}

Element* Element::owner() const
{
    return m_owner;
}

void Element::setOwner(Element* owner)
{
    if (m_owner != owner) {
        Element* oldOwner = m_owner;
        m_owner = owner;
        
        // 注意：不在这里设置 Qt 父对象
        // Qt 父对象应该由 Resource 或顶层容器来管理
        // 这里只管理逻辑上的所有者关系
        
        onOwnerChanged(oldOwner, owner);
        emit ownerChanged(owner);
        emit qualifiedNameChanged(qualifiedName());
        notifyChanged();
    }
}

Namespace* Element::owningNamespace() const
{
    const Element* current = m_owner;
    while (current) {
        // Type 同时继承 Element 和 Namespace，使用 dynamic_cast
        if (auto ns = dynamic_cast<Namespace*>(const_cast<Element*>(current))) {
            return ns;
        }
        current = current->owner();
    }
    return nullptr;
}

QList<Element*> Element::ownedElements() const
{
    return m_ownedElements;
}

void Element::addOwnedElement(Element* element)
{
    if (!element) {
        return;
    }
    
    if (!m_ownedElements.contains(element)) {
        m_ownedElements.append(element);
        element->setOwner(this);
        // 设置 Qt 父对象，确保对象树正确
        element->setParent(this);
        emit childAdded(element);
        notifyChanged();
    }
}

void Element::removeOwnedElement(Element* element)
{
    if (m_ownedElements.removeOne(element)) {
        element->setOwner(nullptr);
        // 注意：不在这里清除父对象，因为元素可能还在 Resource 中
        // 父对象的清理应该由 Resource 来管理
        emit childRemoved(element);
        notifyChanged();
    }
}

bool Element::hasOwnedElement(Element* element) const
{
    return m_ownedElements.contains(element);
}

QList<Relationship*> Element::ownedRelationships() const
{
    return m_ownedRelationships;
}

void Element::addOwnedRelationship(Relationship* relationship)
{
    if (!relationship) {
        return;
    }
    
    if (!m_ownedRelationships.contains(relationship)) {
        m_ownedRelationships.append(relationship);
        relationship->setParent(this);
        emit relationshipAdded(relationship);
        notifyChanged();
    }
}

void Element::removeOwnedRelationship(Relationship* relationship)
{
    if (m_ownedRelationships.removeOne(relationship)) {
        relationship->setParent(nullptr);
        emit relationshipRemoved(relationship);
        notifyChanged();
    }
}

void Element::notifyChanged()
{
    emit elementChanged();
}

void Element::onOwnerChanged(Element* oldOwner, Element* newOwner)
{
    Q_UNUSED(oldOwner)
    Q_UNUSED(newOwner)
    // 子类可以重写此方法以响应所有者变化
}

// 注解相关方法
QList<Element*> Element::ownedAnnotations() const
{
    return m_ownedAnnotations;
}

void Element::addAnnotation(Element* annotation)
{
    if (!annotation) {
        return;
    }
    
    if (!m_ownedAnnotations.contains(annotation)) {
        m_ownedAnnotations.append(annotation);
        annotation->setOwner(this);
        annotation->setParent(this);
        notifyChanged();
    }
}

void Element::removeAnnotation(Element* annotation)
{
    if (m_ownedAnnotations.removeOne(annotation)) {
        annotation->setOwner(nullptr);
        notifyChanged();
    }
}

Element* Element::findAnnotation(const QString& annotationName) const
{
    for (Element* annotation : m_ownedAnnotations) {
        // 检查是否是Annotation类型，并且名称匹配
        if (annotation->elementType() == "Annotation") {
            // 直接转换为Annotation类型来获取名称
            Annotation* ann = qobject_cast<Annotation*>(annotation);
            if (ann && ann->annotationName() == annotationName) {
                return annotation;
            }
        }
    }
    return nullptr;
}

// 获取子元素方法
QList<Element*> Element::getChildParts() const
{
    QList<Element*> childParts;
    for (Element* child : m_ownedElements) {
        if (child->elementType() == "PartDefinition" || 
            child->elementType() == "PartUsage") {
            childParts.append(child);
        }
    }
    return childParts;
}

QList<Element*> Element::getPorts() const
{
    QList<Element*> ports;
    for (Element* child : m_ownedElements) {
        QString type = child->elementType();
        if (type == "PortDefinition" || type == "PortUsage") {
            ports.append(child);
        }
    }
    return ports;
}

QList<Element*> Element::getConnections() const
{
    QList<Element*> connections;
    for (Element* child : m_ownedElements) {
        QString type = child->elementType();
        if (type == "ConnectionDefinition" || type == "ConnectionUsage" ||
            type == "ConnectorDefinition" || type == "ConnectorUsage" ||
            type == "ConnectStatement") { // 支持 connect 语句
            connections.append(child);
        }
    }
    return connections;
}

QList<Element*> Element::getAttributes() const
{
    QList<Element*> attributes;
    for (Element* child : m_ownedElements) {
        QString type = child->elementType();
        if (type == "AttributeDefinition") {
            attributes.append(child);
        } else if (type == "Feature") {
            // Feature 可能是属性（通过 attribute 语法定义）
            // 排除简化的属性（可能是其他用途的 Feature，如端口等）
            Feature* feature = qobject_cast<Feature*>(child);
            if (feature && !feature->isSimplifiedAttribute()) {
                attributes.append(child);
            }
        }
    }
    return attributes;
}

QList<Element*> Element::getItems() const
{
    QList<Element*> items;
    for (Element* child : m_ownedElements) {
        QString type = child->elementType();
        if (type == "ItemDefinition" || type == "ItemUsage") {
            items.append(child);
        }
    }
    return items;
}

Element* Element::getDoc() const
{
    for (Element* child : m_ownedElements) {
        if (child->elementType() == "DocStatement") {
            return child;
        }
    }
    return nullptr;
}

SYSML_NAMESPACE_END

