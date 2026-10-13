#include "ResourceSerializer.h"
#include "Resource.h"
#include "../model/Element.h"
#include "../model/Package.h"
#include "../model/PartDefinition.h"
#include "../model/PartUsage.h"
#include "../model/Feature.h"
#include "../model/Relationship.h"
#include "../model/Import.h"
#include "../model/NamespaceImport.h"
#include "../model/MembershipImport.h"
#include "../model/FeatureTyping.h"
#include "../model/Subclassification.h"
#include "../model/Membership.h"
// 状态机相关类
#include "../model/ExhibitStatement.h"
#include "../model/StateDefinition.h"
#include "../model/StateUsage.h"
#include "../model/Transition.h"
#include "../model/TransitionTriggerPart.h"
#include "../model/TransitionGuardPart.h"
#include "../model/TransitionEffectPart.h"
#include "../model/AcceptClause.h"
#include "../model/SendStatement.h"
#include "../model/StateActionMember.h"
#include "../model/ConstraintStatement.h"
// 定义类型
#include "../model/ActionDefinition.h"
#include "../model/RequirementDefinition.h"
#include "../model/PortDefinition.h"
#include "../model/ConnectorDefinition.h"
#include "../model/InterfaceDefinition.h"
#include "../model/DataTypeDefinition.h"
#include "../model/ClassDefinition.h"
#include "../model/EnumDefinition.h"
#include "../model/AttributeDefinition.h"
#include "../model/ConnectionDefinition.h"
#include "../model/FlowDefinition.h"
#include "../model/AllocationDefinition.h"
#include "../model/ConstraintDefinition.h"
#include "../model/ItemDefinition.h"
// 使用类型
#include "../model/ActionUsage.h"
#include "../model/RequirementUsage.h"
#include "../model/PortUsage.h"
#include "../model/ConnectorUsage.h"
#include "../model/FlowUsage.h"
#include "../model/ConnectionUsage.h"
#include "../model/AllocationUsage.h"
#include "../model/ConstraintUsage.h"
#include "../model/ItemUsage.h"
// 关系类型
#include "../model/Subsetting.h"
#include "../model/Redefinition.h"
#include "../model/Conjugation.h"
#include "../model/Dependency.h"
// 语句类型
#include "../model/BindStatement.h"
#include "../model/ConnectStatement.h"
#include "../model/AllocateStatement.h"
#include "../model/PerformStatement.h"
#include "../model/FlowStatement.h"
#include "../model/EndStatement.h"
#include "../model/DocStatement.h"
#include "../model/AssertStatement.h"
#include "../model/RequireStatement.h"
#include "../model/AssumeStatement.h"
#include "../model/RedefinesStatement.h"
// 其他类型
#include "../model/ActionInputOutput.h"
#include <QFile>
#include <QJsonArray>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

SYSML_NAMESPACE_BEGIN

// 共享的静态变量，在文件作用域定义
static QHash<QString, ResourceSerializer::ElementFactory>* g_factories = nullptr;
static QBasicMutex g_factoriesMutex;
static bool g_factoriesInitialized = false;

QHash<QString, ResourceSerializer::ElementFactory>& ResourceSerializer::factories()
{
    QMutexLocker locker(&g_factoriesMutex);
    
    if (!g_factoriesInitialized) {
        qDebug() << "[ResourceSerializer::factories] Initializing factories...";
        // 创建静态哈希表
        static QHash<QString, ElementFactory> factories_storage;
        g_factories = &factories_storage;
        
        // 注册默认工厂函数（直接操作静态变量，避免递归）
        registerDefaultFactories();
        
        g_factoriesInitialized = true;
        qDebug() << "[ResourceSerializer::factories] Factories initialized, registered" << g_factories->size() << "types";
    }
    
    return *g_factories;
}

