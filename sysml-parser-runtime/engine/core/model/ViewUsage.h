#ifndef SYSML_VIEWUSAGE_H
#define SYSML_VIEWUSAGE_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

// 前向声明
class ViewDefinition;

/**
 * @brief ViewUsage 表示视图使用
 * 
 * 参考: org.omg.sysml.metamodel.ViewUsage
 */
class ViewUsage : public Type {
    Q_OBJECT
    
public:
    explicit ViewUsage(QObject* parent = nullptr);
    
    QString elementType() const override { return "ViewUsage"; }
    
    // 暴露的元素列表
    QList<Element*> exposedElements() const { return m_exposedElements; }
    void addExposedElement(Element* element);
    void removeExposedElement(Element* element);
    
    // 视图类型（ViewDefinition 的引用）
    ViewDefinition* viewDefinition() const { return m_viewDefinition; }
    void setViewDefinition(ViewDefinition* viewDef) { m_viewDefinition = viewDef; }
    
    // 满足的 viewpoint
    Element* satisfiedViewpoint() const { return m_satisfiedViewpoint; }
    void setSatisfiedViewpoint(Element* viewpoint) { m_satisfiedViewpoint = viewpoint; }
    
    // 满足的 viewpoint 标识符（如果有，如 sv:SafetyViewpoint 中的 sv）
    QString satisfiedViewpointIdentifier() const { return m_satisfiedViewpointIdentifier; }
    void setSatisfiedViewpointIdentifier(const QString& identifier) { m_satisfiedViewpointIdentifier = identifier; }
    
    // 过滤表达式
    QString filterExpression() const { return m_filterExpression; }
    void setFilterExpression(const QString& expression) { m_filterExpression = expression; }
    
    // 类型名称（字符串形式，用于序列化）
    QString typeName() const { return m_typeName; }
    void setTypeName(const QString& name) { m_typeName = name; }
    
    // 渲染方式（通过 render 语句）
    QString renderMethod() const { return m_renderMethod; }
    void setRenderMethod(const QString& method) { m_renderMethod = method; }
    
    // 检查是否为需求表格视图
    bool isRequirementTableView() const {
        return m_renderMethod == "asRequirementTable";
    }
    
private:
    QList<Element*> m_exposedElements;
    ViewDefinition* m_viewDefinition = nullptr;
    Element* m_satisfiedViewpoint = nullptr;
    QString m_satisfiedViewpointIdentifier;  // 存储标识符（如果有）
    QString m_filterExpression;
    QString m_typeName; // 保存类型名称字符串
    QString m_renderMethod;  // 例如 "asRequirementTable"
};

SYSML_NAMESPACE_END

#endif // SYSML_VIEWUSAGE_H

