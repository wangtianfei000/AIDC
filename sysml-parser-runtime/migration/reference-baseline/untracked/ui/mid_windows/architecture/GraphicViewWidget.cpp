#include "GraphicViewWidget.h"
#include "PartGraphicsItem.h"
#include "PortGraphicsItem.h"
#include "ConnectionGraphicsItem.h"
#include "ArchitectureScene.h"
#include "PortBasicItem.h"  // 用于基础part的端口显示
#include "../../../parser/src/annotation/AnnotationParser.h"
#include "PartGraphicMetadata.h"
#include "PortGraphicMetadata.h"
#include "ConnectionGraphicMetadata.h"
#include "../../../core/model/PartUsage.h"
#include "../../../core/model/PartDefinition.h"
#include "../../../core/model/ConnectStatement.h"
#include "../../../core/model/Annotation.h"
#include "../../../core/infrastructure/Resource.h"
#include "../../../core/model/FeatureTyping.h"
#include "../../../core/model/Relationship.h"
#include "../../../core/model/Feature.h"
#include "../../../core/model/Type.h"
#include "../../../core/model/Namespace.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QScrollBar>
#include <QtMath>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QRegularExpression>
#include <QSet>
#include <QDebug>

SYSML_NAMESPACE_BEGIN

GraphicViewWidget::GraphicViewWidget(QWidget* parent)
    : QGraphicsView(parent)
    , m_part(nullptr)
    , m_scene(nullptr)
{
    // 设置视图属性
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    // 为避免拖动图元时产生残影，强制全视口重绘
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // 启用右键菜单
    setContextMenuPolicy(Qt::CustomContextMenu);
    
    // 启用拖放
    setAcceptDrops(true);
    
    // 设置场景
    setupScene();
}

void GraphicViewWidget::mousePressEvent(QMouseEvent* event) {
    qDebug() << "[GraphicViewWidget] mousePressEvent: CALLED"
             << "pos:" << event->pos()
             << "button:" << event->button()
             << "modifiers:" << event->modifiers();
    
    // 检查是否点击了端口项
    if (m_scene) {
        QPointF scenePos = mapToScene(event->pos());
        QList<QGraphicsItem*> itemsAtPos = m_scene->items(scenePos);
        
        qDebug() << "[GraphicViewWidget] mousePressEvent: Found" << itemsAtPos.size() << "items at position";
        
        // 检查是否有端口项
        bool hasPortItem = false;
        for (QGraphicsItem* item : itemsAtPos) {
            PortGraphicsItem* portGraphics = qgraphicsitem_cast<PortGraphicsItem*>(item);
            PortBasicItem* portBasic = qgraphicsitem_cast<PortBasicItem*>(item);
            if (portGraphics || portBasic) {
                hasPortItem = true;
                qDebug() << "[GraphicViewWidget] mousePressEvent: ✓ Found port item at position"
                         << "PortGraphicsItem:" << (portGraphics ? "yes" : "no")
                         << "PortBasicItem:" << (portBasic ? "yes" : "no");
                break;
            }
        }
        
        // 如果点击了端口项，临时禁用 RubberBandDrag，让端口项处理事件
        if (hasPortItem && event->button() == Qt::LeftButton) {
            qDebug() << "[GraphicViewWidget] mousePressEvent: Temporarily disabling RubberBandDrag for port item";
            setDragMode(QGraphicsView::NoDrag);
            // 调用基类处理，让事件传递给端口项
            QGraphicsView::mousePressEvent(event);
            // 恢复 RubberBandDrag
            setDragMode(QGraphicsView::RubberBandDrag);
            return;
        }
    }
    
    // 如果没有点击端口项，正常处理（使用 RubberBandDrag）
    QGraphicsView::mousePressEvent(event);
}

GraphicViewWidget::~GraphicViewWidget() {
    // Scene会自动清理，但可以显式删除
    if (m_scene) {
        delete m_scene;
        m_scene = nullptr;
    }
}

void GraphicViewWidget::setupScene() {
    // 创建场景
    m_scene = new ArchitectureScene(this);
    setScene(m_scene);
    
    // 初始场景大小，后续会根据内容动态调整
    m_scene->setSceneRect(0, 0, 3000, 3000);
    m_scene->setBackgroundBrush(QBrush(Qt::white));
}

void GraphicViewWidget::setPart(Element* part) {
    if (m_part == part) {
        return;
    }
    
    qDebug() << "[GraphicViewWidget] setPart:"
             << (part ? part->elementType() : "null")
             << (part ? part->declaredName() : "null");

    m_part = part;
    
    // 清除场景中的现有项
    m_scene->clearAllItems();
    m_scene->clear();
    
    // 渲染part
    if (m_part) {
        renderPart(m_part, nullptr);
        centerOnPart();
    }
}

