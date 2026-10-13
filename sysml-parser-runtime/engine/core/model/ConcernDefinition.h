#ifndef SYSML_CONCERNDEFINITION_H
#define SYSML_CONCERNDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ConcernDefinition 表示关注点定义
 * 
 * 参考: org.omg.sysml.metamodel.ConcernDefinition
 */
class ConcernDefinition : public Type {
    Q_OBJECT
    
public:
    explicit ConcernDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "ConcernDefinition"; }
};

SYSML_NAMESPACE_END

#endif // SYSML_CONCERNDEFINITION_H


