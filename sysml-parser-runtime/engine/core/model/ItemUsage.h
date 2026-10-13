#ifndef SYSML_ITEMUSAGE_H
#define SYSML_ITEMUSAGE_H

#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ItemUsage 表示项使用
 * 
 * 参考: org.omg.sysml.metamodel.ItemUsage
 */
class ItemUsage : public Type {
    Q_OBJECT
    
public:
    enum Direction {
        DirectionNone = 0,
        DirectionIn = 1,
        DirectionOut = 2,
        DirectionInOut = 3
    };
    Q_ENUM(Direction)
    
    enum PortionKind {
        PortionNone = 0,
        PortionSnapshot = 1,
        PortionTimeslice = 2
    };
    Q_ENUM(PortionKind)
    
    Q_PROPERTY(bool isDerived READ isDerived WRITE setIsDerived NOTIFY isDerivedChanged)
    Q_PROPERTY(bool isConstant READ isConstant WRITE setIsConstant NOTIFY isConstantChanged)
    Q_PROPERTY(bool isRef READ isRef WRITE setIsRef NOTIFY isRefChanged)
    Q_PROPERTY(Direction direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(PortionKind portionKind READ portionKind WRITE setPortionKind NOTIFY portionKindChanged)
    
    explicit ItemUsage(QObject* parent = nullptr);
    
    QString elementType() const override { return "ItemUsage"; }
    
    // 类型名称（字符串形式，用于序列化）
    QString typeName() const { return m_typeName; }
    void setTypeName(const QString& name) { m_typeName = name; }
    
    // 使用前缀属性
    bool isDerived() const { return m_isDerived; }
    void setIsDerived(bool isDerived);
    
    bool isConstant() const { return m_isConstant; }
    void setIsConstant(bool isConstant);
    
    bool isRef() const { return m_isRef; }
    void setIsRef(bool isRef);
    
    Direction direction() const { return m_direction; }
    void setDirection(Direction direction);
    
    PortionKind portionKind() const { return m_portionKind; }
    void setPortionKind(PortionKind portionKind);
    
signals:
    void isDerivedChanged(bool isDerived);
    void isConstantChanged(bool isConstant);
    void isRefChanged(bool isRef);
    void directionChanged(Direction direction);
    void portionKindChanged(PortionKind portionKind);
    
private:
    QString m_typeName; // 保存类型名称字符串
    
    // 使用前缀
    bool m_isDerived = false;
    bool m_isConstant = false;
    bool m_isRef = false;
    Direction m_direction = DirectionNone;
    PortionKind m_portionKind = PortionNone;
};

SYSML_NAMESPACE_END

#endif // SYSML_ITEMUSAGE_H

