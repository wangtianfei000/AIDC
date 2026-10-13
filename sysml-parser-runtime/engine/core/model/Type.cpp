#include "Type.h"
#include "Feature.h"
#include "Membership.h"
#include "Import.h"

SYSML_NAMESPACE_BEGIN

Type::Type(QObject* parent)
    : Element(parent)
{
}

QList<Membership*> Type::memberships() const
{
    return m_memberships;
}

void Type::addMembership(Membership* membership)
{
    if (membership && !m_memberships.contains(membership)) {
        m_memberships.append(membership);
        membership->setParent(this);
    }
}

void Type::removeMembership(Membership* membership)
{
    m_memberships.removeOne(membership);
}

QList<Import*> Type::imports() const
{
    return m_imports;
}

void Type::addImport(Import* import)
{
    if (import && !m_imports.contains(import)) {
        m_imports.append(import);
        import->setParent(this);
    }
}

void Type::removeImport(Import* import)
{
    m_imports.removeOne(import);
}

Element* Type::findElement(const QString& name) const
{
    // 在拥有的元素中查找
    for (Element* element : ownedElements()) {
        if (element->declaredName() == name) {
            return element;
        }
    }
    
    // 在特征中查找
    for (Feature* feature : m_ownedFeatures) {
        if (feature->declaredName() == name) {
            return feature;
        }
    }
    
    return nullptr;
}

QList<Element*> Type::findElements(const QString& name) const
{
    QList<Element*> results;
    
    // 在拥有的元素中查找
    for (Element* element : ownedElements()) {
        if (element->declaredName() == name) {
            results.append(element);
        }
    }
    
    // 在特征中查找
    for (Feature* feature : m_ownedFeatures) {
        if (feature->declaredName() == name) {
            results.append(feature);
        }
    }
    
    return results;
}

QList<Feature*> Type::ownedFeatures() const
{
    return m_ownedFeatures;
}

void Type::addOwnedFeature(Feature* feature)
{
    if (feature && !m_ownedFeatures.contains(feature)) {
        m_ownedFeatures.append(feature);
        feature->setOwner(this);
        emit featureAdded(feature);
    }
}

void Type::removeOwnedFeature(Feature* feature)
{
    if (m_ownedFeatures.removeOne(feature)) {
        feature->setOwner(nullptr);
        emit featureRemoved(feature);
    }
}

void Type::setIsAbstract(bool isAbstract)
{
    if (m_isAbstract != isAbstract) {
        m_isAbstract = isAbstract;
        emit isAbstractChanged(isAbstract);
    }
}

void Type::setIsVariation(bool isVariation)
{
    if (m_isVariation != isVariation) {
        m_isVariation = isVariation;
        emit isVariationChanged(isVariation);
    }
}

void Type::setIsIndividual(bool isIndividual)
{
    if (m_isIndividual != isIndividual) {
        m_isIndividual = isIndividual;
        emit isIndividualChanged(isIndividual);
    }
}

SYSML_NAMESPACE_END



