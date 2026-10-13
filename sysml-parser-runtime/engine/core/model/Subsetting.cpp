#include "Subsetting.h"
#include "Feature.h"

SYSML_NAMESPACE_BEGIN

Subsetting::Subsetting(QObject* parent)
    : Relationship(parent)
{
}

Feature* Subsetting::subsettingFeature() const
{
    return m_subsettingFeature;
}

void Subsetting::setSubsettingFeature(Feature* feature)
{
    if (m_subsettingFeature != feature) {
        m_subsettingFeature = feature;
        emit subsettingFeatureChanged(feature);
    }
}

Feature* Subsetting::subsettedFeature() const
{
    return m_subsettedFeature;
}

void Subsetting::setSubsettedFeature(Feature* feature)
{
    if (m_subsettedFeature != feature) {
        m_subsettedFeature = feature;
        emit subsettedFeatureChanged(feature);
    }
}

SYSML_NAMESPACE_END





