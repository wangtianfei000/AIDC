#include "Transition.h"
#include "TransitionTriggerPart.h"
#include "TransitionGuardPart.h"
#include "TransitionEffectPart.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

Transition::Transition(QObject* parent)
    : Element(parent)
{
}

void Transition::setSourceState(const QString& state)
{
    if (m_sourceState != state) {
        m_sourceState = state;
        emit sourceStateChanged(state);
        emit elementChanged();
    }
}

void Transition::setTargetState(const QString& state)
{
    if (m_targetState != state) {
        m_targetState = state;
        emit targetStateChanged(state);
        emit elementChanged();
    }
}

void Transition::setIsInitial(bool initial)
{
    if (m_isInitial != initial) {
        m_isInitial = initial;
        emit isInitialChanged(initial);
        emit elementChanged();
    }
}

void Transition::setTriggerPart(TransitionTriggerPart* part)
{
    if (m_triggerPart != part) {
        if (m_triggerPart) {
            m_triggerPart->setParent(nullptr);
        }
        m_triggerPart = part;
        if (m_triggerPart) {
            m_triggerPart->setParent(this);
        }
        emit triggerPartChanged(part);
        emit elementChanged();
    }
}

void Transition::setGuardPart(TransitionGuardPart* part)
{
    if (m_guardPart != part) {
        if (m_guardPart) {
            m_guardPart->setParent(nullptr);
        }
        m_guardPart = part;
        if (m_guardPart) {
            m_guardPart->setParent(this);
        }
        emit guardPartChanged(part);
        emit elementChanged();
    }
}

void Transition::setEffectPart(TransitionEffectPart* part)
{
    if (m_effectPart != part) {
        if (m_effectPart) {
            m_effectPart->setParent(nullptr);
        }
        m_effectPart = part;
        if (m_effectPart) {
            m_effectPart->setParent(this);
        }
        emit effectPartChanged(part);
        emit elementChanged();
    }
}

SYSML_NAMESPACE_END


