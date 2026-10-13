#ifndef SYSML_ACTIONINPUTOUTPUT_H
#define SYSML_ACTIONINPUTOUTPUT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ActionInputOutput 表示 in item / out item 语法
 * 
 * 参考: org.omg.sysml.metamodel.ActionUsage (作为输入/输出项)
 */
class ActionInputOutput : public Element {
    Q_OBJECT
    
    Q_PROPERTY(bool isInput READ isInput WRITE setIsInput NOTIFY isInputChanged)
    Q_PROPERTY(bool hasItemKeyword READ hasItemKeyword WRITE setHasItemKeyword NOTIFY hasItemKeywordChanged)
    Q_PROPERTY(QString itemName READ itemName WRITE setItemName NOTIFY itemNameChanged)
    Q_PROPERTY(QString itemType READ itemType WRITE setItemType NOTIFY itemTypeChanged)
    Q_PROPERTY(QString defaultValue READ defaultValue WRITE setDefaultValue NOTIFY defaultValueChanged)
    Q_PROPERTY(QString redefinesName READ redefinesName WRITE setRedefinesName NOTIFY redefinesNameChanged)
    Q_PROPERTY(QString multiplicity READ multiplicity WRITE setMultiplicity NOTIFY multiplicityChanged)
    
public:
    explicit ActionInputOutput(QObject* parent = nullptr);
    
    QString elementType() const override { return "ActionInputOutput"; }
    
    bool isInput() const { return m_isInput; }
    void setIsInput(bool isInput);
    
    bool hasItemKeyword() const { return m_hasItemKeyword; }
    void setHasItemKeyword(bool hasItemKeyword);
    
    QString itemName() const { return m_itemName; }
    void setItemName(const QString& itemName);
    
    QString itemType() const { return m_itemType; }
    void setItemType(const QString& itemType);
    
    QString defaultValue() const { return m_defaultValue; }
    void setDefaultValue(const QString& defaultValue);
    
    QString redefinesName() const { return m_redefinesName; }
    void setRedefinesName(const QString& redefinesName);
    
    QString multiplicity() const { return m_multiplicity; }
    void setMultiplicity(const QString& multiplicity);
    
signals:
    void isInputChanged(bool isInput);
    void hasItemKeywordChanged(bool hasItemKeyword);
    void itemNameChanged(const QString& itemName);
    void itemTypeChanged(const QString& itemType);
    void defaultValueChanged(const QString& defaultValue);
    void redefinesNameChanged(const QString& redefinesName);
    void multiplicityChanged(const QString& multiplicity);
    
private:
    bool m_isInput = true;  // true = in, false = out
    bool m_hasItemKeyword = false;  // 是否有 'item' 关键字
    QString m_itemName;  // 项名称（usageDeclaration 中的名称）
    QString m_itemType;  // 项类型（usageDeclaration 中的类型）
    QString m_defaultValue;  // 默认值（valuePart 中的值）
    QString m_redefinesName;  // 重定义名称（redefines 后面的名称）
    QString m_multiplicity;  // 多重性（如 [2] 或 [1..2]）
};

SYSML_NAMESPACE_END

#endif // SYSML_ACTIONINPUTOUTPUT_H

