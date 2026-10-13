#ifndef SYSML_RELATIONSHIP_H
#define SYSML_RELATIONSHIP_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief Relationship 是所有关系的基类
 * 
 * 参考: org.omg.sysml.metamodel.Relationship
 */
class Relationship : public Element {
    Q_OBJECT
    
public:
    explicit Relationship(QObject* parent = nullptr);
    
    QString elementType() const override { return "Relationship"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_RELATIONSHIP_H



