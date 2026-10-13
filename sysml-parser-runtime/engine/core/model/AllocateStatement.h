#ifndef SYSML_ALLOCATESTATEMENT_H
#define SYSML_ALLOCATESTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief AllocateStatement 表示 allocate 语句
 * 
 * 参考: org.omg.sysml.metamodel.Allocation
 */
class AllocateStatement : public Element {
    Q_OBJECT
    
    Q_PROPERTY(QStringList sourceEnds READ sourceEnds WRITE setSourceEnds NOTIFY sourceEndsChanged)
    Q_PROPERTY(QString targetEnd READ targetEnd WRITE setTargetEnd NOTIFY targetEndChanged)
    Q_PROPERTY(bool isBinary READ isBinary WRITE setIsBinary NOTIFY isBinaryChanged)
    
public:
    explicit AllocateStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "AllocateStatement"; }
    
    QStringList sourceEnds() const { return m_sourceEnds; }
    void setSourceEnds(const QStringList& sourceEnds);
    
    QString targetEnd() const { return m_targetEnd; }
    void setTargetEnd(const QString& targetEnd);
    
    bool isBinary() const { return m_isBinary; }
    void setIsBinary(bool isBinary);
    
signals:
    void sourceEndsChanged(const QStringList& sourceEnds);
    void targetEndChanged(const QString& targetEnd);
    void isBinaryChanged(bool isBinary);
    
private:
    QStringList m_sourceEnds;  // 源端点列表（二元时只有一个，n元时有多个）
    QString m_targetEnd;  // 目标端点
    bool m_isBinary = true;  // 是否为二元分配
};

SYSML_NAMESPACE_END

#endif // SYSML_ALLOCATESTATEMENT_H




