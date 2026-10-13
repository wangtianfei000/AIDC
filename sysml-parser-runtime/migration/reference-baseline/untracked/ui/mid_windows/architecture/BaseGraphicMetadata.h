#ifndef BASEGRAPHICMETADATA_H
#define BASEGRAPHICMETADATA_H

#include <QObject>
#include <QVariantMap>
#include <QMetaType>
#include "../../../core/common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief BaseGraphicMetadata 图形化元数据基类
 * 
 * 提供通用的序列化/反序列化接口和工具方法
 */
class BaseGraphicMetadata : public QObject {
    Q_OBJECT

public:
    explicit BaseGraphicMetadata(QObject* parent = nullptr);
    virtual ~BaseGraphicMetadata() = default;
    
    // 从QVariantMap加载数据（子类实现）
    virtual void loadFromMap(const QVariantMap& map) = 0;
    // 导出为QVariantMap（子类实现）
    virtual QVariantMap toMap() const = 0;
    
    // 获取元数据类型标识（用于序列化时确定类型）
    virtual QString metadataType() const = 0;
};

SYSML_NAMESPACE_END

Q_DECLARE_METATYPE(SysML::BaseGraphicMetadata*)

#endif // BASEGRAPHICMETADATA_H
