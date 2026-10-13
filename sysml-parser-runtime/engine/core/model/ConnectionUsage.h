#ifndef SYSML_CONNECTIONUSAGE_H
#define SYSML_CONNECTIONUSAGE_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ConnectionUsage 表示连接使用
 * 
 * 参考: org.omg.sysml.metamodel.ConnectionUsage
 */
class ConnectionUsage : public Type {
    Q_OBJECT
    
public:
    explicit ConnectionUsage(QObject* parent = nullptr);
    
    QString elementType() const override { return "ConnectionUsage"; }
    
    // 类型名称（字符串形式，用于序列化）
    QString typeName() const { return m_typeName; }
    void setTypeName(const QString& name) { m_typeName = name; }
    
private:
    QString m_typeName;
};

SYSML_NAMESPACE_END

#endif // SYSML_CONNECTIONUSAGE_H




