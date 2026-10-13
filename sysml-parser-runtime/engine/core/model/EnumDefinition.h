#ifndef SYSML_ENUMDEFINITION_H
#define SYSML_ENUMDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief EnumDefinition 表示枚举定义
 * 
 * 参考: org.omg.sysml.metamodel.EnumDefinition
 */
class EnumDefinition : public Type {
    Q_OBJECT
    
public:
    explicit EnumDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "EnumDefinition"; }
    
    // 枚举值（暂时使用 Feature 来表示枚举值）
    // 后续可以扩展为专门的 EnumValue 类
};

SYSML_NAMESPACE_END

#endif // SYSML_ENUMDEFINITION_H





