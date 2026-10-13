#ifndef BASICVIEWWIDGET_H
#define BASICVIEWWIDGET_H

#include <QGraphicsView>
#include "../../../core/model/Element.h"
#include "../../../core/common/Global.h"
#include "BasicScene.h"

SYSML_NAMESPACE_BEGIN

// 前向声明
class Type;

/**
 * @brief BasicViewWidget 基础视图Widget
 * 
 * 继承自QGraphicsView，显示BasicScene
 * 用于显示part的基础信息（外形、端口、属性、item、doc）
 */
class BasicViewWidget : public QGraphicsView {
    Q_OBJECT

public:
    explicit BasicViewWidget(QWidget* parent = nullptr);
    ~BasicViewWidget() override;
    
    void setPart(Element* part);
    Element* part() const { return m_part; }
    
    void refresh();
    
    BasicScene* scene() const { return m_scene; }
    
    // 编辑方法
    Element* createAttribute(const QString& name, const QString& type = QString(), const QString& value = QString());
    bool deleteAttribute(Element* attribute);
    
    Element* createPort(const QString& name, const QString& position = "left", const QString& type = QString());
    bool deletePort(Element* port);
    
    Element* createItem(const QString& name, const QString& type = QString());
    bool deleteItem(Element* item);
    
    // 获取可用的端口类型列表（接口类型）
    QStringList getAvailablePortTypes() const;

protected:
    void wheelEvent(QWheelEvent* event) override;  // 缩放
    void resizeEvent(QResizeEvent* event) override;  // 调整大小时居中显示

private:
    void setupScene();
    void renderPart(Element* part);
    void centerOnPart();
    
    // 辅助方法：根据名称查找类型
    Type* findTypeByName(const QString& typeName) const;
    
    Element* m_part;
    BasicScene* m_scene;
};

SYSML_NAMESPACE_END

#endif // BASICVIEWWIDGET_H
