#ifndef SYSML_CONSTRAINTDEFINITION_H
#define SYSML_CONSTRAINTDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ConstraintDefinition 表示约束定义
 * 
 * 参考: org.omg.sysml.metamodel.ConstraintDefinition
 */
class ConstraintDefinition : public Type {
    Q_OBJECT
    
public:
    explicit ConstraintDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "ConstraintDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_CONSTRAINTDEFINITION_H



