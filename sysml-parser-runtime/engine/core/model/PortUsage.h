#ifndef SYSML_PORTUSAGE_H
#define SYSML_PORTUSAGE_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief PortUsage 表示端口使用
 * 
 * 参考: org.omg.sysml.metamodel.PortUsage
 */
class PortUsage : public Type {
    Q_OBJECT
    
public:
    explicit PortUsage(QObject* parent = nullptr);
    
    QString elementType() const override { return "PortUsage"; }
    
    // 类型名称（字符串形式，用于序列化）
    // 当类型引用不存在时，使用此名称
    QString typeName() const { return m_typeName; }
    void setTypeName(const QString& name) { m_typeName = name; }
    
private:
    QString m_typeName; // 保存类型名称字符串
};

SYSML_NAMESPACE_END

#endif // SYSML_PORTUSAGE_H





