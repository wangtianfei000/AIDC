#ifndef SYSML_TRANSITION_H
#define SYSML_TRANSITION_H

#include "Element.h"
#include "TransitionTriggerPart.h"
#include "TransitionGuardPart.h"
#include "TransitionEffectPart.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief Transition 表示状态转换
 * 
 * 参考: org.omg.sysml.metamodel.Transition
 * 
 * 支持两种形式：
 * - 简单转换：transition initial then off; 或 transition off then starting;
 * - 复杂转换：transition name first ... then ...;
 */
class Transition : public Element {
    Q_OBJECT

    Q_PROPERTY(QString sourceState READ sourceState WRITE setSourceState NOTIFY sourceStateChanged)
    Q_PROPERTY(QString targetState READ targetState WRITE setTargetState NOTIFY targetStateChanged)
    Q_PROPERTY(bool isInitial READ isInitial WRITE setIsInitial NOTIFY isInitialChanged)
    Q_PROPERTY(TransitionTriggerPart* triggerPart READ triggerPart WRITE setTriggerPart NOTIFY triggerPartChanged)
    Q_PROPERTY(TransitionGuardPart* guardPart READ guardPart WRITE setGuardPart NOTIFY guardPartChanged)
    Q_PROPERTY(TransitionEffectPart* effectPart READ effectPart WRITE setEffectPart NOTIFY effectPartChanged)

public:
    explicit Transition(QObject* parent = nullptr);

    QString elementType() const override { return "Transition"; }

    // 源状态名称
    QString sourceState() const { return m_sourceState; }
    void setSourceState(const QString& state);

    // 目标状态名称
    QString targetState() const { return m_targetState; }
    void setTargetState(const QString& state);

    // 是否是初始状态转换
    bool isInitial() const { return m_isInitial; }
    void setIsInitial(bool initial);

    // 触发部分
    TransitionTriggerPart* triggerPart() const { return m_triggerPart; }
    void setTriggerPart(TransitionTriggerPart* part);

    // 守卫部分
    TransitionGuardPart* guardPart() const { return m_guardPart; }
    void setGuardPart(TransitionGuardPart* part);

    // 效果部分
    TransitionEffectPart* effectPart() const { return m_effectPart; }
    void setEffectPart(TransitionEffectPart* part);

signals:
    void sourceStateChanged(const QString& state);
    void targetStateChanged(const QString& state);
    void isInitialChanged(bool initial);
    void triggerPartChanged(TransitionTriggerPart* part);
    void guardPartChanged(TransitionGuardPart* part);
    void effectPartChanged(TransitionEffectPart* part);

private:
    QString m_sourceState;      // 源状态名称
    QString m_targetState;      // 目标状态名称
    bool m_isInitial;          // 是否是初始状态转换
    TransitionTriggerPart* m_triggerPart = nullptr;  // 触发部分
    TransitionGuardPart* m_guardPart = nullptr;      // 守卫部分
    TransitionEffectPart* m_effectPart = nullptr;    // 效果部分
};

SYSML_NAMESPACE_END

#endif // SYSML_TRANSITION_H

