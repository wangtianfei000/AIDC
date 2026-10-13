#include "Annotation.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

Annotation::Annotation(QObject* parent)
    : Element(parent)
{
}

void Annotation::setAnnotationName(const QString& name)
{
    if (m_annotationName != name) {
        m_annotationName = name;
        emit annotationNameChanged(name);
        notifyChanged();
    }
}

void Annotation::setMetadataAssignment(const QString& key, const QVariant& value)
{
    if (m_metadataAssignments.value(key) != value) {
        m_metadataAssignments[key] = value;
        emit metadataAssignmentChanged(key, value);
        emit metadataAssignmentsChanged();
        notifyChanged();
    }
}

QVariant Annotation::getMetadataAssignment(const QString& key) const
{
    return m_metadataAssignments.value(key);
}

bool Annotation::hasMetadataAssignment(const QString& key) const
{
    return m_metadataAssignments.contains(key);
}

void Annotation::setMetadataAssignments(const QVariantMap& assignments)
{
    if (m_metadataAssignments != assignments) {
        m_metadataAssignments = assignments;
        emit metadataAssignmentsChanged();
        notifyChanged();
    }
}

void Annotation::clearMetadataAssignments()
{
    if (!m_metadataAssignments.isEmpty()) {
        m_metadataAssignments.clear();
        emit metadataAssignmentsChanged();
        notifyChanged();
    }
}

SYSML_NAMESPACE_END
