#ifndef SYSML_RESOURCE_H
#define SYSML_RESOURCE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>
#include "../model/Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief Resource 表示一个模型资源（文件）
 * 
 * 功能:
 * - 管理资源中的元素
 * - 文件路径管理
 * - 修改状态跟踪
 */
class Resource : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
    
public:
    explicit Resource(QObject* parent = nullptr);
    explicit Resource(const QString& filePath, QObject* parent = nullptr);
    ~Resource();
    
    // 文件操作
    QString filePath() const { return m_filePath; }
    void setFilePath(const QString& filePath);
    
    // 内容管理
    QList<Element*> contents() const { return m_contents; }
    void addContent(Element* element);
    void removeContent(Element* element);
    void clear();
    
    // 修改状态
    bool isModified() const { return m_modified; }
    void setModified(bool modified);
    void markModified();
    void markUnmodified();
    
    // URI
    QString uri() const;
    void setUri(const QString& uri);
    
signals:
    void filePathChanged(const QString& filePath);
    void contentAdded(Element* element);
    void contentRemoved(Element* element);
    void modifiedChanged(bool modified);
    
private slots:
    void onElementChanged();
    
private:
    QString m_filePath;
    QString m_uri;
    QList<Element*> m_contents;
    bool m_modified = false;
    
    void connectElement(Element* element);
    void disconnectElement(Element* element);
};

SYSML_NAMESPACE_END

#endif // SYSML_RESOURCE_H



