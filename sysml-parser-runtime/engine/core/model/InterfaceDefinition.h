#ifndef SYSML_INTERFACEDEFINITION_H
#define SYSML_INTERFACEDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief InterfaceDefinition 表示接口定义
 * 
 * 参考: org.omg.sysml.metamodel.InterfaceDefinition
 */
class InterfaceDefinition : public Type {
    Q_OBJECT
    
public:
    explicit InterfaceDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "InterfaceDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_INTERFACEDEFINITION_H