void ResourceSerializer::registerDefaultFactories()
{
    qDebug() << "[ResourceSerializer::registerDefaultFactories] Registering default factories";
    
    // 注意：此函数在 factories() 中调用，已经持有锁，不需要再次加锁
    
    if (!g_factories) {
        qWarning() << "[ResourceSerializer::registerDefaultFactories] Error: g_factories not initialized";
        return;
    }
    
    if (g_factories->contains("Package")) {
        qDebug() << "[ResourceSerializer::registerDefaultFactories] Warning: Factories already initialized, skipping";
        return;
    }
    
    g_factories->insert("Package", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating Package";
        auto pkg = new Package();
        pkg->setDeclaredName(json["declaredName"].toString());
        return pkg;
    });
    
    g_factories->insert("PartDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating PartDefinition";
        auto def = new PartDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("PartUsage", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating PartUsage";
        auto usage = new PartUsage();
        usage->setDeclaredName(json["declaredName"].toString());
        // 恢复PartUsage的特定属性
        if (json.contains("isRef")) {
            usage->setIsRef(json["isRef"].toBool());
        }
        if (json.contains("typeName")) {
            usage->setTypeName(json["typeName"].toString());
        }
        if (json.contains("isDerived")) {
            usage->setIsDerived(json["isDerived"].toBool());
        }
        if (json.contains("isConstant")) {
            usage->setIsConstant(json["isConstant"].toBool());
        }
        if (json.contains("direction")) {
            usage->setDirection(static_cast<PartUsage::Direction>(json["direction"].toInt()));
        }
        if (json.contains("portionKind")) {
            usage->setPortionKind(static_cast<PartUsage::PortionKind>(json["portionKind"].toInt()));
        }
        if (json.contains("memberKind")) {
            usage->setMemberKind(json["memberKind"].toString());
        }
        return usage;
    });
    
    g_factories->insert("Feature", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating Feature";
        auto feature = new Feature();
        feature->setDeclaredName(json["declaredName"].toString());
        // 恢复Feature的特定属性
        if (json.contains("isAbstract")) {
            feature->setIsAbstract(json["isAbstract"].toBool());
        }
        if (json.contains("isConst")) {
            feature->setIsConst(json["isConst"].toBool());
        }
        if (json.contains("isDerived")) {
            feature->setIsDerived(json["isDerived"].toBool());
        }
        if (json.contains("direction")) {
            feature->setDirection(static_cast<Feature::Direction>(json["direction"].toInt()));
        }
        if (json.contains("isRef")) {
            feature->setIsRef(json["isRef"].toBool());
        }
        if (json.contains("unitExpression")) {
            feature->setUnitExpression(json["unitExpression"].toString());
        }
        if (json.contains("valueExpression")) {
            feature->setValueExpression(json["valueExpression"].toString());
        }
        if (json.contains("isSimplifiedAttribute")) {
            feature->setIsSimplifiedAttribute(json["isSimplifiedAttribute"].toBool());
        }
        if (json.contains("isRedefines")) {
            feature->setIsRedefines(json["isRedefines"].toBool());
        }
        if (json.contains("defaultValue")) {
            feature->setDefaultValue(json["defaultValue"].toString());
        }
        return feature;
    });
    
    g_factories->insert("NamespaceImport", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating NamespaceImport";
        auto import = new NamespaceImport();
        if (json.contains("visibility")) {
            import->setVisibility(static_cast<NamespaceImport::Visibility>(json["visibility"].toInt()));
        }
        if (json.contains("isAll")) {
            import->setIsAll(json["isAll"].toBool());
        }
        if (json.contains("isWildcard")) {
            import->setIsWildcard(json["isWildcard"].toBool());
        }
        if (json.contains("isRecursive")) {
            import->setIsRecursive(json["isRecursive"].toBool());
        }
        if (json.contains("importedNamespaceName")) {
            import->setImportedNamespaceName(json["importedNamespaceName"].toString());
        }
        return import;
    });
    
    g_factories->insert("MembershipImport", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating MembershipImport";
        auto import = new MembershipImport();
        if (json.contains("visibility")) {
            import->setVisibility(static_cast<MembershipImport::Visibility>(json["visibility"].toInt()));
        }
        if (json.contains("isRecursive")) {
            import->setIsRecursive(json["isRecursive"].toBool());
        }
        return import;
    });
    
    g_factories->insert("FeatureTyping", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating FeatureTyping";
        auto typing = new FeatureTyping();
        // 恢复类型名称（用于 :>Type 语法）
        if (json.contains("typeName")) {
            typing->setTypeName(json["typeName"].toString());
        }
        // 注意：typedFeature 和 type 的关联需要在反序列化后通过名称查找重建
        return typing;
    });
    
    g_factories->insert("Subclassification", [](const QJsonObject&) -> Element* {
        qDebug() << "[Factory] Creating Subclassification";
        return new Subclassification();
    });
    
    g_factories->insert("Membership", [](const QJsonObject&) -> Element* {
        qDebug() << "[Factory] Creating Membership";
        return new Membership();
    });
    
    // === 状态机相关类工厂注册 ===
    
    g_factories->insert("ExhibitStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ExhibitStatement";
        auto stmt = new ExhibitStatement();
        stmt->setDeclaredName(json["declaredName"].toString());
        if (json.contains("stateMachineKind")) {
            stmt->setStateMachineKind(json["stateMachineKind"].toString());
        }
        return stmt;
    });
    
    g_factories->insert("StateDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating StateDefinition";
        auto def = new StateDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("StateUsage", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating StateUsage";
        auto usage = new StateUsage();
        usage->setDeclaredName(json["declaredName"].toString());
        return usage;
    });
    
    g_factories->insert("Transition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating Transition";
        auto transition = new Transition();
        transition->setDeclaredName(json["declaredName"].toString());
        if (json.contains("sourceState")) {
            transition->setSourceState(json["sourceState"].toString());
        }
        if (json.contains("targetState")) {
            transition->setTargetState(json["targetState"].toString());
        }
        if (json.contains("isInitial")) {
            transition->setIsInitial(json["isInitial"].toBool());
        }
        return transition;
    });
    
    g_factories->insert("TransitionTriggerPart", [](const QJsonObject&) -> Element* {
        qDebug() << "[Factory] Creating TransitionTriggerPart";
        return new TransitionTriggerPart();
    });
    
    g_factories->insert("TransitionGuardPart", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating TransitionGuardPart";
        auto guard = new TransitionGuardPart();
        if (json.contains("condition")) {
            guard->setCondition(json["condition"].toString());
        }
        return guard;
    });
    
    g_factories->insert("TransitionEffectPart", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating TransitionEffectPart";
        auto effect = new TransitionEffectPart();
        if (json.contains("actionType")) {
            effect->setActionType(json["actionType"].toString());
        }
        if (json.contains("actionReference")) {
            effect->setActionReference(json["actionReference"].toString());
        }
        if (json.contains("actionInvocation")) {
            effect->setActionInvocation(json["actionInvocation"].toString());
        }
        return effect;
    });
    
    g_factories->insert("AcceptClause", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating AcceptClause";
        auto accept = new AcceptClause();
        if (json.contains("acceptType")) {
            accept->setAcceptType(json["acceptType"].toString());
        }
        if (json.contains("usageDeclaration")) {
            accept->setUsageDeclaration(json["usageDeclaration"].toString());
        }
        if (json.contains("viaPort")) {
            accept->setViaPort(json["viaPort"].toString());
        }
        if (json.contains("condition")) {
            accept->setCondition(json["condition"].toString());
        }
        return accept;
    });
    
    g_factories->insert("SendStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating SendStatement";
        auto send = new SendStatement();
        if (json.contains("hasNew")) {
            send->setHasNew(json["hasNew"].toBool());
        }
        if (json.contains("actionInvocation")) {
            send->setActionInvocation(json["actionInvocation"].toString());
        }
        if (json.contains("target")) {
            send->setTarget(json["target"].toString());
        }
        return send;
    });
    
    g_factories->insert("StateActionMember", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating StateActionMember";
        auto action = new StateActionMember();
        if (json.contains("actionKind")) {
            action->setActionKind(json["actionKind"].toString());
        }
        if (json.contains("actionName")) {
            action->setActionName(json["actionName"].toString());
        }
        if (json.contains("hasActionKeyword")) {
            action->setHasActionKeyword(json["hasActionKeyword"].toBool());
        }
        return action;
    });
    
    g_factories->insert("ConstraintStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ConstraintStatement";
        auto constraint = new ConstraintStatement();
        if (json.contains("expression")) {
            constraint->setExpression(json["expression"].toString());
        }
        return constraint;
    });
    
    // === 定义类型工厂注册 ===
    
    g_factories->insert("ActionDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ActionDefinition";
        auto def = new ActionDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("RequirementDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating RequirementDefinition";
        auto def = new RequirementDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("PortDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating PortDefinition";
        auto def = new PortDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("ConnectorDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ConnectorDefinition";
        auto def = new ConnectorDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("InterfaceDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating InterfaceDefinition";
        auto def = new InterfaceDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("DataTypeDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating DataTypeDefinition";
        auto def = new DataTypeDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("ClassDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ClassDefinition";
        auto def = new ClassDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("EnumDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating EnumDefinition";
        auto def = new EnumDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("AttributeDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating AttributeDefinition";
        auto def = new AttributeDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("ConnectionDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ConnectionDefinition";
        auto def = new ConnectionDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("FlowDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating FlowDefinition";
        auto def = new FlowDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("AllocationDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating AllocationDefinition";
        auto def = new AllocationDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("ConstraintDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ConstraintDefinition";
        auto def = new ConstraintDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    g_factories->insert("ItemDefinition", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ItemDefinition";
        auto def = new ItemDefinition();
        def->setDeclaredName(json["declaredName"].toString());
        return def;
    });
    
    // === 使用类型工厂注册 ===
    
    g_factories->insert("ActionUsage", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ActionUsage";
        auto usage = new ActionUsage();
        usage->setDeclaredName(json["declaredName"].toString());
        if (json.contains("typeName")) {
            usage->setTypeName(json["typeName"].toString());
        }
        return usage;
    });
    
    g_factories->insert("RequirementUsage", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating RequirementUsage";
        auto usage = new RequirementUsage();
        usage->setDeclaredName(json["declaredName"].toString());
        if (json.contains("typeName")) {
            usage->setTypeName(json["typeName"].toString());
        }
        return usage;
    });
    
    g_factories->insert("PortUsage", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating PortUsage";
        auto usage = new PortUsage();
        usage->setDeclaredName(json["declaredName"].toString());
        if (json.contains("typeName")) {
            usage->setTypeName(json["typeName"].toString());
        }
        return usage;
    });
    
    g_factories->insert("ConnectorUsage", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ConnectorUsage";
        auto usage = new ConnectorUsage();
        usage->setDeclaredName(json["declaredName"].toString());
        if (json.contains("typeName")) {
            usage->setTypeName(json["typeName"].toString());
        }
        return usage;
    });
    
    g_factories->insert("FlowUsage", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating FlowUsage";
        auto usage = new FlowUsage();
        usage->setDeclaredName(json["declaredName"].toString());
        if (json.contains("typeName")) {
            usage->setTypeName(json["typeName"].toString());
        }
        return usage;
    });
    
    g_factories->insert("ConnectionUsage", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ConnectionUsage";
        auto usage = new ConnectionUsage();
        usage->setDeclaredName(json["declaredName"].toString());
        if (json.contains("typeName")) {
            usage->setTypeName(json["typeName"].toString());
        }
        return usage;
    });
    
    g_factories->insert("AllocationUsage", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating AllocationUsage";
        auto usage = new AllocationUsage();
        usage->setDeclaredName(json["declaredName"].toString());
        if (json.contains("typeName")) {
            usage->setTypeName(json["typeName"].toString());
        }
        return usage;
    });
    
    g_factories->insert("ConstraintUsage", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ConstraintUsage";
        auto usage = new ConstraintUsage();
        usage->setDeclaredName(json["declaredName"].toString());
        if (json.contains("typeName")) {
            usage->setTypeName(json["typeName"].toString());
        }
        return usage;
    });
    
    g_factories->insert("ItemUsage", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ItemUsage";
        auto usage = new ItemUsage();
        usage->setDeclaredName(json["declaredName"].toString());
        if (json.contains("typeName")) {
            usage->setTypeName(json["typeName"].toString());
        }
        if (json.contains("isRef")) {
            usage->setIsRef(json["isRef"].toBool());
        }
        if (json.contains("isDerived")) {
            usage->setIsDerived(json["isDerived"].toBool());
        }
        if (json.contains("isConstant")) {
            usage->setIsConstant(json["isConstant"].toBool());
        }
        if (json.contains("direction")) {
            usage->setDirection(static_cast<ItemUsage::Direction>(json["direction"].toInt()));
        }
        if (json.contains("portionKind")) {
            usage->setPortionKind(static_cast<ItemUsage::PortionKind>(json["portionKind"].toInt()));
        }
        return usage;
    });
    
    // === 关系类型工厂注册 ===
    
    g_factories->insert("Subsetting", [](const QJsonObject&) -> Element* {
        qDebug() << "[Factory] Creating Subsetting";
        return new Subsetting();
    });
    
    g_factories->insert("Redefinition", [](const QJsonObject&) -> Element* {
        qDebug() << "[Factory] Creating Redefinition";
        return new Redefinition();
    });
    
    g_factories->insert("Conjugation", [](const QJsonObject&) -> Element* {
        qDebug() << "[Factory] Creating Conjugation";
        return new Conjugation();
    });
    
    g_factories->insert("Dependency", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating Dependency";
        auto dep = new Dependency();
        // 注意：clientElement和supplierElement的关联需要在反序列化后通过名称查找重建
        // 这里先保存名称，在deserializeElement的后续处理中重建关联
        if (json.contains("clientElementName")) {
            // 名称会在后续处理中用于查找元素
        }
        if (json.contains("supplierElementName")) {
            dep->setSupplierElementName(json["supplierElementName"].toString());
        }
        return dep;
    });
    
    // === 语句类型工厂注册 ===
    
    g_factories->insert("BindStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating BindStatement";
        auto stmt = new BindStatement();
        if (json.contains("sourceEnd")) {
            stmt->setSourceEnd(json["sourceEnd"].toString());
        }
        if (json.contains("targetEnd")) {
            stmt->setTargetEnd(json["targetEnd"].toString());
        }
        return stmt;
    });
    
    g_factories->insert("ConnectStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ConnectStatement";
        auto stmt = new ConnectStatement();
        if (json.contains("ends")) {
            QStringList ends;
            QJsonArray endsArray = json["ends"].toArray();
            for (const QJsonValue& value : endsArray) {
                ends.append(value.toString());
            }
            stmt->setEnds(ends);
        }
        if (json.contains("isBinary")) {
            stmt->setIsBinary(json["isBinary"].toBool());
        }
        return stmt;
    });
    
    g_factories->insert("AllocateStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating AllocateStatement";
        auto stmt = new AllocateStatement();
        stmt->setDeclaredName(json["declaredName"].toString());
        return stmt;
    });
    
    g_factories->insert("PerformStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating PerformStatement";
        auto stmt = new PerformStatement();
        stmt->setDeclaredName(json["declaredName"].toString());
        return stmt;
    });
    
    g_factories->insert("FlowStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating FlowStatement";
        auto stmt = new FlowStatement();
        stmt->setDeclaredName(json["declaredName"].toString());
        return stmt;
    });
    
    g_factories->insert("EndStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating EndStatement";
        auto stmt = new EndStatement();
        if (json.contains("tag")) {
            stmt->setTag(json["tag"].toString());
        }
        if (json.contains("typeName")) {
            stmt->setTypeName(json["typeName"].toString());
        }
        if (json.contains("isNegated")) {
            stmt->setIsNegated(json["isNegated"].toBool());
        }
        if (json.contains("usesReferences")) {
            stmt->setUsesReferences(json["usesReferences"].toBool());
        }
        return stmt;
    });
    
    g_factories->insert("DocStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating DocStatement";
        auto stmt = new DocStatement();
        if (json.contains("content")) {
            stmt->setContent(json["content"].toString());
        }
        return stmt;
    });
    
    g_factories->insert("ConnectStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ConnectStatement";
        auto stmt = new ConnectStatement();
        if (json.contains("ends")) {
            QStringList ends;
            QJsonArray endsArray = json["ends"].toArray();
            for (const QJsonValue& value : endsArray) {
                ends.append(value.toString());
            }
            stmt->setEnds(ends);
        }
        if (json.contains("isBinary")) {
            stmt->setIsBinary(json["isBinary"].toBool());
        }
        return stmt;
    });
    
    g_factories->insert("AllocateStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating AllocateStatement";
        auto stmt = new AllocateStatement();
        if (json.contains("sourceEnds")) {
            QStringList sourceEnds;
            QJsonArray sourceEndsArray = json["sourceEnds"].toArray();
            for (const QJsonValue& value : sourceEndsArray) {
                sourceEnds.append(value.toString());
            }
            stmt->setSourceEnds(sourceEnds);
        }
        if (json.contains("targetEnd")) {
            stmt->setTargetEnd(json["targetEnd"].toString());
        }
        if (json.contains("isBinary")) {
            stmt->setIsBinary(json["isBinary"].toBool());
        }
        return stmt;
    });
    
    g_factories->insert("PerformStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating PerformStatement";
        auto stmt = new PerformStatement();
        if (json.contains("actionReference")) {
            stmt->setActionReference(json["actionReference"].toString());
        }
        if (json.contains("isActionKeyword")) {
            stmt->setIsActionKeyword(json["isActionKeyword"].toBool());
        }
        if (json.contains("actionName")) {
            stmt->setActionName(json["actionName"].toString());
        }
        if (json.contains("actionType")) {
            stmt->setActionType(json["actionType"].toString());
        }
        if (json.contains("specializationType")) {
            stmt->setSpecializationType(json["specializationType"].toString());
        }
        return stmt;
    });
    
    g_factories->insert("FlowStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating FlowStatement";
        auto stmt = new FlowStatement();
        if (json.contains("fromEnd")) {
            stmt->setFromEnd(json["fromEnd"].toString());
        }
        if (json.contains("toEnd")) {
            stmt->setToEnd(json["toEnd"].toString());
        }
        if (json.contains("payload")) {
            stmt->setPayload(json["payload"].toString());
        }
        if (json.contains("flowName")) {
            stmt->setFlowName(json["flowName"].toString());
        }
        if (json.contains("flowType")) {
            stmt->setFlowType(json["flowType"].toString());
        }
        if (json.contains("hasFromKeyword")) {
            stmt->setHasFromKeyword(json["hasFromKeyword"].toBool());
        }
        return stmt;
    });
    
    g_factories->insert("AssertStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating AssertStatement";
        auto stmt = new AssertStatement();
        if (json.contains("expression")) {
            stmt->setExpression(json["expression"].toString());
        }
        return stmt;
    });
    
    g_factories->insert("RequireStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating RequireStatement";
        auto stmt = new RequireStatement();
        if (json.contains("expression")) {
            stmt->setExpression(json["expression"].toString());
        }
        if (json.contains("hasConstraintKeyword")) {
            stmt->setHasConstraintKeyword(json["hasConstraintKeyword"].toBool());
        }
        return stmt;
    });
    
    g_factories->insert("AssumeStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating AssumeStatement";
        auto stmt = new AssumeStatement();
        if (json.contains("expression")) {
            stmt->setExpression(json["expression"].toString());
        }
        return stmt;
    });
    
    g_factories->insert("RedefinesStatement", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating RedefinesStatement";
        auto stmt = new RedefinesStatement();
        if (json.contains("redefinesName")) {
            stmt->setRedefinesName(json["redefinesName"].toString());
        }
        if (json.contains("expression")) {
            stmt->setExpression(json["expression"].toString());
        }
        return stmt;
    });
    
    // === 其他类型工厂注册 ===
    
    g_factories->insert("ActionInputOutput", [](const QJsonObject& json) -> Element* {
        qDebug() << "[Factory] Creating ActionInputOutput";
        auto aio = new ActionInputOutput();
        if (json.contains("isInput")) {
            aio->setIsInput(json["isInput"].toBool());
        }
        if (json.contains("hasItemKeyword")) {
            aio->setHasItemKeyword(json["hasItemKeyword"].toBool());
        }
        if (json.contains("itemName")) {
            aio->setItemName(json["itemName"].toString());
        }
        if (json.contains("itemType")) {
            aio->setItemType(json["itemType"].toString());
        }
        if (json.contains("defaultValue")) {
            aio->setDefaultValue(json["defaultValue"].toString());
        }
        if (json.contains("redefinesName")) {
            aio->setRedefinesName(json["redefinesName"].toString());
        }
        if (json.contains("multiplicity")) {
            aio->setMultiplicity(json["multiplicity"].toString());
        }
        return aio;
    });
    
    qDebug() << "[ResourceSerializer::registerDefaultFactories] Completed, registered" << g_factories->size() << "types";
}

