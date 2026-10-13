#ifndef SYSML_ANNOTATION_H
#define SYSML_ANNOTATION_H

#include "Element.h"
#include <QVariant>
#include <QVariantMap>

SYSML_NAMESPACE_BEGIN

/**
 * @brief Annotation 注解类
 * 
 * 存储元素的注解信息，包括GraphicMetadata等
 * 注解格式：annotation <GraphicMetadata> { "key": value, ... }
 */
class Annotation : public Element {
    Q_OBJECT
    Q_PROPERTY(QString annotationName READ annotationName WRITE setAnnotationName NOTIFY annotationNameChanged)
    Q_PROPERTY(QVariantMap metadataAssignments READ metadataAssignments NOTIFY metadataAssignmentsChanged)
    
public:
    explicit Annotation(QObject* parent = nullptr);
    ~Annotation() override = default;
    
    // 元素类型
    QString elementType() const override { return "Annotation"; }
    
    // 注解名称（如 "GraphicMetadata"）
    QString annotationName() const { return m_annotationName; }
    void setAnnotationName(const QString& name);
    
    // 元数据赋值（key-value对）
    QVariantMap metadataAssignments() const { return m_metadataAssignments; }
    void setMetadataAssignment(const QString& key, const QVariant& value);
    QVariant getMetadataAssignment(const QString& key) const;
    bool hasMetadataAssignment(const QString& key) const;
    
    // 批量设置
    void setMetadataAssignments(const QVariantMap& assignments);
    void clearMetadataAssignments();

signals:
    void annotationNameChanged(const QString& name);
    void metadataAssignmentsChanged();
    void metadataAssignmentChanged(const QString& key, const QVariant& value);

private:
    QString m_annotationName;  // 注解名称，如 "GraphicMetadata"
    QVariantMap m_metadataAssignments;  // 元数据赋值，如 "graphicX": 200.0
};

SYSML_NAMESPACE_END

Q_DECLARE_METATYPE(SysML::Annotation*)

#endif // SYSML_ANNOTATION_H
