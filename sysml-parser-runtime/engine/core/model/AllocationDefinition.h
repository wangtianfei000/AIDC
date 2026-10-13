#ifndef SYSML_ALLOCATIONDEFINITION_H
#define SYSML_ALLOCATIONDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief AllocationDefinition 表示分配定义
 * 
 * 参考: org.omg.sysml.metamodel.AllocationDefinition
 */
class AllocationDefinition : public Type {
    Q_OBJECT
    
public:
    explicit AllocationDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "AllocationDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_ALLOCATIONDEFINITION_H




