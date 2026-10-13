#ifndef SYSML_EXPOSESTATEMENT_H
#define SYSML_EXPOSESTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ExposeStatement 表示 expose 语句
 * 
 * 参考: org.omg.sysml.metamodel.ExposeStatement
 */
class ExposeStatement : public Element {
    Q_OBJECT
    
public:
    enum ExposeType {
        MembershipExpose,    // 暴露成员
        NamespaceExpose       // 暴露命名空间
    };
    
    explicit ExposeStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "ExposeStatement"; }
    
    ExposeType exposeType() const { return m_exposeType; }
    void setExposeType(ExposeType type) { m_exposeType = type; }
    
    QString exposedQualifiedName() const { return m_exposedQualifiedName; }
    void setExposedQualifiedName(const QString& name) { m_exposedQualifiedName = name; }
    
    bool isRecursive() const { return m_isRecursive; }
    void setRecursive(bool recursive) { m_isRecursive = recursive; }
    
private:
    ExposeType m_exposeType = MembershipExpose;
    QString m_exposedQualifiedName;
    bool m_isRecursive = false;
};

SYSML_NAMESPACE_END

#endif // SYSML_EXPOSESTATEMENT_H