PartGraphicsItem* GraphicViewWidget::renderPart(Element* part, PartGraphicsItem* parentItem, double scaleX, double scaleY) {
    if (!part) {
        return nullptr;
    }
    
    qDebug() << "[GraphicViewWidget] renderPart begin:"
             << "elementType =" << part->elementType()
             << "name =" << part->declaredName()
             << "parent =" << (parentItem ? parentItem->part()->declaredName() : "null");

    // 获取GraphicMetadata（用于图形视图）
    // 注意：PartDefinition 返回 PartBasicViewMetadata，PartUsage 返回 PartGraphicMetadata
    // 在图形视图中，我们主要处理 PartUsage（子part），但根part可能是 PartDefinition
    
    // 如果提供了缩放比例（嵌套 PartUsage 实例），说明这是嵌套 part 实例的子 part
    // 对于嵌套 part 实例的子 part，应该从 PartDefinition 类型中加载位置，而不是从实例中加载
    // 这样可以避免多次打开时位置被重复缩放
    bool isNestedChildPart = (scaleX != 1.0 || scaleY != 1.0);
    Element* partDefForMetadata = part;
    if (isNestedChildPart && part->elementType() == "PartUsage") {
        Element* def = getPartDefinition(part);
        if (def) {
            partDefForMetadata = def;  // 从 PartDefinition 中加载元数据
            qDebug() << "[GraphicViewWidget] renderPart: Loading metadata from PartDefinition for nested child part"
                     << "instance:" << part->declaredName()
                     << "definition:" << def->declaredName();
        }
    }
    
    BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(partDefForMetadata);
    PartGraphicMetadata* graphicMeta = qobject_cast<PartGraphicMetadata*>(metadata);
    
    // 如果从 PartDefinition 加载元数据但没有 graphicMeta，检查 Annotation 中是否包含图形信息
    if (!graphicMeta && isNestedChildPart && partDefForMetadata != part) {
        // 对于嵌套 part 实例的子 part，从 PartDefinition 的 Annotation 中加载图形信息
        Annotation* annotation = AnnotationParser::findAnnotation(partDefForMetadata, "GraphicMetadata");
        if (annotation) {
            QVariantMap metadataMap = AnnotationParser::extractMetadata(annotation);
            if (metadataMap.contains("graphicWidth") || metadataMap.contains("graphicHeight") ||
                metadataMap.contains("graphicX") || metadataMap.contains("graphicY")) {
                graphicMeta = new PartGraphicMetadata();
                graphicMeta->loadFromMap(metadataMap);
                qDebug() << "[GraphicViewWidget] renderPart: Loaded PartGraphicMetadata from PartDefinition Annotation for nested child part"
                         << "instance:" << part->declaredName()
                         << "definition:" << partDefForMetadata->declaredName()
                         << "width:" << graphicMeta->graphicWidth()
                         << "height:" << graphicMeta->graphicHeight()
                         << "x:" << graphicMeta->graphicX()
                         << "y:" << graphicMeta->graphicY();
            }
        }
        
        // 如果仍然没有 graphicMeta，尝试从实例中加载（作为后备方案）
        if (!graphicMeta) {
            BaseGraphicMetadata* instanceMetadata = AnnotationParser::parseGraphicMetadata(part);
            PartGraphicMetadata* instanceGraphicMeta = qobject_cast<PartGraphicMetadata*>(instanceMetadata);
            if (instanceGraphicMeta) {
                graphicMeta = instanceGraphicMeta;
                qDebug() << "[GraphicViewWidget] renderPart: Fallback to instance metadata for nested child part"
                         << "instance:" << part->declaredName();
            }
        }
    }
    
    // 如果是 PartDefinition 且没有 graphicMeta，检查 Annotation 中是否包含图形信息
    if (!graphicMeta && part->elementType() == "PartDefinition") {
        // 检查 Annotation 中是否包含图形信息（如 graphicWidth, graphicHeight）
        Annotation* annotation = AnnotationParser::findAnnotation(part, "GraphicMetadata");
        if (annotation) {
            QVariantMap metadataMap = AnnotationParser::extractMetadata(annotation);
            // 如果 Annotation 中包含图形信息，创建 PartGraphicMetadata 并加载
            if (metadataMap.contains("graphicWidth") || metadataMap.contains("graphicHeight") ||
                metadataMap.contains("graphicX") || metadataMap.contains("graphicY")) {
                graphicMeta = new PartGraphicMetadata();
                graphicMeta->loadFromMap(metadataMap);
                qDebug() << "[GraphicViewWidget] renderPart: Loaded PartGraphicMetadata from Annotation for PartDefinition"
                         << "width:" << graphicMeta->graphicWidth()
                         << "height:" << graphicMeta->graphicHeight();
            }
        }
        
        // 如果仍然没有 graphicMeta（Annotation 不存在或不包含图形信息），创建默认值
        if (!graphicMeta) {
            graphicMeta = new PartGraphicMetadata();
            // 设置一些默认值（仅用于显示，不保存到模型）
            graphicMeta->setGraphicWidth(400);
            graphicMeta->setGraphicHeight(300);
            graphicMeta->setShape("Rectangle");
            graphicMeta->setFillColor("#F0F0F0");
            graphicMeta->setBorderColor("#000000");
            qDebug() << "[GraphicViewWidget] renderPart: Created default metadata for PartDefinition (not saved to model)";
        }
    }
    
    // 检查父项是否是 PartDefinition
    bool parentIsPartDefinition = false;
    if (parentItem && parentItem->part()) {
        parentIsPartDefinition = (parentItem->part()->elementType() == "PartDefinition");
    }
    
    // 对于 PartDefinition 的子项，不设置父项（直接添加到场景）
    // 对于 PartUsage 的子项，设置父项（作为子项）
    QGraphicsItem* actualParent = (parentItem && !parentIsPartDefinition) ? parentItem : nullptr;
    
    // 如果提供了缩放比例（嵌套 PartUsage 实例），创建缩放后的元数据副本
    PartGraphicMetadata* scaledGraphicMeta = graphicMeta;
    if (graphicMeta && (scaleX != 1.0 || scaleY != 1.0)) {
        // 使用 toMap() 和 loadFromMap() 来复制元数据（因为 QObject 不支持复制构造函数）
        QVariantMap metadataMap = graphicMeta->toMap();
        scaledGraphicMeta = new PartGraphicMetadata();
        scaledGraphicMeta->loadFromMap(metadataMap);
        
        // 缩放大小
        if (scaledGraphicMeta->graphicWidth() > 0) {
            scaledGraphicMeta->setGraphicWidth(scaledGraphicMeta->graphicWidth() * scaleX);
        }
        if (scaledGraphicMeta->graphicHeight() > 0) {
            scaledGraphicMeta->setGraphicHeight(scaledGraphicMeta->graphicHeight() * scaleY);
        }
        // 缩放位置（包括负数和0，因为位置可能是相对于边界的）
        scaledGraphicMeta->setGraphicX(scaledGraphicMeta->graphicX() * scaleX);
        scaledGraphicMeta->setGraphicY(scaledGraphicMeta->graphicY() * scaleY);
        qDebug() << "[GraphicViewWidget] renderPart: Created scaled metadata"
                 << "originalSize:" << graphicMeta->graphicWidth() << "x" << graphicMeta->graphicHeight()
                 << "originalPos:" << graphicMeta->graphicX() << "," << graphicMeta->graphicY()
                 << "scaledSize:" << scaledGraphicMeta->graphicWidth() << "x" << scaledGraphicMeta->graphicHeight()
                 << "scaledPos:" << scaledGraphicMeta->graphicX() << "," << scaledGraphicMeta->graphicY()
                 << "scaleX:" << scaleX << "scaleY:" << scaleY;
    }
    
    // 创建PartGraphicsItem
    PartGraphicsItem* partItem = new PartGraphicsItem(part, scaledGraphicMeta, actualParent);
    
    // 设置位置
    QPointF position;
    bool isPartDefinition = (part->elementType() == "PartDefinition");
    
    if (isPartDefinition) {
        // PartDefinition 作为画布，在场景中居中显示（用于视觉展示）
        // 但元数据中不保存位置，因为位置是固定的（相对于画布本身是 (0,0)）
        // 如果没有父项（根 part），根据尺寸和场景大小计算居中位置
        if (!parentItem) {
            // 根 PartDefinition：根据尺寸和场景大小计算居中位置
            double partWidth = graphicMeta ? graphicMeta->graphicWidth() : 400.0;
            double partHeight = graphicMeta ? graphicMeta->graphicHeight() : 300.0;
            
            // 获取场景大小
            QRectF sceneRect = m_scene ? m_scene->sceneRect() : QRectF(0, 0, 3000, 3000);
            if (sceneRect.isEmpty()) {
                sceneRect = QRectF(0, 0, 3000, 3000);
            }
            
            // 计算居中位置：场景中心减去 PartDefinition 中心
            double centerX = (sceneRect.width() - partWidth) / 2.0;
            double centerY = (sceneRect.height() - partHeight) / 2.0;
            
            position = QPointF(centerX, centerY);
            
            // 根据 PartDefinition 的尺寸和位置，调整场景大小以确保所有内容可见
            // 场景大小应该至少包含 PartDefinition 及其周围的空间
            double minSceneWidth = qMax(sceneRect.width(), partWidth + 200);
            double minSceneHeight = qMax(sceneRect.height(), partHeight + 200);
            if (minSceneWidth > sceneRect.width() || minSceneHeight > sceneRect.height()) {
                m_scene->setSceneRect(0, 0, minSceneWidth, minSceneHeight);
                qDebug() << "[GraphicViewWidget] renderPart: Adjusted scene size to"
                         << minSceneWidth << "x" << minSceneHeight;
            }
            
            qDebug() << "[GraphicViewWidget] renderPart: PartDefinition position calculation:"
                     << "partWidth:" << partWidth
                     << "partHeight:" << partHeight
                     << "sceneRect:" << (m_scene ? m_scene->sceneRect() : QRectF())
                     << "calculated position:" << position;
        } else {
            // 作为子项的 PartDefinition（不应该发生，但为了安全）：使用 (0,0)
            position = QPointF(0, 0);
            qDebug() << "[GraphicViewWidget] renderPart: PartDefinition as child (unexpected), using (0,0)";
        }
    } else if (scaledGraphicMeta && scaledGraphicMeta->graphicX() > 0 && scaledGraphicMeta->graphicY() > 0) {
        // 对于 PartDefinition 的子 Part，元数据中保存的是相对于 PartDefinition 的本地坐标
        // 如果提供了缩放比例，位置已经在 scaledGraphicMeta 中缩放过了
        double gx = scaledGraphicMeta->graphicX();
        double gy = scaledGraphicMeta->graphicY();
        position = QPointF(gx, gy);
    } else {
        // 默认位置（如果没有元数据）
        if (parentItem) {
            // 子part：相对于父part的位置
            position = QPointF(100, 100);
        } else {
            // 根part：居中
            position = QPointF(400, 300);
        }
    }
    
    // 添加到场景或父项
    // 注意：对于 PartDefinition，其内部的 PartUsage 和 Port 应该直接添加到场景，而不是作为子项
    if (actualParent) {
        // 父项是 PartUsage，正常添加为子项
        partItem->setPos(position);
        parentItem->addChildPart(partItem);
    } else {
        // 没有父项，或者父项是 PartDefinition，直接添加到场景
        if (parentIsPartDefinition && parentItem) {
            // 如果父项是 PartDefinition，需要将子项的本地坐标转换为场景坐标
            QPointF partDefScenePos = parentItem->scenePos();
            QPointF childScenePos = partDefScenePos + position;
            partItem->setPos(childScenePos);
            qDebug() << "[GraphicViewWidget] renderPart: PartDefinition child added to scene directly"
                     << "localPos:" << position
                     << "partDefScenePos:" << partDefScenePos
                     << "childScenePos:" << childScenePos
                     << "parentItem:" << (partItem->parentItem() ? "ERROR: has parent" : "null (correct)");
        } else {
            partItem->setPos(position);
            if (isPartDefinition) {
                qDebug() << "[GraphicViewWidget] renderPart: PartDefinition added to scene"
                         << "position:" << position
                         << "scenePos:" << partItem->scenePos()
                         << "boundingRect:" << partItem->boundingRect();
            }
        }
        m_scene->addItem(partItem);
    }
    
    // 注册到场景
    m_scene->registerPartItem(part, partItem);
    
    // 判断part类型：基础part还是嵌套part
    QList<Element*> childParts = getChildParts(part);   // 当前元素自身的子 part
    QList<Element*> defChildParts;                      // 对于 PartUsage：其 PartDefinition 中的子 part
    bool hasOwnChildParts = !childParts.isEmpty();
    bool hasDefChildParts = false;

    if (part->elementType() == "PartUsage") {
        Element* def = getPartDefinition(part);
        if (def) {
            defChildParts = getChildParts(def);
            hasDefChildParts = !defChildParts.isEmpty();
        }
    }

    bool isNestedByDefinition = hasDefChildParts;
    bool isBasicPart = !hasOwnChildParts && !isNestedByDefinition;  // 没有任何子part的才是基础part

    qDebug() << "[GraphicViewWidget] renderPart:"
             << part->declaredName()
             << "hasOwnChildParts =" << hasOwnChildParts
             << "hasDefChildParts =" << hasDefChildParts
             << "isBasicPart =" << isBasicPart;
    
    if (isBasicPart) {
        // 基础part：仅显示自身端口（使用PortBasicItem）
        renderBasicPartPorts(part, partItem);
    } else {
        // 嵌套part
        if (part->elementType() == "PartUsage" && isNestedByDefinition) {
            // 对于 PartUsage：
            // 1. 显示自身端口（使用PortBasicItem，带标签）
            renderBasicPartPorts(part, partItem);

            // 2. 根据元数据决定是否显示定义中的子part
            bool showChildNestedContent = graphicMeta ? graphicMeta->showChildNestedContent() : false;
            if (!parentItem || showChildNestedContent) {
                // 计算缩放比例：实例大小 / 定义大小
                double scaleX = 1.0;
                double scaleY = 1.0;
                
                Element* def = getPartDefinition(part);
                if (def && graphicMeta) {
                    // 获取实例的大小
                    double instanceWidth = graphicMeta->graphicWidth();
                    double instanceHeight = graphicMeta->graphicHeight();
                    
                    // 获取定义的大小
                    BaseGraphicMetadata* defMetadata = AnnotationParser::parseGraphicMetadata(def);
                    PartGraphicMetadata* defGraphicMeta = qobject_cast<PartGraphicMetadata*>(defMetadata);
                    
                    // 如果定义没有图形元数据，检查 Annotation
                    if (!defGraphicMeta) {
                        Annotation* annotation = AnnotationParser::findAnnotation(def, "GraphicMetadata");
                        if (annotation) {
                            QVariantMap metadataMap = AnnotationParser::extractMetadata(annotation);
                            if (metadataMap.contains("graphicWidth") || metadataMap.contains("graphicHeight")) {
                                defGraphicMeta = new PartGraphicMetadata();
                                defGraphicMeta->loadFromMap(metadataMap);
                            }
                        }
                    }
                    
                    // 如果仍然没有，使用默认值
                    if (!defGraphicMeta) {
                        defGraphicMeta = new PartGraphicMetadata();
                        defGraphicMeta->setGraphicWidth(400);
                        defGraphicMeta->setGraphicHeight(300);
                    }
                    
                    double defWidth = defGraphicMeta->graphicWidth();
                    double defHeight = defGraphicMeta->graphicHeight();
                    
                    // 计算缩放比例
                    if (defWidth > 0) {
                        scaleX = instanceWidth / defWidth;
                    }
                    if (defHeight > 0) {
                        scaleY = instanceHeight / defHeight;
                    }
                    
                    qDebug() << "[GraphicViewWidget] renderPart: Calculating scale for nested PartUsage"
                             << "instance:" << part->declaredName()
                             << "instanceSize:" << instanceWidth << "x" << instanceHeight
                             << "defSize:" << defWidth << "x" << defHeight
                             << "scaleX:" << scaleX << "scaleY:" << scaleY;
                    
                    // 清理临时创建的元数据
                    if (defGraphicMeta != defMetadata) {
                        delete defGraphicMeta;
                    }
                    if (defMetadata && defMetadata != defGraphicMeta) {
                        delete defMetadata;
                    }
                }
                
                // 递归渲染子 part（作为 partItem 的子项）
                // 注意：对于嵌套 part 实例的子 part，renderPart 会从 PartDefinition 类型中加载位置（而不是从实例中加载）
                // 这样可以避免多次打开时位置被重复缩放
                for (Element* childPart : defChildParts) {
                    // 对于 PartUsage 视图中显示的 PartDefinition 内部子 part，
                    // 这些子 part 仅用于展示内部结构，不允许在实例视图中拖动
                    // 需要根据缩放比例调整子 part 的位置和大小
                    PartGraphicsItem* childItem = renderPart(childPart, partItem, scaleX, scaleY);
                    if (childItem) {
                        childItem->setFlag(QGraphicsItem::ItemIsMovable, false);
                    }
                }

                // 3. 渲染 PartDefinition 中的连接（嵌套 part 内部的连接）
                // 连接线作为 partItem 的子项，与直接打开该 PartDefinition 的视图一致
                if (def) {
                    qDebug() << "[GraphicViewWidget] renderPart: rendering connections for nested PartUsage:"
                             << part->declaredName()
                             << "from PartDefinition:" << def->declaredName()
                             << "as child of partItem";
                    // 使用 PartDefinition 作为连接拥有者，使用当前 PartUsage 作为可视根节点查找端口
                    // 连接线作为 partItem 的子项
                    renderConnections(def, part, partItem);
                }
            }
        } else {
            // 普通嵌套 part（例如 PartDefinition 自身有子 part，或 PartUsage 自身有子 part）
            // 1. 显示自身端口（使用PortGraphicsItem，显示在边界上）
            renderPorts(part, partItem);
            
            // 2. 递归显示子part（只显示 PartUsage，不显示嵌套的 PartDefinition）
            bool showChildNestedContent = graphicMeta ? graphicMeta->showChildNestedContent() : false;
            if (!parentItem || showChildNestedContent) {
                // 对于 PartUsage 实例，如果它有自己的子 part，需要计算缩放比例
                double scaleX = 1.0;
                double scaleY = 1.0;
                
                // 如果是 PartUsage 实例且有图形元数据，计算缩放比例
                if (part->elementType() == "PartUsage" && graphicMeta) {
                    Element* def = getPartDefinition(part);
                    if (def) {
                        // 获取实例的大小
                        double instanceWidth = graphicMeta->graphicWidth();
                        double instanceHeight = graphicMeta->graphicHeight();
                        
                        // 获取定义的大小
                        BaseGraphicMetadata* defMetadata = AnnotationParser::parseGraphicMetadata(def);
                        PartGraphicMetadata* defGraphicMeta = qobject_cast<PartGraphicMetadata*>(defMetadata);
                        
                        // 如果定义没有图形元数据，检查 Annotation
                        if (!defGraphicMeta) {
                            Annotation* annotation = AnnotationParser::findAnnotation(def, "GraphicMetadata");
                            if (annotation) {
                                QVariantMap metadataMap = AnnotationParser::extractMetadata(annotation);
                                if (metadataMap.contains("graphicWidth") || metadataMap.contains("graphicHeight")) {
                                    defGraphicMeta = new PartGraphicMetadata();
                                    defGraphicMeta->loadFromMap(metadataMap);
                                }
                            }
                        }
                        
                        // 如果仍然没有，使用默认值
                        if (!defGraphicMeta) {
                            defGraphicMeta = new PartGraphicMetadata();
                            defGraphicMeta->setGraphicWidth(400);
                            defGraphicMeta->setGraphicHeight(300);
                        }
                        
                        double defWidth = defGraphicMeta->graphicWidth();
                        double defHeight = defGraphicMeta->graphicHeight();
                        
                        // 计算缩放比例
                        if (defWidth > 0) {
                            scaleX = instanceWidth / defWidth;
                        }
                        if (defHeight > 0) {
                            scaleY = instanceHeight / defHeight;
                        }
                        
                        qDebug() << "[GraphicViewWidget] renderPart: Calculating scale for PartUsage with own child parts"
                                 << "instance:" << part->declaredName()
                                 << "instanceSize:" << instanceWidth << "x" << instanceHeight
                                 << "defSize:" << defWidth << "x" << defHeight
                                 << "scaleX:" << scaleX << "scaleY:" << scaleY;
                        
                        // 清理临时创建的元数据
                        if (defGraphicMeta != defMetadata) {
                            delete defGraphicMeta;
                        }
                        if (defMetadata && defMetadata != defGraphicMeta) {
                            delete defMetadata;
                        }
                    }
                }
                
                for (Element* childPart : childParts) {
                    // 策略C：只显示 PartUsage 子 part，不显示 PartDefinition 子 part
                    // 例如：part def a { part def b; } - 双击 a 时不显示内部的 b
                    // 但 part def a { part usage b: SomeType; } - 显示 b
                    if (childPart->elementType() == "PartUsage") {
                        // 对于 PartUsage 实例的子 part，应用缩放比例
                        renderPart(childPart, partItem, scaleX, scaleY);
                    } else if (childPart->elementType() == "PartDefinition") {
                        // 跳过嵌套的 PartDefinition，不显示
                        qDebug() << "[GraphicViewWidget] renderPart: skipping nested PartDefinition:"
                                 << childPart->declaredName()
                                 << "in parent:" << part->declaredName();
                    }
                }
            }
        }
    }
    
    // 渲染连接（只在根part时渲染，避免重复）
    // 根 part：连接拥有者和可视根节点相同
    if (!parentItem) {
        renderConnections(part, part);
    }
    
    // 如果创建了默认metadata，需要清理（但PartGraphicsItem会管理它）
    // 注意：这里不删除，因为PartGraphicsItem会管理metadata的生命周期
    // 但是，如果创建了缩放后的元数据副本，需要清理
    if (scaledGraphicMeta != graphicMeta && scaledGraphicMeta) {
        // scaledGraphicMeta 会被 PartGraphicsItem 管理，不需要手动删除
        // 但需要确保 graphicMeta（原始元数据）被正确管理
        if (graphicMeta && graphicMeta != metadata) {
            // 如果 graphicMeta 是新创建的（不是从 parseGraphicMetadata 返回的），需要删除
            // 但这里我们不确定，所以让 PartGraphicsItem 管理 scaledGraphicMeta
        }
    }
    
    return partItem;
}

