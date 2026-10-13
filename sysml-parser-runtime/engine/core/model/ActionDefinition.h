#ifndef SYSML_ACTIONDEFINITION_H
#define SYSML_ACTIONDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ActionDefinition 表示动作定义
 * 
 * 参考: org.omg.sysml.metamodel.ActionDefinition
 */
class ActionDefinition : public Type {
    Q_OBJECT
    
public:
    explicit ActionDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "ActionDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_ACTIONDEFINITION_H





