#ifndef SYSML_SUBSETTING_H
#define SYSML_SUBSETTING_H

#include "Relationship.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class Feature;

/**
 * @brief Subsetting 表示子集化关系
 * 
 * 参考: org.omg.sysml.metamodel.Subsetting
 */
class Subsetting : public Relationship {
    Q_OBJECT
    
public:
    explicit Subsetting(QObject* parent = nullptr);
    
    QString elementType() const override { return "Subsetting"; }
    
    // 子集化的特征
    Feature* subsettingFeature() const;
    void setSubsettingFeature(Feature* feature);
    
    // 被子集化的特征
    Feature* subsettedFeature() const;
    void setSubsettedFeature(Feature* feature);
    
    // 被子集化的特征名称（字符串形式，用于序列化）
    // 当 subsettedFeature() 为 nullptr 时，使用此名称
    QString subsettedFeatureName() const { return m_subsettedFeatureName; }
    void setSubsettedFeatureName(const QString& name) { m_subsettedFeatureName = name; }
    
signals:
    void subsettingFeatureChanged(Feature* feature);
    void subsettedFeatureChanged(Feature* feature);
    
private:
    Feature* m_subsettingFeature = nullptr;
    Feature* m_subsettedFeature = nullptr;
    QString m_subsettedFeatureName; // 保存被子集化特征名称字符串
};

SYSML_NAMESPACE_END

#endif // SYSML_SUBSETTING_H