void GraphicViewWidget::renderPorts(Element* part, PartGraphicsItem* partItem) {
    if (!part || !partItem) {
        return;
    }

    qDebug() << "[GraphicViewWidget] renderPorts for part (nested):"
             << part->declaredName();

    QList<Element*> ports = getPorts(part);
    qDebug() << "[GraphicViewWidget] renderPorts: found" << ports.size() << "ports for part"
             << part->declaredName();
    
    for (Element* port : ports) {
        qDebug() << "[GraphicViewWidget] renderPorts: processing port:"
                 << port->declaredName()
                 << "elementType=" << port->elementType();
        
        // 获取端口的GraphicMetadata
        BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(port);
        PortGraphicMetadata* portMeta = qobject_cast<PortGraphicMetadata*>(metadata);
        
        if (portMeta) {
            qDebug() << "[GraphicViewWidget] renderPorts: port metadata found:"
                     << "graphicX=" << portMeta->graphicX()
                     << "graphicY=" << portMeta->graphicY()
                     << "portSize=" << portMeta->portSize()
                     << "portPosition=" << portMeta->portPosition();
        } else {
            qDebug() << "[GraphicViewWidget] renderPorts: port metadata NOT found";
        }
        
        // 检查 partItem 是否是 PartDefinition
        bool partIsPartDefinition = false;
        if (partItem && partItem->part()) {
            partIsPartDefinition = (partItem->part()->elementType() == "PartDefinition");
        }
        
        // 对于 PartDefinition，不设置父项（直接添加到场景）
        // 对于 PartUsage，设置父项（作为子项）
        PortGraphicsItem* portItem = new PortGraphicsItem(port, portMeta, partIsPartDefinition ? nullptr : partItem);
        
        // 设置位置
        QPointF position;
        // 检查是否有显式的位置设置
        // 注意：graphicX/graphicY 可能为负数（在 Part 外部），也可能很大（在 Part 外部）
        bool hasExplicitPosition = false;
        if (portMeta) {
            double gx = portMeta->graphicX();
            double gy = portMeta->graphicY();
            QRectF partRect = partItem->boundingRect();
            
            // 如果 graphicX 和 graphicY 都不为 0，或者至少有一个非 0，则认为有显式位置
            // 允许位置在 Part 边界附近（包括外部，因为端口可能在 Part 边缘）
            if (gx != 0.0 || gy != 0.0) {
                hasExplicitPosition = true;
                
                // 对于 PartDefinition 的端口，元数据中保存的是相对于 PartDefinition 的本地坐标
                // 直接使用，不需要检测和转换
                position = QPointF(gx, gy);
                
                qDebug() << "[GraphicViewWidget] renderPorts: using metadata position:" << position
                         << "partRect:" << partRect
                         << "partWidth:" << partRect.width()
                         << "partHeight:" << partRect.height();
            }
        }
        
        if (!hasExplicitPosition) {
            // 默认位置（根据portPosition）
            QString portPosition = portMeta ? portMeta->portPosition() : "left";
            double portSize = portMeta ? portMeta->portSize() : 6.0;
            QRectF partRect = partItem->boundingRect();
            
            if (portPosition == "left") {
                position = QPointF(-portSize/2, partRect.height() / 2);
            } else if (portPosition == "right") {
                position = QPointF(partRect.width() + portSize/2, partRect.height() / 2);
            } else if (portPosition == "top") {
                position = QPointF(partRect.width() / 2, -portSize/2);
            } else if (portPosition == "bottom") {
                position = QPointF(partRect.width() / 2, partRect.height() + portSize/2);
            } else {
                // 默认左侧
                position = QPointF(-portSize/2, partRect.height() / 2);
            }
            qDebug() << "[GraphicViewWidget] renderPorts: using calculated position:" << position
                     << "portPosition=" << portPosition
                     << "partRect=" << partRect;
        }
        
        if (partIsPartDefinition) {
            // 对于 PartDefinition，端口应该直接添加到场景，而不是作为子项
            // 需要将本地坐标转换为场景坐标
            QPointF partDefScenePos = partItem->scenePos();
            QPointF portScenePos = partDefScenePos + position;
            portItem->setPos(portScenePos);
            m_scene->addItem(portItem);
            qDebug() << "[GraphicViewWidget] renderPorts: PartDefinition port added to scene directly"
                     << "localPos:" << position
                     << "partDefScenePos:" << partDefScenePos
                     << "portScenePos:" << portScenePos
                     << "parentItem:" << (portItem->parentItem() ? "ERROR: has parent" : "null (correct)");
        } else {
            // 对于 PartUsage，正常添加为子项
            portItem->setPos(position);
            partItem->addPort(portItem);
            qDebug() << "[GraphicViewWidget] renderPorts: port item position set to:" << position
                     << "scene pos:" << portItem->scenePos();
        }
        
        // 注册到场景
        m_scene->registerPortItem(port, portItem);
        
        qDebug() << "[GraphicViewWidget] renderPorts: port item registered,"
                 << "parent:" << (portItem->parentItem() ? "set" : "null")
                 << "scene:" << (portItem->scene() ? "set" : "null");
    }
}

