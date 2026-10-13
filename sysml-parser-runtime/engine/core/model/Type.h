#ifndef SYSML_TYPE_H
#define SYSML_TYPE_H

#include "Element.h"
#include "Namespace.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

// 前向声明
class Feature;

/**
 * @brief Type 是所有类型的基类
 * 
 * 参考: org.omg.sysml.metamodel.Type
 */
class Type : public Element, public Namespace {
    Q_OBJECT
    Q_PROPERTY(bool isAbstract READ isAbstract WRITE setIsAbstract NOTIFY isAbstractChanged)
    Q_PROPERTY(bool isVariation READ isVariation WRITE setIsVariation NOTIFY isVariationChanged)
    Q_PROPERTY(bool isIndividual READ isIndividual WRITE setIsIndividual NOTIFY isIndividualChanged)
    
public:
    explicit Type(QObject* parent = nullptr);
    
    QString elementType() const override { return "Type"; }
    
    // 定义前缀属性
    bool isAbstract() const { return m_isAbstract; }
    void setIsAbstract(bool isAbstract);
    
    bool isVariation() const { return m_isVariation; }
    void setIsVariation(bool isVariation);
    
    bool isIndividual() const { return m_isIndividual; }
    void setIsIndividual(bool isIndividual);
    
    // Namespace 接口实现（默认实现）
    QList<class Membership*> memberships() const override;
    void addMembership(class Membership* membership) override;
    void removeMembership(class Membership* membership) override;
    
    QList<class Import*> imports() const override;
    void addImport(class Import* import) override;
    void removeImport(class Import* import) override;
    
    Element* findElement(const QString& name) const override;
    QList<Element*> findElements(const QString& name) const override;
    
    // 特征
    QList<Feature*> ownedFeatures() const;
    void addOwnedFeature(Feature* feature);
    void removeOwnedFeature(Feature* feature);
    
signals:
    void featureAdded(Feature* feature);
    void featureRemoved(Feature* feature);
    void isAbstractChanged(bool isAbstract);
    void isVariationChanged(bool isVariation);
    void isIndividualChanged(bool isIndividual);
    
protected:
    QList<Feature*> m_ownedFeatures;
    QList<class Membership*> m_memberships;
    QList<class Import*> m_imports;
    
    // 定义前缀
    bool m_isAbstract = false;
    bool m_isVariation = false;
    bool m_isIndividual = false;
};

SYSML_NAMESPACE_END

#endif // SYSML_TYPE_H



