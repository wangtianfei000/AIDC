#include "Redefinition.h"
#include "Feature.h"

SYSML_NAMESPACE_BEGIN

Redefinition::Redefinition(QObject* parent)
    : Relationship(parent)
{
}

Feature* Redefinition::redefiningFeature() const
{
    return m_redefiningFeature;
}

void Redefinition::setRedefiningFeature(Feature* feature)
{
    if (m_redefiningFeature != feature) {
        m_redefiningFeature = feature;
        emit redefiningFeatureChanged(feature);
    }
}

Feature* Redefinition::redefinedFeature() const
{
    return m_redefinedFeature;
}

void Redefinition::setRedefinedFeature(Feature* feature)
{
    if (m_redefinedFeature != feature) {
        m_redefinedFeature = feature;
        emit redefinedFeatureChanged(feature);
    }
}

SYSML_NAMESPACE_END