void GraphicViewWidget::renderBasicPartPorts(Element* part, PartGraphicsItem* partItem) {
    if (!part || !partItem) {
        return;
    }
    
    // 检查 partItem 是否是 PartDefinition
    bool partIsPartDefinition = false;
    if (partItem && partItem->part()) {
        partIsPartDefinition = (partItem->part()->elementType() == "PartDefinition");
    }
    
    // 获取part的端口（处理PartUsage的情况）
    QList<Element*> ports = getPortsForPart(part);
    qDebug() << "[GraphicViewWidget] renderBasicPartPorts for part:"
             << part->declaredName()
             << "ports.count =" << ports.count()
             << "elementType =" << part->elementType()
             << "partIsPartDefinition =" << partIsPartDefinition;
    
    // 按portPosition分组
    QList<Element*> leftPorts, rightPorts, topPorts, bottomPorts;
    for (Element* port : ports) {
        BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(port);
        PortGraphicMetadata* portMeta = qobject_cast<PortGraphicMetadata*>(metadata);
        
        QString position = portMeta ? portMeta->portPosition() : "left";
        
        if (position == "right") {
            rightPorts.append(port);
        } else if (position == "top") {
            topPorts.append(port);
        } else if (position == "bottom") {
            bottomPorts.append(port);
        } else {
            leftPorts.append(port);  // 默认左侧
        }
    }
    
    // 获取part的边界矩形
    QRectF partRect = partItem->boundingRect();
    
    // 对于 PartDefinition，计算场景坐标偏移
    QPointF partDefScenePos = partIsPartDefinition ? partItem->scenePos() : QPointF(0, 0);
    
    // 动态布局参数
    const double SIDE_MARGIN_Y = 15.0;   // 上下边距
    const double SIDE_MARGIN_X = 10.0;   // 左右边距（用于顶部/底部端口）
    
    // 渲染左侧端口（均匀分布在左边缘）
    int leftCount = leftPorts.size();
    double leftSpacing = leftCount > 0
        ? (partRect.height() - 2 * SIDE_MARGIN_Y) / (leftCount + 1)
        : 0.0;
    for (int i = 0; i < leftCount; ++i) {
        Element* port = leftPorts.at(i);
        BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(port);
        PortGraphicMetadata* portMeta = qobject_cast<PortGraphicMetadata*>(metadata);
        double portSize = portMeta ? portMeta->portSize() : 8.0;
        
        // 对于 PartDefinition，不设置父项，直接添加到场景
        PortBasicItem* portItem = new PortBasicItem(port, portMeta, partIsPartDefinition ? nullptr : partItem);
        double localY = SIDE_MARGIN_Y + (i + 1) * leftSpacing;
        // 端口中心在左边缘（x=0），端口完全在矩形外部
        double localX = -portSize / 2.0;
        
        if (partIsPartDefinition) {
            // 对于 PartDefinition，计算场景坐标并直接添加到场景
            QPointF scenePos = partDefScenePos + QPointF(localX, localY);
            portItem->setPos(scenePos);
            m_scene->addItem(portItem);
            qDebug() << "[GraphicViewWidget] renderBasicPartPorts: PartDefinition left port added to scene"
                     << "localPos:" << QPointF(localX, localY)
                     << "partDefScenePos:" << partDefScenePos
                     << "scenePos:" << scenePos;
        } else {
            // 对于 PartUsage，正常设置本地坐标
            portItem->setPos(localX, localY);
            qDebug() << "[GraphicViewWidget] renderBasicPartPorts left port:"
                     << port->declaredName()
                     << "pos =" << portItem->pos()
                     << "portSize =" << portSize;
        }
        portItem->updateLabelPosition();
    }
    
    // 渲染右侧端口（均匀分布在右边缘）
    int rightCount = rightPorts.size();
    double rightSpacing = rightCount > 0
        ? (partRect.height() - 2 * SIDE_MARGIN_Y) / (rightCount + 1)
        : 0.0;
    for (int i = 0; i < rightCount; ++i) {
        Element* port = rightPorts.at(i);
        BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(port);
        PortGraphicMetadata* portMeta = qobject_cast<PortGraphicMetadata*>(metadata);
        double portSize = portMeta ? portMeta->portSize() : 8.0;
        
        // 对于 PartDefinition，不设置父项，直接添加到场景
        PortBasicItem* portItem = new PortBasicItem(port, portMeta, partIsPartDefinition ? nullptr : partItem);
        double localY = SIDE_MARGIN_Y + (i + 1) * rightSpacing;
        // 端口中心在右边缘（x=width），端口完全在矩形外部
        double localX = partRect.width() + portSize / 2.0;
        
        if (partIsPartDefinition) {
            // 对于 PartDefinition，计算场景坐标并直接添加到场景
            QPointF scenePos = partDefScenePos + QPointF(localX, localY);
            portItem->setPos(scenePos);
            m_scene->addItem(portItem);
            qDebug() << "[GraphicViewWidget] renderBasicPartPorts: PartDefinition right port added to scene"
                     << "localPos:" << QPointF(localX, localY)
                     << "partDefScenePos:" << partDefScenePos
                     << "scenePos:" << scenePos;
        } else {
            // 对于 PartUsage，正常设置本地坐标
            portItem->setPos(localX, localY);
            qDebug() << "[GraphicViewWidget] renderBasicPartPorts right port:"
                     << port->declaredName()
                     << "pos =" << portItem->pos()
                     << "portSize =" << portSize;
        }
        portItem->updateLabelPosition();
    }
    
    // 渲染顶部端口（均匀分布在上边缘）
    int topCount = topPorts.size();
    double topSpacing = topCount > 0
        ? (partRect.width() - 2 * SIDE_MARGIN_X) / (topCount + 1)
        : 0.0;
    for (int i = 0; i < topCount; ++i) {
        Element* port = topPorts.at(i);
        BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(port);
        PortGraphicMetadata* portMeta = qobject_cast<PortGraphicMetadata*>(metadata);
        double portSize = portMeta ? portMeta->portSize() : 8.0;
        
        // 对于 PartDefinition，不设置父项，直接添加到场景
        PortBasicItem* portItem = new PortBasicItem(port, portMeta, partIsPartDefinition ? nullptr : partItem);
        double localX = SIDE_MARGIN_X + (i + 1) * topSpacing;
        // 端口中心在上边缘（y=0），端口完全在矩形外部
        double localY = -portSize / 2.0;
        
        if (partIsPartDefinition) {
            // 对于 PartDefinition，计算场景坐标并直接添加到场景
            QPointF scenePos = partDefScenePos + QPointF(localX, localY);
            portItem->setPos(scenePos);
            m_scene->addItem(portItem);
            qDebug() << "[GraphicViewWidget] renderBasicPartPorts: PartDefinition top port added to scene"
                     << "localPos:" << QPointF(localX, localY)
                     << "partDefScenePos:" << partDefScenePos
                     << "scenePos:" << scenePos;
        } else {
            // 对于 PartUsage，正常设置本地坐标
            portItem->setPos(localX, localY);
            qDebug() << "[GraphicViewWidget] renderBasicPartPorts top port:"
                     << port->declaredName()
                     << "pos =" << portItem->pos()
                     << "portSize =" << portSize;
        }
        portItem->updateLabelPosition();
    }
    
    // 渲染底部端口（均匀分布在下边缘）
    int bottomCount = bottomPorts.size();
    double bottomSpacing = bottomCount > 0
        ? (partRect.width() - 2 * SIDE_MARGIN_X) / (bottomCount + 1)
        : 0.0;
    for (int i = 0; i < bottomCount; ++i) {
        Element* port = bottomPorts.at(i);
        BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(port);
        PortGraphicMetadata* portMeta = qobject_cast<PortGraphicMetadata*>(metadata);
        double portSize = portMeta ? portMeta->portSize() : 8.0;
        
        // 对于 PartDefinition，不设置父项，直接添加到场景
        PortBasicItem* portItem = new PortBasicItem(port, portMeta, partIsPartDefinition ? nullptr : partItem);
        double localX = SIDE_MARGIN_X + (i + 1) * bottomSpacing;
        // 端口中心在下边缘（y=height），端口完全在矩形外部
        double localY = partRect.height() + portSize / 2.0;
        
        if (partIsPartDefinition) {
            // 对于 PartDefinition，计算场景坐标并直接添加到场景
            QPointF scenePos = partDefScenePos + QPointF(localX, localY);
            portItem->setPos(scenePos);
            m_scene->addItem(portItem);
            qDebug() << "[GraphicViewWidget] renderBasicPartPorts: PartDefinition bottom port added to scene"
                     << "localPos:" << QPointF(localX, localY)
                     << "partDefScenePos:" << partDefScenePos
                     << "scenePos:" << scenePos;
        } else {
            // 对于 PartUsage，正常设置本地坐标
            portItem->setPos(localX, localY);
            qDebug() << "[GraphicViewWidget] renderBasicPartPorts bottom port:"
                     << port->declaredName()
                     << "pos =" << portItem->pos()
                     << "portSize =" << portSize;
        }
        portItem->updateLabelPosition();
    }
}

void GraphicViewWidget::renderConnections(Element* connectionOwner, Element* visualRoot, QGraphicsItem* parentItem) {
    if (!connectionOwner || !visualRoot) {
        return;
    }
    
    QList<Element*> connections = getConnections(connectionOwner);
    qDebug() << "[GraphicViewWidget] renderConnections for part:"
             << connectionOwner->declaredName()
             << "connections.count =" << connections.count()
             << "parentItem =" << (parentItem ? "set" : "null (scene)");
    for (Element* connection : connections) {
        // 获取连接的GraphicMetadata
        BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(connection);
        ConnectionGraphicMetadata* connMeta = qobject_cast<ConnectionGraphicMetadata*>(metadata);
        
        // 创建ConnectionGraphicsItem，如果 parentItem 不为空，则作为其子项
        ConnectionGraphicsItem* connItem = new ConnectionGraphicsItem(connection, connMeta, parentItem);
        
        // 解析连接语句，获取源和目标端口
        QString sourceEndStr = getConnectionSourceEnd(connection);
        QString targetEndStr = getConnectionTargetEnd(connection);
        qDebug() << "[GraphicViewWidget] renderConnections connection:"
                 << connection->declaredName()
                 << "sourceEnd =" << sourceEndStr
                 << "targetEnd =" << targetEndStr;
        QGraphicsItem* sourcePortItem = findPortByEndString(sourceEndStr, connectionOwner, visualRoot);
        QGraphicsItem* targetPortItem = findPortByEndString(targetEndStr, connectionOwner, visualRoot);
        
        if (sourcePortItem && targetPortItem) {
            // 根据端口类型设置连接
            PortGraphicsItem* sourcePortGraphics = qgraphicsitem_cast<PortGraphicsItem*>(sourcePortItem);
            PortBasicItem* sourcePortBasic = qgraphicsitem_cast<PortBasicItem*>(sourcePortItem);
            PortGraphicsItem* targetPortGraphics = qgraphicsitem_cast<PortGraphicsItem*>(targetPortItem);
            PortBasicItem* targetPortBasic = qgraphicsitem_cast<PortBasicItem*>(targetPortItem);
            
            if (sourcePortGraphics && targetPortGraphics) {
                connItem->setSourcePort(sourcePortGraphics);
                connItem->setTargetPort(targetPortGraphics);
            } else if (sourcePortBasic && targetPortBasic) {
                connItem->setSourcePortBasic(sourcePortBasic);
                connItem->setTargetPortBasic(targetPortBasic);
            } else if (sourcePortGraphics && targetPortBasic) {
                connItem->setSourcePort(sourcePortGraphics);
                connItem->setTargetPortBasic(targetPortBasic);
            } else if (sourcePortBasic && targetPortGraphics) {
                connItem->setSourcePortBasic(sourcePortBasic);
                connItem->setTargetPort(targetPortGraphics);
            }

            qDebug() << "[GraphicViewWidget] renderConnections: found sourcePortItem="
                     << sourcePortItem << "targetPortItem=" << targetPortItem;
            
            // 如果 parentItem 为空，则添加到 scene；否则已经通过构造函数设置了 parent
            if (!parentItem) {
                m_scene->addItem(connItem);
            }
            m_scene->registerConnectionItem(connection, connItem);
        } else {
            // 如果找不到端口，删除连接项
            delete connItem;
            qWarning() << "[GraphicViewWidget] Cannot find ports for connection:"
                      << (connection ? connection->declaredName() : "null");
        }
    }
}

QList<Element*> GraphicViewWidget::getChildParts(Element* part) const {
    if (!part) {
        return QList<Element*>();
    }
    return part->getChildParts();
}

QList<Element*> GraphicViewWidget::getPorts(Element* part) const {
    if (!part) {
        return QList<Element*>();
    }
    return part->getPorts();
}

QList<Element*> GraphicViewWidget::getPortsForPart(Element* part) const {
    if (!part) {
        return QList<Element*>();
    }
    
    // 如果是PartUsage，需要从PartDefinition获取端口
    if (part->elementType() == "PartUsage") {
        qDebug() << "[GraphicViewWidget] getPortsForPart PartUsage:"
                 << part->declaredName();
        Element* partDef = getPartDefinition(part);
        if (partDef) {
            qDebug() << "[GraphicViewWidget] getPortsForPart PartUsage resolved PartDefinition:"
                     << partDef->declaredName();
            return getPorts(partDef);  // 从PartDefinition获取端口
        } else {
            qDebug() << "[GraphicViewWidget] getPortsForPart PartUsage: PartDefinition NOT found for"
                     << part->declaredName();
        }
    }
    
    // 否则直接获取端口
    return getPorts(part);
}

Element* GraphicViewWidget::getPartDefinition(Element* partUsage) const {
    if (!partUsage || partUsage->elementType() != "PartUsage") {
        return nullptr;
    }

    PartUsage* usage = qobject_cast<PartUsage*>(partUsage);
    if (!usage) {
        return nullptr;
    }

    QString typeName = usage->typeName();
    qDebug() << "[GraphicViewWidget] getPartDefinition for PartUsage:"
             << usage->declaredName()
             << "typeName =" << typeName;
    if (typeName.isEmpty()) {
        qWarning() << "[GraphicViewWidget] getPartDefinition: PartUsage has empty typeName:"
                   << usage->declaredName();
        return nullptr;
    }

    // 如果 typeName 是 qualifiedName（包含 ::），直接通过 qualifiedName 查找
    if (typeName.contains("::")) {
        // 从 m_part 或 usage 向上查找到真正的根元素（没有 owner 的元素）
        Element* root = m_part;
        if (!root) {
            root = usage;
        }
        // 向上查找到根元素（没有 owner 的元素）
        while (root && root->owner()) {
            root = root->owner();
        }
        if (root) {
            qDebug() << "[GraphicViewWidget] getPartDefinition: Searching for qualifiedName"
                     << typeName << "starting from root:" << root->declaredName()
                     << "root qualifiedName:" << root->qualifiedName();
            // 使用 findPartDefinitionByQualifiedNameHelper 查找
            Element* found = findPartDefinitionByQualifiedNameHelper(root, typeName);
            if (found && found->elementType() == "PartDefinition") {
                qDebug() << "[GraphicViewWidget] getPartDefinition: resolved PartDefinition by qualifiedName:"
                         << found->declaredName() << "qualifiedName:" << found->qualifiedName()
                         << "for PartUsage =" << usage->declaredName();
                return found;
            } else if (found) {
                qDebug() << "[GraphicViewWidget] getPartDefinition: Found element but not PartDefinition:"
                         << found->declaredName() << "type:" << found->elementType()
                         << "qualifiedName:" << found->qualifiedName();
            } else {
                qDebug() << "[GraphicViewWidget] getPartDefinition: Not found qualifiedName"
                         << typeName << "starting from root:" << root->declaredName()
                         << "root qualifiedName:" << root->qualifiedName();
            }
        } else {
            qDebug() << "[GraphicViewWidget] getPartDefinition: Cannot find root element for qualifiedName lookup";
        }
    }

    // 从当前 owningNamespace 开始，逐级向上查找 typeName 对应的元素
    Namespace* ns = usage->owningNamespace();
    Element* nsElement = dynamic_cast<Element*>(ns);  // 代表当前命名空间自身对应的 Element（如 Package / PartDefinition）
    while (ns && nsElement) {
        // 查找所有同名元素，但只返回 PartDefinition 类型的
        QList<Element*> candidates = ns->findElements(typeName);
        for (Element* typeElement : candidates) {
            if (typeElement->elementType() == "PartDefinition") {
                qDebug() << "[GraphicViewWidget] getPartDefinition: resolved PartDefinition by typeName in namespace:"
                         << (nsElement ? nsElement->declaredName() : "<root>")
                         << "type =" << typeElement->declaredName()
                         << "for PartUsage =" << usage->declaredName();
                return typeElement;
            }
        }
        
        // 如果没找到，尝试直接查找（可能只有一个同名元素）
        Element* typeElement = ns->findElement(typeName);
        if (typeElement && typeElement->elementType() == "PartDefinition") {
            qDebug() << "[GraphicViewWidget] getPartDefinition: resolved PartDefinition by typeName in namespace:"
                     << (nsElement ? nsElement->declaredName() : "<root>")
                     << "type =" << typeElement->declaredName()
                     << "for PartUsage =" << usage->declaredName();
            return typeElement;
        }

        // 向上一个命名空间：即“当前命名空间元素的 owner 是否本身也是 Namespace”
        Element* parent = nsElement->owner();
        if (!parent) {
            break;
        }
        ns = dynamic_cast<Namespace*>(parent);     // 父元素本身通常就是上层命名空间（如 package）
        nsElement = parent;                        // 同步维护 nsElement 指针
    }

    qWarning() << "[GraphicViewWidget] getPartDefinition: cannot resolve typeName in any owningNamespace chain:"
               << "PartUsage =" << usage->declaredName()
               << "typeName =" << typeName;
    return nullptr;
}

