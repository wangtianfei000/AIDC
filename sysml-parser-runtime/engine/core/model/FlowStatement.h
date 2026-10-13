#ifndef SYSML_FLOWSTATEMENT_H
#define SYSML_FLOWSTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief FlowStatement 表示 flow 语句
 * 
 * 参考: org.omg.sysml.metamodel.FlowConnectionUsage
 */
class FlowStatement : public Element {
    Q_OBJECT
    
    Q_PROPERTY(QString fromEnd READ fromEnd WRITE setFromEnd NOTIFY fromEndChanged)
    Q_PROPERTY(QString toEnd READ toEnd WRITE setToEnd NOTIFY toEndChanged)
    Q_PROPERTY(QString payload READ payload WRITE setPayload NOTIFY payloadChanged)
    Q_PROPERTY(QString flowName READ flowName WRITE setFlowName NOTIFY flowNameChanged)
    Q_PROPERTY(QString flowType READ flowType WRITE setFlowType NOTIFY flowTypeChanged)
    Q_PROPERTY(bool hasFromKeyword READ hasFromKeyword WRITE setHasFromKeyword NOTIFY hasFromKeywordChanged)
    
public:
    explicit FlowStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "FlowStatement"; }
    
    QString fromEnd() const { return m_fromEnd; }
    void setFromEnd(const QString& fromEnd);
    
    QString toEnd() const { return m_toEnd; }
    void setToEnd(const QString& toEnd);
    
    QString payload() const { return m_payload; }
    void setPayload(const QString& payload);
    
    QString flowName() const { return m_flowName; }
    void setFlowName(const QString& flowName);
    
    QString flowType() const { return m_flowType; }
    void setFlowType(const QString& flowType);
    
    bool hasFromKeyword() const { return m_hasFromKeyword; }
    void setHasFromKeyword(bool hasFromKeyword);
    
signals:
    void fromEndChanged(const QString& fromEnd);
    void toEndChanged(const QString& toEnd);
    void payloadChanged(const QString& payload);
    void flowNameChanged(const QString& flowName);
    void flowTypeChanged(const QString& flowType);
    void hasFromKeywordChanged(bool hasFromKeyword);
    
private:
    QString m_fromEnd;  // 源端点
    QString m_toEnd;  // 目标端点
    QString m_payload;  // 载荷（可选）
    QString m_flowName;  // 流名称（使用声明中的名称）
    QString m_flowType;  // 流类型（使用声明中的类型）
    bool m_hasFromKeyword = false;  // 是否有 'from' 关键字
};

SYSML_NAMESPACE_END

#endif // SYSML_FLOWSTATEMENT_H

