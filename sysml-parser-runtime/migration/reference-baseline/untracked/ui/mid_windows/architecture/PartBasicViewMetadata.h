#ifndef PARTBASICVIEWMETADATA_H
#define PARTBASICVIEWMETADATA_H

#include "BaseGraphicMetadata.h"
#include <QVariantMap>
#include <QMetaType>

SYSML_NAMESPACE_BEGIN

/**
 * @brief PartBasicViewMetadata Part定义的基础视图显示配置
 * 
 * 仅用于Part def，配置基础视图显示哪些内容
 * 不包含图形信息（位置、大小、样式等）
 */
class PartBasicViewMetadata : public BaseGraphicMetadata {
    Q_OBJECT

public:
    explicit PartBasicViewMetadata(QObject* parent = nullptr);
    
    // 基础视图显示配置
    bool showAttributes() const { return m_showAttributes; }
    void setShowAttributes(bool s) { m_showAttributes = s; }
    bool showItems() const { return m_showItems; }
    void setShowItems(bool s) { m_showItems = s; }
    bool showPorts() const { return m_showPorts; }
    void setShowPorts(bool s) { m_showPorts = s; }
    bool showDoc() const { return m_showDoc; }
    void setShowDoc(bool s) { m_showDoc = s; }
    
    // 实现基类接口
    void loadFromMap(const QVariantMap& map) override;
    QVariantMap toMap() const override;
    QString metadataType() const override { return "PartBasicView"; }

private:
    bool m_showAttributes = true;
    bool m_showItems = true;
    bool m_showPorts = true;
    bool m_showDoc = true;
};

SYSML_NAMESPACE_END

Q_DECLARE_METATYPE(SysML::PartBasicViewMetadata*)

#endif // PARTBASICVIEWMETADATA_H
