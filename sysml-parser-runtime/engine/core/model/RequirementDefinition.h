#ifndef SYSML_REQUIREMENTDEFINITION_H
#define SYSML_REQUIREMENTDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief RequirementDefinition 表示需求定义
 * 
 * 参考: org.omg.sysml.metamodel.RequirementDefinition
 */
class RequirementDefinition : public Type {
    Q_OBJECT
    
public:
    explicit RequirementDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "RequirementDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_REQUIREMENTDEFINITION_H