QList<Element*> GraphicViewWidget::getConnections(Element* part) const {
    if (!part) {
        return QList<Element*>();
    }
    return part->getConnections();
}

QString GraphicViewWidget::getConnectionSourceEnd(Element* connection) const {
    if (!connection || connection->elementType() != "ConnectStatement") {
        return QString();
    }
    
    ConnectStatement* connectStmt = qobject_cast<ConnectStatement*>(connection);
    if (!connectStmt || connectStmt->ends().isEmpty()) {
        return QString();
    }
    
    // 返回第一个端点（源端点）
    QString end = connectStmt->ends().first();
    qDebug() << "[GraphicViewWidget] getConnectionSourceEnd for"
             << connection->declaredName() << "->" << end;
    return end;
}

QString GraphicViewWidget::getConnectionTargetEnd(Element* connection) const {
    if (!connection || connection->elementType() != "ConnectStatement") {
        return QString();
    }
    
    ConnectStatement* connectStmt = qobject_cast<ConnectStatement*>(connection);
    if (!connectStmt || connectStmt->ends().size() < 2) {
        return QString();
    }
    
    // 返回第二个端点（目标端点）
    QString end = connectStmt->ends().at(1);
    qDebug() << "[GraphicViewWidget] getConnectionTargetEnd for"
             << connection->declaredName() << "->" << end;
    return end;
}

QGraphicsItem* GraphicViewWidget::findPortByEndString(const QString& endString,
                                                      Element* connectionOwner,
                                                      Element* visualRoot) {
    if (endString.isEmpty() || !connectionOwner || !visualRoot) {
        qDebug() << "[GraphicViewWidget] findPortByEndString: invalid arguments"
                 << "endString =" << endString
                 << "connectionOwner =" << (connectionOwner ? connectionOwner->declaredName() : "null")
                 << "visualRoot =" << (visualRoot ? visualRoot->declaredName() : "null");
        return nullptr;
    }
    
    // 解析端点字符串
    // 格式可能是：
    // 1. "partName.portName" - 子part的端口
    // 2. "portName" - 当前part的端口
    
    QString partName;
    QString portName;
    
    if (endString.contains('.')) {
        // 包含点号，说明是子part的端口
        QStringList parts = endString.split('.');
        if (parts.size() >= 2) {
            partName = parts.first();
            portName = parts.last();
        } else {
            qWarning() << "[GraphicViewWidget] findPortByEndString: invalid endString (dot but parts.size<2):"
                       << endString;
            return nullptr;
        }
    } else {
        // 不包含点号，说明是当前 connectionOwner 的端口
        portName = endString;
    }
    
    // 1) 语义层面解析：从 connectionOwner 出发找到目标 part 和 port 元素
    Element* targetPartSemantic = connectionOwner;
    if (!partName.isEmpty()) {
        targetPartSemantic = findPartByName(connectionOwner, partName);
        if (!targetPartSemantic) {
            qWarning() << "[GraphicViewWidget] Cannot find part:" << partName;
            return nullptr;
        }
    }
    
    Element* port = findPortByName(targetPartSemantic, portName);
    if (!port) {
        qWarning() << "[GraphicViewWidget] Cannot find port:" << portName
                   << "in part:" << (targetPartSemantic ? targetPartSemantic->declaredName() : "null");
        return nullptr;
    }
    qDebug() << "[GraphicViewWidget] findPortByEndString parsed:"
             << "endString =" << endString
             << "partName =" << partName
             << "portName =" << portName
             << "resolvedPart =" << (targetPartSemantic ? targetPartSemantic->declaredName() : "null")
             << "resolvedPort =" << port->declaredName();
    
    // 2) 图形层面解析：在场景中查找端口图元
    // 2.1 先直接通过端口元素找 PortGraphicsItem（适用于 PortGraphicsItem）
    PortGraphicsItem* portGraphicsItem = m_scene->findPortItem(port);
    if (portGraphicsItem) {
        return portGraphicsItem;
    }
    
    // 2.2 找 PortBasicItem
    // 由于基础 part 端口有两种挂载方式，需要考虑两种宿主 part 图元：
    //   a) 语义上的 part（targetPartSemantic）
    //   b) 对应的可视根 part（visualRoot），例如 PartDefinition 的端口被画在 PartUsage 上
    // 注意：对于 PartDefinition，PortBasicItem 直接添加到场景，不是子项
    auto findPortBasicOnPart = [this, port](Element* partElem) -> PortBasicItem* {
        if (!partElem) {
            return nullptr;
        }
        PartGraphicsItem* partItem = m_scene->findPartItem(partElem);
        if (!partItem) {
            return nullptr;
        }
        
        // 检查是否是 PartDefinition（其端口直接添加到场景）
        bool isPartDefinition = (partElem->elementType() == "PartDefinition");
        
        if (isPartDefinition) {
            // 对于 PartDefinition，PortBasicItem 直接添加到场景，需要遍历场景中的项
            QList<QGraphicsItem*> sceneItems = m_scene->items();
            for (QGraphicsItem* item : sceneItems) {
                PortBasicItem* portBasicItem = qgraphicsitem_cast<PortBasicItem*>(item);
                if (portBasicItem && portBasicItem->port() == port) {
                    qDebug() << "[GraphicViewWidget] findPortByEndString: found PortBasicItem for port"
                             << port->declaredName()
                             << "on PartDefinition" << partElem->declaredName()
                             << "(directly in scene)";
                    return portBasicItem;
                }
            }
        } else {
            // 对于 PartUsage，PortBasicItem 是子项
            for (QGraphicsItem* child : partItem->childItems()) {
                PortBasicItem* portBasicItem = qgraphicsitem_cast<PortBasicItem*>(child);
                if (portBasicItem && portBasicItem->port() == port) {
                    qDebug() << "[GraphicViewWidget] findPortByEndString: found PortBasicItem for port"
                             << port->declaredName()
                             << "on part" << partElem->declaredName();
                    return portBasicItem;
                }
            }
        }
        return nullptr;
    };

    // 2.2.1 优先在语义 part 上查找（如 wheelSpeedSensorA/B）
    PortBasicItem* basicOnSemantic = findPortBasicOnPart(targetPartSemantic);
    if (basicOnSemantic) {
        return basicOnSemantic;
    }

    // 2.2.2 对于 \"PhysIn/PhysOut\" 这类定义在 PartDefinition 上、但实际画在 PartUsage 上的端口，
    // 如果 connectionOwner 是 PartDefinition，visualRoot 是引用它的 PartUsage，则在 visualRoot 上再找一次
    Element* defFromVisualRoot = getPartDefinition(visualRoot);
    if (connectionOwner->elementType() == "PartDefinition"
        && visualRoot->elementType() == "PartUsage"
        && defFromVisualRoot == connectionOwner) {
        PortBasicItem* basicOnVisual = findPortBasicOnPart(visualRoot);
        if (basicOnVisual) {
            return basicOnVisual;
        }
    }

    qDebug() << "[GraphicViewWidget] findPortByEndString: no PortBasicItem found for port"
             << port->declaredName();
    return nullptr;
}

Element* GraphicViewWidget::findPartByName(Element* parentPart, const QString& partName) const {
    if (!parentPart || partName.isEmpty()) {
        return nullptr;
    }
    
    // 在当前part的子part中查找
    QList<Element*> childParts = getChildParts(parentPart);
    for (Element* childPart : childParts) {
        if (childPart->declaredName() == partName) {
            qDebug() << "[GraphicViewWidget] findPartByName:"
                     << "parent =" << parentPart->declaredName()
                     << "child found =" << childPart->declaredName();
            return childPart;
        }
    }
    qWarning() << "[GraphicViewWidget] findPartByName: NOT found"
               << "parent =" << parentPart->declaredName()
               << "partName =" << partName;
    
    return nullptr;
}

Element* GraphicViewWidget::findPortByName(Element* part, const QString& portName) const {
    if (!part || portName.isEmpty()) {
        return nullptr;
    }
    
    // 获取part的端口（处理PartUsage的情况）
    QList<Element*> ports = getPortsForPart(part);
    for (Element* port : ports) {
        if (port->declaredName() == portName) {
            qDebug() << "[GraphicViewWidget] findPortByName: found port"
                     << portName << "in part" << part->declaredName();
            return port;
        }
    }
    qWarning() << "[GraphicViewWidget] findPortByName: NOT found port"
               << portName << "in part" << part->declaredName();
    
    return nullptr;
}

void GraphicViewWidget::refresh() {
    if (m_part) {
        setPart(m_part);  // 重新设置会触发重新渲染
    }
}

void GraphicViewWidget::centerOnPart() {
    if (!m_scene || m_scene->items().isEmpty()) {
        qDebug() << "[GraphicViewWidget] centerOnPart: Scene is null or empty";
        return;
    }
    
    // 找到根 PartDefinition 的 PartGraphicsItem 并居中显示
    QList<QGraphicsItem*> items = m_scene->items();
    qDebug() << "[GraphicViewWidget] centerOnPart: Total items in scene:" << items.size();
    
    PartGraphicsItem* rootPartDefItem = nullptr;
    for (QGraphicsItem* item : items) {
        PartGraphicsItem* partItem = qgraphicsitem_cast<PartGraphicsItem*>(item);
        if (partItem && !partItem->parentItem()) {  // 根项（没有父项）
            // 检查是否是 PartDefinition
            if (partItem->part() && partItem->part()->elementType() == "PartDefinition") {
                rootPartDefItem = partItem;
                break;
            }
        }
    }
    
    if (rootPartDefItem) {
        qDebug() << "[GraphicViewWidget] centerOnPart: Found root PartDefinition"
                 << "part:" << (rootPartDefItem->part() ? rootPartDefItem->part()->declaredName() : "null")
                 << "pos:" << rootPartDefItem->pos()
                 << "scenePos:" << rootPartDefItem->scenePos()
                 << "boundingRect:" << rootPartDefItem->boundingRect()
                 << "viewportSize:" << (viewport() ? viewport()->size() : QSize())
                 << "sceneRect:" << m_scene->sceneRect();
        centerOn(rootPartDefItem);
        qDebug() << "[GraphicViewWidget] centerOnPart: After centerOn, viewport center:" 
                 << (viewport() ? mapToScene(viewport()->rect().center()) : QPointF());
    } else {
        qDebug() << "[GraphicViewWidget] centerOnPart: No root PartDefinition found";
    }
}

