#ifndef SYSML_RETURNPARAMETER_H
#define SYSML_RETURNPARAMETER_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class ReturnParameter : public Element {
    Q_OBJECT
    Q_PROPERTY(QString typeName READ typeName WRITE setTypeName)
    Q_PROPERTY(QString expression READ expression WRITE setExpression)
public:
    explicit ReturnParameter(QObject* parent = nullptr);
    QString elementType() const override { return "ReturnParameter"; }
    QString typeName() const { return m_typeName; }
    void setTypeName(const QString& value) { m_typeName = value; }
    QString expression() const { return m_expression; }
    void setExpression(const QString& value) { m_expression = value; }
private:
    QString m_typeName;
    QString m_expression;
};

SYSML_NAMESPACE_END
#endif