void ResourceSerializer::registerFactory(const QString& typeName, ElementFactory factory)
{
    qDebug() << "[ResourceSerializer::registerFactory] Registering type:" << typeName;
    QHash<QString, ElementFactory>& s_factories = factories();
    s_factories[typeName] = factory;
    qDebug() << "[ResourceSerializer::registerFactory] Completed, total types:" << s_factories.size();
}

QJsonObject ResourceSerializer::serializeResource(Resource* resource)
{
    if (!resource) {
        qDebug() << "[ResourceSerializer::serializeResource] Resource is nullptr!";
        return QJsonObject();
    }
    
    qDebug() << "[ResourceSerializer::serializeResource] Starting serialization of resource:" << resource->uri();
    qDebug() << "[ResourceSerializer::serializeResource] Contents count:" << resource->contents().size();
    
    QJsonObject json;
    json["version"] = "0.1.0";
    json["uri"] = resource->uri();
    
    QJsonArray contents;
    int contentIndex = 0;
    for (Element* element : resource->contents()) {
        contentIndex++;
        qDebug() << "[ResourceSerializer::serializeResource] Serializing content #" << contentIndex 
                 << "of type:" << element->elementType() << "named:" << element->declaredName();
        contents.append(serializeElement(element));
    }
    json["contents"] = contents;
    
    qDebug() << "[ResourceSerializer::serializeResource] Serialization completed, serialized" << contents.size() << "elements";
    
    return json;
}

