#ifndef SYSML_ELEMENT_H
#define SYSML_ELEMENT_H

#include "../common/Global.h"
#include <QObject>
#include <QString>
#include <QList>
#include <QMetaType>

SYSML_NAMESPACE_BEGIN

// 前向声明
class Element;
class Namespace;
class Relationship;

/**
 * @brief Element 是所有 SysML 元素的基类
 * 
 * 参考: org.omg.sysml.metamodel.Element
 * 
 * 核心功能:
 * - 基本属性（名称、限定名）
 * - 所有权关系（owner, owningNamespace）
 * - 包含关系（ownedElements, ownedRelationships）
 * - 变更通知（Qt 信号槽）
 */
class Element : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString declaredName READ declaredName WRITE setDeclaredName NOTIFY declaredNameChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString qualifiedName READ qualifiedName NOTIFY qualifiedNameChanged)
    
public:
    explicit Element(QObject* parent = nullptr);
    virtual ~Element() = default;
    
    // 基本属性
    virtual QString declaredName() const;
    virtual void setDeclaredName(const QString& name);
    virtual QString name() const;
    virtual QString qualifiedName() const;
    
    // 所有权关系
    Element* owner() const;
    void setOwner(Element* owner);
    Namespace* owningNamespace() const;
    
    // 包含关系
    QList<Element*> ownedElements() const;
    void addOwnedElement(Element* element);
    void removeOwnedElement(Element* element);
    bool hasOwnedElement(Element* element) const;
    
    // 关系
    QList<Relationship*> ownedRelationships() const;
    void addOwnedRelationship(Relationship* relationship);
    void removeOwnedRelationship(Relationship* relationship);
    
    // 类型信息
    virtual QString elementType() const = 0;
    
    // 注解相关
    QList<Element*> ownedAnnotations() const;  // 获取拥有的注解列表
    void addAnnotation(Element* annotation);    // 添加注解
    void removeAnnotation(Element* annotation); // 移除注解
    Element* findAnnotation(const QString& annotationName) const;  // 查找指定名称的注解
    
    // 获取子元素（用于架构视图）
    QList<Element*> getChildParts() const;      // 获取子part列表
    QList<Element*> getPorts() const;          // 获取端口列表
    QList<Element*> getConnections() const;     // 获取连接列表
    QList<Element*> getAttributes() const;       // 获取属性列表
    QList<Element*> getItems() const;           // 获取item列表
    Element* getDoc() const;                    // 获取文档
    
signals:
    void declaredNameChanged(const QString& name);
    void nameChanged(const QString& name);
    void qualifiedNameChanged(const QString& qualifiedName);
    void ownerChanged(Element* owner);
    void childAdded(Element* child);
    void childRemoved(Element* child);
    void relationshipAdded(Relationship* relationship);
    void relationshipRemoved(Relationship* relationship);
    void elementChanged();
    
protected:
    QString m_declaredName;
    Element* m_owner = nullptr;
    QList<Element*> m_ownedElements;
    QList<Relationship*> m_ownedRelationships;
    QList<Element*> m_ownedAnnotations;  // 注解列表
    
    void notifyChanged();
    virtual void onOwnerChanged(Element* oldOwner, Element* newOwner);
};

SYSML_NAMESPACE_END

Q_DECLARE_METATYPE(SysML::Element*)

#endif // SYSML_ELEMENT_H

