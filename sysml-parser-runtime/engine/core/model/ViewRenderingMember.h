#ifndef SYSML_VIEWRENDERINGMEMBER_H
#define SYSML_VIEWRENDERINGMEMBER_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ViewRenderingMember 表示视图渲染成员
 * 
 * 用于 view 定义和 view 使用中的 render 语句
 */
class ViewRenderingMember : public Element {
    Q_OBJECT
    
public:
    explicit ViewRenderingMember(QObject* parent = nullptr);
    
    QString elementType() const override { return "ViewRenderingMember"; }
    
    Element* renderingUsage() const { return m_renderingUsage; }
    void setRenderingUsage(Element* usage) { m_renderingUsage = usage; }
    
private:
    Element* m_renderingUsage = nullptr;
};

SYSML_NAMESPACE_END

#endif // SYSML_VIEWRENDERINGMEMBER_H


