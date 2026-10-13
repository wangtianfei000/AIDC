#ifndef SYSML_ENDSTATEMENT_H
#define SYSML_ENDSTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief EndStatement 表示 end 语句
 * 
 * 参考: org.omg.sysml.metamodel.EndFeature
 */
class EndStatement : public Element {
    Q_OBJECT
    
    Q_PROPERTY(QString tag READ tag WRITE setTag NOTIFY tagChanged)
    Q_PROPERTY(QString typeName READ typeName WRITE setTypeName NOTIFY typeNameChanged)
    Q_PROPERTY(bool isNegated READ isNegated WRITE setIsNegated NOTIFY isNegatedChanged)
    Q_PROPERTY(bool usesReferences READ usesReferences WRITE setUsesReferences NOTIFY usesReferencesChanged)
    
public:
    explicit EndStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "EndStatement"; }
    
    QString tag() const { return m_tag; }
    void setTag(const QString& tag);
    
    QString typeName() const { return m_typeName; }
    void setTypeName(const QString& typeName);
    
    bool isNegated() const { return m_isNegated; }
    void setIsNegated(bool isNegated);
    
    // 是否使用 ::> 语法（references）而不是 : 语法（typing）
    bool usesReferences() const { return m_usesReferences; }
    void setUsesReferences(bool usesReferences);
    
signals:
    void tagChanged(const QString& tag);
    void typeNameChanged(const QString& typeName);
    void isNegatedChanged(bool isNegated);
    void usesReferencesChanged(bool usesReferences);
    
private:
    QString m_tag;  // 标签（如 #logical, #physical）
    QString m_typeName;  // 类型名称或引用路径
    bool m_isNegated = false;  // 是否否定（~）
    bool m_usesReferences = false;  // 是否使用 ::> 语法
};

SYSML_NAMESPACE_END

#endif // SYSML_ENDSTATEMENT_H


