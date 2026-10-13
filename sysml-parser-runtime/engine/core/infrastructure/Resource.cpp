#include "Resource.h"
#include "../model/Element.h"

SYSML_NAMESPACE_BEGIN

Resource::Resource(QObject* parent)
    : QObject(parent)
{
}

Resource::Resource(const QString& filePath, QObject* parent)
    : QObject(parent)
    , m_filePath(filePath)
{
}

Resource::~Resource()
{
    // 先断开所有连接，避免访问已销毁的对象
    for (Element* element : m_contents) {
        disconnectElement(element);
    }
    // 清空列表，让 Qt 的父子机制自动删除对象
    m_contents.clear();
}

void Resource::setFilePath(const QString& filePath)
{
    if (m_filePath != filePath) {
        m_filePath = filePath;
        emit filePathChanged(filePath);
    }
}

void Resource::addContent(Element* element)
{
    if (!element) {
        return;
    }
    
    // 只允许添加顶层元素（没有 owner 的元素）
    if (element->owner()) {
        qWarning() << "Resource::addContent: 只能添加顶层元素，该元素已有 owner:" << element->qualifiedName();
        return;
    }
    
    if (!m_contents.contains(element)) {
        m_contents.append(element);
        // 设置顶层元素的父对象为 Resource
        element->setParent(this);
        connectElement(element);
        markModified();
        emit contentAdded(element);
    }
}

void Resource::removeContent(Element* element)
{
    if (m_contents.removeOne(element)) {
        disconnectElement(element);
        element->setParent(nullptr);
        markModified();
        emit contentRemoved(element);
    }
}

void Resource::clear()
{
    // 先断开所有连接
    for (Element* element : m_contents) {
        disconnectElement(element);
    }
    // 移除父对象关系，让对象独立存在
    // 注意：不在这里删除对象，由调用者决定是否删除
    for (Element* element : m_contents) {
        if (element->parent() == this) {
            element->setParent(nullptr);
        }
    }
    m_contents.clear();
    markModified();
}

void Resource::setModified(bool modified)
{
    if (m_modified != modified) {
        m_modified = modified;
        emit modifiedChanged(modified);
    }
}

void Resource::markModified()
{
    setModified(true);
}

void Resource::markUnmodified()
{
    setModified(false);
}

QString Resource::uri() const
{
    if (!m_uri.isEmpty()) {
        return m_uri;
    }
    
    if (!m_filePath.isEmpty()) {
        return "file://" + m_filePath;
    }
    
    return QString();
}

void Resource::setUri(const QString& uri)
{
    m_uri = uri;
}

void Resource::onElementChanged()
{
    // 检查发送者是否仍然有效
    Element* sender = qobject_cast<Element*>(QObject::sender());
    if (sender && m_contents.contains(sender)) {
        markModified();
    }
}

void Resource::connectElement(Element* element)
{
    if (!element) {
        return;
    }
    // 使用 Qt::QueuedConnection 避免在对象销毁过程中访问已销毁的对象
    connect(element, &Element::elementChanged, this, &Resource::onElementChanged, Qt::DirectConnection);
    connect(element, &Element::childAdded, this, &Resource::onElementChanged, Qt::DirectConnection);
    connect(element, &Element::childRemoved, this, &Resource::onElementChanged, Qt::DirectConnection);
    connect(element, &Element::relationshipAdded, this, &Resource::onElementChanged, Qt::DirectConnection);
    connect(element, &Element::relationshipRemoved, this, &Resource::onElementChanged, Qt::DirectConnection);
}

void Resource::disconnectElement(Element* element)
{
    disconnect(element, nullptr, this, nullptr);
}

SYSML_NAMESPACE_END

