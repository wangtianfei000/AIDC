#ifndef SYSML_STATEACTIONMEMBER_H
#define SYSML_STATEACTIONMEMBER_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

// 前向声明
class ActionUsage;

/**
 * @brief StateActionMember 表示状态动作成员（entry/do/exit）
 * 
 * 参考: org.omg.sysml.metamodel.StateActionMember
 * 
 * 支持三种动作类型：
 * - entry: 进入状态时执行
 * - do: 在状态中持续执行
 * - exit: 离开状态时执行
 */
class StateActionMember : public Element {
    Q_OBJECT

    Q_PROPERTY(QString actionKind READ actionKind WRITE setActionKind NOTIFY actionKindChanged)
    Q_PROPERTY(QString actionName READ actionName WRITE setActionName NOTIFY actionNameChanged)
    Q_PROPERTY(bool hasActionKeyword READ hasActionKeyword WRITE setHasActionKeyword NOTIFY hasActionKeywordChanged)

public:
    explicit StateActionMember(QObject* parent = nullptr);

    QString elementType() const override { return "StateActionMember"; }

    // 动作类型："entry", "do", "exit"
    QString actionKind() const { return m_actionKind; }
    void setActionKind(const QString& kind);

    // 动作名称（从 stateActionUsage 中提取）
    QString actionName() const { return m_actionName; }
    void setActionName(const QString& name);

    // 是否有 "action" 关键字（entry action initial; vs entry performSelfTest;）
    bool hasActionKeyword() const { return m_hasActionKeyword; }
    void setHasActionKeyword(bool hasKeyword);

signals:
    void actionKindChanged(const QString& kind);
    void actionNameChanged(const QString& name);
    void hasActionKeywordChanged(bool hasKeyword);

private:
    QString m_actionKind;      // "entry", "do", "exit"
    QString m_actionName;      // 动作名称（如 "initial", "performSelfTest"）
    bool m_hasActionKeyword;   // 是否有 "action" 关键字
};

SYSML_NAMESPACE_END

#endif // SYSML_STATEACTIONMEMBER_H


