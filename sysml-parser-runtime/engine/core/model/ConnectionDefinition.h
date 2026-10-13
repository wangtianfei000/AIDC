#ifndef SYSML_CONNECTIONDEFINITION_H
#define SYSML_CONNECTIONDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ConnectionDefinition 表示连接定义
 * 
 * 参考: org.omg.sysml.metamodel.ConnectionDefinition
 */
class ConnectionDefinition : public Type {
    Q_OBJECT
    
public:
    explicit ConnectionDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "ConnectionDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_CONNECTIONDEFINITION_H




