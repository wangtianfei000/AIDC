#ifndef SYSML_ITEMDEFINITION_H
#define SYSML_ITEMDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ItemDefinition 表示项定义
 * 
 * 参考: org.omg.sysml.metamodel.ItemDefinition
 */
class ItemDefinition : public Type {
    Q_OBJECT
    
public:
    explicit ItemDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "ItemDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_ITEMDEFINITION_H




