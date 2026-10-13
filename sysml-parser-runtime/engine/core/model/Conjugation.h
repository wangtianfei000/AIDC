#ifndef SYSML_CONJUGATION_H
#define SYSML_CONJUGATION_H

#include "Relationship.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class Type;

/**
 * @brief Conjugation 表示共轭关系
 * 
 * 参考: org.omg.sysml.metamodel.Conjugation
 */
class Conjugation : public Relationship {
    Q_OBJECT
    
public:
    explicit Conjugation(QObject* parent = nullptr);
    
    QString elementType() const override { return "Conjugation"; }
    
    // 共轭的类型
    Type* conjugatedType() const;
    void setConjugatedType(Type* type);
    
    // 被共轭的类型
    Type* originalType() const;
    void setOriginalType(Type* type);
    
    // 被共轭的类型名称（字符串形式，用于序列化）
    // 当 originalType() 为 nullptr 时，使用此名称
    QString originalTypeName() const { return m_originalTypeName; }
    void setOriginalTypeName(const QString& name) { m_originalTypeName = name; }
    
signals:
    void conjugatedTypeChanged(Type* type);
    void originalTypeChanged(Type* type);
    
private:
    Type* m_conjugatedType = nullptr;
    Type* m_originalType = nullptr;
    QString m_originalTypeName; // 保存被共轭类型名称字符串
};

SYSML_NAMESPACE_END

#endif // SYSML_CONJUGATION_H

