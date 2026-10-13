#ifndef SYSML_FEATURE_H
#define SYSML_FEATURE_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief Feature 是所有特征的基类
 * 
 * 参考: org.omg.sysml.metamodel.Feature
 */
class Feature : public Element {
    Q_OBJECT
    
public:
    // 方向枚举
    enum Direction {
        DirectionNone,  // 无方向
        DirectionIn,    // 输入
        DirectionOut,   // 输出
        DirectionInOut  // 输入输出
    };
    Q_ENUM(Direction)
    
    explicit Feature(QObject* parent = nullptr);
    
    QString elementType() const override { return "Feature"; }
    
    // 特征前缀属性
    bool isAbstract() const { return m_isAbstract; }
    void setIsAbstract(bool isAbstract);
    
    bool isConst() const { return m_isConst; }
    void setIsConst(bool isConst);
    
    bool isDerived() const { return m_isDerived; }
    void setIsDerived(bool isDerived);
    
    Direction direction() const { return m_direction; }
    void setDirection(Direction direction);
    
    bool isRef() const { return m_isRef; }
    void setIsRef(bool isRef);
    
    // 单位表达式支持
    QString unitExpression() const { return m_unitExpression; }
    void setUnitExpression(const QString& unitExpression);
    
    // 值表达式支持（用于存储 attribute = value 中的值）
    QString valueExpression() const { return m_valueExpression; }
    void setValueExpression(const QString& valueExpression);
    
    // 简化属性标志（用于序列化：name : Type; 而不是 attribute name : Type;）
    bool isSimplifiedAttribute() const { return m_isSimplifiedAttribute; }
    void setIsSimplifiedAttribute(bool isSimplified);
    
    // redefines 支持（用于 attribute redefines name = value）
    bool isRedefines() const { return m_isRedefines; }
    void setIsRedefines(bool isRedefines);
    
    // 默认值表达式（用于 attribute redefines name default value）
    QString defaultValue() const { return m_defaultValue; }
    void setDefaultValue(const QString& defaultValue);

signals:
    void isAbstractChanged(bool isAbstract);
    void isConstChanged(bool isConst);
    void isDerivedChanged(bool isDerived);
    void directionChanged(Direction direction);
    void isRefChanged(bool isRef);
    void unitExpressionChanged(const QString& unitExpression);
    void valueExpressionChanged(const QString& valueExpression);
    void isSimplifiedAttributeChanged(bool isSimplified);
    void isRedefinesChanged(bool isRedefines);
    void defaultValueChanged(const QString& defaultValue);

private:
    bool m_isAbstract = false;
    bool m_isConst = false;
    bool m_isDerived = false;
    Direction m_direction = DirectionNone;
    bool m_isRef = false;
    QString m_unitExpression; // 单位表达式，如 "kg", "km / L"
    QString m_valueExpression; // 值表达式，如 "2000 [kg]", "vehicle.dryMass + fuelMassActual"
    bool m_isSimplifiedAttribute = false; // 是否为简化属性
    bool m_isRedefines = false; // 是否为 redefines
    QString m_defaultValue; // 默认值表达式，如 "vehicle.dryMass + fuelMassActual"
};

SYSML_NAMESPACE_END

#endif // SYSML_FEATURE_H



