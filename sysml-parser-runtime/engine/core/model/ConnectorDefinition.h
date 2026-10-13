#ifndef SYSML_CONNECTORDEFINITION_H
#define SYSML_CONNECTORDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ConnectorDefinition 表示连接器定义
 * 
 * 参考: org.omg.sysml.metamodel.ConnectorDefinition
 */
class ConnectorDefinition : public Type {
    Q_OBJECT
    
public:
    explicit ConnectorDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "ConnectorDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_CONNECTORDEFINITION_H





