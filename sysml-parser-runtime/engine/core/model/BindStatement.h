#ifndef SYSML_BINDSTATEMENT_H
#define SYSML_BINDSTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief BindStatement 表示 bind 语句
 * 
 * 参考: org.omg.sysml.metamodel.BindingConnector
 */
class BindStatement : public Element {
    Q_OBJECT
    
    Q_PROPERTY(QString sourceEnd READ sourceEnd WRITE setSourceEnd NOTIFY sourceEndChanged)
    Q_PROPERTY(QString targetEnd READ targetEnd WRITE setTargetEnd NOTIFY targetEndChanged)
    
public:
    explicit BindStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "BindStatement"; }
    
    QString sourceEnd() const { return m_sourceEnd; }
    void setSourceEnd(const QString& sourceEnd);
    
    QString targetEnd() const { return m_targetEnd; }
    void setTargetEnd(const QString& targetEnd);
    
signals:
    void sourceEndChanged(const QString& sourceEnd);
    void targetEndChanged(const QString& targetEnd);
    
private:
    QString m_sourceEnd;  // 源端点
    QString m_targetEnd;  // 目标端点
};

SYSML_NAMESPACE_END

#endif // SYSML_BINDSTATEMENT_H