bool ResourceSerializer::deserializeResource(Resource* resource, const QJsonObject& json)
{
    qDebug() << "[ResourceSerializer::deserializeResource] Starting deserialization";
    
    if (!resource) {
        qWarning() << "[ResourceSerializer::deserializeResource] Resource pointer is null";
        return false;
    }
    
    resource->setUri(json["uri"].toString());
    qDebug() << "[ResourceSerializer::deserializeResource] URI:" << resource->uri();
    
    QJsonArray contents = json["contents"].toArray();
    qDebug() << "[ResourceSerializer::deserializeResource] Content count:" << contents.size();
    
    for (int i = 0; i < contents.size(); ++i) {
        const QJsonValue& value = contents[i];
        QJsonObject elementJson = value.toObject();
        qDebug() << "[ResourceSerializer::deserializeResource] Processing element" << (i+1);
        Element* element = deserializeElement(elementJson, resource);
        if (element) {
            qDebug() << "[ResourceSerializer::deserializeResource] Adding element to resource:" << element->qualifiedName();
            resource->addContent(element);
        } else {
            qWarning() << "[ResourceSerializer::deserializeResource] Element deserialization failed";
        }
    }
    
    resource->markUnmodified();
    qDebug() << "[ResourceSerializer::deserializeResource] Deserialization completed";
    return true;
}

