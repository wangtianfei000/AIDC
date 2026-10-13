#ifndef SYSML_ATTRIBUTEDEFINITION_H
#define SYSML_ATTRIBUTEDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief AttributeDefinition 表示属性定义
 * 
 * 参考: org.omg.sysml.metamodel.AttributeDefinition
 */
class AttributeDefinition : public Type {
    Q_OBJECT
    
public:
    explicit AttributeDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "AttributeDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_ATTRIBUTEDEFINITION_H





