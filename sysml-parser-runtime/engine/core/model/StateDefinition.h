#ifndef SYSML_STATEDEFINITION_H
#define SYSML_STATEDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief StateDefinition 表示状态定义
 * 
 * 参考: org.omg.sysml.metamodel.StateDefinition
 */
class StateDefinition : public Type {
    Q_OBJECT
    
public:
    explicit StateDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "StateDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_STATEDEFINITION_H





