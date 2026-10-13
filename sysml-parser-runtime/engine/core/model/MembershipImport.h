#ifndef SYSML_MEMBERSHIPIMPORT_H
#define SYSML_MEMBERSHIPIMPORT_H

#include "Import.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class Membership;

/**
 * @brief MembershipImport 表示成员导入
 * 
 * 参考: org.omg.sysml.metamodel.MembershipImport
 */
class MembershipImport : public Import {
    Q_OBJECT
    
public:
    enum Visibility {
        VisibilityNone = 0,
        VisibilityPublic = 1,
        VisibilityPrivate = 2,
        VisibilityProtected = 3
    };
    Q_ENUM(Visibility)
    
    Q_PROPERTY(Visibility visibility READ visibility WRITE setVisibility NOTIFY visibilityChanged)
    Q_PROPERTY(bool isRecursive READ isRecursive WRITE setIsRecursive NOTIFY isRecursiveChanged)
    
    explicit MembershipImport(QObject* parent = nullptr);
    
    QString elementType() const override { return "MembershipImport"; }
    
    // 导入的成员关系
    Membership* importedMembership() const;
    void setImportedMembership(Membership* membership);
    
    // 可见性
    Visibility visibility() const { return m_visibility; }
    void setVisibility(Visibility visibility);
    
    // 递归通配符 ::**
    bool isRecursive() const { return m_isRecursive; }
    void setIsRecursive(bool isRecursive);
    
signals:
    void importedMembershipChanged(Membership* membership);
    void visibilityChanged(Visibility visibility);
    void isRecursiveChanged(bool isRecursive);
    
private:
    Membership* m_importedMembership = nullptr;
    Visibility m_visibility = VisibilityNone;
    bool m_isRecursive = false;
};

SYSML_NAMESPACE_END

#endif // SYSML_MEMBERSHIPIMPORT_H



