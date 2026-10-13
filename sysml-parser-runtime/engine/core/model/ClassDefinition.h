#ifndef SYSML_CLASSDEFINITION_H
#define SYSML_CLASSDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ClassDefinition 表示类定义
 * 
 * 参考: org.omg.sysml.metamodel.ClassDefinition
 */
class ClassDefinition : public Type {
    Q_OBJECT
    
public:
    explicit ClassDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "ClassDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_CLASSDEFINITION_H





