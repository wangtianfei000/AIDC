#include "Package.h"
#include "Membership.h"
#include "Import.h"

SYSML_NAMESPACE_BEGIN

Package::Package(QObject* parent)
    : Type(parent)
{
}

QList<Membership*> Package::memberships() const
{
    return Type::memberships();
}

void Package::addMembership(Membership* membership)
{
    Type::addMembership(membership);
    emit membershipAdded(membership);
}

void Package::removeMembership(Membership* membership)
{
    Type::removeMembership(membership);
    emit membershipRemoved(membership);
}

QList<Import*> Package::imports() const
{
    return Type::imports();
}

void Package::addImport(Import* import)
{
    Type::addImport(import);
    emit importAdded(import);
}

void Package::removeImport(Import* import)
{
    Type::removeImport(import);
    emit importRemoved(import);
}

Element* Package::findElement(const QString& name) const
{
    // 先调用基类方法
    Element* found = Type::findElement(name);
    if (found) {
        return found;
    }
    
    // 在导入的元素中查找
    for (Import* import : imports()) {
        Q_UNUSED(import);
        // TODO: 实现导入元素的查找
    }
    
    return nullptr;
}

QList<Element*> Package::findElements(const QString& name) const
{
    QList<Element*> results = Type::findElements(name);
    
    // TODO: 在导入的元素中查找
    
    return results;
}

SYSML_NAMESPACE_END



