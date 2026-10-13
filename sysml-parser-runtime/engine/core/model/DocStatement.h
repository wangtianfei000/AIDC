#ifndef SYSML_DOCSTATEMENT_H
#define SYSML_DOCSTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief DocStatement 表示 doc 语句
 * 
 * 参考: org.omg.sysml.metamodel.Documentation
 */
class DocStatement : public Element {
    Q_OBJECT
    
    Q_PROPERTY(QString content READ content WRITE setContent NOTIFY contentChanged)
    
public:
    explicit DocStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "DocStatement"; }
    
    QString content() const { return m_content; }
    void setContent(const QString& content);
    
signals:
    void contentChanged(const QString& content);
    
private:
    QString m_content;  // 文档内容（注释文本）
};

SYSML_NAMESPACE_END

#endif // SYSML_DOCSTATEMENT_H


