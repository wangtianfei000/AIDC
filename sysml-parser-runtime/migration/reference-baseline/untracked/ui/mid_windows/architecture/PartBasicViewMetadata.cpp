#include "PartBasicViewMetadata.h"

SYSML_NAMESPACE_BEGIN

PartBasicViewMetadata::PartBasicViewMetadata(QObject* parent)
    : BaseGraphicMetadata(parent)
{
}

void PartBasicViewMetadata::loadFromMap(const QVariantMap& map) {
    if (map.contains("showAttributes")) {
        setShowAttributes(map.value("showAttributes").toBool());
    }
    if (map.contains("showItems")) {
        setShowItems(map.value("showItems").toBool());
    }
    if (map.contains("showPorts")) {
        setShowPorts(map.value("showPorts").toBool());
    }
    if (map.contains("showDoc")) {
        setShowDoc(map.value("showDoc").toBool());
    }
}

QVariantMap PartBasicViewMetadata::toMap() const {
    QVariantMap map;
    map["showAttributes"] = m_showAttributes;
    map["showItems"] = m_showItems;
    map["showPorts"] = m_showPorts;
    map["showDoc"] = m_showDoc;
    return map;
}

SYSML_NAMESPACE_END
