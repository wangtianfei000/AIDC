#ifndef SYSML_DATATYPEDEFINITION_H
#define SYSML_DATATYPEDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief DataTypeDefinition 表示数据类型定义
 * 
 * 参考: org.omg.sysml.metamodel.DataTypeDefinition
 */
class DataTypeDefinition : public Type {
    Q_OBJECT
    
public:
    explicit DataTypeDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "DataTypeDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_DATATYPEDEFINITION_H





