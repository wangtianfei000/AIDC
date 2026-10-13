#ifndef SYSML_CONSTRAINTUSAGE_H
#define SYSML_CONSTRAINTUSAGE_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ConstraintUsage 表示约束使用
 * 
 * 参考: org.omg.sysml.metamodel.ConstraintUsage
 */
class ConstraintUsage : public Type {
    Q_OBJECT
    
public:
    explicit ConstraintUsage(QObject* parent = nullptr);
    
    QString elementType() const override { return "ConstraintUsage"; }
    
    // 类型名称（字符串形式，用于序列化）
    // 当类型引用不存在时，使用此名称
    QString typeName() const { return m_typeName; }
    void setTypeName(const QString& name) { m_typeName = name; }
    
private:
    QString m_typeName; // 保存类型名称字符串
};

SYSML_NAMESPACE_END

#endif // SYSML_CONSTRAINTUSAGE_H



