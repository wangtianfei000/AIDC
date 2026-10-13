#ifndef SYSML_DEPENDENCY_H
#define SYSML_DEPENDENCY_H

#include "Relationship.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class Element;

/**
 * @brief Dependency 表示依赖关系
 * 
 * 参考: org.omg.sysml.metamodel.Dependency
 */
class Dependency : public Relationship {
    Q_OBJECT
    
public:
    explicit Dependency(QObject* parent = nullptr);
    
    QString elementType() const override { return "Dependency"; }
    
    // 依赖的元素
    Element* clientElement() const;
    void setClientElement(Element* element);
    
    // 被依赖的元素
    Element* supplierElement() const;
    void setSupplierElement(Element* element);
    
    // 被依赖的元素名称（字符串形式，用于序列化）
    // 当 supplierElement() 为 nullptr 时，使用此名称
    QString supplierElementName() const { return m_supplierElementName; }
    void setSupplierElementName(const QString& name) { m_supplierElementName = name; }
    
signals:
    void clientElementChanged(Element* element);
    void supplierElementChanged(Element* element);
    
private:
    Element* m_clientElement = nullptr;
    Element* m_supplierElement = nullptr;
    QString m_supplierElementName; // 保存被依赖元素名称字符串
};

SYSML_NAMESPACE_END

#endif // SYSML_DEPENDENCY_H

