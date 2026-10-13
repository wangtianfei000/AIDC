#ifndef SYSML_NAMESPACEIMPORT_H
#define SYSML_NAMESPACEIMPORT_H

#include "Import.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief NamespaceImport 表示命名空间导入
 * 
 * 参考: org.omg.sysml.metamodel.NamespaceImport
 */
class NamespaceImport : public Import {
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
    Q_PROPERTY(bool isAll READ isAll WRITE setIsAll NOTIFY isAllChanged)
    Q_PROPERTY(bool isWildcard READ isWildcard WRITE setIsWildcard NOTIFY isWildcardChanged)
    Q_PROPERTY(bool isRecursive READ isRecursive WRITE setIsRecursive NOTIFY isRecursiveChanged)
    
    explicit NamespaceImport(QObject* parent = nullptr);
    
    QString elementType() const override { return "NamespaceImport"; }
    
    // 导入的命名空间名称（字符串形式，用于序列化）
    // 当 importedNamespace() 为 nullptr 时，使用此名称
    QString importedNamespaceName() const { return m_importedNamespaceName; }
    void setImportedNamespaceName(const QString& name) { m_importedNamespaceName = name; }
    
    // 可见性
    Visibility visibility() const { return m_visibility; }
    void setVisibility(Visibility visibility);
    
    // import all
    bool isAll() const { return m_isAll; }
    void setIsAll(bool isAll);
    
    // 通配符 ::*
    bool isWildcard() const { return m_isWildcard; }
    void setIsWildcard(bool isWildcard);
    
    // 递归通配符 ::**
    bool isRecursive() const { return m_isRecursive; }
    void setIsRecursive(bool isRecursive);
    
signals:
    void visibilityChanged(Visibility visibility);
    void isAllChanged(bool isAll);
    void isWildcardChanged(bool isWildcard);
    void isRecursiveChanged(bool isRecursive);
    
private:
    QString m_importedNamespaceName;
    
    // 导入前缀
    Visibility m_visibility = VisibilityNone;
    bool m_isAll = false;
    bool m_isWildcard = false;
    bool m_isRecursive = false;
};

SYSML_NAMESPACE_END

#endif // SYSML_NAMESPACEIMPORT_H



