#ifndef SYSML_FLOWDEFINITION_H
#define SYSML_FLOWDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief FlowDefinition 表示流定义
 * 
 * 参考: org.omg.sysml.metamodel.FlowDefinition
 */
class FlowDefinition : public Type {
    Q_OBJECT
    
public:
    explicit FlowDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "FlowDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_FLOWDEFINITION_H




