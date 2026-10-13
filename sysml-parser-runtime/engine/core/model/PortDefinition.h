#ifndef SYSML_PORTDEFINITION_H
#define SYSML_PORTDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief PortDefinition 表示端口定义
 * 
 * 参考: org.omg.sysml.metamodel.PortDefinition
 */
class PortDefinition : public Type {
    Q_OBJECT
    
public:
    explicit PortDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "PortDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_PORTDEFINITION_H





