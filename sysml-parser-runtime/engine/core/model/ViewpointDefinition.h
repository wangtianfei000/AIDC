#ifndef SYSML_VIEWPOINTDEFINITION_H
#define SYSML_VIEWPOINTDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ViewpointDefinition 表示视点定义
 * 
 * 参考: org.omg.sysml.metamodel.ViewpointDefinition
 */
class ViewpointDefinition : public Type {
    Q_OBJECT
    
public:
    explicit ViewpointDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "ViewpointDefinition"; }
    
    // 框架关注点
    QList<Element*> framedConcerns() const { return m_framedConcerns; }
    void addFramedConcern(Element* concern);
    void removeFramedConcern(Element* concern);
    
private:
    QList<Element*> m_framedConcerns;
};

SYSML_NAMESPACE_END

#endif // SYSML_VIEWPOINTDEFINITION_H


