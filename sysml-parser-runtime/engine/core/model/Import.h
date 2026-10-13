#ifndef SYSML_IMPORT_H
#define SYSML_IMPORT_H

#include "Relationship.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class Namespace;

/**
 * @brief Import 是导入关系的基类
 * 
 * 参考: org.omg.sysml.metamodel.Import
 */
class Import : public Relationship {
    Q_OBJECT
    
public:
    explicit Import(QObject* parent = nullptr);
    
    QString elementType() const override { return "Import"; }
    
    // 导入的命名空间
    Namespace* importedNamespace() const;
    void setImportedNamespace(Namespace* namespace_);
    
signals:
    void importedNamespaceChanged(Namespace* namespace_);
    
protected:
    Namespace* m_importedNamespace = nullptr;
};

SYSML_NAMESPACE_END

#endif // SYSML_IMPORT_H



