#include "FeatureTyping.h"
#include "Feature.h"
#include "Type.h"

SYSML_NAMESPACE_BEGIN

FeatureTyping::FeatureTyping(QObject* parent)
    : Relationship(parent)
{
}

Feature* FeatureTyping::typedFeature() const
{
    return m_typedFeature;
}

void FeatureTyping::setTypedFeature(Feature* feature)
{
    if (m_typedFeature != feature) {
        m_typedFeature = feature;
        emit typedFeatureChanged(feature);
    }
}

Type* FeatureTyping::type() const
{
    return m_type;
}

void FeatureTyping::setType(Type* type)
{
    if (m_type != type) {
        m_type = type;
        emit typeChanged(type);
    }
}

SYSML_NAMESPACE_END



