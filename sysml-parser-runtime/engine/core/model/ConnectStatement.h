#ifndef SYSML_CONNECTSTATEMENT_H
#define SYSML_CONNECTSTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ConnectStatement 表示 connect 语句
 * 
 * 参考: org.omg.sysml.metamodel.Connector
 */
class ConnectStatement : public Element {
    Q_OBJECT
    
    Q_PROPERTY(QStringList ends READ ends WRITE setEnds NOTIFY endsChanged)
    Q_PROPERTY(bool isBinary READ isBinary WRITE setIsBinary NOTIFY isBinaryChanged)
    
public:
    explicit ConnectStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "ConnectStatement"; }
    
    QStringList ends() const { return m_ends; }
    void setEnds(const QStringList& ends);
    
    bool isBinary() const { return m_isBinary; }
    void setIsBinary(bool isBinary);
    
signals:
    void endsChanged(const QStringList& ends);
    void isBinaryChanged(bool isBinary);
    
private:
    QStringList m_ends;  // 端点列表（二元连接时只有两个，n元连接时有多个）
    bool m_isBinary = true;  // 是否为二元连接
};

SYSML_NAMESPACE_END

#endif // SYSML_CONNECTSTATEMENT_H




