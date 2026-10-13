#ifndef SYSML_ANALYSISCASE_H
#define SYSML_ANALYSISCASE_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class AnalysisCase : public Type {
    Q_OBJECT
    Q_PROPERTY(QString subjectReference READ subjectReference WRITE setSubjectReference)
public:
    explicit AnalysisCase(QObject* parent = nullptr);
    QString elementType() const override { return "AnalysisCase"; }
    QString subjectReference() const { return m_subjectReference; }
    void setSubjectReference(const QString& value) { m_subjectReference = value; }
private:
    QString m_subjectReference;
};

SYSML_NAMESPACE_END
#endif
