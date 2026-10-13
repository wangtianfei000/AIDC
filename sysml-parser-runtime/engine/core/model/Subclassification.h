#ifndef SYSML_SUBCLASSIFICATION_H
#define SYSML_SUBCLASSIFICATION_H

#include "Relationship.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class Type;

/**
 * @brief Subclassification 表示子类化关系
 * 
 * 参考: org.omg.sysml.metamodel.Subclassification
 */
class Subclassification : public Relationship {
    Q_OBJECT
    
public:
    explicit Subclassification(QObject* parent = nullptr);
    
    QString elementType() const override { return "Subclassification"; }
    
    // 子类型
    Type* subtype() const;
    void setSubtype(Type* type);
    
    // 超类型
    Type* supertype() const;
    void setSupertype(Type* type);
    
signals:
    void subtypeChanged(Type* type);
    void supertypeChanged(Type* type);
    
private:
    Type* m_subtype = nullptr;
    Type* m_supertype = nullptr;
};

SYSML_NAMESPACE_END

#endif // SYSML_SUBCLASSIFICATION_H