void GraphicViewWidget::wheelEvent(QWheelEvent* event) {
    // 缩放功能
    if (event->modifiers() & Qt::ControlModifier) {
        // Ctrl + 滚轮：缩放
        double scaleFactor = qPow(1.2, event->angleDelta().y() / 240.0);
        scale(scaleFactor, scaleFactor);
        event->accept();
    } else {
        // 普通滚轮：滚动
        QGraphicsView::wheelEvent(event);
    }
}

void GraphicViewWidget::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    
    // 调整大小时居中显示part
    centerOnPart();
}

void GraphicViewWidget::keyPressEvent(QKeyEvent* event) {
    // 处理Delete键删除选中的项
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
        for (QGraphicsItem* item : selectedItems) {
            // 尝试转换为PartGraphicsItem
            PartGraphicsItem* partItem = qgraphicsitem_cast<PartGraphicsItem*>(item);
            if (partItem && partItem->part()) {
                deletePart(partItem->part());
                continue;
            }
            
            // 尝试转换为PortGraphicsItem
            PortGraphicsItem* portItem = qgraphicsitem_cast<PortGraphicsItem*>(item);
            if (portItem && portItem->port()) {
                deletePort(portItem->port());
                continue;
            }
            
            // 尝试转换为ConnectionGraphicsItem
            ConnectionGraphicsItem* connItem = qgraphicsitem_cast<ConnectionGraphicsItem*>(item);
            if (connItem && connItem->connection()) {
                deleteConnection(connItem->connection());
                continue;
            }
        }
        event->accept();
        return;
    }
    
    QGraphicsView::keyPressEvent(event);
}

bool GraphicViewWidget::deletePart(Element* partUsage) {
    if (!m_part || !partUsage) {
        return false;
    }
    
    // 检查是否属于当前 part
    if (partUsage->owner() != m_part) {
        return false;
    }
    
    // 检查是否是 PartUsage
    if (partUsage->elementType() != "PartUsage") {
        return false;
    }
    
    // 从场景中移除图形项
    PartGraphicsItem* partItem = m_scene->findPartItem(partUsage);
    if (partItem) {
        m_scene->unregisterPartItem(partUsage);
        delete partItem;  // Qt会自动清理子项
    }
    
    // 从 part 中移除
    m_part->removeOwnedElement(partUsage);
    
    // 删除元素
    partUsage->deleteLater();
    
    // 触发变更通知
    emit m_part->elementChanged();
    
    return true;
}

bool GraphicViewWidget::deletePort(Element* portElement) {
    if (!m_part || !portElement) {
        return false;
    }
    
    // 检查是否是端口类型
    if (portElement->elementType() != "PortUsage" && portElement->elementType() != "PortDefinition") {
        return false;
    }
    
    // 从场景中移除图形项
    PortGraphicsItem* portItem = m_scene->findPortItem(portElement);
    if (portItem) {
        m_scene->unregisterPortItem(portElement);
        delete portItem;
    }
    
    // 查找拥有该端口的 part
    Element* ownerPart = portElement->owner();
    if (ownerPart) {
        ownerPart->removeOwnedElement(portElement);
        portElement->deleteLater();
        emit ownerPart->elementChanged();
        return true;
    }
    
    return false;
}

bool GraphicViewWidget::deleteConnection(Element* connection) {
    if (!m_part || !connection) {
        return false;
    }
    
    // 检查是否属于当前 part
    if (connection->owner() != m_part) {
        return false;
    }
    
    // 检查是否是连接类型
    if (connection->elementType() != "ConnectStatement") {
        return false;
    }
    
    // 从场景中移除图形项
    ConnectionGraphicsItem* connItem = m_scene->findConnectionItem(connection);
    if (connItem) {
        m_scene->unregisterConnectionItem(connection);
        delete connItem;
    }
    
    // 从 part 中移除
    m_part->removeOwnedElement(connection);
    
    // 删除元素
    connection->deleteLater();
    
    // 触发变更通知
    emit m_part->elementChanged();
    
    return true;
}

Element* GraphicViewWidget::createConnection(Element* sourcePort, Element* targetPort, QGraphicsItem* sourcePortItem, QGraphicsItem* targetPortItem)
{
    qDebug() << "[GraphicViewWidget] createConnection: called"
             << "m_part:" << (m_part ? m_part->declaredName() : "null")
             << "sourcePort:" << (sourcePort ? (sourcePort->declaredName() + " (" + sourcePort->elementType() + ")") : "null")
             << "targetPort:" << (targetPort ? (targetPort->declaredName() + " (" + targetPort->elementType() + ")") : "null");
    
    if (!m_part || !sourcePort || !targetPort) {
        qDebug() << "[GraphicViewWidget] createConnection: ✗ Invalid arguments"
                 << "m_part:" << (m_part ? "valid" : "null")
                 << "sourcePort:" << (sourcePort ? "valid" : "null")
                 << "targetPort:" << (targetPort ? "valid" : "null");
        return nullptr;
    }
    
    // 生成端点字符串
    qDebug() << "[GraphicViewWidget] createConnection: Generating end strings";
    // 从图形项找到端口所属的 PartUsage 实例
    Element* sourcePartUsage = findPartUsageForPortFromItem(sourcePort, sourcePortItem);
    Element* targetPartUsage = findPartUsageForPortFromItem(targetPort, targetPortItem);
    qDebug() << "[GraphicViewWidget] createConnection: Found PartUsage instances"
             << "sourcePartUsage:" << (sourcePartUsage ? sourcePartUsage->declaredName() : "null")
             << "targetPartUsage:" << (targetPartUsage ? targetPartUsage->declaredName() : "null");
    
    // 检查两个端口是否相同（端口元素相同且属于同一个 PartUsage 实例）
    if (sourcePort == targetPort && sourcePartUsage == targetPartUsage) {
        qDebug() << "[GraphicViewWidget] createConnection: ✗ Source and target ports are the same (same port element and same PartUsage instance)";
        return nullptr;
    }
    QString sourceEnd = getPortEndString(sourcePort, sourcePartUsage, m_part);
    QString targetEnd = getPortEndString(targetPort, targetPartUsage, m_part);
    
    qDebug() << "[GraphicViewWidget] createConnection: Generated end strings"
             << "sourceEnd:" << sourceEnd
             << "targetEnd:" << targetEnd;
    
    if (sourceEnd.isEmpty() || targetEnd.isEmpty()) {
        qDebug() << "[GraphicViewWidget] createConnection: ✗ Failed to generate end strings"
                 << "sourceEnd:" << (sourceEnd.isEmpty() ? "empty" : sourceEnd)
                 << "targetEnd:" << (targetEnd.isEmpty() ? "empty" : targetEnd);
        return nullptr;
    }
    
    // 检查连接是否已存在
    qDebug() << "[GraphicViewWidget] createConnection: Checking for existing connections";
    QList<Element*> existingConnections = getConnections(m_part);
    qDebug() << "[GraphicViewWidget] createConnection: Found" << existingConnections.size() << "existing connections";
    
    for (Element* conn : existingConnections) {
        if (conn->elementType() == "ConnectStatement") {
            ConnectStatement* connectStmt = qobject_cast<ConnectStatement*>(conn);
            if (connectStmt && connectStmt->ends().size() >= 2) {
                QString existingSource = connectStmt->ends().first();
                QString existingTarget = connectStmt->ends().at(1);
                
                qDebug() << "[GraphicViewWidget] createConnection: Checking existing connection"
                         << "existingSource:" << existingSource
                         << "existingTarget:" << existingTarget
                         << "newSource:" << sourceEnd
                         << "newTarget:" << targetEnd;
                
                if ((existingSource == sourceEnd && existingTarget == targetEnd) ||
                    (existingSource == targetEnd && existingTarget == sourceEnd)) {
                    qDebug() << "[GraphicViewWidget] createConnection: ✗ Connection already exists";
                    return nullptr;
                }
            }
        }
    }
    
    qDebug() << "[GraphicViewWidget] createConnection: ✓ No duplicate connection found, creating new connection"
             << "sourcePort:" << sourcePort->declaredName()
             << "targetPort:" << targetPort->declaredName()
             << "sourceEnd:" << sourceEnd
             << "targetEnd:" << targetEnd;
    
    // 创建 ConnectStatement
    ConnectStatement* connect = new ConnectStatement(m_part);
    
    // 设置端点
    QStringList ends;
    ends << sourceEnd << targetEnd;
    connect->setEnds(ends);
    connect->setIsBinary(true);
    
    // 添加到 part
    m_part->addOwnedElement(connect);
    
    // 创建图形元数据注解
    Annotation* annotation = new Annotation(connect);
    annotation->setAnnotationName("GraphicMetadata");
    annotation->setMetadataAssignment("pathType", "Orthogonal");  // 设置为正交路径
    annotation->setMetadataAssignment("lineType", "Solid");
    annotation->setMetadataAssignment("lineColor", "#000000");
    annotation->setMetadataAssignment("lineWidth", 1.0);
    connect->addAnnotation(annotation);
    
    // 触发变更通知
    emit m_part->elementChanged();
    
    // 立即渲染新创建的连接
    if (m_scene) {
        renderConnections(m_part, m_part, nullptr);
    }
    
    qDebug() << "[GraphicViewWidget] createConnection: Connection created successfully"
             << "connect:" << connect->declaredName();
    
    return connect;
}

void GraphicViewWidget::startConnectionPreview(QGraphicsItem* sourcePortItem, const QPointF& startPos)
{
    if (!m_scene || !sourcePortItem) {
        return;
    }
    
    // 清除之前的预览
    endConnectionPreview();
    
    // 创建临时预览路径（使用 QGraphicsPathItem 以支持正交路径）
    QGraphicsPathItem* previewPath = new QGraphicsPathItem();
    previewPath->setPen(QPen(QColor("#888888"), 2, Qt::DashLine));
    previewPath->setZValue(50);  // 在端口之上，但在连接线之下
    
    QPointF sourcePos = sourcePortItem->scenePos();
    QPainterPath path;
    path.moveTo(sourcePos);
    path.lineTo(startPos);
    previewPath->setPath(path);
    
    m_scene->addItem(previewPath);
    m_connectionPreviewItem = previewPath;
    m_previewSourcePortItem = sourcePortItem;
}

void GraphicViewWidget::updateConnectionPreview(const QPointF& currentPos)
{
    if (!m_connectionPreviewItem || !m_previewSourcePortItem) {
        return;
    }
    
    QGraphicsPathItem* previewPath = qgraphicsitem_cast<QGraphicsPathItem*>(m_connectionPreviewItem);
    if (!previewPath) {
        return;
    }
    
    // 获取源端口位置
    QPointF sourcePos = m_previewSourcePortItem->scenePos();
    
    // 创建正交路径（先水平再垂直）
    QPainterPath path;
    path.moveTo(sourcePos);
    QPointF mid1(sourcePos.x(), currentPos.y());
    path.lineTo(mid1);
    path.lineTo(currentPos);
    
    previewPath->setPath(path);
}

void GraphicViewWidget::endConnectionPreview()
{
    if (m_connectionPreviewItem && m_scene) {
        m_scene->removeItem(m_connectionPreviewItem);
        delete m_connectionPreviewItem;
        m_connectionPreviewItem = nullptr;
    }
    m_previewSourcePortItem = nullptr;
}

Element* GraphicViewWidget::findPartUsageForPortFromItem(Element* port, QGraphicsItem* portItem) const
{
    qDebug() << "[GraphicViewWidget] findPartUsageForPortFromItem: called"
             << "port:" << (port ? port->declaredName() : "null")
             << "portItem:" << (portItem ? "valid" : "null");
    
    if (!port || !portItem) {
        qDebug() << "[GraphicViewWidget] findPartUsageForPortFromItem: ✗ Invalid arguments";
        return nullptr;
    }
    
    // 从图形项的父项找到 PartGraphicsItem
    QGraphicsItem* parent = portItem->parentItem();
    PartGraphicsItem* partItem = qgraphicsitem_cast<PartGraphicsItem*>(parent);
    if (partItem && partItem->part()) {
        Element* part = partItem->part();
        qDebug() << "[GraphicViewWidget] findPartUsageForPortFromItem: ✓ Found PartUsage instance:"
                 << part->declaredName()
                 << "partType:" << part->elementType();
        return part;
    }
    
    // 如果父项不是 PartGraphicsItem，可能是 PartDefinition 的端口（直接添加到场景）
    if (!parent) {
        // 端口属于 m_part 本身
        qDebug() << "[GraphicViewWidget] findPartUsageForPortFromItem: ✓ Port belongs to m_part:"
                 << (m_part ? m_part->declaredName() : "null");
        return m_part;
    }
    
    qDebug() << "[GraphicViewWidget] findPartUsageForPortFromItem: ✗ Cannot find PartUsage from item";
    return nullptr;
}