QJsonObject ResourceSerializer::serializeElement(Element* element)
{
    if (!element) {
        qDebug() << "[ResourceSerializer::serializeElement] Element is nullptr!";
        return QJsonObject();
    }
    
    QString type = element->elementType();
    QString name = element->declaredName();
    
    // 只在序列化 Feature 和 FeatureTyping 时输出详细调试信息
    bool verboseDebug = (type == "Feature" || type == "FeatureTyping");
    
    if (verboseDebug) {
        qDebug() << "[ResourceSerializer::serializeElement] ===== Serializing" << type << ":" << name << "=====";
    }
    
    QJsonObject json;
    json["type"] = type;
    json["declaredName"] = name;
    
    // === 状态机相关类的特定属性序列化 ===
    
    if (type == "ExhibitStatement") {
        ExhibitStatement* stmt = qobject_cast<ExhibitStatement*>(element);
        if (stmt) {
            json["stateMachineKind"] = stmt->stateMachineKind();
        }
    }
    else if (type == "Transition") {
        Transition* transition = qobject_cast<Transition*>(element);
        if (transition) {
            json["sourceState"] = transition->sourceState();
            json["targetState"] = transition->targetState();
            json["isInitial"] = transition->isInitial();
        }
    }
    else if (type == "TransitionGuardPart") {
        TransitionGuardPart* guard = qobject_cast<TransitionGuardPart*>(element);
        if (guard) {
            json["condition"] = guard->condition();
        }
    }
    else if (type == "TransitionEffectPart") {
        TransitionEffectPart* effect = qobject_cast<TransitionEffectPart*>(element);
        if (effect) {
            json["actionType"] = effect->actionType();
            json["actionReference"] = effect->actionReference();
            json["actionInvocation"] = effect->actionInvocation();
        }
    }
    else if (type == "AcceptClause") {
        AcceptClause* accept = qobject_cast<AcceptClause*>(element);
        if (accept) {
            json["acceptType"] = accept->acceptType();
            json["usageDeclaration"] = accept->usageDeclaration();
            json["viaPort"] = accept->viaPort();
            json["condition"] = accept->condition();
        }
    }
    else if (type == "SendStatement") {
        SendStatement* send = qobject_cast<SendStatement*>(element);
        if (send) {
            json["hasNew"] = send->hasNew();
            json["actionInvocation"] = send->actionInvocation();
            json["target"] = send->target();
        }
    }
    else if (type == "StateActionMember") {
        StateActionMember* action = qobject_cast<StateActionMember*>(element);
        if (action) {
            json["actionKind"] = action->actionKind();
            json["actionName"] = action->actionName();
            json["hasActionKeyword"] = action->hasActionKeyword();
        }
    }
    else if (type == "ConstraintStatement") {
        ConstraintStatement* constraint = qobject_cast<ConstraintStatement*>(element);
        if (constraint) {
            json["expression"] = constraint->expression();
        }
    }
    else if (type == "PartUsage") {
        PartUsage* partUsage = qobject_cast<PartUsage*>(element);
        if (partUsage) {
            json["isRef"] = partUsage->isRef();
            QString typeName = partUsage->typeName();
            if (!typeName.isEmpty()) {
                json["typeName"] = typeName;
            }
            // 也序列化其他PartUsage属性
            json["isDerived"] = partUsage->isDerived();
            json["isConstant"] = partUsage->isConstant();
            json["direction"] = static_cast<int>(partUsage->direction());
            json["portionKind"] = static_cast<int>(partUsage->portionKind());
            // 成员类型（subject/actor/stakeholder）
            QString memberKind = partUsage->memberKind();
            if (!memberKind.isEmpty()) {
                json["memberKind"] = memberKind;
            }
        }
    }
    else if (type == "BindStatement") {
        BindStatement* bind = qobject_cast<BindStatement*>(element);
        if (bind) {
            json["sourceEnd"] = bind->sourceEnd();
            json["targetEnd"] = bind->targetEnd();
        }
    }
    else if (type == "ConnectStatement") {
        ConnectStatement* connect = qobject_cast<ConnectStatement*>(element);
        if (connect) {
            QJsonArray endsArray;
            for (const QString& end : connect->ends()) {
                endsArray.append(end);
            }
            json["ends"] = endsArray;
            json["isBinary"] = connect->isBinary();
        }
    }
    else if (type == "AllocateStatement") {
        AllocateStatement* allocate = qobject_cast<AllocateStatement*>(element);
        if (allocate) {
            QJsonArray sourceEndsArray;
            for (const QString& end : allocate->sourceEnds()) {
                sourceEndsArray.append(end);
            }
            json["sourceEnds"] = sourceEndsArray;
            json["targetEnd"] = allocate->targetEnd();
            json["isBinary"] = allocate->isBinary();
        }
    }
    else if (type == "PerformStatement") {
        PerformStatement* perform = qobject_cast<PerformStatement*>(element);
        if (perform) {
            json["actionReference"] = perform->actionReference();
            json["isActionKeyword"] = perform->isActionKeyword();
            json["actionName"] = perform->actionName();
            json["actionType"] = perform->actionType();
            json["specializationType"] = perform->specializationType();
        }
    }
    else if (type == "FlowStatement") {
        FlowStatement* flow = qobject_cast<FlowStatement*>(element);
        if (flow) {
            json["fromEnd"] = flow->fromEnd();
            json["toEnd"] = flow->toEnd();
            json["payload"] = flow->payload();
            json["flowName"] = flow->flowName();
            json["flowType"] = flow->flowType();
            json["hasFromKeyword"] = flow->hasFromKeyword();
        }
    }
    else if (type == "EndStatement") {
        EndStatement* end = qobject_cast<EndStatement*>(element);
        if (end) {
            json["tag"] = end->tag();
            json["typeName"] = end->typeName();
            json["isNegated"] = end->isNegated();
            json["usesReferences"] = end->usesReferences();
        }
    }
    else if (type == "DocStatement") {
        DocStatement* doc = qobject_cast<DocStatement*>(element);
        if (doc) {
            json["content"] = doc->content();
        }
    }
    else if (type == "ActionInputOutput") {
        ActionInputOutput* aio = qobject_cast<ActionInputOutput*>(element);
        if (aio) {
            json["isInput"] = aio->isInput();
            json["hasItemKeyword"] = aio->hasItemKeyword();
            json["itemName"] = aio->itemName();
            json["itemType"] = aio->itemType();
            json["defaultValue"] = aio->defaultValue();
            json["redefinesName"] = aio->redefinesName();
            json["multiplicity"] = aio->multiplicity();
        }
    }
    else if (type == "ActionUsage") {
        ActionUsage* usage = qobject_cast<ActionUsage*>(element);
        if (usage) {
            QString typeName = usage->typeName();
            if (!typeName.isEmpty()) {
                json["typeName"] = typeName;
            }
        }
    }
    else if (type == "RequirementUsage") {
        RequirementUsage* usage = qobject_cast<RequirementUsage*>(element);
        if (usage) {
            QString typeName = usage->typeName();
            if (!typeName.isEmpty()) {
                json["typeName"] = typeName;
            }
        }
    }
    else if (type == "PortUsage") {
        PortUsage* usage = qobject_cast<PortUsage*>(element);
        if (usage) {
            QString typeName = usage->typeName();
            if (!typeName.isEmpty()) {
                json["typeName"] = typeName;
            }
        }
    }
    else if (type == "ConnectorUsage") {
        ConnectorUsage* usage = qobject_cast<ConnectorUsage*>(element);
        if (usage) {
            QString typeName = usage->typeName();
            if (!typeName.isEmpty()) {
                json["typeName"] = typeName;
            }
        }
    }
    else if (type == "FlowUsage") {
        FlowUsage* usage = qobject_cast<FlowUsage*>(element);
        if (usage) {
            QString typeName = usage->typeName();
            if (!typeName.isEmpty()) {
                json["typeName"] = typeName;
            }
        }
    }
    else if (type == "ConnectionUsage") {
        ConnectionUsage* usage = qobject_cast<ConnectionUsage*>(element);
        if (usage) {
            QString typeName = usage->typeName();
            if (!typeName.isEmpty()) {
                json["typeName"] = typeName;
            }
        }
    }
    else if (type == "AllocationUsage") {
        AllocationUsage* usage = qobject_cast<AllocationUsage*>(element);
        if (usage) {
            QString typeName = usage->typeName();
            if (!typeName.isEmpty()) {
                json["typeName"] = typeName;
            }
        }
    }
    else if (type == "ConstraintUsage") {
        ConstraintUsage* usage = qobject_cast<ConstraintUsage*>(element);
        if (usage) {
            QString typeName = usage->typeName();
            if (!typeName.isEmpty()) {
                json["typeName"] = typeName;
            }
        }
    }
    else if (type == "ItemUsage") {
        ItemUsage* usage = qobject_cast<ItemUsage*>(element);
        if (usage) {
            QString typeName = usage->typeName();
            if (!typeName.isEmpty()) {
                json["typeName"] = typeName;
            }
            json["isRef"] = usage->isRef();
            json["isDerived"] = usage->isDerived();
            json["isConstant"] = usage->isConstant();
            json["direction"] = static_cast<int>(usage->direction());
            json["portionKind"] = static_cast<int>(usage->portionKind());
        }
    }
    else if (type == "AssertStatement") {
        AssertStatement* assert = qobject_cast<AssertStatement*>(element);
        if (assert) {
            json["expression"] = assert->expression();
        }
    }
    else if (type == "RequireStatement") {
        RequireStatement* require = qobject_cast<RequireStatement*>(element);
        if (require) {
            json["expression"] = require->expression();
            json["hasConstraintKeyword"] = require->hasConstraintKeyword();
        }
    }
    else if (type == "AssumeStatement") {
        AssumeStatement* assume = qobject_cast<AssumeStatement*>(element);
        if (assume) {
            json["expression"] = assume->expression();
        }
    }
    else if (type == "RedefinesStatement") {
        RedefinesStatement* redefines = qobject_cast<RedefinesStatement*>(element);
        if (redefines) {
            json["redefinesName"] = redefines->redefinesName();
            json["expression"] = redefines->expression();
        }
    }
    else if (type == "Dependency") {
        Dependency* dep = qobject_cast<Dependency*>(element);
        if (dep) {
            // 保存client和supplier元素的名称
            if (dep->clientElement()) {
                json["clientElementName"] = dep->clientElement()->declaredName();
            }
            QString supplierName = dep->supplierElementName();
            if (supplierName.isEmpty() && dep->supplierElement()) {
                supplierName = dep->supplierElement()->declaredName();
            }
            if (!supplierName.isEmpty()) {
                json["supplierElementName"] = supplierName;
            }
        }
    }
    else if (type == "Feature") {
        Feature* feature = qobject_cast<Feature*>(element);
        if (feature) {
            qDebug() << "[ResourceSerializer::serializeElement] Serializing Feature:" << feature->declaredName();
            qDebug() << "  - ownedRelationships count:" << feature->ownedRelationships().size();
            
            // 检查是否有 FeatureTyping 关系
            for (Relationship* rel : feature->ownedRelationships()) {
                if (rel->elementType() == "FeatureTyping") {
                    FeatureTyping* typing = qobject_cast<FeatureTyping*>(rel);
                    if (typing) {
                        qDebug() << "  - Found FeatureTyping with typeName:" << typing->typeName();
                    }
                }
            }
            
            // 序列化Feature的特定属性
            json["isAbstract"] = feature->isAbstract();
            json["isConst"] = feature->isConst();
            json["isDerived"] = feature->isDerived();
            json["direction"] = static_cast<int>(feature->direction());
            json["isRef"] = feature->isRef();
            QString unitExpr = feature->unitExpression();
            if (!unitExpr.isEmpty()) {
                json["unitExpression"] = unitExpr;
            }
            QString valueExpr = feature->valueExpression();
            if (!valueExpr.isEmpty()) {
                json["valueExpression"] = valueExpr;
            }
            json["isSimplifiedAttribute"] = feature->isSimplifiedAttribute();
            json["isRedefines"] = feature->isRedefines();
            QString defaultValue = feature->defaultValue();
            if (!defaultValue.isEmpty()) {
                json["defaultValue"] = defaultValue;
            }
        }
    }
    else if (type == "NamespaceImport") {
        NamespaceImport* import = qobject_cast<NamespaceImport*>(element);
        if (import) {
            json["visibility"] = static_cast<int>(import->visibility());
            json["isAll"] = import->isAll();
            json["isWildcard"] = import->isWildcard();
            json["isRecursive"] = import->isRecursive();
            QString importedName = import->importedNamespaceName();
            if (!importedName.isEmpty()) {
                json["importedNamespaceName"] = importedName;
            }
        }
    }
    else if (type == "MembershipImport") {
        MembershipImport* import = qobject_cast<MembershipImport*>(element);
        if (import) {
            json["visibility"] = static_cast<int>(import->visibility());
            json["isRecursive"] = import->isRecursive();
        }
    }
    else if (type == "FeatureTyping") {
        FeatureTyping* typing = qobject_cast<FeatureTyping*>(element);
        if (typing) {
            qDebug() << "[ResourceSerializer::serializeElement] Serializing FeatureTyping";
            qDebug() << "  - declaredName:" << typing->declaredName();
            
            // 保存类型名称（用于 :>Type 语法）
            QString typeName = typing->typeName();
            qDebug() << "  - typeName:" << typeName;
            if (!typeName.isEmpty()) {
                json["typeName"] = typeName;
                qDebug() << "  - ✓ Added typeName to JSON:" << typeName;
            } else {
                qDebug() << "  - ✗ typeName is empty!";
            }
            
            // 如果 type() 不为空，也可以保存类型元素的名称
            if (typing->type()) {
                QString typeElementName = typing->type()->declaredName();
                json["typeElementName"] = typeElementName;
                qDebug() << "  - typeElementName:" << typeElementName;
            } else {
                qDebug() << "  - type() is nullptr";
            }
            
            // 检查 typedFeature
            if (typing->typedFeature()) {
                qDebug() << "  - typedFeature:" << typing->typedFeature()->declaredName();
            } else {
                qDebug() << "  - typedFeature() is nullptr";
            }
        } else {
            qDebug() << "[ResourceSerializer::serializeElement] Failed to cast to FeatureTyping!";
        }
    }
    
    // 序列化拥有的元素
    QJsonArray ownedElements;
    for (Element* child : element->ownedElements()) {
        ownedElements.append(serializeElement(child));
    }
    json["ownedElements"] = ownedElements;
    
    // 序列化关系
    QJsonArray relationships;
    int relationshipCount = 0;
    int featureTypingCount = 0;
    for (Relationship* rel : element->ownedRelationships()) {
        relationshipCount++;
        QString relType = rel->elementType();
        qDebug() << "[ResourceSerializer::serializeElement] Serializing relationship #" << relationshipCount 
                 << "of type:" << relType << "for element:" << element->declaredName();
        
        if (relType == "FeatureTyping") {
            featureTypingCount++;
            FeatureTyping* typing = qobject_cast<FeatureTyping*>(rel);
            if (typing) {
                qDebug() << "[ResourceSerializer::serializeElement] Found FeatureTyping relationship:";
                qDebug() << "  - typeName:" << typing->typeName();
                qDebug() << "  - typedFeature:" << (typing->typedFeature() ? typing->typedFeature()->declaredName() : "nullptr");
                qDebug() << "  - type:" << (typing->type() ? typing->type()->declaredName() : "nullptr");
            }
        }
        
        relationships.append(serializeElement(rel));
    }
    json["ownedRelationships"] = relationships;
    
    // 如果是 Feature，特别记录关系信息
    if (type == "Feature") {
        qDebug() << "[ResourceSerializer::serializeElement] Feature" << element->declaredName() 
                 << "has" << relationshipCount << "relationships, including" << featureTypingCount << "FeatureTyping";
    }
    
    return json;
}

