#ifndef SYSML_FRAMECONCERNMEMBER_H
#define SYSML_FRAMECONCERNMEMBER_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief FrameConcernMember 表示框架关注点成员
 * 
 * 用于 viewpoint 定义中的 frame concern 语句
 */
class FrameConcernMember : public Element {
    Q_OBJECT
    
public:
    explicit FrameConcernMember(QObject* parent = nullptr);
    
    QString elementType() const override { return "FrameConcernMember"; }
    
    Element* concern() const { return m_concern; }
    void setConcern(Element* concern) { m_concern = concern; }
    
    // 标识符（如果有，如 vs:VehicleSafety 中的 vs）
    QString identifier() const { return m_identifier; }
    void setIdentifier(const QString& identifier) { m_identifier = identifier; }
    
    // 临时存储 qualifiedName（用于前向引用解析）
    QString concernQualifiedName() const { return m_concernQualifiedName; }
    void setConcernQualifiedName(const QString& name) { m_concernQualifiedName = name; }
    
private:
    Element* m_concern = nullptr;
    QString m_identifier;  // 存储标识符（如果有）
    QString m_concernQualifiedName;  // 临时存储 qualifiedName（用于前向引用解析）
};

SYSML_NAMESPACE_END

#endif // SYSML_FRAMECONCERNMEMBER_H

