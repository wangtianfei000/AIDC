#ifndef SYSML_MEMBERSHIP_H
#define SYSML_MEMBERSHIP_H

#include "Relationship.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class Element;

/**
 * @brief Membership 表示成员关系
 * 
 * 参考: org.omg.sysml.metamodel.Membership
 */
class Membership : public Relationship {
    Q_OBJECT
    
public:
    explicit Membership(QObject* parent = nullptr);
    
    QString elementType() const override { return "Membership"; }
    
    // 成员元素
    Element* memberElement() const;
    void setMemberElement(Element* element);
    
signals:
    void memberElementChanged(Element* element);
    
private:
    Element* m_memberElement = nullptr;
};

SYSML_NAMESPACE_END

#endif // SYSML_MEMBERSHIP_H