Element* ResourceSerializer::deserializeElement(const QJsonObject& json, QObject* /*parent*/)
{
    QString typeName = json["type"].toString();
    qDebug() << "[ResourceSerializer::deserializeElement] Deserializing type:" << typeName;
    
    QHash<QString, ElementFactory>& s_factories = factories();
    auto factory = s_factories.value(typeName);
    if (!factory) {
        qWarning() << "[ResourceSerializer::deserializeElement] Unknown element type:" << typeName;
        qWarning() << "[ResourceSerializer::deserializeElement] Available types:" << s_factories.keys();
        return nullptr;
    }
    
    qDebug() << "[ResourceSerializer::deserializeElement] Calling factory to create" << typeName;
    Element* element = factory(json);
    if (!element) {
        qWarning() << "[ResourceSerializer::deserializeElement] Factory returned null pointer";
        return nullptr;
    }
    qDebug() << "[ResourceSerializer::deserializeElement] Successfully created" << typeName << ", address:" << element;
    
    // 注意：不在这里设置父对象
    // 对于顶层元素，父对象由 Resource::addContent 设置
    // 对于子元素，父对象由 Element::addOwnedElement 设置
    
    // 反序列化拥有的元素（先创建所有子元素）
    QJsonArray ownedElements = json["ownedElements"].toArray();
    qDebug() << "[ResourceSerializer::deserializeElement] Has" << ownedElements.size() << "owned elements";
    QList<Element*> children;
    for (const QJsonValue& value : ownedElements) {
        Element* child = deserializeElement(value.toObject(), element);
        if (child) {
            children.append(child);
        }
    }
    
    // 然后添加子元素（这会设置父对象）
    // 对于某些特殊类型，需要在添加前进行关联
    if (typeName == "Transition") {
        Transition* transition = qobject_cast<Transition*>(element);
        if (transition) {
            // 识别并关联 triggerPart, guardPart, effectPart
            for (Element* child : children) {
                QString childType = child->elementType();
                if (childType == "TransitionTriggerPart") {
                    TransitionTriggerPart* triggerPart = qobject_cast<TransitionTriggerPart*>(child);
                    if (triggerPart) {
                        transition->setTriggerPart(triggerPart);
                    }
                } else if (childType == "TransitionGuardPart") {
                    TransitionGuardPart* guardPart = qobject_cast<TransitionGuardPart*>(child);
                    if (guardPart) {
                        transition->setGuardPart(guardPart);
                    }
                } else if (childType == "TransitionEffectPart") {
                    TransitionEffectPart* effectPart = qobject_cast<TransitionEffectPart*>(child);
                    if (effectPart) {
                        transition->setEffectPart(effectPart);
                    }
                }
                // 仍然需要添加到ownedElements
                element->addOwnedElement(child);
            }
        } else {
            // 如果不是Transition，按正常方式添加
            for (Element* child : children) {
                element->addOwnedElement(child);
            }
        }
    } else if (typeName == "TransitionTriggerPart") {
        TransitionTriggerPart* triggerPart = qobject_cast<TransitionTriggerPart*>(element);
        if (triggerPart) {
            // 识别并关联 acceptClause
            for (Element* child : children) {
                if (child->elementType() == "AcceptClause") {
                    AcceptClause* acceptClause = qobject_cast<AcceptClause*>(child);
                    if (acceptClause) {
                        triggerPart->setAcceptClause(acceptClause);
                    }
                }
                element->addOwnedElement(child);
            }
        } else {
            for (Element* child : children) {
                element->addOwnedElement(child);
            }
        }
    } else if (typeName == "TransitionEffectPart") {
        TransitionEffectPart* effectPart = qobject_cast<TransitionEffectPart*>(element);
        if (effectPart) {
            // 识别并关联 sendStatement
            for (Element* child : children) {
                if (child->elementType() == "SendStatement") {
                    SendStatement* sendStatement = qobject_cast<SendStatement*>(child);
                    if (sendStatement) {
                        effectPart->setSendStatement(sendStatement);
                    }
                }
                element->addOwnedElement(child);
            }
        } else {
            for (Element* child : children) {
                element->addOwnedElement(child);
            }
        }
    } else {
        // 普通元素，按正常方式添加
        for (Element* child : children) {
            qDebug() << "[ResourceSerializer::deserializeElement] Adding child element to" << typeName;
            element->addOwnedElement(child);
        }
    }
    
    // 反序列化关系
    QJsonArray relationships = json["ownedRelationships"].toArray();
    qDebug() << "[ResourceSerializer::deserializeElement] Has" << relationships.size() << "relationships";
    QList<Relationship*> rels;
    for (const QJsonValue& value : relationships) {
        Relationship* rel = qobject_cast<Relationship*>(deserializeElement(value.toObject(), element));
        if (rel) {
            rels.append(rel);
        }
    }
    
    // 然后添加关系，并重建关联
    for (Relationship* rel : rels) {
        qDebug() << "[ResourceSerializer::deserializeElement] Adding relationship to" << typeName;
        
        // 如果是 FeatureTyping 关系，且 typedFeature 为空，则设置为当前元素（如果当前元素是 Feature）
        FeatureTyping* featureTyping = qobject_cast<FeatureTyping*>(rel);
        if (featureTyping) {
            Feature* feature = qobject_cast<Feature*>(element);
            if (feature && !featureTyping->typedFeature()) {
                featureTyping->setTypedFeature(feature);
                qDebug() << "[ResourceSerializer::deserializeElement] Set typedFeature for FeatureTyping:" 
                         << feature->declaredName() << "->" << featureTyping->typeName();
            }
        }
        
        element->addOwnedRelationship(rel);
    }
    
    qDebug() << "[ResourceSerializer::deserializeElement] Completed deserialization of" << typeName;
    return element;
}

