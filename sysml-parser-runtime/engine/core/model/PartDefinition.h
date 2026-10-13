#ifndef SYSML_PARTDEFINITION_H
#define SYSML_PARTDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief PartDefinition 表示部件定义
 * 
 * 参考: org.omg.sysml.metamodel.PartDefinition
 */
class PartDefinition : public Type {
    Q_OBJECT
    
public:
    explicit PartDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "PartDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_PARTDEFINITION_H



