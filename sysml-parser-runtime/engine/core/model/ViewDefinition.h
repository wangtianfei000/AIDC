#ifndef SYSML_VIEWDEFINITION_H
#define SYSML_VIEWDEFINITION_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ViewDefinition 表示视图定义
 * 
 * 参考: org.omg.sysml.metamodel.ViewDefinition
 */
class ViewDefinition : public Type {
    Q_OBJECT
    Q_PROPERTY(QString viewType READ viewType WRITE setViewType)
    
public:
    explicit ViewDefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "ViewDefinition"; }
    
    // 过滤表达式
    QString filterExpression() const { return m_filterExpression; }
    void setFilterExpression(const QString& expression) { m_filterExpression = expression; }
    
    // 渲染使用
    Element* renderingUsage() const { return m_renderingUsage; }
    void setRenderingUsage(Element* usage) { m_renderingUsage = usage; }

    QString viewType() const { return m_viewType; }
    void setViewType(const QString& value) { m_viewType = value; }
    
private:
    QString m_filterExpression;
    Element* m_renderingUsage = nullptr;
    QString m_viewType;
};

SYSML_NAMESPACE_END

#endif // SYSML_VIEWDEFINITION_H


