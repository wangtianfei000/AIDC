#ifndef SYSML_ANALYSISSUBJECT_H
#define SYSML_ANALYSISSUBJECT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class AnalysisSubject : public Element {
    Q_OBJECT
    Q_PROPERTY(QString reference READ reference WRITE setReference)
public:
    explicit AnalysisSubject(QObject* parent = nullptr);
    QString elementType() const override { return "AnalysisSubject"; }
    QString reference() const { return m_reference; }
    void setReference(const QString& value) { m_reference = value; }
private:
    QString m_reference;
};

SYSML_NAMESPACE_END
#endif
