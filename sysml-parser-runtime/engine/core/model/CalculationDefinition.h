#ifndef SYSML_CALCULATIONDEFINITION_H
#define SYSML_CALCULATIONDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class CalculationDefinition : public Type {
    Q_OBJECT
public:
    explicit CalculationDefinition(QObject* parent = nullptr);
    QString elementType() const override { return "CalculationDefinition"; }
};

SYSML_NAMESPACE_END
#endif
