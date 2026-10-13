#ifndef SYSML_PERFORMSTATEMENT_H
#define SYSML_PERFORMSTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief PerformStatement 表示 perform 语句
 * 
 * 参考: org.omg.sysml.metamodel.PerformActionUsage
 */
class PerformStatement : public Element {
    Q_OBJECT
    
    Q_PROPERTY(QString actionReference READ actionReference WRITE setActionReference NOTIFY actionReferenceChanged)
    Q_PROPERTY(bool isActionKeyword READ isActionKeyword WRITE setIsActionKeyword NOTIFY isActionKeywordChanged)
    Q_PROPERTY(QString actionName READ actionName WRITE setActionName NOTIFY actionNameChanged)
    Q_PROPERTY(QString actionType READ actionType WRITE setActionType NOTIFY actionTypeChanged)
    Q_PROPERTY(QString specializationType READ specializationType WRITE setSpecializationType NOTIFY specializationTypeChanged)
    
public:
    explicit PerformStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "PerformStatement"; }
    
    QString actionReference() const { return m_actionReference; }
    void setActionReference(const QString& actionReference);
    
    bool isActionKeyword() const { return m_isActionKeyword; }
    void setIsActionKeyword(bool isActionKeyword);
    
    QString actionName() const { return m_actionName; }
    void setActionName(const QString& actionName);
    
    QString actionType() const { return m_actionType; }
    void setActionType(const QString& actionType);
    
    QString specializationType() const { return m_specializationType; }
    void setSpecializationType(const QString& specializationType);
    
signals:
    void actionReferenceChanged(const QString& actionReference);
    void isActionKeywordChanged(bool isActionKeyword);
    void actionNameChanged(const QString& actionName);
    void actionTypeChanged(const QString& actionType);
    void specializationTypeChanged(const QString& specializationType);
    
private:
    QString m_actionReference;  // 动作引用（ownedReferenceSubsetting 或 action usageDeclaration）
    bool m_isActionKeyword = false;  // 是否使用 'action' 关键字
    QString m_actionName;  // 动作名称（usageDeclaration 中的名称）
    QString m_actionType;  // 动作类型（usageDeclaration 中的类型）
    QString m_specializationType;  // 特征特殊化类型（:> BaseAction）
};

SYSML_NAMESPACE_END

#endif // SYSML_PERFORMSTATEMENT_H

