#ifndef SYSML_ELEMENTFILTERMEMBER_H
#define SYSML_ELEMENTFILTERMEMBER_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ElementFilterMember 表示元素过滤成员
 * 
 * 用于 view 定义和 view 使用中的 filter 语句
 */
class ElementFilterMember : public Element {
    Q_OBJECT
    
public:
    explicit ElementFilterMember(QObject* parent = nullptr);
    
    QString elementType() const override { return "ElementFilterMember"; }
    
    QString filterExpression() const { return m_filterExpression; }
    void setFilterExpression(const QString& expression) { m_filterExpression = expression; }
    
private:
    QString m_filterExpression;
};

SYSML_NAMESPACE_END

#endif // SYSML_ELEMENTFILTERMEMBER_H


