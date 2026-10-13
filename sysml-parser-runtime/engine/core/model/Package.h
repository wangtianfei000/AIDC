#ifndef SYSML_PACKAGE_H
#define SYSML_PACKAGE_H

#include "Element.h"
#include "Type.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief Package 表示 SysML 包
 * 
 * 参考: org.omg.sysml.metamodel.Package
 */
class Package : public Type {
    Q_OBJECT
    
public:
    explicit Package(QObject* parent = nullptr);
    
    QString elementType() const override { return "Package"; }
    
    // Namespace 接口实现
    QList<Membership*> memberships() const override;
    void addMembership(Membership* membership) override;
    void removeMembership(Membership* membership) override;
    
    QList<class Import*> imports() const override;
    void addImport(class Import* import) override;
    void removeImport(class Import* import) override;
    
    Element* findElement(const QString& name) const override;
    QList<Element*> findElements(const QString& name) const override;
    
signals:
    void membershipAdded(Membership* membership);
    void membershipRemoved(Membership* membership);
    void importAdded(class Import* import);
    void importRemoved(class Import* import);
};

SYSML_NAMESPACE_END

#endif // SYSML_PACKAGE_H



