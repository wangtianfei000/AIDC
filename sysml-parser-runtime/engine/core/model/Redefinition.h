#ifndef SYSML_REDEFINITION_H
#define SYSML_REDEFINITION_H

#include "Relationship.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class Feature;

/**
 * @brief Redefinition 表示重定义关系
 * 
 * 参考: org.omg.sysml.metamodel.Redefinition
 */
class Redefinition : public Relationship {
    Q_OBJECT
    
public:
    explicit Redefinition(QObject* parent = nullptr);
    
    QString elementType() const override { return "Redefinition"; }
    
    // 重定义的特征
    Feature* redefiningFeature() const;
    void setRedefiningFeature(Feature* feature);
    
    // 被重定义的特征
    Feature* redefinedFeature() const;
    void setRedefinedFeature(Feature* feature);
    
    // 被重定义的特征名称（字符串形式，用于序列化）
    // 当 redefinedFeature() 为 nullptr 时，使用此名称
    QString redefinedFeatureName() const { return m_redefinedFeatureName; }
    void setRedefinedFeatureName(const QString& name) { m_redefinedFeatureName = name; }
    
signals:
    void redefiningFeatureChanged(Feature* feature);
    void redefinedFeatureChanged(Feature* feature);
    
private:
    Feature* m_redefiningFeature = nullptr;
    Feature* m_redefinedFeature = nullptr;
    QString m_redefinedFeatureName; // 保存被重定义特征名称字符串
};

SYSML_NAMESPACE_END

#endif // SYSML_REDEFINITION_H

