#ifndef SYSML_ALLOCATIONUSAGE_H
#define SYSML_ALLOCATIONUSAGE_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief AllocationUsage 表示分配使用
 * 
 * 参考: org.omg.sysml.metamodel.AllocationUsage
 */
class AllocationUsage : public Type {
    Q_OBJECT
    
public:
    explicit AllocationUsage(QObject* parent = nullptr);
    
    QString elementType() const override { return "AllocationUsage"; }
    
    // 类型名称（字符串形式，用于序列化）
    QString typeName() const { return m_typeName; }
    void setTypeName(const QString& name) { m_typeName = name; }
    
private:
    QString m_typeName;
};

SYSML_NAMESPACE_END

#endif // SYSML_ALLOCATIONUSAGE_H