Element* GraphicViewWidget::findPartUsageForPort(Element* port) const
{
    qDebug() << "[GraphicViewWidget] findPartUsageForPort: called"
             << "port:" << (port ? port->declaredName() : "null")
             << "m_part:" << (m_part ? m_part->declaredName() : "null");
    
    if (!port || !m_part) {
        qDebug() << "[GraphicViewWidget] findPartUsageForPort: ✗ Invalid arguments";
        return nullptr;
    }
    
    // 从 m_part 的所有子 PartUsage 实例中查找包含该端口的实例
    // 因为端口可能属于 PartDefinition，但我们需要找到使用该端口的 PartUsage 实例
    QList<Element*> childParts = getChildParts(m_part);
    qDebug() << "[GraphicViewWidget] findPartUsageForPort: Checking" << childParts.size() << "child parts";
    
    for (Element* childPart : childParts) {
        if (childPart->elementType() != "PartUsage") {
            continue;
        }
        
        // 获取该 PartUsage 的端口列表
        QList<Element*> ports = getPortsForPart(childPart);
        qDebug() << "[GraphicViewWidget] findPartUsageForPort: Checking part:"
                 << childPart->declaredName()
                 << "ports.count:" << ports.size();
        
        // 检查该端口是否属于这个 PartUsage
        if (ports.contains(port)) {
            qDebug() << "[GraphicViewWidget] findPartUsageForPort: ✓ Found PartUsage instance:"
                     << childPart->declaredName();
            return childPart;
        }
    }
    
    // 如果端口属于 m_part 本身，返回 m_part
    QList<Element*> partPorts = getPortsForPart(m_part);
    if (partPorts.contains(port)) {
        qDebug() << "[GraphicViewWidget] findPartUsageForPort: ✓ Port belongs to m_part:"
                 << m_part->declaredName();
        return m_part;
    }
    
    // 如果找不到，返回端口的 owner（可能是 PartDefinition）
    Element* owner = port->owner();
    qDebug() << "[GraphicViewWidget] findPartUsageForPort: ✗ Not found in child parts, returning port owner:"
             << (owner ? owner->declaredName() : "null");
    return owner;
}

QString GraphicViewWidget::getPortEndString(Element* port, Element* partUsage, Element* connectionOwner) const
{
    qDebug() << "[GraphicViewWidget] getPortEndString: called"
             << "port:" << (port ? port->declaredName() : "null")
             << "partUsage:" << (partUsage ? partUsage->declaredName() : "null")
             << "connectionOwner:" << (connectionOwner ? connectionOwner->declaredName() : "null");
    
    if (!port || !connectionOwner) {
        qDebug() << "[GraphicViewWidget] getPortEndString: ✗ Invalid arguments";
        return QString();
    }
    
    // 如果端口属于 connectionOwner，直接返回端口名称
    if (partUsage == connectionOwner) {
        qDebug() << "[GraphicViewWidget] getPortEndString: ✓ Port belongs to connectionOwner, returning port name:"
                 << port->declaredName();
        return port->declaredName();
    }
    
    // 如果端口属于 connectionOwner 的子 part，返回 "partName.portName"
    // 先查找直接子 part
    qDebug() << "[GraphicViewWidget] getPortEndString: Checking direct child parts";
    QList<Element*> childParts = getChildParts(connectionOwner);
    qDebug() << "[GraphicViewWidget] getPortEndString: Found" << childParts.size() << "child parts";
    
    for (Element* childPart : childParts) {
        qDebug() << "[GraphicViewWidget] getPortEndString: Checking child part:"
                 << childPart->declaredName()
                 << "matches partUsage:" << (childPart == partUsage);
        if (childPart == partUsage) {
            QString result = QString("%1.%2").arg(childPart->declaredName(), port->declaredName());
            qDebug() << "[GraphicViewWidget] getPortEndString: ✓ Found in direct children, returning:"
                     << result;
            return result;
        }
    }
    
    // 如果找不到，尝试递归查找（查找嵌套的子 part）
    qDebug() << "[GraphicViewWidget] getPortEndString: Not found in direct children, searching recursively";
    QString path = findPathToPart(connectionOwner, partUsage);
    if (!path.isEmpty()) {
        QString result = QString("%1.%2").arg(path, port->declaredName());
        qDebug() << "[GraphicViewWidget] getPortEndString: ✓ Found in nested children, returning:"
                 << result;
        return result;
    }
    
    qWarning() << "[GraphicViewWidget] getPortEndString: ✗ Cannot find partUsage in connection owner's children"
               << "port:" << port->declaredName()
               << "partUsage:" << (partUsage ? partUsage->declaredName() : "null")
               << "connectionOwner:" << connectionOwner->declaredName();
    
    // 如果找不到，返回简单的格式（可能不准确，但至少能工作）
    QString fallback = QString("%1.%2").arg(partUsage ? partUsage->declaredName() : "unknown", port->declaredName());
    qDebug() << "[GraphicViewWidget] getPortEndString: Using fallback format:"
             << fallback;
    return fallback;
}

QString GraphicViewWidget::getPortEndStringRecursive(Element* port, Element* currentPart, Element* rootPart) const
{
    // 这个方法暂时不使用，保留接口以保持兼容性
    Q_UNUSED(port)
    Q_UNUSED(currentPart)
    Q_UNUSED(rootPart)
    return QString();
}

QString GraphicViewWidget::findPathToPart(Element* rootPart, Element* targetPart) const
{
    qDebug() << "[GraphicViewWidget] findPathToPart: called"
             << "rootPart:" << (rootPart ? rootPart->declaredName() : "null")
             << "targetPart:" << (targetPart ? targetPart->declaredName() : "null");
    
    if (!rootPart || !targetPart || rootPart == targetPart) {
        qDebug() << "[GraphicViewWidget] findPathToPart: ✗ Invalid arguments or same part";
        return QString();
    }
    
    // 使用广度优先搜索找到路径
    struct PartPath {
        Element* part;
        QString path;
    };
    
    QList<PartPath> queue;
    queue.append({rootPart, QString()});
    QSet<Element*> visited;
    visited.insert(rootPart);
    
    int iteration = 0;
    while (!queue.isEmpty()) {
        iteration++;
        PartPath current = queue.takeFirst();
        
        qDebug() << "[GraphicViewWidget] findPathToPart: Iteration" << iteration
                 << "current:" << current.part->declaredName()
                 << "path:" << current.path;
        
        if (current.part == targetPart) {
            qDebug() << "[GraphicViewWidget] findPathToPart: ✓ Found target, returning path:"
                     << current.path;
            return current.path;
        }
        
        QList<Element*> childParts = getChildParts(current.part);
        qDebug() << "[GraphicViewWidget] findPathToPart: Found" << childParts.size() << "child parts";
        
        for (Element* childPart : childParts) {
            if (!visited.contains(childPart)) {
                visited.insert(childPart);
                QString newPath = current.path.isEmpty() 
                    ? childPart->declaredName() 
                    : QString("%1.%2").arg(current.path, childPart->declaredName());
                qDebug() << "[GraphicViewWidget] findPathToPart: Adding to queue:"
                         << childPart->declaredName() << "path:" << newPath;
                queue.append({childPart, newPath});
            }
        }
    }
    
    qDebug() << "[GraphicViewWidget] findPathToPart: ✗ Target not found after" << iteration << "iterations";
    return QString();
}

void GraphicViewWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event || !event->mimeData()) {
        event->ignore();
        return;
    }
    
    if (event->mimeData()->hasFormat("application/x-sysml-partdefinition")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void GraphicViewWidget::dragMoveEvent(QDragMoveEvent* event)
{
    // 必须重写 dragMoveEvent 并接受事件，否则 dropEvent 不会被调用
    if (event->mimeData()->hasFormat("application/x-sysml-partdefinition")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void GraphicViewWidget::dropEvent(QDropEvent* event)
{
    if (!event || !event->mimeData()) {
        event->ignore();
        return;
    }
    
    if (!event->mimeData()->hasFormat("application/x-sysml-partdefinition")) {
        event->ignore();
        return;
    }
    
    // 获取 PartDefinition 的 qualifiedName
    QByteArray data = event->mimeData()->data("application/x-sysml-partdefinition");
    QString qualifiedName = QString::fromUtf8(data);
    
    // 检查当前 part 是否存在
    if (!m_part) {
        event->ignore();
        return;
    }
    
    // 查找 PartDefinition 元素
    Element* partDef = findPartDefinitionByQualifiedName(qualifiedName);
    if (!partDef) {
        event->ignore();
        return;
    }
    
    // 获取 drop 位置（场景坐标）
    QPointF scenePos = mapToScene(event->position().toPoint());
    
    // 创建 PartUsage 实例
    Element* partUsage = createPartInstance(partDef, scenePos);
    if (partUsage) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

Element* GraphicViewWidget::createPartInstance(Element* partDef, const QPointF& scenePos, Element* parentPartUsage)
{
    if (!partDef) {
        return nullptr;
    }
    
    // 确定父元素：如果提供了 parentPartUsage，使用它；否则使用 m_part
    Element* parentElement = parentPartUsage ? parentPartUsage : m_part;
    if (!parentElement) {
        return nullptr;
    }
    
    // 检查 partDef 是否是 PartDefinition
    if (partDef->elementType() != "PartDefinition") {
        return nullptr;
    }
    
    // 生成唯一名称：类型名称首字母小写 + 数字
    QString baseName = partDef->declaredName();
    if (baseName.isEmpty()) {
        baseName = "PartInstance";
    }
    
    // 将类型名称首字母转为小写，其余保持不变
    QString prefix = baseName;
    if (!prefix.isEmpty()) {
        prefix[0] = prefix[0].toLower();
    }
    
    // 生成唯一名称：类型名称首字母小写 + 数字
    QString uniqueName = generateUniqueNameByPrefix(prefix, parentElement);
    
    // 创建 PartUsage
    PartUsage* partUsage = new PartUsage(parentElement);
    partUsage->setDeclaredName(uniqueName);
    // 使用 qualifiedName 作为 typeName，确保能正确查找到 PartDefinition
    partUsage->setTypeName(partDef->qualifiedName());
    
    // 添加到父元素
    parentElement->addOwnedElement(partUsage);
    
    // 创建图形元数据注解
    Annotation* annotation = new Annotation(partUsage);
    annotation->setAnnotationName("GraphicMetadata");
    
    // 计算相对于父 part 的本地坐标
    QPointF localPos = scenePos;
    Element* visualParent = parentElement;
    if (visualParent->elementType() == "PartDefinition") {
        // 如果父 part 是 PartDefinition，需要将场景坐标转换为本地坐标
        // 查找父 part 的图形项
        if (m_scene) {
            PartGraphicsItem* parentItem = m_scene->findPartItem(visualParent);
            if (parentItem) {
                localPos = parentItem->mapFromScene(scenePos);
            }
        }
    } else if (visualParent->elementType() == "PartUsage") {
        // 如果父 part 是 PartUsage，坐标相对于父 PartUsage
        // 需要将场景坐标转换为相对于父 PartUsage 的本地坐标
        if (m_scene) {
            PartGraphicsItem* parentItem = m_scene->findPartItem(visualParent);
            if (parentItem) {
                localPos = parentItem->mapFromScene(scenePos);
            }
        }
    }
    
    annotation->setMetadataAssignment("graphicX", localPos.x());
    annotation->setMetadataAssignment("graphicY", localPos.y());
    
    // 从 PartDefinition 获取默认大小，如果没有则使用默认值
    // 实例大小 = 类型大小 * 0.2
    BaseGraphicMetadata* defMetadata = AnnotationParser::parseGraphicMetadata(partDef);
    PartGraphicMetadata* defGraphicMeta = qobject_cast<PartGraphicMetadata*>(defMetadata);
    
    // 如果元数据不存在，尝试从 Annotation 中加载
    if (!defGraphicMeta) {
        Annotation* defAnnotation = AnnotationParser::findAnnotation(partDef, "GraphicMetadata");
        if (defAnnotation) {
            QVariantMap metadataMap = AnnotationParser::extractMetadata(defAnnotation);
            if (metadataMap.contains("graphicWidth") || metadataMap.contains("graphicHeight")) {
                defGraphicMeta = new PartGraphicMetadata();
                defGraphicMeta->loadFromMap(metadataMap);
            }
        }
    }
    
    double defaultWidth = 100.0;
    double defaultHeight = 80.0;
    
    if (defGraphicMeta) {
        double defWidth = defGraphicMeta->graphicWidth();
        double defHeight = defGraphicMeta->graphicHeight();
        
        if (defWidth > 0) {
            defaultWidth = defWidth * 0.2;  // 实例大小为类型的 0.2 倍
        }
        if (defHeight > 0) {
            defaultHeight = defHeight * 0.2;  // 实例大小为类型的 0.2 倍
        }
    }
    
    // 清理临时创建的元数据
    if (defGraphicMeta && defGraphicMeta != defMetadata) {
        delete defGraphicMeta;
    }
    if (defMetadata && defMetadata != defGraphicMeta) {
        delete defMetadata;
    }
    
    annotation->setMetadataAssignment("graphicWidth", defaultWidth);
    annotation->setMetadataAssignment("graphicHeight", defaultHeight);
    
    // 如果 PartDefinition 有子 part，需要递归创建对应的 PartUsage
    QList<Element*> defChildParts = getChildParts(partDef);
    
    // 如果 PartDefinition 有子 part，设置 showChildNestedContent = true，以便显示子 part
    if (!defChildParts.isEmpty()) {
        annotation->setMetadataAssignment("showChildNestedContent", true);
    }
    
    partUsage->addAnnotation(annotation);
    qDebug() << "[GraphicViewWidget] createPartInstance: Checking for child parts"
             << "partDef:" << partDef->declaredName()
             << "defChildParts count:" << defChildParts.size();
    
    if (!defChildParts.isEmpty()) {
        qDebug() << "[GraphicViewWidget] createPartInstance: Found child parts, creating instances...";
        // 计算缩放比例：实例大小 / 定义大小 = 0.2
        double scaleX = 0.2;
        double scaleY = 0.2;
        
        // 获取定义的大小，用于验证和计算子元素位置
        BaseGraphicMetadata* defMetadataForScale = AnnotationParser::parseGraphicMetadata(partDef);
        PartGraphicMetadata* defGraphicMetaForScale = qobject_cast<PartGraphicMetadata*>(defMetadataForScale);
        
        if (!defGraphicMetaForScale) {
            // 尝试从 Annotation 中加载
            Annotation* defAnnotation = AnnotationParser::findAnnotation(partDef, "GraphicMetadata");
            if (defAnnotation) {
                QVariantMap metadataMap = AnnotationParser::extractMetadata(defAnnotation);
                if (metadataMap.contains("graphicWidth") || metadataMap.contains("graphicHeight")) {
                    defGraphicMetaForScale = new PartGraphicMetadata();
                    defGraphicMetaForScale->loadFromMap(metadataMap);
                }
            }
        }
        
        // 如果获取到了定义的大小，验证缩放比例是否正确
        if (defGraphicMetaForScale) {
            double defWidth = defGraphicMetaForScale->graphicWidth();
            double defHeight = defGraphicMetaForScale->graphicHeight();
            
            // 验证：实例大小应该是定义大小的 0.2 倍
            if (defWidth > 0) {
                double expectedWidth = defWidth * 0.2;
                if (qAbs(defaultWidth - expectedWidth) > 0.01) {
                    // 如果不一致，使用计算出的缩放比例
                    scaleX = defaultWidth / defWidth;
                }
            }
            if (defHeight > 0) {
                double expectedHeight = defHeight * 0.2;
                if (qAbs(defaultHeight - expectedHeight) > 0.01) {
                    // 如果不一致，使用计算出的缩放比例
                    scaleY = defaultHeight / defHeight;
                }
            }
            
            // 清理临时创建的元数据
            if (defGraphicMetaForScale != defMetadataForScale) {
                delete defGraphicMetaForScale;
            }
        }
        if (defMetadataForScale && defMetadataForScale != defGraphicMetaForScale) {
            delete defMetadataForScale;
        }
        
        // 为每个子 part 创建对应的 PartUsage
        for (Element* childDef : defChildParts) {
            qDebug() << "[GraphicViewWidget] createPartInstance: Processing child part"
                     << "name:" << childDef->declaredName()
                     << "type:" << childDef->elementType();
            
            if (childDef->elementType() == "PartDefinition") {
                qDebug() << "[GraphicViewWidget] createPartInstance: Creating child PartUsage for"
                         << childDef->declaredName();
                
                // 从 PartDefinition 的图形元数据中获取子 part 的原始位置
                BaseGraphicMetadata* childDefMetadata = AnnotationParser::parseGraphicMetadata(childDef);
                PartGraphicMetadata* childDefGraphicMeta = qobject_cast<PartGraphicMetadata*>(childDefMetadata);
                
                QPointF childLocalPos = localPos + QPointF(20, 20);  // 默认位置
                
                if (childDefGraphicMeta) {
                    // 使用子 part 在定义中的位置，并应用缩放
                    double childX = childDefGraphicMeta->graphicX();
                    double childY = childDefGraphicMeta->graphicY();
                    childLocalPos = QPointF(childX * scaleX, childY * scaleY);
                } else {
                    // 尝试从 Annotation 中加载
                    Annotation* childAnnotation = AnnotationParser::findAnnotation(childDef, "GraphicMetadata");
                    if (childAnnotation) {
                        QVariantMap childMetadataMap = AnnotationParser::extractMetadata(childAnnotation);
                        if (childMetadataMap.contains("graphicX") && childMetadataMap.contains("graphicY")) {
                            double childX = childMetadataMap["graphicX"].toDouble();
                            double childY = childMetadataMap["graphicY"].toDouble();
                            childLocalPos = QPointF(childX * scaleX, childY * scaleY);
                        }
                    }
                }
                
                // 清理临时创建的元数据
                if (childDefGraphicMeta && childDefGraphicMeta != childDefMetadata) {
                    delete childDefGraphicMeta;
                }
                if (childDefMetadata && childDefMetadata != childDefGraphicMeta) {
                    delete childDefMetadata;
                }
                
                // 递归创建子 PartUsage
                // childLocalPos 已经是相对于父 PartUsage 的本地坐标
                // 由于 partUsage 还没有渲染，我们直接传递本地坐标
                // createPartInstance 会检测到父 PartUsage 还没有渲染，直接使用作为本地坐标
                qDebug() << "[GraphicViewWidget] createPartInstance: Calling createPartInstance for child"
                         << "childDef:" << childDef->declaredName()
                         << "childLocalPos:" << childLocalPos
                         << "parentPartUsage:" << partUsage->declaredName();
                Element* childPartUsage = createPartInstance(childDef, childLocalPos, partUsage);
                qDebug() << "[GraphicViewWidget] createPartInstance: Child PartUsage created"
                         << "childPartUsage:" << (childPartUsage ? childPartUsage->declaredName() : "null");
            } else {
                qDebug() << "[GraphicViewWidget] createPartInstance: Skipping child (not PartDefinition):"
                         << childDef->elementType();
            }
        }
    } else {
        qDebug() << "[GraphicViewWidget] createPartInstance: No child parts found for"
                 << partDef->declaredName();
    }
    
    // 直接渲染新创建的 PartUsage，而不是刷新整个视图
    // 这样可以避免清除场景并重新渲染所有内容
    if (m_scene) {
        PartGraphicsItem* parentItem = nullptr;
        if (visualParent->elementType() == "PartDefinition") {
            // 如果父 part 是 PartDefinition，查找其图形项
            parentItem = m_scene->findPartItem(visualParent);
        } else if (visualParent->elementType() == "PartUsage") {
            // 如果父 part 是 PartUsage，查找其图形项（作为嵌套 part）
            parentItem = m_scene->findPartItem(visualParent);
        }
        renderPart(partUsage, parentItem);
    }
    
    // 触发变更通知
    Element* notifyParent = parentElement;
    if (notifyParent) {
        emit notifyParent->elementChanged();
    }
    
    qDebug() << "[GraphicViewWidget] createPartInstance: Created PartUsage"
             << "name:" << uniqueName
             << "type:" << partDef->declaredName()
             << "position:" << localPos
             << "size:" << defaultWidth << "x" << defaultHeight
             << "hasChildren:" << !defChildParts.isEmpty();
    
    return partUsage;
}

Element* GraphicViewWidget::findPartDefinitionByQualifiedName(const QString& qualifiedName)
{
    if (!m_part) {
        return nullptr;
    }
    
    // 从当前 part 开始向上查找根元素（没有 owner 的元素）
    Element* root = m_part;
    while (root && root->owner()) {
        root = root->owner();
    }
    
    // 如果找到了根元素，从根元素开始递归查找
    if (root) {
        return findElementByQualifiedNameRecursive(root, qualifiedName);
    }
    
    // 如果没找到根元素，从当前 part 开始查找（作为后备方案）
    return findElementByQualifiedNameRecursive(m_part, qualifiedName);
}

Element* GraphicViewWidget::findElementByQualifiedNameRecursive(Element* parent, const QString& qualifiedName)
{
    if (!parent) {
        return nullptr;
    }
    
    // 检查当前元素是否匹配
    if (parent->qualifiedName() == qualifiedName) {
        return parent;
    }
    
    // 递归查找子元素
    QList<Element*> children = parent->ownedElements();
    for (Element* child : children) {
        Element* found = findElementByQualifiedNameRecursive(child, qualifiedName);
        if (found) {
            return found;
        }
    }
    
    return nullptr;
}

Element* GraphicViewWidget::findPartDefinitionByQualifiedNameHelper(Element* parent, const QString& qualifiedName) const
{
    if (!parent) {
        return nullptr;
    }
    
    // 检查当前元素是否匹配
    if (parent->qualifiedName() == qualifiedName) {
        return parent;
    }
    
    // 递归查找子元素
    QList<Element*> children = parent->ownedElements();
    for (Element* child : children) {
        Element* found = findPartDefinitionByQualifiedNameHelper(child, qualifiedName);
        if (found) {
            return found;
        }
    }
    
    return nullptr;
}

QString GraphicViewWidget::generateUniqueName(const QString& baseName, Element* parent)
{
    if (!parent) {
        return baseName;
    }
    
    QList<Element*> existingParts = parent->getChildParts();
    QStringList existingNames;
    for (Element* part : existingParts) {
        existingNames << part->declaredName();
    }
    
    QString name = baseName;
    int counter = 1;
    while (existingNames.contains(name)) {
        name = QString("%1_%2").arg(baseName).arg(counter);
        counter++;
    }
    
    return name;
}

QString GraphicViewWidget::generateUniqueNameByPrefix(const QString& prefix, Element* parent)
{
    if (!parent) {
        return prefix + "1";
    }
    
    QList<Element*> existingParts = parent->getChildParts();
    QStringList existingNames;
    for (Element* part : existingParts) {
        existingNames << part->declaredName();
    }
    
    // 查找已使用的数字
    QSet<int> usedNumbers;
    QRegularExpression regex(QString("^%1(\\d+)$").arg(QRegularExpression::escape(prefix)));
    for (const QString& name : existingNames) {
        QRegularExpressionMatch match = regex.match(name);
        if (match.hasMatch()) {
            bool ok;
            int num = match.captured(1).toInt(&ok);
            if (ok) {
                usedNumbers.insert(num);
            }
        }
    }
    
    // 从 1 开始查找第一个未使用的数字
    int counter = 1;
    while (usedNumbers.contains(counter)) {
        counter++;
    }
    
    return prefix + QString::number(counter);
}

SYSML_NAMESPACE_END