bool ResourceSerializer::saveToFile(Resource* resource, const QString& filePath)
{
    qDebug() << "[ResourceSerializer::saveToFile] Saving to file:" << filePath;
    
    if (!resource) {
        qWarning() << "[ResourceSerializer::saveToFile] Resource pointer is null";
        return false;
    }
    
    qDebug() << "[ResourceSerializer::saveToFile] Serializing resource";
    QJsonObject json = serializeResource(resource);
    QJsonDocument doc(json);
    
    qDebug() << "[ResourceSerializer::saveToFile] Opening file";
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[ResourceSerializer::saveToFile] Failed to open file:" << filePath;
        return false;
    }
    
    qDebug() << "[ResourceSerializer::saveToFile] Writing to file";
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    resource->setFilePath(filePath);
    resource->markUnmodified();
    
    qDebug() << "[ResourceSerializer::saveToFile] Save completed";
    return true;
}

bool ResourceSerializer::loadFromFile(Resource* resource, const QString& filePath)
{
    qDebug() << "[ResourceSerializer::loadFromFile] Loading from file:" << filePath;
    
    if (!resource) {
        qWarning() << "[ResourceSerializer::loadFromFile] Resource pointer is null";
        return false;
    }
    
    qDebug() << "[ResourceSerializer::loadFromFile] Opening file";
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[ResourceSerializer::loadFromFile] Failed to open file:" << filePath;
        return false;
    }
    
    qDebug() << "[ResourceSerializer::loadFromFile] Reading file";
    QByteArray data = file.readAll();
    file.close();
    qDebug() << "[ResourceSerializer::loadFromFile] File size:" << data.size() << "bytes";
    
    qDebug() << "[ResourceSerializer::loadFromFile] Parsing JSON";
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "[ResourceSerializer::loadFromFile] JSON parse error:" << error.errorString();
        qWarning() << "[ResourceSerializer::loadFromFile] Error position:" << error.offset;
        return false;
    }
    
    resource->setFilePath(filePath);
    qDebug() << "[ResourceSerializer::loadFromFile] Starting deserialization";
    bool result = deserializeResource(resource, doc.object());
    qDebug() << "[ResourceSerializer::loadFromFile] Load" << (result ? "succeeded" : "failed");
    return result;
}

SYSML_NAMESPACE_END

