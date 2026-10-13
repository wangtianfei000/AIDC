#ifndef SYSML_RENDERINGUSAGE_H
#define SYSML_RENDERINGUSAGE_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class RenderingUsage : public Type {
    Q_OBJECT
    Q_PROPERTY(QString renderingType READ renderingType WRITE setRenderingType)
public:
    explicit RenderingUsage(QObject* parent = nullptr);
    QString elementType() const override { return "RenderingUsage"; }
    QString renderingType() const { return m_renderingType; }
    void setRenderingType(const QString& value) { m_renderingType = value; }
private:
    QString m_renderingType;
};

SYSML_NAMESPACE_END
#endif
