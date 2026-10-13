#ifndef SYSML_NAMESPACE_H
#define SYSML_NAMESPACE_H

#include "../common/Global.h"
#include <QString>
#include <QList>

SYSML_NAMESPACE_BEGIN

// 前向声明
class Element;
class Membership;
class Import;

/**
 * @brief Namespace 接口，定义命名空间的行为
 * 
 * 参考: org.omg.sysml.metamodel.Namespace
 */
class Namespace {
public:
    virtual ~Namespace() = default;
    
    // 成员关系
    virtual QList<Membership*> memberships() const = 0;
    virtual void addMembership(Membership* membership) = 0;
    virtual void removeMembership(Membership* membership) = 0;
    
    // 导入
    virtual QList<Import*> imports() const = 0;
    virtual void addImport(Import* import) = 0;
    virtual void removeImport(Import* import) = 0;
    
    // 查找
    virtual Element* findElement(const QString& name) const = 0;
    virtual QList<Element*> findElements(const QString& name) const = 0;
};

SYSML_NAMESPACE_END

#endif // SYSML_NAMESPACE_H

