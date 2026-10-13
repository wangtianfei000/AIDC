#ifndef SYSML_FEATURETYPING_H
#define SYSML_FEATURETYPING_H

#include "Relationship.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class Feature;
class Type;

/**
 * @brief FeatureTyping 表示特征类型化关系
 * 
 * 参考: org.omg.sysml.metamodel.FeatureTyping
 */
class FeatureTyping : public Relationship {
    Q_OBJECT
    
public:
    explicit FeatureTyping(QObject* parent = nullptr);
    
    QString elementType() const override { return "FeatureTyping"; }
    
    // 类型化的特征
    Feature* typedFeature() const;
    void setTypedFeature(Feature* feature);
    
    // 类型
    Type* type() const;
    void setType(Type* type);
    
    // 类型名称（字符串形式，用于序列化）
    // 当 type() 为 nullptr 时，使用此名称
    QString typeName() const { return m_typeName; }
    void setTypeName(const QString& name) { m_typeName = name; }
    
signals:
    void typedFeatureChanged(Feature* feature);
    void typeChanged(Type* type);
    
private:
    Feature* m_typedFeature = nullptr;
    Type* m_type = nullptr;
    QString m_typeName; // 保存类型名称字符串
};

SYSML_NAMESPACE_END

#endif // SYSML_FEATURETYPING_H



