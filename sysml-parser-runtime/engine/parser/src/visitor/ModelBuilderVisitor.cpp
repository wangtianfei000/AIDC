#include "ModelBuilderVisitor.h"
#include <functional>
#include "../../core/infrastructure/Resource.h"
#include "../../core/model/Package.h"
#include "../../core/model/PartDefinition.h"
#include "../../core/model/PartUsage.h"
#include "../../core/model/ActionUsage.h"
#include "../../core/model/StateUsage.h"
#include "../../core/model/RequirementUsage.h"
#include "../../core/model/ConstraintUsage.h"
#include "../../core/model/PortUsage.h"
#include "../../core/model/ConnectorUsage.h"
#include "../../core/model/FlowUsage.h"
#include "../../core/model/Feature.h"
#include "../../core/model/NamespaceImport.h"
#include "../../core/model/Import.h"
#include "../../core/model/FeatureTyping.h"
#include "../../core/model/Subclassification.h"
#include "../../core/model/Subsetting.h"
#include "../../core/model/Redefinition.h"
#include "../../core/model/Conjugation.h"
#include "../../core/model/Dependency.h"
#include "../../core/model/Type.h"
#include "../../core/model/Namespace.h"
#include "../../core/model/ActionDefinition.h"
#include "../../core/model/CalculationDefinition.h"
#include "../../core/model/AnalysisCase.h"
#include "../../core/model/RenderingUsage.h"
#include "../../core/model/ReturnParameter.h"
#include "../../core/model/AnalysisSubject.h"
#include "../../core/model/StateDefinition.h"
#include "../../core/model/RequirementDefinition.h"
#include "../../core/model/ConstraintDefinition.h"
#include "../../core/model/PortDefinition.h"
#include "../../core/model/ConnectorDefinition.h"
#include "../../core/model/InterfaceDefinition.h"
#include "../../core/model/DataTypeDefinition.h"
#include "../../core/model/ClassDefinition.h"
#include "../../core/model/EnumDefinition.h"
#include "../../core/model/AttributeDefinition.h"
#include "../../core/model/ConnectionDefinition.h"
#include "../../core/model/ConnectionUsage.h"
#include "../../core/model/AllocationDefinition.h"
#include "../../core/model/AllocationUsage.h"
#include "../../core/model/ItemDefinition.h"
#include "../../core/model/ItemUsage.h"
#include "../../core/model/ViewpointDefinition.h"
#include "../../core/model/ViewDefinition.h"
#include "../../core/model/ViewUsage.h"
#include "../../core/model/ConcernDefinition.h"
#include "../../core/model/ExposeStatement.h"
#include "../../core/model/ElementFilterMember.h"
#include "../../core/model/ViewRenderingMember.h"
#include "../../core/model/FrameConcernMember.h"
#include "../../core/model/EndStatement.h"
#include <QRegularExpression>
#include "../../core/model/BindStatement.h"
#include "../../core/model/ConnectStatement.h"
#include "../../core/model/AllocateStatement.h"
#include "../../core/model/FlowDefinition.h"
#include "../../core/model/FlowStatement.h"
#include "../../core/model/PerformStatement.h"
#include "../../core/model/AssertStatement.h"
#include "../../core/model/RequireStatement.h"
#include "../../core/model/AssumeStatement.h"
#include "../../core/model/ConstraintStatement.h"
#include "../../core/model/DocStatement.h"
#include "../../core/model/ExhibitStatement.h"
#include "../../core/model/StateActionMember.h"
#include "../../core/model/Transition.h"
#include "../../core/model/TransitionTriggerPart.h"
#include "../../core/model/AcceptClause.h"
#include "../../core/model/TransitionGuardPart.h"
#include "../../core/model/TransitionEffectPart.h"
#include "../../core/model/SendStatement.h"
#include "../../core/model/RedefinesStatement.h"
#include "../../core/model/ActionInputOutput.h"
#include "../../core/model/Annotation.h"
#include <QDebug>
#include <any>
#include <stdexcept>

SYSML_NAMESPACE_BEGIN

ModelBuilderVisitor::ModelBuilderVisitor(Resource* resource)
    : m_resource(resource)
{
    if (!m_resource) {
        qWarning() << "[ModelBuilderVisitor] Resource is null";
    }
}

ModelBuilderVisitor::~ModelBuilderVisitor()
{
}

std::any ModelBuilderVisitor::visitFile(SysML::SysMLParser::FileContext* ctx)
{
    qDebug() << "[ModelBuilderVisitor] Visiting file";
    
    if (!ctx) {
        reportError("File context is null");
        return nullptr;
    }
    
    // 访问所有包
    for (auto packageCtx : ctx->package()) {
        if (packageCtx) {
            visitPackage(packageCtx);
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitPackage(SysML::SysMLParser::PackageContext* ctx)
{
    if (!ctx) {
        reportError("Package context is null");
        return nullptr;
    }
    
    // 获取包名
    auto nameCtx = ctx->qualifiedName();
    if (!nameCtx) {
        reportError("Package qualified name is null", ctx);
        return nullptr;
    }
    
    QStringList nameParts = getQualifiedNameParts(nameCtx);
    QString packageName = nameParts.isEmpty() ? "UnnamedPackage" : nameParts.last();
    
    qDebug() << "[ModelBuilderVisitor] Creating package:" << packageName;
    
    // 创建包
    Package* package = new Package();
    package->setDeclaredName(packageName);
    
    // 检查重复名称
    checkDuplicateName(package, packageName, ctx);
    
    // 添加到资源或父元素
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(package);
    } else {
        if (m_resource) {
            m_resource->addContent(package);
        } else {
            reportError("Resource is null, cannot add package", ctx);
            delete package;
            return nullptr;
        }
    }
    
    // 推入父栈
    pushParent(package);
    
    // 访问包体
    auto bodyCtx = ctx->packageBody();
    if (bodyCtx) {
        visitPackageBody(bodyCtx);
    }
    
    // 弹出父栈
    popParent();
    
    return package;
}

std::any ModelBuilderVisitor::visitPackageBody(SysML::SysMLParser::PackageBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问所有子元素（包括关系）
    for (auto relCtx : ctx->relationship()) {
        if (relCtx) visitRelationship(relCtx);
    }
    
    // 访问所有子元素
    for (auto packageCtx : ctx->package()) {
        if (packageCtx) {
            visitPackage(packageCtx);
        }
    }
    
    // 访问定义类型
    for (auto defCtx : ctx->definition()) {
        if (defCtx) {
            visitDefinition(defCtx);
        }
    }
    
    // 访问使用类型
    for (auto usageCtx : ctx->usage()) {
        if (usageCtx) {
            visitUsage(usageCtx);
        }
    }
    
    for (auto importCtx : ctx->importStatement()) {
        if (importCtx) {
            visitImportStatement(importCtx);
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitPartDefinition(SysML::SysMLParser::PartDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("PartDefinition context is null");
        return nullptr;
    }
    
    // 获取定义前缀
    auto prefixCtx = ctx->occurrenceDefinitionPrefix();
    bool isAbstract = false;
    bool isVariation = false;
    bool isIndividual = false;
    
    if (prefixCtx) {
        // 处理 basicDefinitionPrefix
        auto basicPrefixCtx = prefixCtx->basicDefinitionPrefix();
        if (basicPrefixCtx) {
            // basicDefinitionPrefix 可以是 abstract 或 variation
            // 检查实际的文本内容
            QString prefixText = QString::fromStdString(basicPrefixCtx->getText());
            if (prefixText == "abstract") {
                isAbstract = true;
            } else if (prefixText == "variation") {
                isVariation = true;
            }
        }
        
        // 处理 individual 前缀
        if (prefixCtx->emptyMultiplicityMember()) {
            isIndividual = true;
        }
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("PartDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedPart";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        // identification 可以是 <id> name 或 name
        // 如果有两个标识符，第二个是名称；如果只有一个，就是名称
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating PartDefinition:" << name;
    
    // 创建部件定义
    PartDefinition* partDef = new PartDefinition();
    partDef->setDeclaredName(name);
    
    // 设置定义前缀
    if (isAbstract) {
        partDef->setIsAbstract(true);
    }
    if (isVariation) {
        partDef->setIsVariation(true);
    }
    if (isIndividual) {
        partDef->setIsIndividual(true);
    }
    
    // 检查重复名称
    checkDuplicateName(partDef, name, ctx);
    
    // 添加到父元素
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(partDef);
    } else {
        if (m_resource) {
            m_resource->addContent(partDef);
        } else {
            reportError("Resource is null, cannot add PartDefinition", ctx);
            delete partDef;
            return nullptr;
        }
    }
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        // 处理多个超类型
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(partDef, supertypeParts);
                }
            }
        }
    }
    
    // 推入父栈
    pushParent(partDef);
    
    // 访问定义体
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    // 弹出父栈
    popParent();
    
    return partDef;
}

std::any ModelBuilderVisitor::visitPartUsage(SysML::SysMLParser::PartUsageContext* ctx)
{
    if (!ctx) {
        reportError("PartUsage context is null");
        return nullptr;
    }
    
    // 获取使用前缀
    auto prefixCtx = ctx->occurrenceUsagePrefix();
    bool isDerived = false;
    bool isConstant = false;
    bool isRef = false;
    bool isAbstract = false;
    bool isVariation = false;
    bool isIndividual = false;
    PartUsage::Direction direction = PartUsage::DirectionNone;
    PartUsage::PortionKind portionKind = PartUsage::PortionNone;
    
    if (prefixCtx) {
        // 处理 endUsagePrefix
        auto endPrefixCtx = prefixCtx->endUsagePrefix();
        if (endPrefixCtx) {
            // end 前缀暂时不处理（后续可扩展）
        }
        
        // 处理 basicUsagePrefix
        auto basicPrefixCtx = prefixCtx->basicUsagePrefix();
        if (basicPrefixCtx) {
            // 处理 refPrefix
            auto refPrefixCtx = basicPrefixCtx->refPrefix();
            if (refPrefixCtx) {
                // 处理 featureDirection
                auto directionCtx = refPrefixCtx->featureDirection();
                if (directionCtx) {
                    QString dirText = QString::fromStdString(directionCtx->getText());
                    if (dirText == "in") {
                        direction = PartUsage::DirectionIn;
                    } else if (dirText == "out") {
                        direction = PartUsage::DirectionOut;
                    } else if (dirText == "inout") {
                        direction = PartUsage::DirectionInOut;
                    }
                }
                
                // 检查是否有 derived
                QString refPrefixText = QString::fromStdString(refPrefixCtx->getText());
                if (refPrefixText.contains("derived")) {
                    isDerived = true;
                }
                
                // 检查 abstract 或 variation
                if (refPrefixText.contains("abstract")) {
                    isAbstract = true;
                } else if (refPrefixText.contains("variation")) {
                    isVariation = true;
                }
                
                // 检查 constant
                if (refPrefixText.contains("constant")) {
                    isConstant = true;
                }
            }
            
            // 检查 ref
            QString basicPrefixText = QString::fromStdString(basicPrefixCtx->getText());
            if (basicPrefixText.contains("ref")) {
                isRef = true;
            }
        }
        
        // 处理 individual
        QString prefixText = QString::fromStdString(prefixCtx->getText());
        if (prefixText.contains("individual")) {
            isIndividual = true;
        }
        
        // 处理 portionKind
        auto portionCtx = prefixCtx->portionKind();
        if (portionCtx) {
            QString portionText = QString::fromStdString(portionCtx->getText());
            if (portionText == "snapshot") {
                portionKind = PartUsage::PortionSnapshot;
            } else if (portionText == "timeslice") {
                portionKind = PartUsage::PortionTimeslice;
            }
        }
    }
    
    // 获取使用声明（可选）
    QString name = "UnnamedPart";
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
            
            // 处理特征特殊化部分（类型化、子集化、重定义等）
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                // 处理类型化
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        // 将在创建 PartUsage 后处理类型化
                    }
                }
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating PartUsage:" << name;
    
    // 创建部件使用
    PartUsage* partUsage = new PartUsage();
    partUsage->setDeclaredName(name);
    
    // 设置使用前缀
    if (isDerived) {
        partUsage->setIsDerived(true);
    }
    if (isConstant) {
        partUsage->setIsConstant(true);
    }
    if (isRef) {
        partUsage->setIsRef(true);
    }
    if (isAbstract) {
        partUsage->setIsAbstract(true);
    }
    if (isVariation) {
        partUsage->setIsVariation(true);
    }
    if (isIndividual) {
        partUsage->setIsIndividual(true);
    }
    if (direction != PartUsage::DirectionNone) {
        partUsage->setDirection(direction);
    }
    if (portionKind != PartUsage::PortionNone) {
        partUsage->setPortionKind(portionKind);
    }
    
    // 检查重复名称
    checkDuplicateName(partUsage, name, ctx);
    
    // 处理类型（从特征特殊化部分）
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        QString typeName = typeParts.join("::");
                        qDebug() << "[ModelBuilderVisitor] PartUsage type:" << typeName;
                        
                        // 验证类型引用
                        validateTypeReference(typeParts, ctx);
                        
                        // 创建类型化关系
                        createPartUsageTyping(partUsage, typeParts);
                    }
                }
            }
        }
    }
    
    // 添加到父元素
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(partUsage);
    } else {
        if (m_resource) {
            m_resource->addContent(partUsage);
        } else {
            reportError("Resource is null, cannot add PartUsage", ctx);
            delete partUsage;
            return nullptr;
        }
    }
    
    // 推入父栈
    pushParent(partUsage);
    
    // 访问使用完成（包含值部分和使用体）
    auto completionCtx = ctx->usageCompletion();
    if (completionCtx) {
        // 处理 value part（如果有）
        processUsageCompletionValuePart(completionCtx);
        
        auto usageBodyCtx = completionCtx->usageBody();
        if (usageBodyCtx) {
            visitUsageBody(usageBodyCtx);
        }
    }
    
    // 弹出父栈
    popParent();
    
    qDebug() << "[ModelBuilderVisitor] visitPartUsage: finished, annotations count =" << partUsage->ownedAnnotations().size();
    return partUsage;
}

std::any ModelBuilderVisitor::visitAnalysisUsage(SysML::SysMLParser::AnalysisUsageContext* ctx)
{
    if (!ctx) return nullptr;
    QString name = "UnnamedAnalysis";
    if (auto* declaration = ctx->usageDeclaration()) {
        if (auto* feature = declaration->featureDeclaration()) {
            if (auto* identification = feature->identification()) {
                const auto identifiers = identification->identifier();
                if (!identifiers.empty()) name = getText(identifiers.back()->IDENTIFIER());
            }
        }
    }
    auto* analysis = new AnalysisCase();
    analysis->setDeclaredName(name);
    checkDuplicateName(analysis, name, ctx);
    if (auto* parent = getCurrentParent()) parent->addOwnedElement(analysis);
    else if (m_resource) m_resource->addContent(analysis);
    else { delete analysis; return nullptr; }
    pushParent(analysis);
    if (ctx->analysisBody()) visitAnalysisBody(ctx->analysisBody());
    popParent();
    return analysis;
}

std::any ModelBuilderVisitor::visitAnalysisBody(SysML::SysMLParser::AnalysisBodyContext* ctx)
{
    if (!ctx) return nullptr;
    for (auto* item : ctx->analysisBodyItem()) {
        if (item->subjectAssignment()) visitSubjectAssignment(item->subjectAssignment());
        else if (item->definitionBodyItem()) visitDefinitionBodyItem(item->definitionBodyItem());
    }
    return nullptr;
}

std::any ModelBuilderVisitor::visitSubjectAssignment(SysML::SysMLParser::SubjectAssignmentContext* ctx)
{
    if (!ctx || !ctx->expression()) return nullptr;
    const QString reference = QString::fromStdString(ctx->expression()->getText());
    auto* subject = new AnalysisSubject();
    subject->setReference(reference);
    if (auto* parent = getCurrentParent()) {
        parent->addOwnedElement(subject);
        if (auto* analysis = qobject_cast<AnalysisCase*>(parent)) analysis->setSubjectReference(reference);
    } else if (m_resource) m_resource->addContent(subject);
    else { delete subject; return nullptr; }
    return subject;
}

std::any ModelBuilderVisitor::visitRenderingUsage(SysML::SysMLParser::RenderingUsageContext* ctx)
{
    if (!ctx) return nullptr;
    QString name = "UnnamedRendering";
    QString typeName;
    if (auto* declaration = ctx->usageDeclaration()) {
        if (auto* feature = declaration->featureDeclaration()) {
            if (auto* identification = feature->identification()) {
                const auto identifiers = identification->identifier();
                if (!identifiers.empty()) name = getText(identifiers.back()->IDENTIFIER());
            }
            if (auto* specialization = feature->featureSpecializationPart()) {
                SysML::SysMLParser::QualifiedNameContext* type = nullptr;
                if (specialization->typingExpression()) type = specialization->typingExpression()->qualifiedName();
                else if (specialization->subsettingExpression()) type = specialization->subsettingExpression()->qualifiedName();
                else if (specialization->redefinitionExpression()) type = specialization->redefinitionExpression()->qualifiedName();
                if (type) typeName = getQualifiedNameParts(type).join("::");
            }
        }
    }
    auto* rendering = new RenderingUsage();
    rendering->setDeclaredName(name);
    rendering->setRenderingType(typeName);
    checkDuplicateName(rendering, name, ctx);
    if (auto* parent = getCurrentParent()) parent->addOwnedElement(rendering);
    else if (m_resource) m_resource->addContent(rendering);
    else { delete rendering; return nullptr; }
    pushParent(rendering);
    if (ctx->definitionBody()) visitDefinitionBody(ctx->definitionBody());
    popParent();
    return rendering;
}

std::any ModelBuilderVisitor::visitActionUsage(SysML::SysMLParser::ActionUsageContext* ctx)
{
    if (!ctx) {
        reportError("ActionUsage context is null");
        return nullptr;
    }
    
    // 获取使用声明（可选）
    QString name = "UnnamedAction";
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ActionUsage:" << name;
    
    ActionUsage* actionUsage = new ActionUsage();
    actionUsage->setDeclaredName(name);
    
    checkDuplicateName(actionUsage, name, ctx);
    
    // 处理类型（从特征特殊化部分）
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        validateTypeReference(typeParts, ctx);
                        createUsageTyping(actionUsage, typeParts);
                    }
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(actionUsage);
    } else {
        if (m_resource) {
            m_resource->addContent(actionUsage);
        } else {
            reportError("Resource is null, cannot add ActionUsage", ctx);
            delete actionUsage;
            return nullptr;
        }
    }
    
    pushParent(actionUsage);
    
    // 访问使用完成（包含值部分和使用体）
    auto completionCtx = ctx->usageCompletion();
    if (completionCtx) {
        // 处理 value part（如果有）
        processUsageCompletionValuePart(completionCtx);
        
        auto usageBodyCtx = completionCtx->usageBody();
        if (usageBodyCtx) {
            visitUsageBody(usageBodyCtx);
        }
    }
    
    popParent();
    
    return actionUsage;
}

std::any ModelBuilderVisitor::visitStateUsage(SysML::SysMLParser::StateUsageContext* ctx)
{
    if (!ctx) {
        reportError("StateUsage context is null");
        return nullptr;
    }
    
    // 获取使用声明（可选）
    QString name = "UnnamedState";
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
        }
    }
    
    // 添加调试信息：记录所有访问的 stateUsage
    QString stateUsageText = QString::fromStdString(ctx->getText());
    qDebug() << "[ModelBuilderVisitor] visitStateUsage called for:" << name << "text:" << stateUsageText;
    
    qDebug() << "[ModelBuilderVisitor] Creating StateUsage:" << name;
    
    StateUsage* stateUsage = new StateUsage();
    stateUsage->setDeclaredName(name);
    
    checkDuplicateName(stateUsage, name, ctx);
    
    // 处理类型（从特征特殊化部分）
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        validateTypeReference(typeParts, ctx);
                        createUsageTyping(stateUsage, typeParts);
                    }
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(stateUsage);
    } else {
        if (m_resource) {
            m_resource->addContent(stateUsage);
        } else {
            reportError("Resource is null, cannot add StateUsage", ctx);
            delete stateUsage;
            return nullptr;
        }
    }
    
    pushParent(stateUsage);
    
    // 访问使用完成（包含值部分和使用体）
    auto completionCtx = ctx->usageCompletion();
    if (completionCtx) {
        // 处理 value part（如果有）
        processUsageCompletionValuePart(completionCtx);
        
        auto usageBodyCtx = completionCtx->usageBody();
        if (usageBodyCtx) {
            visitUsageBody(usageBodyCtx);
        }
    }
    
    popParent();
    
    return stateUsage;
}

std::any ModelBuilderVisitor::visitRequirementUsage(SysML::SysMLParser::RequirementUsageContext* ctx)
{
    if (!ctx) {
        reportError("RequirementUsage context is null");
        return nullptr;
    }
    
    // 获取使用声明（可选）
    QString name = "UnnamedRequirement";
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating RequirementUsage:" << name;
    
    RequirementUsage* requirementUsage = new RequirementUsage();
    requirementUsage->setDeclaredName(name);
    
    checkDuplicateName(requirementUsage, name, ctx);
    
    // 处理类型（从特征特殊化部分）
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        validateTypeReference(typeParts, ctx);
                        createUsageTyping(requirementUsage, typeParts);
                    }
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(requirementUsage);
    } else {
        if (m_resource) {
            m_resource->addContent(requirementUsage);
        } else {
            reportError("Resource is null, cannot add RequirementUsage", ctx);
            delete requirementUsage;
            return nullptr;
        }
    }
    
    pushParent(requirementUsage);
    
    // 访问使用完成（包含值部分和使用体）
    auto completionCtx = ctx->usageCompletion();
    if (completionCtx) {
        // 处理 value part（如果有）
        processUsageCompletionValuePart(completionCtx);
        
        auto usageBodyCtx = completionCtx->usageBody();
        if (usageBodyCtx) {
            visitUsageBody(usageBodyCtx);
        }
    }
    
    popParent();
    
    return requirementUsage;
}

std::any ModelBuilderVisitor::visitConstraintUsage(SysML::SysMLParser::ConstraintUsageContext* ctx)
{
    if (!ctx) {
        reportError("ConstraintUsage context is null");
        return nullptr;
    }
    
    // 获取使用声明（可选）
    QString name = "UnnamedConstraint";
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ConstraintUsage:" << name;
    
    ConstraintUsage* constraintUsage = new ConstraintUsage();
    constraintUsage->setDeclaredName(name);
    
    checkDuplicateName(constraintUsage, name, ctx);
    
    // 处理类型（从特征特殊化部分）
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        validateTypeReference(typeParts, ctx);
                        createUsageTyping(constraintUsage, typeParts);
                        constraintUsage->setTypeName(typeParts.join("::"));
                    }
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(constraintUsage);
    } else {
        if (m_resource) {
            m_resource->addContent(constraintUsage);
        } else {
            reportError("Resource is null, cannot add ConstraintUsage", ctx);
            delete constraintUsage;
            return nullptr;
        }
    }
    
    pushParent(constraintUsage);
    
    // 访问使用完成（包含值部分和使用体）
    auto completionCtx = ctx->usageCompletion();
    if (completionCtx) {
        // 处理 value part（如果有）
        processUsageCompletionValuePart(completionCtx);
        
        auto usageBodyCtx = completionCtx->usageBody();
        if (usageBodyCtx) {
            visitUsageBody(usageBodyCtx);
        }
    }
    
    popParent();
    
    return constraintUsage;
}

std::any ModelBuilderVisitor::visitPortUsage(SysML::SysMLParser::PortUsageContext* ctx)
{
    if (!ctx) {
        reportError("PortUsage context is null");
        return nullptr;
    }
    
    // 获取使用声明（可选）
    QString name = "UnnamedPort";
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating PortUsage:" << name;
    
    PortUsage* portUsage = new PortUsage();
    portUsage->setDeclaredName(name);
    
    checkDuplicateName(portUsage, name, ctx);
    
    // 处理类型（从特征特殊化部分）
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        validateTypeReference(typeParts, ctx);
                        createUsageTyping(portUsage, typeParts);
                    }
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(portUsage);
    } else {
        if (m_resource) {
            m_resource->addContent(portUsage);
        } else {
            reportError("Resource is null, cannot add PortUsage", ctx);
            delete portUsage;
            return nullptr;
        }
    }
    
    pushParent(portUsage);
    
    // 访问使用完成（包含值部分和使用体）
    auto completionCtx = ctx->usageCompletion();
    if (completionCtx) {
        // 处理 value part（如果有）
        processUsageCompletionValuePart(completionCtx);
        
        auto usageBodyCtx = completionCtx->usageBody();
        if (usageBodyCtx) {
            visitUsageBody(usageBodyCtx);
        }
    }
    
    popParent();
    
    qDebug() << "[ModelBuilderVisitor] visitPortUsage: finished, annotations count =" << portUsage->ownedAnnotations().size();
    return portUsage;
}

std::any ModelBuilderVisitor::visitConnectorUsage(SysML::SysMLParser::ConnectorUsageContext* ctx)
{
    if (!ctx) {
        reportError("ConnectorUsage context is null");
        return nullptr;
    }
    
    // 获取使用声明（可选）
    QString name = "UnnamedConnector";
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ConnectorUsage:" << name;
    
    ConnectorUsage* connectorUsage = new ConnectorUsage();
    connectorUsage->setDeclaredName(name);
    
    checkDuplicateName(connectorUsage, name, ctx);
    
    // 处理类型（从特征特殊化部分）
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        validateTypeReference(typeParts, ctx);
                        createUsageTyping(connectorUsage, typeParts);
                    }
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(connectorUsage);
    } else {
        if (m_resource) {
            m_resource->addContent(connectorUsage);
        } else {
            reportError("Resource is null, cannot add ConnectorUsage", ctx);
            delete connectorUsage;
            return nullptr;
        }
    }
    
    pushParent(connectorUsage);
    
    // 访问使用完成（包含值部分和使用体）
    auto completionCtx = ctx->usageCompletion();
    if (completionCtx) {
        // 处理 value part（如果有）
        processUsageCompletionValuePart(completionCtx);
        
        auto usageBodyCtx = completionCtx->usageBody();
        if (usageBodyCtx) {
            visitUsageBody(usageBodyCtx);
        }
    }
    
    popParent();
    
    return connectorUsage;
}

std::any ModelBuilderVisitor::visitFlowUsage(SysML::SysMLParser::FlowUsageContext* ctx)
{
    if (!ctx) {
        reportError("FlowUsage context is null");
        return nullptr;
    }
    
    // 获取使用声明（可选）
    QString name = "UnnamedFlow";
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating FlowUsage:" << name;
    
    FlowUsage* flowUsage = new FlowUsage();
    flowUsage->setDeclaredName(name);
    
    checkDuplicateName(flowUsage, name, ctx);
    
    // 处理类型（从特征特殊化部分）
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        validateTypeReference(typeParts, ctx);
                        createUsageTyping(flowUsage, typeParts);
                    }
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(flowUsage);
    } else {
        if (m_resource) {
            m_resource->addContent(flowUsage);
        } else {
            reportError("Resource is null, cannot add FlowUsage", ctx);
            delete flowUsage;
            return nullptr;
        }
    }
    
    pushParent(flowUsage);
    
    // 访问使用完成（包含值部分和使用体）
    auto completionCtx = ctx->usageCompletion();
    if (completionCtx) {
        // 处理 value part（如果有）
        processUsageCompletionValuePart(completionCtx);
        
        auto usageBodyCtx = completionCtx->usageBody();
        if (usageBodyCtx) {
            visitUsageBody(usageBodyCtx);
        }
    }
    
    popParent();
    
    return flowUsage;
}

std::any ModelBuilderVisitor::visitDefinitionBody(SysML::SysMLParser::DefinitionBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问所有定义体项
    auto bodyItems = ctx->definitionBodyItem();
    qDebug() << "[ModelBuilderVisitor] visitDefinitionBody: processing" << bodyItems.size() << "body items";
    
    for (auto itemCtx : bodyItems) {
        if (itemCtx) {
            QString itemText = QString::fromStdString(itemCtx->getText());
            qDebug() << "[ModelBuilderVisitor] visitDefinitionBody: processing body item, text:" << itemText;
            
            // 处理定义成员
            auto defMemberCtx = itemCtx->definitionMember();
            if (defMemberCtx) {
                auto defCtx = defMemberCtx->definition();
                if (defCtx) {
                    visitDefinition(defCtx);
                }
            }
            
            // 处理使用成员
            auto usageMemberCtx = itemCtx->usageMember();
            if (usageMemberCtx) {
                auto usageCtx = usageMemberCtx->usage();
                if (usageCtx) {
                    visitUsage(usageCtx);
                }
            }
            
            // 处理属性
            auto attributeCtx = itemCtx->attribute();
            if (attributeCtx) {
                visitAttribute(attributeCtx);
            }
            
            // 处理简化的属性语法
            auto simplifiedAttributeCtx = itemCtx->simplifiedAttribute();
            if (simplifiedAttributeCtx) {
                visitSimplifiedAttribute(simplifiedAttributeCtx);
            }
            
            // 处理导入语句
            auto importCtx = itemCtx->importStatement();
            if (importCtx) {
                visitImportStatement(importCtx);
            }
            
            // 处理关系
            auto relationshipCtx = itemCtx->relationship();
            if (relationshipCtx) {
                visitRelationship(relationshipCtx);
            }
            
            // 处理语句
            auto statementCtx = itemCtx->statement();
            if (statementCtx) {
                visitStatement(statementCtx);
            }
            
            // 处理动作输入输出（in item / out item）
            auto actionInputOutputCtx = itemCtx->actionInputOutput();
            qDebug() << "[ModelBuilderVisitor] visitDefinitionBody: actionInputOutputCtx =" << (actionInputOutputCtx != nullptr);
            if (actionInputOutputCtx) {
                qDebug() << "[ModelBuilderVisitor] visitDefinitionBody: calling visitActionInputOutput";
                visitActionInputOutput(actionInputOutputCtx);
            } else {
                qDebug() << "[ModelBuilderVisitor] visitDefinitionBody: no actionInputOutput found in body item";
            }
            
            // 处理 doc 语句
            auto docCtx = itemCtx->docStatement();
            if (docCtx) {
                visitDocStatement(docCtx);
            }
            
            // 处理 comment 语句
            auto commentCtx = itemCtx->commentStatement();
            if (commentCtx) {
                visitCommentStatement(commentCtx);
            }
            
            // 处理 requirement 特殊成员（subject, actor, stakeholder）
            auto requirementMemberCtx = itemCtx->requirementMember();
            if (requirementMemberCtx) {
                visitRequirementMember(requirementMemberCtx);
            }
            
            // 处理 requirement 约束成员（assume, require）
            auto requirementConstraintMemberCtx = itemCtx->requirementConstraintMember();
            if (requirementConstraintMemberCtx) {
                visitRequirementConstraintMember(requirementConstraintMemberCtx);
            }
            
            // 处理 metadata annotation
            auto metadataAnnotationCtx = itemCtx->metadataAnnotation();
            if (metadataAnnotationCtx) {
                visitMetadataAnnotation(metadataAnnotationCtx);
            }
            
            // 处理 exhibit statement
            auto exhibitCtx = itemCtx->exhibitStatement();
            if (exhibitCtx) {
                qDebug() << "[ModelBuilderVisitor] visitDefinitionBody: found exhibitStatement";
                visitExhibitStatement(exhibitCtx);
            }
            
            // 处理 connection end statement（如果parser支持）
            // 注意：如果生成的parser没有connectionEndStatement()方法，这段代码会导致编译错误
            // 需要根据实际的parser生成结果调整
            // auto connectionEndCtx = itemCtx->connectionEndStatement();
            // if (connectionEndCtx) {
            //     // TODO: 实现 connection end statement 的处理
            //     qDebug() << "[ModelBuilderVisitor] visitDefinitionBody: found connectionEndStatement";
            // }
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitUsageBody(SysML::SysMLParser::UsageBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 优先检查 stateBody（用于 stateUsage）
    auto stateBodyCtx = ctx->stateBody();
    if (stateBodyCtx) {
        return visitStateBody(stateBodyCtx);
    }
    
    // 否则访问定义体（用于其他 usage）
    auto defBodyCtx = ctx->definitionBody();
    if (defBodyCtx) {
        visitDefinitionBody(defBodyCtx);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitPartBody(SysML::SysMLParser::PartBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问所有子元素
    for (auto defCtx : ctx->definition()) {
        if (defCtx) {
            visitDefinition(defCtx);
        }
    }
    
    for (auto usageCtx : ctx->usage()) {
        if (usageCtx) {
            visitUsage(usageCtx);
        }
    }
    
    for (auto attributeCtx : ctx->attribute()) {
        if (attributeCtx) {
            visitAttribute(attributeCtx);
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitAttribute(SysML::SysMLParser::AttributeContext* ctx)
{
    if (!ctx) {
        reportError("Attribute context is null");
        return nullptr;
    }
    
    // 检查是否是 attributeDefinition（attribute 规则可以匹配 attributeDefinition）
    if (ctx->attributeDefinition()) {
        return visitAttributeDefinition(ctx->attributeDefinition());
    }
    
    // 否则是普通的 attribute 使用
    auto contentCtx = ctx->attributeContent();
    if (!contentCtx) {
        reportError("Attribute content is null", ctx);
        return nullptr;
    }
    
    // 先处理特征前缀（如果有）
    Feature* feature = nullptr;
    auto prefixCtxs = ctx->featurePrefix();
    if (!prefixCtxs.empty()) {
        // 先创建特征（从 attributeContent 中获取）
        auto result = visitAttributeContent(contentCtx);
        feature = std::any_cast<Feature*>(result);
        
        if (feature) {
            // 应用前缀
            for (auto* prefixCtx : prefixCtxs) {
                if (prefixCtx) {
                    visitFeaturePrefix(prefixCtx, feature);
                }
            }
        }
        
        return feature;
    }
    
    return visitAttributeContent(contentCtx);
}

std::any ModelBuilderVisitor::visitSimplifiedAttribute(SysML::SysMLParser::SimplifiedAttributeContext* ctx)
{
    if (!ctx) {
        reportError("SimplifiedAttribute context is null");
        return nullptr;
    }
    
    // 获取标识符
    auto identCtx = ctx->identifier();
    if (!identCtx) {
        reportError("SimplifiedAttribute identifier is null", ctx);
        return nullptr;
    }
    
    QString name = getText(identCtx->IDENTIFIER());
    if (name.isEmpty()) {
        reportError("SimplifiedAttribute name is empty", ctx);
        return nullptr;
    }
    
    // 获取类型
    auto typeCtx = ctx->qualifiedName();
    if (!typeCtx) {
        reportError("SimplifiedAttribute qualifiedName is null", ctx);
        return nullptr;
    }
    
    QStringList typeParts = getQualifiedNameParts(typeCtx);
    QString typeName = typeParts.join("::");
    
    qDebug() << "[ModelBuilderVisitor] Creating Feature (from simplifiedAttribute):" << name << ":" << typeName;
    
    // 创建 Feature
    Feature* feature = new Feature();
    feature->setDeclaredName(name);
    feature->setIsSimplifiedAttribute(true); // 标记为简化属性
    
    checkDuplicateName(feature, name, ctx);
    
    // 创建类型化关系
    validateTypeReference(typeParts, ctx);
    createFeatureTyping(feature, typeParts);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(feature);
    } else {
        if (m_resource) {
            m_resource->addContent(feature);
        } else {
            reportError("Resource is null, cannot add Feature", ctx);
            delete feature;
            return nullptr;
        }
    }
    
    return feature;
}

std::any ModelBuilderVisitor::visitAttributeContent(SysML::SysMLParser::AttributeContentContext* ctx)
{
    if (!ctx) {
        reportError("AttributeContent context is null");
        return nullptr;
    }
    
    Feature* feature = nullptr;
    QString name;
    
    // 处理标识符（如果有）
    auto idCtx = ctx->identifier();
    if (idCtx) {
        name = getText(idCtx->IDENTIFIER());
        if (name.isEmpty()) {
            name = "UnnamedAttribute";
        }
        
        qDebug() << "[ModelBuilderVisitor] Creating Feature (from attribute):" << name;
        
        // 创建特征（在元模型中，attribute 语法对应 Feature 类）
        feature = new Feature();
        feature->setDeclaredName(name);
        
        // 检查重复名称
        checkDuplicateName(feature, name, ctx);
    }
    
    // 处理重定义表达式（先检查 redefinitionExpression）
    auto redefCtx = ctx->redefinitionExpression();
    
    // 处理直接形式的 redefines：'redefines' qualifiedName ('=' | ':=') expression
    // 检查是否有 redefines 关键字（通过检查上下文文本）
    QString ctxText = QString::fromStdString(ctx->getText());
    qDebug() << "[ModelBuilderVisitor] visitAttributeContent: ctxText =" << ctxText;
    if (!idCtx && !redefCtx && ctxText.contains("redefines")) {
        // 尝试从上下文中提取 qualifiedName
        // 注意：ANTLR4 可能没有为这种形式生成专门的访问方法，需要手动解析
        // 格式：redefines qualifiedName = expression 或 redefines qualifiedName := expression
        // 注意：ctxText 中 redefines 和 qualifiedName 之间可能没有空格（如 "redefinesMassRequirement::reqId"）
        // 使用正则表达式提取 qualifiedName（支持 :: 分隔的限定名）
        // 匹配：redefines[可选的空格或直接连接]qualifiedName[空格] = 或 :=
        // 注意：ctxText 可能是 "redefinesMassRequirement::reqId=\"1\""（没有空格）
        // 需要匹配：redefines + 标识符（可能直接连接）+ :: + 标识符 + = 或 :=
        QRegularExpression redefinesPattern(R"(redefines\s*([a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*)+)\s*[=:])");
        QRegularExpressionMatch match = redefinesPattern.match(ctxText);
        qDebug() << "[ModelBuilderVisitor] visitAttributeContent: redefines pattern match result:" << match.hasMatch();
        if (match.hasMatch()) {
            QString qualifiedName = match.captured(1);
            qDebug() << "[ModelBuilderVisitor] visitAttributeContent: captured qualifiedName:" << qualifiedName;
            QStringList redefinedParts = qualifiedName.split("::");
            
            // 从限定名称中提取特征名称（最后一个部分）
            if (!redefinedParts.isEmpty()) {
                name = redefinedParts.last();
            } else {
                name = "UnnamedAttribute";
            }
            
            qDebug() << "[ModelBuilderVisitor] Creating Feature (redefines direct form):" << name << "from" << qualifiedName;
            
            feature = new Feature();
            feature->setDeclaredName(name);
            checkDuplicateName(feature, name, ctx);
            
            // 创建重定义关系
            createRedefinition(feature, redefinedParts);
            
            // 处理值表达式
            // 首先检查 valuePart 是否存在
            auto valuePartCtx = ctx->valuePart();
            qDebug() << "[ModelBuilderVisitor] visitAttributeContent: valuePartCtx exists:" << (valuePartCtx != nullptr);
            if (!valuePartCtx) {
                // valuePart 不存在，尝试从 ctxText 中手动提取值表达式
                // 例如：redefinesMassRequirement::reqId="1" 或 redefinesMassRequirement::massRequired=2000[kg]
                int equalPos = ctxText.indexOf('=', ctxText.indexOf("redefines"));
                if (equalPos > 0) {
                    QString valueExpr = ctxText.mid(equalPos + 1).trimmed();
                    // 去掉末尾的分号（如果有）
                    if (valueExpr.endsWith(';')) {
                        valueExpr = valueExpr.left(valueExpr.length() - 1).trimmed();
                    }
                    if (!valueExpr.isEmpty()) {
                        feature->setValueExpression(valueExpr);
                        qDebug() << "[ModelBuilderVisitor] Set value expression on redefines feature (from ctxText):" << valueExpr;
                        
                        // 提取单位表达式（如果有）
                        if (valueExpr.contains('[') && valueExpr.contains(']')) {
                            int bracketStart = valueExpr.indexOf('[');
                            int bracketEnd = valueExpr.lastIndexOf(']');
                            if (bracketStart >= 0 && bracketEnd > bracketStart) {
                                QString unitText = valueExpr.mid(bracketStart + 1, bracketEnd - bracketStart - 1).trimmed();
                                if (!unitText.isEmpty()) {
                                    feature->setUnitExpression(unitText);
                                    qDebug() << "[ModelBuilderVisitor] Set unit expression on redefines feature:" << unitText;
                                }
                            }
                        }
                    }
                }
            } else {
                qDebug() << "[ModelBuilderVisitor] visitAttributeContent: valuePart exists, will be processed later";
            }
        } else {
            qDebug() << "[ModelBuilderVisitor] visitAttributeContent: redefines pattern not matched for:" << ctxText;
            qDebug() << "[ModelBuilderVisitor] visitAttributeContent: idCtx=" << (idCtx ? "exists" : "null") 
                     << "redefCtx=" << (redefCtx ? "exists" : "null");
            // 尝试另一种模式：redefines 后面直接跟标识符（没有空格）
            // 例如：redefinesMassRequirement::reqId
            QRegularExpression redefinesPattern2(R"(redefines([a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*)+)\s*[=:])");
            QRegularExpressionMatch match2 = redefinesPattern2.match(ctxText);
            if (match2.hasMatch()) {
                QString qualifiedName = match2.captured(1);
                qDebug() << "[ModelBuilderVisitor] visitAttributeContent: pattern2 matched, qualifiedName:" << qualifiedName;
                QStringList redefinedParts = qualifiedName.split("::");
                
                if (!redefinedParts.isEmpty()) {
                    name = redefinedParts.last();
                } else {
                    name = "UnnamedAttribute";
                }
                
                qDebug() << "[ModelBuilderVisitor] Creating Feature (redefines direct form, pattern2):" << name << "from" << qualifiedName;
                
                feature = new Feature();
                feature->setDeclaredName(name);
                checkDuplicateName(feature, name, ctx);
                
                createRedefinition(feature, redefinedParts);
                
                // 同样处理值表达式（如果 valuePart 不存在）
                auto valuePartCtx2 = ctx->valuePart();
                if (!valuePartCtx2) {
                    int equalPos = ctxText.indexOf('=', ctxText.indexOf("redefines"));
                    if (equalPos > 0) {
                        QString valueExpr = ctxText.mid(equalPos + 1).trimmed();
                        if (valueExpr.endsWith(';')) {
                            valueExpr = valueExpr.left(valueExpr.length() - 1).trimmed();
                        }
                        if (!valueExpr.isEmpty()) {
                            feature->setValueExpression(valueExpr);
                            qDebug() << "[ModelBuilderVisitor] Set value expression on redefines feature (pattern2, from ctxText):" << valueExpr;
                            
                            // 提取单位表达式（如果有）
                            if (valueExpr.contains('[') && valueExpr.contains(']')) {
                                int bracketStart = valueExpr.indexOf('[');
                                int bracketEnd = valueExpr.lastIndexOf(']');
                                if (bracketStart >= 0 && bracketEnd > bracketStart) {
                                    QString unitText = valueExpr.mid(bracketStart + 1, bracketEnd - bracketStart - 1).trimmed();
                                    if (!unitText.isEmpty()) {
                                        feature->setUnitExpression(unitText);
                                        qDebug() << "[ModelBuilderVisitor] Set unit expression on redefines feature (pattern2):" << unitText;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 处理重定义表达式（如果存在，且没有标识符，且不是直接形式）
    if (!feature && redefCtx && !idCtx) {
        // 重定义但没有标识符的情况：attribute redefines Type::feature
        QStringList redefinedParts = getQualifiedNameParts(redefCtx->qualifiedName());
        
        // 从限定名称中提取特征名称（最后一个部分）
        if (!redefinedParts.isEmpty()) {
            name = redefinedParts.last();
        } else {
            name = "UnnamedAttribute";
        }
        
        qDebug() << "[ModelBuilderVisitor] Creating Feature (redefinition without identifier):" << name;
        
        feature = new Feature();
        feature->setDeclaredName(name);
        checkDuplicateName(feature, name, ctx);
        
        // 创建重定义关系
        createRedefinition(feature, redefinedParts);
    }
    
    // 如果没有创建 feature，说明语法错误
    if (!feature) {
        reportError("Attribute must have identifier or redefinition expression", ctx);
        return nullptr;
    }
    
    // 处理属性表达式（子集化、重定义、类型化）
    auto exprCtx = ctx->attributeExpressions();
    if (exprCtx) {
        // 处理类型化表达式（: Type）
        auto typingCtx = exprCtx->typingExpression();
        bool hasTyping = false;
        if (typingCtx) {
            auto nameCtx = typingCtx->qualifiedName();
            if (nameCtx) {
                QStringList nameParts = getQualifiedNameParts(nameCtx);
                createFeatureTyping(feature, nameParts);
                hasTyping = true;
            }
        }
        
        // 处理子集化表达式
        auto subsetCtx = exprCtx->subsettingExpression();
        if (subsetCtx) {
            auto nameCtx = subsetCtx->qualifiedName();
            if (nameCtx) {
                QStringList nameParts = getQualifiedNameParts(nameCtx);
                // 在 attribute 中，如果没有 typingExpression，:> 表示类型化而不是子集化
                if (!hasTyping) {
                    // 将 :> 作为类型化处理
                    qDebug() << "[ModelBuilderVisitor] Processing :> as typing for attribute:" << feature->declaredName() << "->" << nameParts.join("::");
                    createFeatureTyping(feature, nameParts);
                } else {
                    // 如果已有类型化，则 :> 作为子集化处理
                    createSubsetting(feature, nameParts);
                }
            }
        }
        
        // 处理重定义表达式（如果还没有处理）
        if (!redefCtx || idCtx) {
            auto redefExprCtx = exprCtx->redefinitionExpression();
            if (redefExprCtx) {
                auto nameCtx = redefExprCtx->qualifiedName();
                if (nameCtx) {
                    QStringList nameParts = getQualifiedNameParts(nameCtx);
                    createRedefinition(feature, nameParts);
                }
            }
        }
    }
    
        // 处理 value part（如果有）
        auto valuePartCtx = ctx->valuePart();
        if (valuePartCtx) {
            auto featureValueCtx = valuePartCtx->featureValue();
            if (featureValueCtx) {
                auto exprCtx = featureValueCtx->expression();
                if (exprCtx) {
                    // 临时保存当前 feature，以便在访问表达式时设置单位表达式
                    Feature* currentFeature = feature;
                    
                    // 使用表达式访问方法来解析表达式结构，包括单位表达式
                    auto result = visitExpression(exprCtx);
                    QString valueText;
                    if (result.has_value()) {
                        valueText = std::any_cast<QString>(result);
                    } else {
                        valueText = QString::fromStdString(exprCtx->getText());
                    }
                    
                    // 保存完整的值表达式（包括单位）
                    QString exprText = QString::fromStdString(exprCtx->getText());
                    if (currentFeature) {
                        currentFeature->setValueExpression(exprText);
                        qDebug() << "[ModelBuilderVisitor] Set value expression on feature:" << exprText;
                    }
                    
                    // 从表达式文本中提取单位表达式
                    if (exprText.contains('[') && exprText.contains(']')) {
                        int bracketStart = exprText.indexOf('[');
                        int bracketEnd = exprText.lastIndexOf(']');
                        if (bracketStart >= 0 && bracketEnd > bracketStart) {
                            QString unitText = exprText.mid(bracketStart + 1, bracketEnd - bracketStart - 1).trimmed();
                            if (!unitText.isEmpty() && currentFeature) {
                                currentFeature->setUnitExpression(unitText);
                                qDebug() << "[ModelBuilderVisitor] Set unit expression on feature:" << unitText;
                            }
                        }
                        qDebug() << "[ModelBuilderVisitor] Attribute value with units:" << exprText;
                    } else {
                        qDebug() << "[ModelBuilderVisitor] Attribute value:" << valueText;
                    }
                }
            }
        }
    
    // 添加到父元素
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(feature);
    } else {
        reportError("No parent element for Attribute", ctx);
        delete feature;
        return nullptr;
    }
    
    return feature;
}

std::any ModelBuilderVisitor::visitAttributeExpressions(SysML::SysMLParser::AttributeExpressionsContext* ctx)
{
    // 这个方法现在由 visitAttributeContent 直接处理
    // 保留此方法以符合访问者接口，但实际处理在 visitAttributeContent 中
    Q_UNUSED(ctx);
    return nullptr;
}

std::any ModelBuilderVisitor::visitSubsettingExpression(SysML::SysMLParser::SubsettingExpressionContext* ctx)
{
    // 这个方法现在由 visitAttributeContent 直接处理
    Q_UNUSED(ctx);
    return nullptr;
}

std::any ModelBuilderVisitor::visitRedefinitionExpression(SysML::SysMLParser::RedefinitionExpressionContext* ctx)
{
    // 这个方法现在由 visitAttributeContent 直接处理
    Q_UNUSED(ctx);
    return nullptr;
}

std::any ModelBuilderVisitor::visitTypingExpression(SysML::SysMLParser::TypingExpressionContext* ctx)
{
    // 这个方法现在由 visitAttributeContent 直接处理
    Q_UNUSED(ctx);
    return nullptr;
}

std::any ModelBuilderVisitor::visitImportStatement(SysML::SysMLParser::ImportStatementContext* ctx)
{
    if (!ctx) {
        reportError("ImportStatement context is null");
        return nullptr;
    }
    
    // 处理成员导入或命名空间导入
    auto membershipImportCtx = ctx->membershipImport();
    auto namespaceImportCtx = ctx->namespaceImport();
    
    // 获取导入前缀（可见性、all 关键字）
    NamespaceImport::Visibility visibility = NamespaceImport::VisibilityNone;
    bool isAll = false;
    bool isWildcard = false;
    bool isRecursive = false;
    
    if (membershipImportCtx) {
        // 成员导入的前缀
        auto importPrefixCtx = membershipImportCtx->importPrefix();
        if (importPrefixCtx) {
            auto visibilityCtx = importPrefixCtx->visibilityIndicator();
            if (visibilityCtx) {
                QString visText = QString::fromStdString(visibilityCtx->getText());
                if (visText == "public") {
                    visibility = NamespaceImport::VisibilityPublic;
                } else if (visText == "private") {
                    visibility = NamespaceImport::VisibilityPrivate;
                } else if (visText == "protected") {
                    visibility = NamespaceImport::VisibilityProtected;
                }
            }
            
            // 检查 all 关键字
            QString prefixText = QString::fromStdString(importPrefixCtx->getText());
            if (prefixText.contains("all")) {
                isAll = true;
            }
        }
        
        // 检查通配符 ::** (递归)
        auto importedMembershipCtx = membershipImportCtx->importedMembership();
        if (importedMembershipCtx) {
            QString membershipText = QString::fromStdString(importedMembershipCtx->getText());
            if (membershipText.contains("::**")) {
                isRecursive = true;
            }
        }
    } else if (namespaceImportCtx) {
        // 命名空间导入的前缀
        auto importPrefixCtx = namespaceImportCtx->importPrefix();
        if (importPrefixCtx) {
            auto visibilityCtx = importPrefixCtx->visibilityIndicator();
            if (visibilityCtx) {
                QString visText = QString::fromStdString(visibilityCtx->getText());
                if (visText == "public") {
                    visibility = NamespaceImport::VisibilityPublic;
                } else if (visText == "private") {
                    visibility = NamespaceImport::VisibilityPrivate;
                } else if (visText == "protected") {
                    visibility = NamespaceImport::VisibilityProtected;
                }
            }
            
            // 检查 all 关键字
            QString prefixText = QString::fromStdString(importPrefixCtx->getText());
            if (prefixText.contains("all")) {
                isAll = true;
            }
        }
        
        // 检查通配符
        auto importedNamespaceCtx = namespaceImportCtx->importedNamespace();
        if (importedNamespaceCtx) {
            QString namespaceText = QString::fromStdString(importedNamespaceCtx->getText());
            if (namespaceText.contains("::**")) {
                isRecursive = true;
            } else if (namespaceText.contains("::*")) {
                isWildcard = true;
            }
        } else {
            // 过滤包（filterPackage）总是通配符
            auto filterPackageCtx = namespaceImportCtx->filterPackage();
            if (filterPackageCtx) {
                isWildcard = true;
            }
        }
    }
    
    QStringList nameParts;
    QString namespaceName;
    
    if (membershipImportCtx) {
        // 成员导入
        auto importedMembershipCtx = membershipImportCtx->importedMembership();
        if (importedMembershipCtx) {
            auto qnameCtx = importedMembershipCtx->qualifiedName();
            if (qnameCtx) {
                nameParts = getQualifiedNameParts(qnameCtx);
                namespaceName = nameParts.join("::");
            }
        }
    } else if (namespaceImportCtx) {
        // 命名空间导入
        auto importedNamespaceCtx = namespaceImportCtx->importedNamespace();
        if (importedNamespaceCtx) {
            auto qnameCtx = importedNamespaceCtx->qualifiedName();
            if (qnameCtx) {
                nameParts = getQualifiedNameParts(qnameCtx);
                namespaceName = nameParts.join("::");
            }
        } else {
            // 过滤包
            auto filterPackageCtx = namespaceImportCtx->filterPackage();
            if (filterPackageCtx) {
                auto qnameCtx = filterPackageCtx->qualifiedName();
                if (qnameCtx) {
                    nameParts = getQualifiedNameParts(qnameCtx);
                    namespaceName = nameParts.join("::");
                }
            }
        }
    }
    
    if (nameParts.isEmpty()) {
        reportError("ImportStatement: cannot determine imported name", ctx);
        return nullptr;
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating import:" << namespaceName;
    
    // 创建命名空间导入
    NamespaceImport* import = new NamespaceImport();
    
    // 保存原始命名空间名称（即使找不到命名空间，也要保存名称用于序列化）
    import->setImportedNamespaceName(namespaceName);
    
    // 设置导入前缀
    if (visibility != NamespaceImport::VisibilityNone) {
        import->setVisibility(visibility);
    }
    if (isAll) {
        import->setIsAll(true);
    }
    if (isWildcard) {
        import->setIsWildcard(true);
    }
    if (isRecursive) {
        import->setIsRecursive(true);
    }
    
    // 设置导入的命名空间引用
    setImportedNamespace(import, nameParts);
    
    // 添加到当前父元素
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedRelationship(import);
    } else {
        reportError("No parent element for Import", ctx);
        delete import;
        return nullptr;
    }
    
    return import;
}

std::any ModelBuilderVisitor::visitQualifiedName(SysML::SysMLParser::QualifiedNameContext* ctx)
{
    // 这个方法通常不需要实现，因为 qualifiedName 是作为其他规则的子规则被访问的
    // 但为了完整性，我们调用默认行为
    return visitChildren(ctx);
}

// 辅助方法实现
QString ModelBuilderVisitor::getText(antlr4::tree::TerminalNode* node) const
{
    if (!node) {
        return QString();
    }
    return QString::fromStdString(node->getText());
}

QStringList ModelBuilderVisitor::getQualifiedNameParts(SysML::SysMLParser::QualifiedNameContext* ctx)
{
    QStringList parts;
    if (!ctx) {
        return parts;
    }
    
    for (auto idCtx : ctx->identifier()) {
        if (idCtx && idCtx->IDENTIFIER()) {
            parts.append(getText(idCtx->IDENTIFIER()));
        }
    }
    
    return parts;
}

Element* ModelBuilderVisitor::getCurrentParent()
{
    return m_parentStack.isEmpty() ? nullptr : m_parentStack.top();
}

void ModelBuilderVisitor::pushParent(Element* element)
{
    m_parentStack.push(element);
}

void ModelBuilderVisitor::popParent()
{
    if (!m_parentStack.isEmpty()) {
        m_parentStack.pop();
    }
}

void ModelBuilderVisitor::reportError(const QString& message, antlr4::ParserRuleContext* ctx)
{
    QString errorMsg = formatError(message, ctx, "ERROR");
    m_errors.append(errorMsg);
    qWarning() << "[ModelBuilderVisitor]" << errorMsg;
}

void ModelBuilderVisitor::reportWarning(const QString& message, antlr4::ParserRuleContext* ctx)
{
    QString warningMsg = formatError(message, ctx, "WARNING");
    m_warnings.append(warningMsg);
    qDebug() << "[ModelBuilderVisitor]" << warningMsg;
}

void ModelBuilderVisitor::processUsageCompletionValuePart(SysML::SysMLParser::UsageCompletionContext* completionCtx)
{
    if (!completionCtx) {
        return;
    }
    
    // 处理 value part（如果有）
    auto valuePartCtx = completionCtx->valuePart();
    if (valuePartCtx) {
        auto featureValueCtx = valuePartCtx->featureValue();
        if (featureValueCtx) {
            auto exprCtx = featureValueCtx->expression();
            if (exprCtx) {
                // 使用表达式访问方法来解析表达式结构，包括单位表达式
                auto result = visitExpression(exprCtx);
                QString valueText;
                if (result.has_value()) {
                    valueText = std::any_cast<QString>(result);
                } else {
                    valueText = QString::fromStdString(exprCtx->getText());
                }
                
                // 检查是否有单位表达式
                QString exprText = QString::fromStdString(exprCtx->getText());
                if (exprText.contains('[') && exprText.contains(']')) {
                    qDebug() << "[ModelBuilderVisitor] Usage value with units:" << exprText;
                } else {
                    qDebug() << "[ModelBuilderVisitor] Usage value:" << valueText;
                }
            }
        }
    }
}

QString ModelBuilderVisitor::formatError(const QString& message, antlr4::ParserRuleContext* ctx, const QString& severity)
{
    QString formatted = QString("[%1]").arg(severity);
    
    if (ctx) {
        int line = ctx->getStart() ? ctx->getStart()->getLine() : -1;
        int column = ctx->getStart() ? ctx->getStart()->getCharPositionInLine() : -1;
        if (line >= 0) {
            formatted += QString(" Line %1").arg(line);
            if (column >= 0) {
                formatted += QString(", Column %1").arg(column);
            }
            formatted += ":";
        }
    }
    
    formatted += " " + message;
    return formatted;
}

bool ModelBuilderVisitor::checkDuplicateName(Element* element, const QString& name, antlr4::ParserRuleContext* ctx)
{
    if (!element || name.isEmpty()) {
        return false;
    }
    
    // 检查当前父命名空间中是否有同名元素
    Element* parent = getCurrentParent();
    if (parent) {
        Namespace* ns = dynamic_cast<Namespace*>(parent);
        if (ns) {
            Element* existing = ns->findElement(name);
            if (existing && existing != element) {
                reportError(QString("Duplicate element name '%1' in namespace '%2'")
                           .arg(name)
                           .arg(parent->declaredName()), ctx);
                return true;
            }
        }
    } else if (m_resource) {
        // 检查资源顶层是否有同名元素
        for (Element* existing : m_resource->contents()) {
            if (existing != element && existing->declaredName() == name) {
                reportError(QString("Duplicate top-level element name '%1'").arg(name), ctx);
                return true;
            }
        }
    }
    
    return false;
}

void ModelBuilderVisitor::validateTypeReference(const QStringList& typeNameParts, antlr4::ParserRuleContext* ctx)
{
    if (typeNameParts.isEmpty()) {
        reportError("Empty type name", ctx);
        return;
    }
    
    // 检查类型名称的格式
    for (const QString& part : typeNameParts) {
        if (part.isEmpty()) {
            reportError("Type name contains empty part", ctx);
            return;
        }
        
        // 检查标识符格式（应该只包含字母、数字、下划线）
        for (QChar ch : part) {
            if (!ch.isLetterOrNumber() && ch != '_') {
                reportWarning(QString("Type name part '%1' contains invalid character '%2'")
                            .arg(part).arg(ch), ctx);
            }
        }
    }
}

// 查找方法实现
Element* ModelBuilderVisitor::findElementByQualifiedName(const QStringList& nameParts)
{
    if (nameParts.isEmpty()) {
        return nullptr;
    }
    
    // 从当前父元素开始查找
    Element* currentScope = getCurrentParent();
    if (!currentScope) {
        // 如果没有父元素，从资源开始查找
        if (!m_resource) {
            return nullptr;
        }
        // 在资源的所有内容中查找第一个匹配的元素
        for (Element* elem : m_resource->contents()) {
            if (elem->declaredName() == nameParts.first()) {
                currentScope = elem;
                break;
            }
        }
        if (!currentScope) {
            return nullptr;
        }
    }
    
    // 如果只有一个部分，直接在当前作用域查找
    if (nameParts.size() == 1) {
        if (auto ns = dynamic_cast<Namespace*>(currentScope)) {
            return ns->findElement(nameParts.first());
        }
        return nullptr;
    }
    
    // 多个部分：递归查找
    Element* found = currentScope;
    for (int i = 0; i < nameParts.size(); ++i) {
        if (!found) {
            break;
        }
        
        Namespace* ns = dynamic_cast<Namespace*>(found);
        if (!ns) {
            return nullptr;
        }
        
        if (i < nameParts.size() - 1) {
            // 中间部分：查找命名空间
            found = ns->findElement(nameParts[i]);
            if (!found) {
                return nullptr;
            }
        } else {
            // 最后部分：查找目标元素
            return ns->findElement(nameParts[i]);
        }
    }
    
    return found;
}

Element* ModelBuilderVisitor::findElementByQualifiedNameFromScope(const QStringList& nameParts, Element* startScope)
{
    if (nameParts.isEmpty()) {
        return nullptr;
    }
    
    // 从指定的起始范围开始查找
    Element* currentScope = startScope;
    if (!currentScope) {
        // 如果没有指定范围，回退到当前父元素
        currentScope = getCurrentParent();
        if (!currentScope) {
            // 如果仍然没有，从资源开始查找
            if (!m_resource) {
                return nullptr;
            }
            // 在资源的所有内容中查找第一个匹配的元素
            for (Element* elem : m_resource->contents()) {
                if (elem->declaredName() == nameParts.first()) {
                    currentScope = elem;
                    break;
                }
            }
            if (!currentScope) {
                return nullptr;
            }
        }
    }
    
    // 如果只有一个部分，直接在当前作用域查找
    if (nameParts.size() == 1) {
        if (auto ns = dynamic_cast<Namespace*>(currentScope)) {
            Element* found = ns->findElement(nameParts.first());
            qDebug() << "[ModelBuilderVisitor] findElementByQualifiedNameFromScope: single part"
                     << nameParts.first() << "in scope" << currentScope->declaredName()
                     << "found:" << (found ? found->declaredName() : "null");
            return found;
        }
        return nullptr;
    }
    
    // 多个部分：递归查找
    Element* found = currentScope;
    qDebug() << "[ModelBuilderVisitor] findElementByQualifiedNameFromScope: multi-part lookup"
             << nameParts.join("::") << "starting from" << (currentScope ? currentScope->declaredName() : "null");
    
    for (int i = 0; i < nameParts.size(); ++i) {
        if (!found) {
            qDebug() << "[ModelBuilderVisitor] findElementByQualifiedNameFromScope: found is null at index" << i;
            break;
        }
        
        Namespace* ns = dynamic_cast<Namespace*>(found);
        if (!ns) {
            qDebug() << "[ModelBuilderVisitor] findElementByQualifiedNameFromScope: found element is not a Namespace"
                     << found->declaredName() << "at index" << i;
            return nullptr;
        }
        
        if (i < nameParts.size() - 1) {
            // 中间部分：查找命名空间
            found = ns->findElement(nameParts[i]);
            Element* nsElement = dynamic_cast<Element*>(ns);
            qDebug() << "[ModelBuilderVisitor] findElementByQualifiedNameFromScope: looking for namespace"
                     << nameParts[i] << "in" << (nsElement ? nsElement->declaredName() : "unknown") << "found:" << (found ? found->declaredName() : "null");
            if (!found) {
                return nullptr;
            }
        } else {
            // 最后部分：查找目标元素
            Element* result = ns->findElement(nameParts[i]);
            Element* nsElement = dynamic_cast<Element*>(ns);
            qDebug() << "[ModelBuilderVisitor] findElementByQualifiedNameFromScope: looking for element"
                     << nameParts[i] << "in" << (nsElement ? nsElement->declaredName() : "unknown") << "found:" << (result ? result->declaredName() : "null");
            return result;
        }
    }
    
    return found;
}

Type* ModelBuilderVisitor::findType(const QStringList& nameParts)
{
    Element* element = findElementByQualifiedName(nameParts);
    if (element) {
        Type* type = dynamic_cast<Type*>(element);
        if (type) {
            return type;
        }
    }
    
    // 如果找不到，记录警告（但不报错，因为类型可能在其他资源中）
    QString typeName = nameParts.join("::");
    qWarning() << "[ModelBuilderVisitor] Type not found:" << typeName;
    return nullptr;
}

Namespace* ModelBuilderVisitor::findNamespace(const QStringList& nameParts)
{
    Element* element = findElementByQualifiedName(nameParts);
    if (element) {
        Namespace* ns = dynamic_cast<Namespace*>(element);
        if (ns) {
            return ns;
        }
    }
    
    // 如果找不到，记录警告（但不报错，因为命名空间可能在其他资源中）
    QString namespaceName = nameParts.join("::");
    qWarning() << "[ModelBuilderVisitor] Namespace not found:" << namespaceName;
    return nullptr;
}

void ModelBuilderVisitor::createFeatureTyping(Feature* feature, const QStringList& typeNameParts)
{
    if (!feature || typeNameParts.isEmpty()) {
        return;
    }
    
    // 验证类型引用格式
    validateTypeReference(typeNameParts, nullptr);
    
    // 保存类型名称（即使找不到类型，也要保存名称用于序列化）
    QString typeName = typeNameParts.join("::");
    
    // 查找类型
    Type* type = findType(typeNameParts);
    if (!type) {
        // 这是一个警告而不是错误，因为类型可能在其他资源中定义
        reportWarning(QString("Type not found: %1 (may be defined in another resource)")
                     .arg(typeName));
        // 即使找不到类型，也创建关系并保存类型名称
    }
    
    // 创建 FeatureTyping 关系
    FeatureTyping* featureTyping = new FeatureTyping();
    featureTyping->setTypedFeature(feature);
    if (type) {
        featureTyping->setType(type);
    }
    // 保存类型名称字符串（用于序列化）
    featureTyping->setTypeName(typeName);
    
    // FeatureTyping 关系应该添加到被类型化的 Feature 的 ownedRelationships 中
    // 这是 SysML 元模型的标准设计：关系属于被类型化的元素
    feature->addOwnedRelationship(featureTyping);
    
    if (type) {
        qDebug() << "[ModelBuilderVisitor] Created FeatureTyping:"
                 << feature->declaredName() << "->" << type->declaredName();
    } else {
        qDebug() << "[ModelBuilderVisitor] Created FeatureTyping (type not found):"
                 << feature->declaredName() << "->" << typeName;
    }
}

void ModelBuilderVisitor::createPartUsageTyping(PartUsage* partUsage, const QStringList& typeNameParts)
{
    if (!partUsage || typeNameParts.isEmpty()) {
        return;
    }
    
    // 验证类型引用格式
    validateTypeReference(typeNameParts, nullptr);
    
    // 保存类型名称（即使找不到类型，也要保存名称用于序列化）
    QString typeName = typeNameParts.join("::");
    partUsage->setTypeName(typeName);
    
    // 查找类型
    Type* type = findType(typeNameParts);
    if (!type) {
        // 这是一个警告而不是错误，因为类型可能在其他资源中定义
        reportWarning(QString("Type not found for PartUsage: %1 (may be defined in another resource)")
                     .arg(typeName));
        // 即使找不到类型，也保存类型名称用于序列化
        return;
    }
    
    // 注意：在 SysML v2 中，PartUsage 应该是一种 Feature
    // 但在当前实现中，PartUsage 继承自 Type 而不是 Feature
    // 为了支持类型化，我们需要检查 PartUsage 是否可以被视为 Feature
    // 或者使用其他关系类型
    
    // 尝试将 PartUsage 转换为 Feature
    // 如果 PartUsage 在元模型中应该继承自 Feature，这里应该能转换成功
    Feature* feature = qobject_cast<Feature*>(partUsage);
    if (feature) {
        // 如果可以转换，使用 FeatureTyping
        createFeatureTyping(feature, typeNameParts);
        if (type) {
            qDebug() << "[ModelBuilderVisitor] Created PartUsage typing via FeatureTyping:"
                     << partUsage->declaredName() << "->" << type->declaredName();
        } else {
            qDebug() << "[ModelBuilderVisitor] Created PartUsage typing via FeatureTyping (type not found):"
                     << partUsage->declaredName() << "->" << typeName;
        }
    } else {
        // 如果不能转换，说明 PartUsage 不是 Feature
        // 类型名称已经保存在 partUsage->setTypeName() 中，用于序列化
        if (type) {
            qDebug() << "[ModelBuilderVisitor] PartUsage type saved:"
                     << partUsage->declaredName() << "->" << type->declaredName();
        } else {
            qDebug() << "[ModelBuilderVisitor] PartUsage type name saved:"
                     << partUsage->declaredName() << "->" << typeName;
        }
    }
}

void ModelBuilderVisitor::createUsageTyping(Type* usage, const QStringList& typeNameParts)
{
    if (!usage || typeNameParts.isEmpty()) {
        return;
    }
    
    // 验证类型引用格式
    validateTypeReference(typeNameParts, nullptr);
    
    // 保存类型名称（即使找不到类型，也要保存名称用于序列化）
    QString typeName = typeNameParts.join("::");
    
    // 根据使用类型设置类型名称
    if (auto* actionUsage = qobject_cast<ActionUsage*>(usage)) {
        actionUsage->setTypeName(typeName);
    } else if (auto* stateUsage = qobject_cast<StateUsage*>(usage)) {
        stateUsage->setTypeName(typeName);
    } else if (auto* requirementUsage = qobject_cast<RequirementUsage*>(usage)) {
        requirementUsage->setTypeName(typeName);
    } else if (auto* portUsage = qobject_cast<PortUsage*>(usage)) {
        portUsage->setTypeName(typeName);
    } else if (auto* connectorUsage = qobject_cast<ConnectorUsage*>(usage)) {
        connectorUsage->setTypeName(typeName);
    } else if (auto* flowUsage = qobject_cast<FlowUsage*>(usage)) {
        flowUsage->setTypeName(typeName);
    }
    
    // 查找类型
    Type* type = findType(typeNameParts);
    if (!type) {
        // 这是一个警告而不是错误，因为类型可能在其他资源中定义
        reportWarning(QString("Type not found for Usage: %1 (may be defined in another resource)")
                     .arg(typeName));
        return;
    }
    
    // 尝试将 Usage 转换为 Feature
    Feature* feature = qobject_cast<Feature*>(usage);
    if (feature) {
        // 如果可以转换，使用 FeatureTyping
        createFeatureTyping(feature, typeNameParts);
        if (type) {
            qDebug() << "[ModelBuilderVisitor] Created Usage typing via FeatureTyping:"
                     << usage->declaredName() << "->" << type->declaredName();
        }
    } else {
        // 如果不能转换，类型名称已经保存在 usage 中，用于序列化
        if (type) {
            qDebug() << "[ModelBuilderVisitor] Usage type saved:"
                     << usage->declaredName() << "->" << type->declaredName();
        } else {
            qDebug() << "[ModelBuilderVisitor] Usage type name saved:"
                     << usage->declaredName() << "->" << typeName;
        }
    }
}

void ModelBuilderVisitor::setImportedNamespace(NamespaceImport* import, const QStringList& namespaceNameParts)
{
    if (!import || namespaceNameParts.isEmpty()) {
        return;
    }
    
    // 验证命名空间引用格式
    validateTypeReference(namespaceNameParts, nullptr);
    
    // 查找命名空间
    Namespace* namespace_ = findNamespace(namespaceNameParts);
    if (!namespace_) {
        QString namespaceName = namespaceNameParts.join("::");
        // 这是一个警告而不是错误，因为命名空间可能在其他资源中定义
        reportWarning(QString("Imported namespace not found: %1 (may be defined in another resource)")
                     .arg(namespaceName));
        // 注意：这里不返回，因为导入可能引用外部资源中的命名空间
        // 在后续阶段（如链接阶段）可以解析这些引用
        return;
    }
    
    // 设置导入的命名空间
    import->setImportedNamespace(namespace_);
    
    // 获取命名空间的名称（Namespace 是接口，需要通过 Element 获取）
    Element* nsElement = dynamic_cast<Element*>(namespace_);
    if (nsElement) {
        qDebug() << "[ModelBuilderVisitor] Set imported namespace:"
                 << nsElement->declaredName();
    } else {
        qDebug() << "[ModelBuilderVisitor] Set imported namespace (name unavailable)";
    }
}

// ============================================
// 新的定义类型访问方法
// ============================================

std::any ModelBuilderVisitor::visitDefinition(SysML::SysMLParser::DefinitionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 根据定义类型分发到相应的访问方法
    if (ctx->partDefinition()) {
        return visitPartDefinition(ctx->partDefinition());
    } else if (ctx->calculationDefinition()) {
        return visitCalculationDefinition(ctx->calculationDefinition());
    } else if (ctx->actionDefinition()) {
        return visitActionDefinition(ctx->actionDefinition());
    } else if (ctx->stateDefinition()) {
        return visitStateDefinition(ctx->stateDefinition());
    } else if (ctx->requirementDefinition()) {
        return visitRequirementDefinition(ctx->requirementDefinition());
    } else if (ctx->constraintDefinition()) {
        return visitConstraintDefinition(ctx->constraintDefinition());
    } else if (ctx->portDefinition()) {
        return visitPortDefinition(ctx->portDefinition());
    } else if (ctx->connectorDefinition()) {
        return visitConnectorDefinition(ctx->connectorDefinition());
    } else if (ctx->interfaceDefinition()) {
        return visitInterfaceDefinition(ctx->interfaceDefinition());
    } else if (ctx->datatypeDefinition()) {
        return visitDatatypeDefinition(ctx->datatypeDefinition());
    } else if (ctx->classDefinition()) {
        return visitClassDefinition(ctx->classDefinition());
    } else if (ctx->enumDefinition()) {
        return visitEnumDefinition(ctx->enumDefinition());
    } else if (ctx->attributeDefinition()) {
        return visitAttributeDefinition(ctx->attributeDefinition());
    } else if (ctx->connectionDefinition()) {
        return visitConnectionDefinition(ctx->connectionDefinition());
    } else if (ctx->allocationDefinition()) {
        return visitAllocationDefinition(ctx->allocationDefinition());
    } else if (ctx->itemDefinition()) {
        return visitItemDefinition(ctx->itemDefinition());
    } else if (ctx->flowDefinition()) {
        return visitFlowDefinition(ctx->flowDefinition());
    } else if (ctx->viewpointDefinition()) {
        return visitViewpointDefinition(ctx->viewpointDefinition());
    } else if (ctx->viewDefinition()) {
        return visitViewDefinition(ctx->viewDefinition());
    } else if (ctx->concernDefinition()) {
        return visitConcernDefinition(ctx->concernDefinition());
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitUsage(SysML::SysMLParser::UsageContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 根据使用类型分发到相应的访问方法
    if (ctx->partUsage()) {
        return visitPartUsage(ctx->partUsage());
    } else if (ctx->analysisUsage()) {
        return visitAnalysisUsage(ctx->analysisUsage());
    } else if (ctx->renderingUsage()) {
        return visitRenderingUsage(ctx->renderingUsage());
    } else if (ctx->actionUsage()) {
        return visitActionUsage(ctx->actionUsage());
    } else if (ctx->stateUsage()) {
        return visitStateUsage(ctx->stateUsage());
    } else if (ctx->requirementUsage()) {
        return visitRequirementUsage(ctx->requirementUsage());
    } else if (ctx->constraintUsage()) {
        return visitConstraintUsage(ctx->constraintUsage());
    } else if (ctx->portUsage()) {
        return visitPortUsage(ctx->portUsage());
    } else if (ctx->connectorUsage()) {
        return visitConnectorUsage(ctx->connectorUsage());
    } else if (ctx->flowUsage()) {
        return visitFlowUsage(ctx->flowUsage());
    } else if (ctx->connectionUsage()) {
        return visitConnectionUsage(ctx->connectionUsage());
    } else if (ctx->allocationUsage()) {
        return visitAllocationUsage(ctx->allocationUsage());
    } else if (ctx->itemUsage()) {
        return visitItemUsage(ctx->itemUsage());
    } else if (ctx->viewUsage()) {
        return visitViewUsage(ctx->viewUsage());
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitCalculationDefinition(SysML::SysMLParser::CalculationDefinitionContext* ctx)
{
    if (!ctx || !ctx->definitionDeclaration()) return nullptr;
    QString name = "UnnamedCalculation";
    if (auto* identification = ctx->definitionDeclaration()->identification()) {
        const auto identifiers = identification->identifier();
        if (!identifiers.empty()) name = getText(identifiers.back()->IDENTIFIER());
    }
    auto* calculation = new CalculationDefinition();
    calculation->setDeclaredName(name);
    checkDuplicateName(calculation, name, ctx);
    if (auto* parent = getCurrentParent()) parent->addOwnedElement(calculation);
    else if (m_resource) m_resource->addContent(calculation);
    else { delete calculation; return nullptr; }
    pushParent(calculation);
    if (ctx->calculationBody()) visitCalculationBody(ctx->calculationBody());
    popParent();
    return calculation;
}

std::any ModelBuilderVisitor::visitCalculationBody(SysML::SysMLParser::CalculationBodyContext* ctx)
{
    if (!ctx) return nullptr;
    for (auto* item : ctx->calculationBodyItem()) {
        if (item->returnParameter()) visitReturnParameter(item->returnParameter());
        else if (item->definitionBodyItem()) visitDefinitionBodyItem(item->definitionBodyItem());
    }
    return nullptr;
}

std::any ModelBuilderVisitor::visitReturnParameter(SysML::SysMLParser::ReturnParameterContext* ctx)
{
    if (!ctx) return nullptr;
    QString name = "result";
    QString typeName;
    if (auto* declaration = ctx->usageDeclaration()) {
        if (auto* feature = declaration->featureDeclaration()) {
            if (auto* identification = feature->identification()) {
                const auto identifiers = identification->identifier();
                if (!identifiers.empty()) name = getText(identifiers.back()->IDENTIFIER());
            }
            if (auto* specialization = feature->featureSpecializationPart()) {
                SysML::SysMLParser::QualifiedNameContext* type = nullptr;
                if (specialization->typingExpression()) type = specialization->typingExpression()->qualifiedName();
                else if (specialization->subsettingExpression()) type = specialization->subsettingExpression()->qualifiedName();
                if (type) typeName = getQualifiedNameParts(type).join("::");
            }
        }
    }
    QString expression;
    if (ctx->valuePart() && ctx->valuePart()->featureValue() &&
        ctx->valuePart()->featureValue()->expression()) {
        expression = QString::fromStdString(ctx->valuePart()->featureValue()->expression()->getText());
    }
    auto* parameter = new ReturnParameter();
    parameter->setDeclaredName(name);
    parameter->setTypeName(typeName);
    parameter->setExpression(expression);
    if (auto* parent = getCurrentParent()) parent->addOwnedElement(parameter);
    else if (m_resource) m_resource->addContent(parameter);
    else { delete parameter; return nullptr; }
    return parameter;
}

std::any ModelBuilderVisitor::visitActionDefinition(SysML::SysMLParser::ActionDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("ActionDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("ActionDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedAction";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ActionDefinition:" << name;
    
    ActionDefinition* actionDef = new ActionDefinition();
    actionDef->setDeclaredName(name);
    
    // 检查重复名称
    checkDuplicateName(actionDef, name, ctx);
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(actionDef, supertypeParts);
                }
            }
        }
    }
    
    // 添加到父元素
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(actionDef);
    } else {
        if (m_resource) {
            m_resource->addContent(actionDef);
        } else {
            reportError("Resource is null, cannot add ActionDefinition", ctx);
            delete actionDef;
            return nullptr;
        }
    }
    
    pushParent(actionDef);
    
    // 访问定义体
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return actionDef;
}

std::any ModelBuilderVisitor::visitStateDefinition(SysML::SysMLParser::StateDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("StateDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("StateDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedState";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating StateDefinition:" << name;
    
    StateDefinition* stateDef = new StateDefinition();
    stateDef->setDeclaredName(name);
    
    checkDuplicateName(stateDef, name, ctx);
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(stateDef, supertypeParts);
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(stateDef);
    } else {
        if (m_resource) {
            m_resource->addContent(stateDef);
        } else {
            reportError("Resource is null, cannot add StateDefinition", ctx);
            delete stateDef;
            return nullptr;
        }
    }
    
    pushParent(stateDef);
    
    // 访问状态定义体（使用新的 stateDefinitionBody）
    auto bodyCtx = ctx->stateDefinitionBody();
    if (bodyCtx) {
        visitStateDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return stateDef;
}

std::any ModelBuilderVisitor::visitRequirementDefinition(SysML::SysMLParser::RequirementDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("RequirementDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("RequirementDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedRequirement";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating RequirementDefinition:" << name;
    
    RequirementDefinition* reqDef = new RequirementDefinition();
    reqDef->setDeclaredName(name);
    
    checkDuplicateName(reqDef, name, ctx);
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(reqDef, supertypeParts);
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(reqDef);
    } else {
        if (m_resource) {
            m_resource->addContent(reqDef);
        } else {
            reportError("Resource is null, cannot add RequirementDefinition", ctx);
            delete reqDef;
            return nullptr;
        }
    }
    
    pushParent(reqDef);
    
    // 访问定义体
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return reqDef;
}

std::any ModelBuilderVisitor::visitConstraintDefinition(SysML::SysMLParser::ConstraintDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("ConstraintDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("ConstraintDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedConstraint";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ConstraintDefinition:" << name;
    
    ConstraintDefinition* constraintDef = new ConstraintDefinition();
    constraintDef->setDeclaredName(name);
    
    checkDuplicateName(constraintDef, name, ctx);
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(constraintDef, supertypeParts);
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(constraintDef);
    } else {
        if (m_resource) {
            m_resource->addContent(constraintDef);
        } else {
            reportError("Resource is null, cannot add ConstraintDefinition", ctx);
            delete constraintDef;
            return nullptr;
        }
    }
    
    pushParent(constraintDef);
    
    // 访问定义体
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return constraintDef;
}

std::any ModelBuilderVisitor::visitPortDefinition(SysML::SysMLParser::PortDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("PortDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("PortDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedPort";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating PortDefinition:" << name;
    
    PortDefinition* portDef = new PortDefinition();
    portDef->setDeclaredName(name);
    
    checkDuplicateName(portDef, name, ctx);
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(portDef, supertypeParts);
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(portDef);
    } else {
        if (m_resource) {
            m_resource->addContent(portDef);
        } else {
            reportError("Resource is null, cannot add PortDefinition", ctx);
            delete portDef;
            return nullptr;
        }
    }
    
    pushParent(portDef);
    
    // 访问定义体
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return portDef;
}

std::any ModelBuilderVisitor::visitConnectorDefinition(SysML::SysMLParser::ConnectorDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("ConnectorDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("ConnectorDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedConnector";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ConnectorDefinition:" << name;
    
    ConnectorDefinition* connDef = new ConnectorDefinition();
    connDef->setDeclaredName(name);
    
    checkDuplicateName(connDef, name, ctx);
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(connDef, supertypeParts);
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(connDef);
    } else {
        if (m_resource) {
            m_resource->addContent(connDef);
        } else {
            reportError("Resource is null, cannot add ConnectorDefinition", ctx);
            delete connDef;
            return nullptr;
        }
    }
    
    pushParent(connDef);
    
    // 访问定义体
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return connDef;
}

std::any ModelBuilderVisitor::visitInterfaceDefinition(SysML::SysMLParser::InterfaceDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("InterfaceDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("InterfaceDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedInterface";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating InterfaceDefinition:" << name;
    
    InterfaceDefinition* ifaceDef = new InterfaceDefinition();
    ifaceDef->setDeclaredName(name);
    
    checkDuplicateName(ifaceDef, name, ctx);
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(ifaceDef, supertypeParts);
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(ifaceDef);
    } else {
        if (m_resource) {
            m_resource->addContent(ifaceDef);
        } else {
            reportError("Resource is null, cannot add InterfaceDefinition", ctx);
            delete ifaceDef;
            return nullptr;
        }
    }
    
    pushParent(ifaceDef);
    
    // 访问定义体
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return ifaceDef;
}

std::any ModelBuilderVisitor::visitDatatypeDefinition(SysML::SysMLParser::DatatypeDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("DataTypeDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("DataTypeDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedDataType";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating DataTypeDefinition:" << name;
    
    DataTypeDefinition* dtDef = new DataTypeDefinition();
    dtDef->setDeclaredName(name);
    
    checkDuplicateName(dtDef, name, ctx);
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(dtDef, supertypeParts);
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(dtDef);
    } else {
        if (m_resource) {
            m_resource->addContent(dtDef);
        } else {
            reportError("Resource is null, cannot add DataTypeDefinition", ctx);
            delete dtDef;
            return nullptr;
        }
    }
    
    pushParent(dtDef);
    
    // 访问定义体
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return dtDef;
}

std::any ModelBuilderVisitor::visitClassDefinition(SysML::SysMLParser::ClassDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("ClassDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("ClassDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedClass";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ClassDefinition:" << name;
    
    ClassDefinition* classDef = new ClassDefinition();
    classDef->setDeclaredName(name);
    
    checkDuplicateName(classDef, name, ctx);
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(classDef, supertypeParts);
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(classDef);
    } else {
        if (m_resource) {
            m_resource->addContent(classDef);
        } else {
            reportError("Resource is null, cannot add ClassDefinition", ctx);
            delete classDef;
            return nullptr;
        }
    }
    
    pushParent(classDef);
    
    // 访问定义体
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return classDef;
}

std::any ModelBuilderVisitor::visitEnumDefinition(SysML::SysMLParser::EnumDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("EnumDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("EnumDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedEnum";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating EnumDefinition:" << name;
    
    EnumDefinition* enumDef = new EnumDefinition();
    enumDef->setDeclaredName(name);
    
    checkDuplicateName(enumDef, name, ctx);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(enumDef);
    } else {
        if (m_resource) {
            m_resource->addContent(enumDef);
        } else {
            reportError("Resource is null, cannot add EnumDefinition", ctx);
            delete enumDef;
            return nullptr;
        }
    }
    
    pushParent(enumDef);
    
    // 枚举体使用特殊的 enumBody，不是 definitionBody
    if (ctx->enumBody()) {
        visitEnumBody(ctx->enumBody());
    }
    
    popParent();
    
    return enumDef;
}

// ============================================
// 体访问方法
// ============================================

std::any ModelBuilderVisitor::visitActionBody(SysML::SysMLParser::ActionBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 根据新的语法规则，actionBody 包含 actionBodyItem*
    // actionBodyItem 是 definitionBodyItem
    auto actionBodyItems = ctx->actionBodyItem();
    for (auto itemCtx : actionBodyItems) {
        if (itemCtx) {
            // actionBodyItem 是 definitionBodyItem，访问 definitionBodyItem
            auto defBodyItemCtx = itemCtx->definitionBodyItem();
            if (defBodyItemCtx) {
                // 使用与 visitDefinitionBody 相同的逻辑处理 definitionBodyItem
                // 处理定义成员
                auto defMemberCtx = defBodyItemCtx->definitionMember();
                if (defMemberCtx) {
                    auto defCtx = defMemberCtx->definition();
                    if (defCtx) {
                        visitDefinition(defCtx);
                    }
                }
                
                // 处理使用成员
                auto usageMemberCtx = defBodyItemCtx->usageMember();
                if (usageMemberCtx) {
                    auto usageCtx = usageMemberCtx->usage();
                    if (usageCtx) {
                        visitUsage(usageCtx);
                    }
                }
                
                // 处理属性
                auto attributeCtx = defBodyItemCtx->attribute();
                if (attributeCtx) {
                    visitAttribute(attributeCtx);
                }
                
                // 处理简化的属性语法
                auto simplifiedAttributeCtx = defBodyItemCtx->simplifiedAttribute();
                if (simplifiedAttributeCtx) {
                    visitSimplifiedAttribute(simplifiedAttributeCtx);
                }
                
                // 处理导入语句
                auto importCtx = defBodyItemCtx->importStatement();
                if (importCtx) {
                    visitImportStatement(importCtx);
                }
                
                // 处理关系
                auto relationshipCtx = defBodyItemCtx->relationship();
                if (relationshipCtx) {
                    visitRelationship(relationshipCtx);
                }
                
                // 处理语句
                auto statementCtx = defBodyItemCtx->statement();
                if (statementCtx) {
                    visitStatement(statementCtx);
                }
                
                // 处理动作输入输出（in item / out item）
                auto actionInputOutputCtx = defBodyItemCtx->actionInputOutput();
                if (actionInputOutputCtx) {
                    visitActionInputOutput(actionInputOutputCtx);
                }
                
                // 处理 doc 语句
                auto docCtx = defBodyItemCtx->docStatement();
                if (docCtx) {
                    visitDocStatement(docCtx);
                }
                
                // 处理 comment 语句
                auto commentCtx = defBodyItemCtx->commentStatement();
                if (commentCtx) {
                    visitCommentStatement(commentCtx);
                }
                
                // 处理 metadata annotation
                auto metadataAnnotationCtx = defBodyItemCtx->metadataAnnotation();
                if (metadataAnnotationCtx) {
                    visitMetadataAnnotation(metadataAnnotationCtx);
                }
                
                // 处理 exhibit statement
                auto exhibitCtx = defBodyItemCtx->exhibitStatement();
                if (exhibitCtx) {
                    visitExhibitStatement(exhibitCtx);
                }
            }
        }
    }
    
    return nullptr;
}

// 注意：旧的 visitStateBody 实现已删除，新的实现在文件后面（使用 stateBodyItem）

std::any ModelBuilderVisitor::visitRequirementBody(SysML::SysMLParser::RequirementBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // requirementBody 现在使用 definitionBody，所以直接访问 definitionBody
    auto defBodyCtx = ctx->definitionBody();
    if (defBodyCtx) {
        visitDefinitionBody(defBodyCtx);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitRequirementMember(SysML::SysMLParser::RequirementMemberContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取关键字类型（subject, actor, stakeholder）
    QString ctxText = QString::fromStdString(ctx->getText());
    QString keyword;
    if (ctxText.startsWith("subject")) {
        keyword = "subject";
    } else if (ctxText.startsWith("actor")) {
        keyword = "actor";
    } else if (ctxText.startsWith("stakeholder")) {
        keyword = "stakeholder";
    }
    
    // 获取使用声明
    QString name = "Unnamed" + keyword;
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating RequirementMember:" << keyword << name;
    
    // 创建为 PartUsage（简化实现，实际应该是特殊的 requirement member）
    // 这里我们使用 PartUsage 作为占位符，实际应该创建专门的 RequirementMember 类型
    PartUsage* member = new PartUsage();
    member->setDeclaredName(keyword + " " + name);
    
    // 处理类型（从特征特殊化部分）
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        QString typeName = typeParts.join("::");
                        validateTypeReference(typeParts, ctx);
                        createUsageTyping(member, typeParts);
                        // 同时保存类型名称字符串，以便序列化时使用
                        member->setTypeName(typeName);
                    }
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(member);
    } else {
        if (m_resource) {
            m_resource->addContent(member);
        } else {
            reportError("Resource is null, cannot add RequirementMember", ctx);
            delete member;
            return nullptr;
        }
    }
    
    pushParent(member);
    
    // 访问使用完成（包含值部分和使用体）
    auto completionCtx = ctx->usageCompletion();
    if (completionCtx) {
        // 处理 value part（如果有）
        processUsageCompletionValuePart(completionCtx);
        
        auto usageBodyCtx = completionCtx->usageBody();
        if (usageBodyCtx) {
            visitUsageBody(usageBodyCtx);
        }
    }
    
    popParent();
    
    return member;
}

std::any ModelBuilderVisitor::visitRequirementConstraintMember(SysML::SysMLParser::RequirementConstraintMemberContext* ctx)
{
    if (!ctx) {
        reportError("RequirementConstraintMember context is null");
        return nullptr;
    }
    
    // 获取关键字类型（assume 或 require）
    QString ctxText = QString::fromStdString(ctx->getText());
    bool isAssume = ctxText.startsWith("assume");
    
    QString expression;
    
    // 使用 ANTLR4 生成的 expression() 方法
    auto exprCtx = ctx->expression();
    if (exprCtx) {
        // 使用表达式访问方法来解析表达式结构
        auto result = visitExpression(exprCtx);
        if (result.has_value()) {
            expression = std::any_cast<QString>(result);
        } else {
            // 如果访问方法返回空，使用文本
            expression = QString::fromStdString(exprCtx->getText());
        }
        
        // 检查表达式中的单位表达式
        QString exprText = QString::fromStdString(exprCtx->getText());
        if (exprText.contains('[') && exprText.contains(']')) {
            qDebug() << "[ModelBuilderVisitor] RequirementConstraintMember expression contains units:" << exprText;
        }
    } else {
        // 如果 expression() 返回 null，从文本中提取表达式
        // 检查是否是 assume/require constraint {expression} 形式
        if (ctxText.contains("constraint {")) {
            // 第二种形式：assume/require constraint {expression}
            int startIdx = ctxText.indexOf('{');
            int endIdx = ctxText.lastIndexOf('}');
            if (startIdx >= 0 && endIdx > startIdx) {
                expression = ctxText.mid(startIdx + 1, endIdx - startIdx - 1).trimmed();
            } else {
                expression = ctxText; // 如果找不到大括号，使用整个文本
            }
        } else {
            // 第一种形式：assume/require expression;
            // 提取分号之前的内容（去掉关键字）
            int keywordEnd = isAssume ? 6 : 7; // "assume " 或 "require "
            int semicolonIdx = ctxText.lastIndexOf(';');
            if (semicolonIdx > keywordEnd) {
                expression = ctxText.mid(keywordEnd, semicolonIdx - keywordEnd).trimmed();
            } else {
                expression = ctxText.mid(keywordEnd).trimmed();
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating RequirementConstraintMember:" << (isAssume ? "assume" : "require") << expression;
    
    if (isAssume) {
        AssumeStatement* assumeStmt = new AssumeStatement();
        assumeStmt->setExpression(expression);
        assumeStmt->setDeclaredName("assume " + expression);
        
        Element* parent = getCurrentParent();
        if (parent) {
            parent->addOwnedElement(assumeStmt);
        } else {
            if (m_resource) {
                m_resource->addContent(assumeStmt);
            } else {
                reportError("Resource is null, cannot add AssumeStatement", ctx);
                delete assumeStmt;
                return nullptr;
            }
        }
        
        return assumeStmt;
    } else {
        RequireStatement* requireStmt = new RequireStatement();
        requireStmt->setExpression(expression);
        requireStmt->setDeclaredName("require " + expression);
        
        // 检查是否包含 constraint 关键字
        // 如果上下文文本包含 "constraint {"，说明是 require constraint {expression} 格式
        bool hasConstraintKeyword = ctxText.contains("constraint") && 
                                     ctxText.contains("{") && 
                                     ctxText.contains("}");
        requireStmt->setHasConstraintKeyword(hasConstraintKeyword);
        
        Element* parent = getCurrentParent();
        if (parent) {
            parent->addOwnedElement(requireStmt);
        } else {
            if (m_resource) {
                m_resource->addContent(requireStmt);
            } else {
                reportError("Resource is null, cannot add RequireStatement", ctx);
                delete requireStmt;
                return nullptr;
            }
        }
        
        return requireStmt;
    }
}

std::any ModelBuilderVisitor::visitPortBody(SysML::SysMLParser::PortBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    for (auto defCtx : ctx->definition()) {
        if (defCtx) visitDefinition(defCtx);
    }
    for (auto usageCtx : ctx->usage()) {
        if (usageCtx) visitUsage(usageCtx);
    }
    for (auto attributeCtx : ctx->attribute()) {
        if (attributeCtx) visitAttribute(attributeCtx);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitConnectorBody(SysML::SysMLParser::ConnectorBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    for (auto defCtx : ctx->definition()) {
        if (defCtx) visitDefinition(defCtx);
    }
    for (auto usageCtx : ctx->usage()) {
        if (usageCtx) visitUsage(usageCtx);
    }
    for (auto attributeCtx : ctx->attribute()) {
        if (attributeCtx) visitAttribute(attributeCtx);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitFlowBody(SysML::SysMLParser::FlowBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    for (auto defCtx : ctx->definition()) {
        if (defCtx) visitDefinition(defCtx);
    }
    for (auto usageCtx : ctx->usage()) {
        if (usageCtx) visitUsage(usageCtx);
    }
    for (auto attributeCtx : ctx->attribute()) {
        if (attributeCtx) visitAttribute(attributeCtx);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitInterfaceBody(SysML::SysMLParser::InterfaceBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    for (auto defCtx : ctx->definition()) {
        if (defCtx) visitDefinition(defCtx);
    }
    for (auto usageCtx : ctx->usage()) {
        if (usageCtx) visitUsage(usageCtx);
    }
    for (auto attributeCtx : ctx->attribute()) {
        if (attributeCtx) visitAttribute(attributeCtx);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitDatatypeBody(SysML::SysMLParser::DatatypeBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    for (auto attributeCtx : ctx->attribute()) {
        if (attributeCtx) visitAttribute(attributeCtx);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitClassBody(SysML::SysMLParser::ClassBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    for (auto defCtx : ctx->definition()) {
        if (defCtx) visitDefinition(defCtx);
    }
    for (auto usageCtx : ctx->usage()) {
        if (usageCtx) visitUsage(usageCtx);
    }
    for (auto attributeCtx : ctx->attribute()) {
        if (attributeCtx) visitAttribute(attributeCtx);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitEnumBody(SysML::SysMLParser::EnumBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    for (auto enumValueCtx : ctx->enumValue()) {
        if (enumValueCtx) visitEnumValue(enumValueCtx);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitAttributeDefinition(SysML::SysMLParser::AttributeDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("AttributeDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("AttributeDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedAttribute";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating AttributeDefinition:" << name;
    
    AttributeDefinition* attrDef = new AttributeDefinition();
    attrDef->setDeclaredName(name);
    
    checkDuplicateName(attrDef, name, ctx);
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(attrDef, supertypeParts);
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(attrDef);
    } else {
        if (m_resource) {
            m_resource->addContent(attrDef);
        } else {
            reportError("Resource is null, cannot add AttributeDefinition", ctx);
            delete attrDef;
            return nullptr;
        }
    }
    
    pushParent(attrDef);
    
    // 访问定义体
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return attrDef;
}

std::any ModelBuilderVisitor::visitConnectionDefinition(SysML::SysMLParser::ConnectionDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("ConnectionDefinition context is null");
        return nullptr;
    }
    
    // 获取定义前缀
    auto prefixCtx = ctx->occurrenceDefinitionPrefix();
    bool isAbstract = false;
    bool isVariation = false;
    bool isIndividual = false;
    
    if (prefixCtx) {
        auto basicPrefixCtx = prefixCtx->basicDefinitionPrefix();
        if (basicPrefixCtx) {
            QString prefixText = QString::fromStdString(basicPrefixCtx->getText());
            if (prefixText == "abstract") {
                isAbstract = true;
            } else if (prefixText == "variation") {
                isVariation = true;
            }
        }
        if (prefixCtx->emptyMultiplicityMember()) {
            isIndividual = true;
        }
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("ConnectionDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedConnection";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ConnectionDefinition:" << name;
    
    ConnectionDefinition* connDef = new ConnectionDefinition();
    connDef->setDeclaredName(name);
    
    if (isAbstract) {
        connDef->setIsAbstract(true);
    }
    if (isVariation) {
        connDef->setIsVariation(true);
    }
    if (isIndividual) {
        connDef->setIsIndividual(true);
    }
    
    checkDuplicateName(connDef, name, ctx);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(connDef);
    } else {
        if (m_resource) {
            m_resource->addContent(connDef);
        } else {
            reportError("Resource is null, cannot add ConnectionDefinition", ctx);
            delete connDef;
            return nullptr;
        }
    }
    
    // 处理子类化部分
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(connDef, supertypeParts);
                }
            }
        }
    }
    
    pushParent(connDef);
    
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return connDef;
}

std::any ModelBuilderVisitor::visitAllocationDefinition(SysML::SysMLParser::AllocationDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("AllocationDefinition context is null");
        return nullptr;
    }
    
    // 获取定义前缀
    auto prefixCtx = ctx->occurrenceDefinitionPrefix();
    bool isAbstract = false;
    bool isVariation = false;
    bool isIndividual = false;
    
    if (prefixCtx) {
        auto basicPrefixCtx = prefixCtx->basicDefinitionPrefix();
        if (basicPrefixCtx) {
            QString prefixText = QString::fromStdString(basicPrefixCtx->getText());
            if (prefixText == "abstract") {
                isAbstract = true;
            } else if (prefixText == "variation") {
                isVariation = true;
            }
        }
        if (prefixCtx->emptyMultiplicityMember()) {
            isIndividual = true;
        }
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("AllocationDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedAllocation";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating AllocationDefinition:" << name;
    
    AllocationDefinition* allocDef = new AllocationDefinition();
    allocDef->setDeclaredName(name);
    
    if (isAbstract) {
        allocDef->setIsAbstract(true);
    }
    if (isVariation) {
        allocDef->setIsVariation(true);
    }
    if (isIndividual) {
        allocDef->setIsIndividual(true);
    }
    
    checkDuplicateName(allocDef, name, ctx);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(allocDef);
    } else {
        if (m_resource) {
            m_resource->addContent(allocDef);
        } else {
            reportError("Resource is null, cannot add AllocationDefinition", ctx);
            delete allocDef;
            return nullptr;
        }
    }
    
    // 处理子类化部分
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(allocDef, supertypeParts);
                }
            }
        }
    }
    
    pushParent(allocDef);
    
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return allocDef;
}

std::any ModelBuilderVisitor::visitItemDefinition(SysML::SysMLParser::ItemDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("ItemDefinition context is null");
        return nullptr;
    }
    
    // 获取定义前缀
    auto prefixCtx = ctx->occurrenceDefinitionPrefix();
    bool isAbstract = false;
    bool isVariation = false;
    bool isIndividual = false;
    
    if (prefixCtx) {
        auto basicPrefixCtx = prefixCtx->basicDefinitionPrefix();
        if (basicPrefixCtx) {
            QString prefixText = QString::fromStdString(basicPrefixCtx->getText());
            if (prefixText == "abstract") {
                isAbstract = true;
            } else if (prefixText == "variation") {
                isVariation = true;
            }
        }
        if (prefixCtx->emptyMultiplicityMember()) {
            isIndividual = true;
        }
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("ItemDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedItem";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ItemDefinition:" << name;
    
    ItemDefinition* itemDef = new ItemDefinition();
    itemDef->setDeclaredName(name);
    
    if (isAbstract) {
        itemDef->setIsAbstract(true);
    }
    if (isVariation) {
        itemDef->setIsVariation(true);
    }
    if (isIndividual) {
        itemDef->setIsIndividual(true);
    }
    
    checkDuplicateName(itemDef, name, ctx);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(itemDef);
    } else {
        if (m_resource) {
            m_resource->addContent(itemDef);
        } else {
            reportError("Resource is null, cannot add ItemDefinition", ctx);
            delete itemDef;
            return nullptr;
        }
    }
    
    // 处理子类化部分
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(itemDef, supertypeParts);
                }
            }
        }
    }
    
    pushParent(itemDef);
    
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return itemDef;
}

std::any ModelBuilderVisitor::visitAttributeBody(SysML::SysMLParser::AttributeBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    for (auto defCtx : ctx->definition()) {
        if (defCtx) visitDefinition(defCtx);
    }
    for (auto usageCtx : ctx->usage()) {
        if (usageCtx) visitUsage(usageCtx);
    }
    for (auto attributeCtx : ctx->attribute()) {
        if (attributeCtx) visitAttribute(attributeCtx);
    }
    
    return nullptr;
}

// 注意：visitTransition, visitTransitionBody, visitSimpleTransitionBody, visitStateReference 
// 的实现在上面已经定义，这里删除重复定义

std::any ModelBuilderVisitor::visitTransition(SysML::SysMLParser::TransitionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取转换名称（可选）
    // 注意：转换名称可以是 identifier 或 qualifiedName
    QString name = "UnnamedTransition";
    if (ctx->identifier()) {
        name = getText(ctx->identifier()->IDENTIFIER());
    } else if (ctx->qualifiedName()) {
        QStringList nameParts = getQualifiedNameParts(ctx->qualifiedName());
        name = nameParts.join("::");
    }
    
    qDebug() << "[ModelBuilderVisitor] Found transition:" << name;
    
    // 创建 Transition 对象
    Transition* transition = new Transition();
    transition->setDeclaredName(name);
    
    // 访问转换体
    auto transitionBodyCtx = ctx->transitionBody();
    if (transitionBodyCtx) {
        pushParent(transition);
        auto result = visitTransitionBody(transitionBodyCtx);
        popParent();
        
        // 如果 visitTransitionBody 返回了 Transition 对象，使用它
        if (result.has_value()) {
            try {
                Transition* bodyTransition = std::any_cast<Transition*>(result);
                if (bodyTransition && bodyTransition != transition) {
                    // 复制属性
                    transition->setSourceState(bodyTransition->sourceState());
                    transition->setTargetState(bodyTransition->targetState());
                    transition->setIsInitial(bodyTransition->isInitial());
                    transition->setTriggerPart(bodyTransition->triggerPart());
                    transition->setGuardPart(bodyTransition->guardPart());
                    transition->setEffectPart(bodyTransition->effectPart());
                    delete bodyTransition;
                }
            } catch (const std::bad_any_cast& e) {
                qDebug() << "[ModelBuilderVisitor] visitTransition: bad_any_cast when casting visitTransitionBody result:" << e.what();
                qDebug() << "[ModelBuilderVisitor] visitTransition: result type info:" << result.type().name();
            }
        } else {
            qDebug() << "[ModelBuilderVisitor] visitTransition: visitTransitionBody returned empty result";
        }
        
        // 如果名称是 "UnnamedTransition"，根据源状态和目标状态生成显示名称
        if (name == "UnnamedTransition") {
            QString sourceState = transition->sourceState();
            QString targetState = transition->targetState();
            if (!sourceState.isEmpty() && !targetState.isEmpty()) {
                QString displayName = QString("transition<%1,%2>").arg(sourceState, targetState);
                transition->setDeclaredName(displayName);
                name = displayName;
            }
        }
    }
    
    // 添加到父元素
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(transition);
        qDebug() << "[ModelBuilderVisitor] Created Transition:" << name;
    } else {
        delete transition;
        return nullptr;
    }
    
    return transition;
}

std::any ModelBuilderVisitor::visitTransitionBody(SysML::SysMLParser::TransitionBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问简单转换体或复杂转换体
    auto simpleBodyCtx = ctx->simpleTransitionBody();
    if (simpleBodyCtx) {
        return visitSimpleTransitionBody(simpleBodyCtx);
    }
    
    auto complexBodyCtx = ctx->complexTransitionBody();
    if (complexBodyCtx) {
        return visitComplexTransitionBody(complexBodyCtx);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitSimpleTransitionBody(SysML::SysMLParser::SimpleTransitionBodyContext* ctx)
{
    if (!ctx) {
        qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: ctx is null";
        return nullptr;
    }
    
    QString sourceState;
    QString targetState;
    bool isInitial = false;
    
    // 根据新的语法规则：
    // simpleTransitionBody
    //     : 'initial' 'then' stateReference
    //     | stateReference 'then' stateReference
    //     ;
    
    QString contextText = QString::fromStdString(ctx->getText());
    qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: text=" << contextText;
    
    auto stateRefs = ctx->stateReference();
    qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: stateRefs.size()=" << stateRefs.size();
    
    if (stateRefs.empty()) {
        qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: stateRefs is empty";
        return nullptr;
    }
    
    // 检查是否是 'initial' 'then' stateReference 形式
    QString firstText = QString::fromStdString(ctx->getText());
    qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: firstText=" << firstText;
    if (firstText.startsWith("initial")) {
        // 第一种形式：'initial' 'then' stateReference
        qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: detected initial then form";
        isInitial = true;
        
        // 获取目标状态（第二个 stateReference，索引为 0）
        if (stateRefs.size() > 0) {
            qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: visiting target stateRef[0], text=" 
                     << QString::fromStdString(stateRefs[0]->getText());
            auto targetResult = visitStateReference(stateRefs[0]);
            if (targetResult.has_value()) {
                try {
                    targetState = std::any_cast<QString>(targetResult);
                    qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: targetState from any_cast=" << targetState;
                } catch (const std::bad_any_cast& e) {
                    qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: bad_any_cast for targetState:" << e.what();
                    targetState = QString::fromStdString(stateRefs[0]->getText());
                }
            } else {
                targetState = QString::fromStdString(stateRefs[0]->getText());
                qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: targetState from getText=" << targetState;
            }
        }
    } else {
        // 第二种形式：stateReference 'then' stateReference
        qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: detected stateReference then stateReference form";
        // 第一个是源状态，第二个是目标状态
        if (stateRefs.size() >= 2) {
            qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: stateRefs.size() >= 2";
            // 源状态
            qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: visiting source stateRef[0], text=" 
                     << QString::fromStdString(stateRefs[0]->getText());
            auto sourceResult = visitStateReference(stateRefs[0]);
            if (sourceResult.has_value()) {
                try {
                    sourceState = std::any_cast<QString>(sourceResult);
                    qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: sourceState from any_cast=" << sourceState;
                } catch (const std::bad_any_cast& e) {
                    qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: bad_any_cast for sourceState:" << e.what();
                    sourceState = QString::fromStdString(stateRefs[0]->getText());
                }
            } else {
                sourceState = QString::fromStdString(stateRefs[0]->getText());
                qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: sourceState from getText=" << sourceState;
            }
            
            // 目标状态
            qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: visiting target stateRef[1], text=" 
                     << QString::fromStdString(stateRefs[1]->getText());
            auto targetResult = visitStateReference(stateRefs[1]);
            if (targetResult.has_value()) {
                try {
                    targetState = std::any_cast<QString>(targetResult);
                    qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: targetState from any_cast=" << targetState;
                } catch (const std::bad_any_cast& e) {
                    qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: bad_any_cast for targetState:" << e.what();
                    targetState = QString::fromStdString(stateRefs[1]->getText());
                }
            } else {
                targetState = QString::fromStdString(stateRefs[1]->getText());
                qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: targetState from getText=" << targetState;
            }
        } else if (stateRefs.size() == 1) {
            // 只有一个 stateReference，可能是解析错误，但尝试处理
            qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: WARNING - only one stateRef, text=" 
                     << QString::fromStdString(stateRefs[0]->getText());
            auto targetResult = visitStateReference(stateRefs[0]);
            if (targetResult.has_value()) {
                try {
                    targetState = std::any_cast<QString>(targetResult);
                    qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: targetState from any_cast=" << targetState;
                } catch (const std::bad_any_cast& e) {
                    qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: bad_any_cast for targetState (single stateRef):" << e.what();
                    targetState = QString::fromStdString(stateRefs[0]->getText());
                }
            } else {
                targetState = QString::fromStdString(stateRefs[0]->getText());
                qDebug() << "[ModelBuilderVisitor] visitSimpleTransitionBody: targetState from getText=" << targetState;
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] SimpleTransitionBody:"
             << (isInitial ? "initial" : sourceState)
             << "->" << targetState;
    
    // 创建 Transition 模型对象
    Transition* transition = new Transition();
    transition->setSourceState(sourceState);
    transition->setTargetState(targetState);
    transition->setIsInitial(isInitial);
    
    return transition;
}

std::any ModelBuilderVisitor::visitStateReference(SysML::SysMLParser::StateReferenceContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 检查是否是 'initial' 关键字
    // 注意：由于 stateReference 规则中允许 'initial' 作为状态名称，
    // 当匹配 'initial' 时，identifier() 和 qualifiedName() 都会返回 null
    // 所以我们需要通过检查 getText() 来判断
    QString text = QString::fromStdString(ctx->getText());
    if (text == "initial") {
        return QString("initial");
    }
    
    // 访问标识符或限定名称
    auto identifierCtx = ctx->identifier();
    if (identifierCtx) {
        QString name = getText(identifierCtx->IDENTIFIER());
        return name;
    }
    
    auto qnameCtx = ctx->qualifiedName();
    if (qnameCtx) {
        QStringList nameParts = getQualifiedNameParts(qnameCtx);
        QString name = nameParts.join("::");
        return name;
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitComplexTransitionBody(SysML::SysMLParser::ComplexTransitionBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    Transition* transition = new Transition();
    
    // 获取源状态（first stateReference）
    auto stateRefs = ctx->stateReference();
    if (stateRefs.size() > 0) {
        auto result = visitStateReference(stateRefs[0]);
        if (result.has_value()) {
            QString sourceState = std::any_cast<QString>(result);
            transition->setSourceState(sourceState);
        } else {
            transition->setSourceState(QString::fromStdString(stateRefs[0]->getText()));
        }
    }
    
    // 获取目标状态（then stateReference）
    if (stateRefs.size() > 1) {
        auto result = visitStateReference(stateRefs[1]);
        if (result.has_value()) {
            QString targetState = std::any_cast<QString>(result);
            transition->setTargetState(targetState);
        } else {
            transition->setTargetState(QString::fromStdString(stateRefs[1]->getText()));
        }
    }
    
    // 访问触发部分
    auto triggerPartCtx = ctx->transitionTriggerPart();
    if (triggerPartCtx) {
        auto result = visitTransitionTriggerPart(triggerPartCtx);
        if (result.has_value()) {
            TransitionTriggerPart* triggerPart = std::any_cast<TransitionTriggerPart*>(result);
            transition->setTriggerPart(triggerPart);
        }
    }
    
    // 访问守卫部分
    auto guardPartCtx = ctx->transitionGuardPart();
    if (guardPartCtx) {
        auto result = visitTransitionGuardPart(guardPartCtx);
        if (result.has_value()) {
            TransitionGuardPart* guardPart = std::any_cast<TransitionGuardPart*>(result);
            transition->setGuardPart(guardPart);
        }
    }
    
    // 访问效果部分
    auto effectPartCtx = ctx->transitionEffectPart();
    if (effectPartCtx) {
        auto result = visitTransitionEffectPart(effectPartCtx);
        if (result.has_value()) {
            TransitionEffectPart* effectPart = std::any_cast<TransitionEffectPart*>(result);
            transition->setEffectPart(effectPart);
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] ComplexTransitionBody:"
             << transition->sourceState() << "->" << transition->targetState();
    
    return transition;
}

std::any ModelBuilderVisitor::visitTransitionTriggerPart(SysML::SysMLParser::TransitionTriggerPartContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    TransitionTriggerPart* triggerPart = new TransitionTriggerPart();
    
    // 访问 accept 子句
    auto acceptClauseCtx = ctx->acceptClause();
    if (acceptClauseCtx) {
        auto result = visitAcceptClause(acceptClauseCtx);
        if (result.has_value()) {
            AcceptClause* acceptClause = std::any_cast<AcceptClause*>(result);
            triggerPart->setAcceptClause(acceptClause);
        }
    }
    
    return triggerPart;
}

std::any ModelBuilderVisitor::visitAcceptClause(SysML::SysMLParser::AcceptClauseContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    AcceptClause* acceptClause = new AcceptClause();
    pushParent(acceptClause);
    
    // 访问 acceptItem 或 acceptExpression
    auto acceptItemCtx = ctx->acceptItem();
    if (acceptItemCtx) {
        acceptClause->setAcceptType("item");
        visitAcceptItem(acceptItemCtx);
    } else {
        auto acceptExprCtx = ctx->acceptExpression();
        if (acceptExprCtx) {
            visitAcceptExpression(acceptExprCtx);
        }
    }
    
    popParent();
    return acceptClause;
}

std::any ModelBuilderVisitor::visitAcceptItem(SysML::SysMLParser::AcceptItemContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取父 AcceptClause（应该在 visitAcceptClause 中设置）
    AcceptClause* acceptClause = qobject_cast<AcceptClause*>(getCurrentParent());
    if (!acceptClause) {
        return nullptr;
    }
    
    // 访问使用声明
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        QString declText = QString::fromStdString(usageDeclCtx->getText());
        acceptClause->setUsageDeclaration(declText);
    }
    
    // 访问 via 端口
    auto viaRefCtx = ctx->ownedReferenceSubsetting();
    if (viaRefCtx) {
        QString viaText = QString::fromStdString(viaRefCtx->getText());
        acceptClause->setViaPort(viaText);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitAcceptExpression(SysML::SysMLParser::AcceptExpressionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取父 AcceptClause
    AcceptClause* acceptClause = qobject_cast<AcceptClause*>(getCurrentParent());
    if (!acceptClause) {
        return nullptr;
    }
    
    QString text = QString::fromStdString(ctx->getText());
    
    // 检查是否是 'when' 表达式
    if (text.startsWith("when")) {
        acceptClause->setAcceptType("when");
        auto exprCtx = ctx->expression();
        if (exprCtx) {
            QString condText = QString::fromStdString(exprCtx->getText());
            acceptClause->setCondition(condText);
        }
    } else if (text.startsWith("at")) {
        // 检查是否是 'at' 表达式
        acceptClause->setAcceptType("at");
        auto exprCtx = ctx->expression();
        if (exprCtx) {
            QString condText = QString::fromStdString(exprCtx->getText());
            acceptClause->setCondition(condText);
        }
    } else {
        // 简单事件引用（expression）
        acceptClause->setAcceptType("event");
        auto exprCtx = ctx->expression();
        if (exprCtx) {
            QString eventText = QString::fromStdString(exprCtx->getText());
            acceptClause->setCondition(eventText);
        } else {
            acceptClause->setCondition(text);
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitTransitionGuardPart(SysML::SysMLParser::TransitionGuardPartContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    TransitionGuardPart* guardPart = new TransitionGuardPart();
    
    // 访问 if 表达式
    auto ifExprCtx = ctx->expression();
    if (ifExprCtx) {
        QString condText = QString::fromStdString(ifExprCtx->getText());
        guardPart->setCondition(condText);
    }
    
    return guardPart;
}

std::any ModelBuilderVisitor::visitTransitionEffectPart(SysML::SysMLParser::TransitionEffectPartContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    TransitionEffectPart* effectPart = new TransitionEffectPart();
    pushParent(effectPart);
    
    // 访问转换动作
    auto actionCtx = ctx->transitionAction();
    if (actionCtx) {
        visitTransitionAction(actionCtx);
    }
    
    popParent();
    return effectPart;
}

std::any ModelBuilderVisitor::visitTransitionAction(SysML::SysMLParser::TransitionActionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    TransitionEffectPart* effectPart = qobject_cast<TransitionEffectPart*>(getCurrentParent());
    if (!effectPart) {
        return nullptr;
    }
    
    // 访问 sendStatement
    auto sendStmtCtx = ctx->sendStatement();
    if (sendStmtCtx) {
        effectPart->setActionType("send");
        auto result = visitSendStatement(sendStmtCtx);
        if (result.has_value()) {
            SendStatement* sendStmt = std::any_cast<SendStatement*>(result);
            effectPart->setSendStatement(sendStmt);
        }
        return nullptr;
    }
    
    // 访问 actionReference
    auto actionRefCtx = ctx->actionReference();
    if (actionRefCtx) {
        effectPart->setActionType("reference");
        QString refText = QString::fromStdString(actionRefCtx->getText());
        effectPart->setActionReference(refText);
        return nullptr;
    }
    
    // 访问 actionInvocation
    auto actionInvCtx = ctx->actionInvocation();
    if (actionInvCtx) {
        effectPart->setActionType("invocation");
        QString invText = QString::fromStdString(actionInvCtx->getText());
        effectPart->setActionInvocation(invText);
        return nullptr;
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitSendStatement(SysML::SysMLParser::SendStatementContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    SendStatement* sendStmt = new SendStatement();
    
    // 访问 sendActionPart（如果有）
    auto sendActionPartCtx = ctx->sendActionPart();
    if (sendActionPartCtx) {
        // 检查是否有 'new' 关键字（通过检查第一个子节点）
        QString partText = QString::fromStdString(sendActionPartCtx->getText());
        bool hasNew = partText.startsWith("new");
        sendStmt->setHasNew(hasNew);
        
        // 访问 actionInvocation
        auto actionInvCtx = sendActionPartCtx->actionInvocation();
        if (actionInvCtx) {
            QString invText = QString::fromStdString(actionInvCtx->getText());
            sendStmt->setActionInvocation(invText);
        }
    } else {
        // 没有 sendActionPart，表示 send to target 形式
        sendStmt->setHasNew(false);
    }
    
    // 访问目标（to ownedReferenceSubsetting）
    auto targetRefCtx = ctx->ownedReferenceSubsetting();
    if (targetRefCtx) {
        QString targetText = QString::fromStdString(targetRefCtx->getText());
        sendStmt->setTarget(targetText);
    }
    
    return sendStmt;
}

std::any ModelBuilderVisitor::visitStateActionMember(SysML::SysMLParser::StateActionMemberContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 分发到相应的访问者方法
    auto entryCtx = ctx->entryActionMember();
    if (entryCtx) {
        return visitEntryActionMember(entryCtx);
    }
    
    auto doCtx = ctx->doActionMember();
    if (doCtx) {
        return visitDoActionMember(doCtx);
    }
    
    auto exitCtx = ctx->exitActionMember();
    if (exitCtx) {
        return visitExitActionMember(exitCtx);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitEntryActionMember(SysML::SysMLParser::EntryActionMemberContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    StateActionMember* actionMember = new StateActionMember();
    actionMember->setActionKind("entry");
    
    // 访问状态动作使用
    auto actionUsageCtx = ctx->stateActionUsage();
    if (actionUsageCtx) {
        auto result = visitStateActionUsage(actionUsageCtx);
        if (result.has_value()) {
            QString actionName = std::any_cast<QString>(result);
            actionMember->setActionName(actionName);
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(actionMember);
        qDebug() << "[ModelBuilderVisitor] Created EntryActionMember:" << actionMember->actionName();
    } else {
        delete actionMember;
        return nullptr;
    }
    
    return actionMember;
}

std::any ModelBuilderVisitor::visitDoActionMember(SysML::SysMLParser::DoActionMemberContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    StateActionMember* actionMember = new StateActionMember();
    actionMember->setActionKind("do");
    
    // 访问状态动作使用
    auto actionUsageCtx = ctx->stateActionUsage();
    if (actionUsageCtx) {
        auto result = visitStateActionUsage(actionUsageCtx);
        if (result.has_value()) {
            QString actionName = std::any_cast<QString>(result);
            actionMember->setActionName(actionName);
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(actionMember);
        qDebug() << "[ModelBuilderVisitor] Created DoActionMember:" << actionMember->actionName();
    } else {
        delete actionMember;
        return nullptr;
    }
    
    return actionMember;
}

std::any ModelBuilderVisitor::visitExitActionMember(SysML::SysMLParser::ExitActionMemberContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    StateActionMember* actionMember = new StateActionMember();
    actionMember->setActionKind("exit");
    
    // 访问状态动作使用
    auto actionUsageCtx = ctx->stateActionUsage();
    if (actionUsageCtx) {
        auto result = visitStateActionUsage(actionUsageCtx);
        if (result.has_value()) {
            QString actionName = std::any_cast<QString>(result);
            actionMember->setActionName(actionName);
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(actionMember);
        qDebug() << "[ModelBuilderVisitor] Created ExitActionMember:" << actionMember->actionName();
    } else {
        delete actionMember;
        return nullptr;
    }
    
    return actionMember;
}

std::any ModelBuilderVisitor::visitStateActionUsage(SysML::SysMLParser::StateActionUsageContext* ctx)
{
    if (!ctx) {
        return QString();
    }
    
    QString actionName;
    bool hasActionKeyword = false;
    
    // 检查是否是 'action' identifier ';' 形式
    auto identifierCtx = ctx->identifier();
    if (identifierCtx) {
        // 第一种形式：entry action initial;
        hasActionKeyword = true;
        actionName = getText(identifierCtx->IDENTIFIER());
    } else {
        // 第二种或第三种形式：entry performSelfTest; 或 do senseTemperature { ... }
        auto refSubsetCtx = ctx->ownedReferenceSubsetting();
        if (refSubsetCtx) {
            // 从 ownedReferenceSubsetting 提取名称
            if (refSubsetCtx->qualifiedName()) {
                QStringList nameParts = getQualifiedNameParts(refSubsetCtx->qualifiedName());
                actionName = nameParts.join("::");
            } else if (refSubsetCtx->memberAccessExpression()) {
                // 处理成员访问表达式（如 senseTemperature.temp）
                actionName = QString::fromStdString(refSubsetCtx->memberAccessExpression()->getText());
            } else if (refSubsetCtx->identifier()) {
                actionName = getText(refSubsetCtx->identifier()->IDENTIFIER());
            }
        }
    }
    
    // 如果有 actionBody，暂时只记录（阶段二暂不处理 actionBody 的详细内容）
    auto actionBodyCtx = ctx->actionBody();
    if (actionBodyCtx) {
        qDebug() << "[ModelBuilderVisitor] StateActionUsage has actionBody (content not processed in phase 2)";
    }
    
    // 返回动作名称（如果需要返回更多信息，可以返回一个结构体）
    return actionName;
}

std::any ModelBuilderVisitor::visitEnumValue(SysML::SysMLParser::EnumValueContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    auto idCtx = ctx->identifier();
    if (!idCtx) {
        return nullptr;
    }
    
    QString name = getText(idCtx->IDENTIFIER());
    // 暂时使用 Feature 来表示枚举值
    Feature* enumValue = new Feature();
    enumValue->setDeclaredName(name);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(enumValue);
    }
    
    // 如果有值，可以设置（暂时忽略）
    if (ctx->Integer()) {
        QString value = getText(ctx->Integer());
        qDebug() << "[ModelBuilderVisitor] Enum value:" << name << "=" << value;
    }
    
    return enumValue;
}

void ModelBuilderVisitor::createSubclassification(Type* subtype, const QStringList& supertypeNameParts)
{
    if (!subtype || supertypeNameParts.isEmpty()) {
        return;
    }
    
    validateTypeReference(supertypeNameParts, nullptr);
    QString supertypeName = supertypeNameParts.join("::");
    
    // 查找超类型
    Type* supertype = findType(supertypeNameParts);
    if (!supertype) {
        reportWarning(QString("Supertype not found for Subclassification: %1 (may be defined in another resource)")
                     .arg(supertypeName));
        return;
    }
    
    // 创建 Subclassification 关系
    Subclassification* subclassification = new Subclassification();
    subclassification->setSubtype(subtype);
    subclassification->setSupertype(supertype);
    
    // 添加到父元素的关系列表（而不是作为内容）
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedRelationship(subclassification);
        qDebug() << "[ModelBuilderVisitor] Created Subclassification:"
                 << subtype->declaredName() << "->" << supertype->declaredName();
    } else {
        // 如果没有父元素，尝试从子类型的所有者获取
        Element* owner = subtype->owner();
        if (owner) {
            owner->addOwnedRelationship(subclassification);
            qDebug() << "[ModelBuilderVisitor] Created Subclassification (via owner):"
                     << subtype->declaredName() << "->" << supertype->declaredName();
        } else {
            reportError("Cannot find owner for Subclassification relationship");
            delete subclassification;
        }
    }
}

void ModelBuilderVisitor::createSubsetting(Feature* subsettingFeature, const QStringList& subsettedFeatureNameParts)
{
    if (!subsettingFeature || subsettedFeatureNameParts.isEmpty()) {
        return;
    }
    
    validateTypeReference(subsettedFeatureNameParts, nullptr);
    QString subsettedFeatureName = subsettedFeatureNameParts.join("::");
    
    // 查找被子集化的特征
    Element* element = findElementByQualifiedName(subsettedFeatureNameParts);
    Feature* subsettedFeature = qobject_cast<Feature*>(element);
    if (!subsettedFeature) {
        reportWarning(QString("Subsetted feature not found for Subsetting: %1 (may be defined in another resource)")
                     .arg(subsettedFeatureName));
        // 即使找不到特征，也创建关系并保存名称
    }
    
    // 创建 Subsetting 关系
    Subsetting* subsetting = new Subsetting();
    subsetting->setSubsettingFeature(subsettingFeature);
    if (subsettedFeature) {
        subsetting->setSubsettedFeature(subsettedFeature);
    } else {
        subsetting->setSubsettedFeatureName(subsettedFeatureName);
    }
    
    // 添加到父元素的关系列表（而不是作为内容）
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedRelationship(subsetting);
        if (subsettedFeature) {
            qDebug() << "[ModelBuilderVisitor] Created Subsetting:"
                     << subsettingFeature->declaredName() << "->" << subsettedFeature->declaredName();
        } else {
            qDebug() << "[ModelBuilderVisitor] Created Subsetting (feature not found):"
                     << subsettingFeature->declaredName() << "->" << subsettedFeatureName;
        }
    } else {
        // 如果没有父元素，尝试从特征的所有者获取
        Element* owner = subsettingFeature->owner();
        if (owner) {
            owner->addOwnedRelationship(subsetting);
            if (subsettedFeature) {
                qDebug() << "[ModelBuilderVisitor] Created Subsetting (via owner):"
                         << subsettingFeature->declaredName() << "->" << subsettedFeature->declaredName();
            } else {
                qDebug() << "[ModelBuilderVisitor] Created Subsetting (via owner, feature not found):"
                         << subsettingFeature->declaredName() << "->" << subsettedFeatureName;
            }
        } else {
            reportError("Cannot find owner for Subsetting relationship");
            delete subsetting;
        }
    }
}

void ModelBuilderVisitor::createRedefinition(Feature* redefiningFeature, const QStringList& redefinedFeatureNameParts)
{
    if (!redefiningFeature || redefinedFeatureNameParts.isEmpty()) {
        return;
    }
    
    validateTypeReference(redefinedFeatureNameParts, nullptr);
    QString redefinedFeatureName = redefinedFeatureNameParts.join("::");
    
    // 查找被重定义的特征
    Element* element = findElementByQualifiedName(redefinedFeatureNameParts);
    Feature* redefinedFeature = qobject_cast<Feature*>(element);
    if (!redefinedFeature) {
        reportWarning(QString("Redefined feature not found for Redefinition: %1 (may be defined in another resource)")
                     .arg(redefinedFeatureName));
        // 即使找不到特征，也创建关系并保存名称
    }
    
    // 创建 Redefinition 关系
    Redefinition* redefinition = new Redefinition();
    redefinition->setRedefiningFeature(redefiningFeature);
    if (redefinedFeature) {
        redefinition->setRedefinedFeature(redefinedFeature);
    } else {
        redefinition->setRedefinedFeatureName(redefinedFeatureName);
    }
    
    // 添加到父元素的关系列表（而不是作为内容）
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedRelationship(redefinition);
        if (redefinedFeature) {
            qDebug() << "[ModelBuilderVisitor] Created Redefinition:"
                     << redefiningFeature->declaredName() << "->" << redefinedFeature->declaredName();
        } else {
            qDebug() << "[ModelBuilderVisitor] Created Redefinition (feature not found):"
                     << redefiningFeature->declaredName() << "->" << redefinedFeatureName;
        }
    } else {
        // 如果没有父元素，尝试从特征的所有者获取
        Element* owner = redefiningFeature->owner();
        if (owner) {
            owner->addOwnedRelationship(redefinition);
            if (redefinedFeature) {
                qDebug() << "[ModelBuilderVisitor] Created Redefinition (via owner):"
                         << redefiningFeature->declaredName() << "->" << redefinedFeature->declaredName();
            } else {
                qDebug() << "[ModelBuilderVisitor] Created Redefinition (via owner, feature not found):"
                         << redefiningFeature->declaredName() << "->" << redefinedFeatureName;
            }
        } else {
            reportError("Cannot find owner for Redefinition relationship");
            delete redefinition;
        }
    }
}

void ModelBuilderVisitor::createConjugation(Type* conjugatedType, const QStringList& originalTypeNameParts)
{
    if (!conjugatedType || originalTypeNameParts.isEmpty()) {
        return;
    }
    
    validateTypeReference(originalTypeNameParts, nullptr);
    QString originalTypeName = originalTypeNameParts.join("::");
    
    // 查找原始类型
    Type* originalType = findType(originalTypeNameParts);
    if (!originalType) {
        reportWarning(QString("Original type not found for Conjugation: %1 (may be defined in another resource)")
                     .arg(originalTypeName));
        // 即使找不到类型，也创建关系并保存名称
    }
    
    // 创建 Conjugation 关系
    Conjugation* conjugation = new Conjugation();
    conjugation->setConjugatedType(conjugatedType);
    if (originalType) {
        conjugation->setOriginalType(originalType);
    } else {
        conjugation->setOriginalTypeName(originalTypeName);
    }
    
    // 添加到父元素的关系列表（而不是作为内容）
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedRelationship(conjugation);
        if (originalType) {
            qDebug() << "[ModelBuilderVisitor] Created Conjugation:"
                     << conjugatedType->declaredName() << "->" << originalType->declaredName();
        } else {
            qDebug() << "[ModelBuilderVisitor] Created Conjugation (type not found):"
                     << conjugatedType->declaredName() << "->" << originalTypeName;
        }
    } else {
        // 如果没有父元素，尝试从类型的所有者获取
        Element* owner = conjugatedType->owner();
        if (owner) {
            owner->addOwnedRelationship(conjugation);
            if (originalType) {
                qDebug() << "[ModelBuilderVisitor] Created Conjugation (via owner):"
                         << conjugatedType->declaredName() << "->" << originalType->declaredName();
            } else {
                qDebug() << "[ModelBuilderVisitor] Created Conjugation (via owner, type not found):"
                         << conjugatedType->declaredName() << "->" << originalTypeName;
            }
        } else {
            reportError("Cannot find owner for Conjugation relationship");
            delete conjugation;
        }
    }
}

void ModelBuilderVisitor::createDependency(Element* clientElement, const QStringList& supplierElementNameParts)
{
    if (!clientElement || supplierElementNameParts.isEmpty()) {
        return;
    }
    
    validateTypeReference(supplierElementNameParts, nullptr);
    QString supplierElementName = supplierElementNameParts.join("::");
    
    // 查找被依赖的元素
    Element* supplierElement = findElementByQualifiedName(supplierElementNameParts);
    if (!supplierElement) {
        reportWarning(QString("Supplier element not found for Dependency: %1 (may be defined in another resource)")
                     .arg(supplierElementName));
        // 即使找不到元素，也创建关系并保存名称
    }
    
    // 创建 Dependency 关系
    Dependency* dependency = new Dependency();
    dependency->setClientElement(clientElement);
    if (supplierElement) {
        dependency->setSupplierElement(supplierElement);
    } else {
        dependency->setSupplierElementName(supplierElementName);
    }
    
    // 添加到父元素的关系列表（而不是作为内容）
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedRelationship(dependency);
        if (supplierElement) {
            qDebug() << "[ModelBuilderVisitor] Created Dependency:"
                     << clientElement->declaredName() << "->" << supplierElement->declaredName();
        } else {
            qDebug() << "[ModelBuilderVisitor] Created Dependency (element not found):"
                     << clientElement->declaredName() << "->" << supplierElementName;
        }
    } else {
        // 如果没有父元素，尝试从客户端元素的所有者获取
        Element* owner = clientElement->owner();
        if (owner) {
            owner->addOwnedRelationship(dependency);
            if (supplierElement) {
                qDebug() << "[ModelBuilderVisitor] Created Dependency (via owner):"
                         << clientElement->declaredName() << "->" << supplierElement->declaredName();
            } else {
                qDebug() << "[ModelBuilderVisitor] Created Dependency (via owner, element not found):"
                         << clientElement->declaredName() << "->" << supplierElementName;
            }
        } else {
            reportError("Cannot find owner for Dependency relationship");
            delete dependency;
        }
    }
}

// 关系类型访问方法实现
std::any ModelBuilderVisitor::visitRelationship(SysML::SysMLParser::RelationshipContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    if (ctx->subclassification()) {
        return visitSubclassification(ctx->subclassification());
    } else if (ctx->subsetting()) {
        return visitSubsetting(ctx->subsetting());
    } else if (ctx->redefinition()) {
        return visitRedefinition(ctx->redefinition());
    } else if (ctx->conjugation()) {
        return visitConjugation(ctx->conjugation());
    } else if (ctx->dependency()) {
        return visitDependency(ctx->dependency());
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitSubclassification(SysML::SysMLParser::SubclassificationContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取类型表达式
    auto typeExprCtx = ctx->typeExpression();
    if (!typeExprCtx) {
        reportError("Subclassification type expression is null", ctx);
        return nullptr;
    }
    
    QStringList typeParts = getQualifiedNameParts(typeExprCtx->qualifiedName());
    Type* subtype = findType(typeParts);
    if (!subtype) {
        reportError(QString("Subtype not found for Subclassification: %1").arg(typeParts.join("::")), ctx);
        return nullptr;
    }
    
    // 获取超类型
    auto supertypeCtx = ctx->qualifiedName(0);
    if (supertypeCtx) {
        QStringList supertypeParts = getQualifiedNameParts(supertypeCtx);
        createSubclassification(subtype, supertypeParts);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitSubsetting(SysML::SysMLParser::SubsettingContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取特征表达式
    auto featureExprCtx = ctx->featureExpression();
    if (!featureExprCtx) {
        reportError("Subsetting feature expression is null", ctx);
        return nullptr;
    }
    
    QStringList featureParts = getQualifiedNameParts(featureExprCtx->qualifiedName());
    Element* element = findElementByQualifiedName(featureParts);
    Feature* feature = qobject_cast<Feature*>(element);
    if (!feature) {
        reportError(QString("Feature not found for Subsetting: %1").arg(featureParts.join("::")), ctx);
        return nullptr;
    }
    
    // 获取被子集化的特征
    auto subsettedCtx = ctx->qualifiedName();
    if (subsettedCtx) {
        QStringList subsettedParts = getQualifiedNameParts(subsettedCtx);
        createSubsetting(feature, subsettedParts);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitRedefinition(SysML::SysMLParser::RedefinitionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取特征表达式
    auto featureExprCtx = ctx->featureExpression();
    if (!featureExprCtx) {
        reportError("Redefinition feature expression is null", ctx);
        return nullptr;
    }
    
    QStringList featureParts = getQualifiedNameParts(featureExprCtx->qualifiedName());
    Element* element = findElementByQualifiedName(featureParts);
    Feature* feature = qobject_cast<Feature*>(element);
    if (!feature) {
        reportError(QString("Feature not found for Redefinition: %1").arg(featureParts.join("::")), ctx);
        return nullptr;
    }
    
    // 获取被重定义的特征
    auto redefinedCtx = ctx->qualifiedName();
    if (redefinedCtx) {
        QStringList redefinedParts = getQualifiedNameParts(redefinedCtx);
        createRedefinition(feature, redefinedParts);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitConjugation(SysML::SysMLParser::ConjugationContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取类型表达式
    auto typeExprCtx = ctx->typeExpression();
    if (!typeExprCtx) {
        reportError("Conjugation type expression is null", ctx);
        return nullptr;
    }
    
    QStringList typeParts = getQualifiedNameParts(typeExprCtx->qualifiedName());
    Type* type = findType(typeParts);
    if (!type) {
        reportError(QString("Type not found for Conjugation: %1").arg(typeParts.join("::")), ctx);
        return nullptr;
    }
    
    // 获取原始类型
    auto originalCtx = ctx->qualifiedName();
    if (originalCtx) {
        QStringList originalParts = getQualifiedNameParts(originalCtx);
        createConjugation(type, originalParts);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitDependency(SysML::SysMLParser::DependencyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 处理两种形式的 dependency：
    // 1. elementExpression 'dependency' qualifiedName ';'
    // 2. 'dependency' 'from' qualifiedName 'to' qualifiedName ';'
    
    auto elementExprCtx = ctx->elementExpression();
    if (elementExprCtx) {
        // 第一种形式：elementExpression 'dependency' qualifiedName
        QStringList elementParts = getQualifiedNameParts(elementExprCtx->qualifiedName());
        Element* element = findElementByQualifiedName(elementParts);
        if (!element) {
            reportError(QString("Element not found for Dependency: %1").arg(elementParts.join("::")), ctx);
            return nullptr;
        }
        
        // 获取被依赖的元素
        auto qualifiedNames = ctx->qualifiedName();
        if (!qualifiedNames.empty()) {
            QStringList supplierParts = getQualifiedNameParts(qualifiedNames[0]);
            createDependency(element, supplierParts);
        }
    } else {
        // 第二种形式：'dependency' 'from' qualifiedName 'to' qualifiedName
        auto qualifiedNames = ctx->qualifiedName();
        if (qualifiedNames.size() >= 2) {
            QStringList fromParts = getQualifiedNameParts(qualifiedNames[0]);
            QStringList toParts = getQualifiedNameParts(qualifiedNames[1]);
            
            Element* fromElement = findElementByQualifiedName(fromParts);
            if (!fromElement) {
                reportError(QString("Element not found for Dependency 'from': %1").arg(fromParts.join("::")), ctx);
                return nullptr;
            }
            
            createDependency(fromElement, toParts);
        } else {
            reportError("Dependency requires 'from' and 'to' qualified names", ctx);
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitSubclassificationExpression(SysML::SysMLParser::SubclassificationExpressionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取当前正在处理的类型（从父上下文）
    // 这个方法在 visitDefinition 中被调用，需要从定义上下文中获取类型
    // TODO: 改进实现，从上下文栈中获取当前正在处理的类型
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitFeaturePrefix(SysML::SysMLParser::FeaturePrefixContext* ctx)
{
    // 这个方法由 visitAttribute 调用，传入 feature 参数
    // 这里只返回前缀类型，实际应用在 visitAttribute 中完成
    Q_UNUSED(ctx);
    return nullptr;
}

void ModelBuilderVisitor::visitFeaturePrefix(SysML::SysMLParser::FeaturePrefixContext* ctx, Feature* feature)
{
    if (!ctx || !feature) {
        return;
    }
    
    // 检查前缀类型并应用到特征
    if (ctx->getText() == "abstract") {
        feature->setIsAbstract(true);
    } else if (ctx->getText() == "const" || ctx->getText() == "constant") {
        feature->setIsConst(true);
    } else if (ctx->getText() == "derived") {
        feature->setIsDerived(true);
    } else if (ctx->getText() == "in") {
        feature->setDirection(Feature::DirectionIn);
    } else if (ctx->getText() == "out") {
        feature->setDirection(Feature::DirectionOut);
    } else if (ctx->getText() == "inout") {
        feature->setDirection(Feature::DirectionInOut);
    } else if (ctx->getText() == "ref") {
        feature->setIsRef(true);
    }
}

std::any ModelBuilderVisitor::visitConnectionUsage(SysML::SysMLParser::ConnectionUsageContext* ctx)
{
    if (!ctx) {
        reportError("ConnectionUsage context is null");
        return nullptr;
    }
    
    // 获取使用声明（可选）
    QString name = "UnnamedConnection";
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
            
            // 处理类型化
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        QString typeName = typeParts.join("::");
                        qDebug() << "[ModelBuilderVisitor] ConnectionUsage type:" << typeName;
                        validateTypeReference(typeParts, ctx);
                        // 保存类型名称
                        // 将在创建 ConnectionUsage 后处理
                    }
                }
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ConnectionUsage:" << name;
    
    ConnectionUsage* connUsage = new ConnectionUsage();
    connUsage->setDeclaredName(name);
    
    checkDuplicateName(connUsage, name, ctx);
    
    // 处理类型
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        QString typeName = typeParts.join("::");
                        connUsage->setTypeName(typeName);
                        validateTypeReference(typeParts, ctx);
                    }
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(connUsage);
    } else {
        if (m_resource) {
            m_resource->addContent(connUsage);
        } else {
            reportError("Resource is null, cannot add ConnectionUsage", ctx);
            delete connUsage;
            return nullptr;
        }
    }
    
    pushParent(connUsage);
    
    // 访问使用完成（包含值部分和使用体）
    auto completionCtx = ctx->usageCompletion();
    if (completionCtx) {
        // 处理 value part（如果有）
        processUsageCompletionValuePart(completionCtx);
        
        auto bodyCtx = completionCtx->usageBody();
        if (bodyCtx) {
            visitUsageBody(bodyCtx);
        }
    }
    
    popParent();
    
    return connUsage;
}

std::any ModelBuilderVisitor::visitAllocationUsage(SysML::SysMLParser::AllocationUsageContext* ctx)
{
    if (!ctx) {
        reportError("AllocationUsage context is null");
        return nullptr;
    }
    
    // 获取使用声明（可选）
    QString name = "UnnamedAllocation";
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
            
            // 处理类型化
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        QString typeName = typeParts.join("::");
                        qDebug() << "[ModelBuilderVisitor] AllocationUsage type:" << typeName;
                        validateTypeReference(typeParts, ctx);
                    }
                }
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating AllocationUsage:" << name;
    
    AllocationUsage* allocUsage = new AllocationUsage();
    allocUsage->setDeclaredName(name);
    
    checkDuplicateName(allocUsage, name, ctx);
    
    // 处理类型
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        QString typeName = typeParts.join("::");
                        allocUsage->setTypeName(typeName);
                        validateTypeReference(typeParts, ctx);
                    }
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(allocUsage);
    } else {
        if (m_resource) {
            m_resource->addContent(allocUsage);
        } else {
            reportError("Resource is null, cannot add AllocationUsage", ctx);
            delete allocUsage;
            return nullptr;
        }
    }
    
    pushParent(allocUsage);
    
    // 访问使用完成（包含值部分和使用体）
    auto completionCtx = ctx->usageCompletion();
    if (completionCtx) {
        // 处理 value part（如果有）
        processUsageCompletionValuePart(completionCtx);
        
        auto bodyCtx = completionCtx->usageBody();
        if (bodyCtx) {
            visitUsageBody(bodyCtx);
        }
    }
    
    popParent();
    
    return allocUsage;
}

std::any ModelBuilderVisitor::visitItemUsage(SysML::SysMLParser::ItemUsageContext* ctx)
{
    if (!ctx) {
        reportError("ItemUsage context is null");
        return nullptr;
    }
    
    // 获取使用前缀
    auto prefixCtx = ctx->occurrenceUsagePrefix();
    bool isDerived = false;
    bool isConstant = false;
    bool isRef = false;
    ItemUsage::Direction direction = ItemUsage::DirectionNone;
    ItemUsage::PortionKind portionKind = ItemUsage::PortionNone;
    
    if (prefixCtx) {
        // 处理 endUsagePrefix
        auto endPrefixCtx = prefixCtx->endUsagePrefix();
        if (endPrefixCtx) {
            // end 前缀暂时不处理（后续可扩展）
        }
        
        // 处理 basicUsagePrefix
        auto basicPrefixCtx = prefixCtx->basicUsagePrefix();
        if (basicPrefixCtx) {
            // 处理 refPrefix
            auto refPrefixCtx = basicPrefixCtx->refPrefix();
            if (refPrefixCtx) {
                // 处理 featureDirection
                auto directionCtx = refPrefixCtx->featureDirection();
                if (directionCtx) {
                    QString dirText = QString::fromStdString(directionCtx->getText());
                    if (dirText == "in") {
                        direction = ItemUsage::DirectionIn;
                    } else if (dirText == "out") {
                        direction = ItemUsage::DirectionOut;
                    } else if (dirText == "inout") {
                        direction = ItemUsage::DirectionInOut;
                    }
                }
                
                // 检查是否有 derived
                QString refPrefixText = QString::fromStdString(refPrefixCtx->getText());
                if (refPrefixText.contains("derived")) {
                    isDerived = true;
                }
                
                // 检查 constant
                if (refPrefixText.contains("constant")) {
                    isConstant = true;
                }
                
                // 检查 ref
                if (refPrefixText.contains("ref")) {
                    isRef = true;
                }
            } else {
                // 没有 refPrefix，直接检查 basicUsagePrefix
                QString basicPrefixText = QString::fromStdString(basicPrefixCtx->getText());
                if (basicPrefixText.contains("derived")) {
                    isDerived = true;
                }
                if (basicPrefixText.contains("constant")) {
                    isConstant = true;
                }
            }
        }
        
        // 处理 portionKind
        auto portionCtx = prefixCtx->portionKind();
        if (portionCtx) {
            QString portionText = QString::fromStdString(portionCtx->getText());
            if (portionText == "snapshot") {
                portionKind = ItemUsage::PortionSnapshot;
            } else if (portionText == "timeslice") {
                portionKind = ItemUsage::PortionTimeslice;
            }
        }
    }
    
    // 获取使用声明（可选）
    QString name = "UnnamedItem";
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
            
            // 处理类型化
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        QString typeName = typeParts.join("::");
                        qDebug() << "[ModelBuilderVisitor] ItemUsage type:" << typeName;
                        validateTypeReference(typeParts, ctx);
                    }
                }
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ItemUsage:" << name;
    
    ItemUsage* itemUsage = new ItemUsage();
    itemUsage->setDeclaredName(name);
    
    // 设置前缀属性
    itemUsage->setIsDerived(isDerived);
    itemUsage->setIsConstant(isConstant);
    itemUsage->setIsRef(isRef);
    itemUsage->setDirection(direction);
    itemUsage->setPortionKind(portionKind);
    
    checkDuplicateName(itemUsage, name, ctx);
    
    // 处理类型
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        QString typeName = typeParts.join("::");
                        itemUsage->setTypeName(typeName);
                        validateTypeReference(typeParts, ctx);
                    }
                }
            }
        }
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(itemUsage);
    } else {
        if (m_resource) {
            m_resource->addContent(itemUsage);
        } else {
            reportError("Resource is null, cannot add ItemUsage", ctx);
            delete itemUsage;
            return nullptr;
        }
    }
    
    pushParent(itemUsage);
    
    // 访问使用完成（包含值部分和使用体）
    auto completionCtx = ctx->usageCompletion();
    if (completionCtx) {
        // 处理 value part（如果有）
        processUsageCompletionValuePart(completionCtx);
        
        auto bodyCtx = completionCtx->usageBody();
        if (bodyCtx) {
            visitUsageBody(bodyCtx);
        }
    }
    
    popParent();
    
    return itemUsage;
}

std::any ModelBuilderVisitor::visitStatement(SysML::SysMLParser::StatementContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    if (ctx->bindStatement()) {
        return visitBindStatement(ctx->bindStatement());
    } else if (ctx->connectStatement()) {
        return visitConnectStatement(ctx->connectStatement());
    } else if (ctx->allocateStatement()) {
        return visitAllocateStatement(ctx->allocateStatement());
    } else if (ctx->endStatement()) {
        return visitEndStatement(ctx->endStatement());
    } else if (ctx->flowStatement()) {
        return visitFlowStatement(ctx->flowStatement());
    } else if (ctx->performStatement()) {
        return visitPerformStatement(ctx->performStatement());
    } else if (ctx->assertStatement()) {
        return visitAssertStatement(ctx->assertStatement());
    } else if (ctx->requireStatement()) {
        return visitRequireStatement(ctx->requireStatement());
    } else if (ctx->assumeStatement()) {
        return visitAssumeStatement(ctx->assumeStatement());
    } else if (ctx->redefinesStatement()) {
        return visitRedefinesStatement(ctx->redefinesStatement());
    } else if (ctx->constraintStatement()) {
        return visitConstraintStatement(ctx->constraintStatement());
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitBindStatement(SysML::SysMLParser::BindStatementContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取两个连接器端点成员（bind 语句的语法是 bind end1 = end2）
    auto ends = ctx->connectorEndMember();
    if (ends.size() < 2) {
        return nullptr;
    }
    
    QString sourceEnd = getConnectorEndText(ends[0]);
    QString targetEnd = getConnectorEndText(ends[1]);
    
    QString displayName = "bind<" + sourceEnd + "," + targetEnd + ">";
    qDebug() << "[ModelBuilderVisitor] Creating BindStatement:" << displayName;
    
    BindStatement* bindStmt = new BindStatement();
    bindStmt->setSourceEnd(sourceEnd);
    bindStmt->setTargetEnd(targetEnd);
    bindStmt->setDeclaredName(displayName);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(bindStmt);
    } else {
        if (m_resource) {
            m_resource->addContent(bindStmt);
        } else {
            reportError("Resource is null, cannot add BindStatement", ctx);
            delete bindStmt;
            return nullptr;
        }
    }
    
    return bindStmt;
}

std::any ModelBuilderVisitor::visitConnectStatement(SysML::SysMLParser::ConnectStatementContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取连接器部分
    auto connectorCtx = ctx->connectorPart();
    if (!connectorCtx) {
        return nullptr;
    }
    
    QStringList ends;
    bool isBinary = false;
    
    // 处理二元连接
    auto binaryCtx = connectorCtx->binaryConnectorPart();
    if (binaryCtx) {
        isBinary = true;
        auto endMembers = binaryCtx->connectorEndMember();
        for (auto endMember : endMembers) {
            ends.append(getConnectorEndText(endMember));
        }
    } else {
        // 处理 n元连接
        auto naryCtx = connectorCtx->naryConnectorPart();
        if (naryCtx) {
            isBinary = false;
            auto endMembers = naryCtx->connectorEndMember();
            for (auto endMember : endMembers) {
                ends.append(getConnectorEndText(endMember));
            }
        }
    }
    
    QString displayName = "connect<" + ends.join(",") + ">";
    qDebug() << "[ModelBuilderVisitor] Creating ConnectStatement:" << displayName;
    
    ConnectStatement* connectStmt = new ConnectStatement();
    connectStmt->setEnds(ends);
    connectStmt->setIsBinary(isBinary);
    connectStmt->setDeclaredName(displayName);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(connectStmt);
    } else {
        if (m_resource) {
            m_resource->addContent(connectStmt);
        } else {
            reportError("Resource is null, cannot add ConnectStatement", ctx);
            delete connectStmt;
            return nullptr;
        }
    }
    
    // 如果 connect 语句有体，访问定义体项
    auto bodyItems = ctx->definitionBodyItem();
    if (!bodyItems.empty()) {
        pushParent(connectStmt);
        for (auto itemCtx : bodyItems) {
            if (itemCtx) {
                // 处理定义成员
                auto defMemberCtx = itemCtx->definitionMember();
                if (defMemberCtx) {
                    auto defCtx = defMemberCtx->definition();
                    if (defCtx) {
                        visitDefinition(defCtx);
                    }
                }
                
                // 处理使用成员
                auto usageMemberCtx = itemCtx->usageMember();
                if (usageMemberCtx) {
                    auto usageCtx = usageMemberCtx->usage();
                    if (usageCtx) {
                        visitUsage(usageCtx);
                    }
                }
                
                // 处理属性
                auto attributeCtx = itemCtx->attribute();
                if (attributeCtx) {
                    visitAttribute(attributeCtx);
                }
                
                // 处理导入语句
                auto importCtx = itemCtx->importStatement();
                if (importCtx) {
                    visitImportStatement(importCtx);
                }
                
                // 处理关系
                auto relationshipCtx = itemCtx->relationship();
                if (relationshipCtx) {
                    visitRelationship(relationshipCtx);
                }
                
                // 处理语句
                auto statementCtx = itemCtx->statement();
                if (statementCtx) {
                    visitStatement(statementCtx);
                }
                
                // 处理 metadata annotation
                auto metadataAnnotationCtx = itemCtx->metadataAnnotation();
                if (metadataAnnotationCtx) {
                    qDebug() << "[ModelBuilderVisitor] visitConnectStatement: found metadataAnnotation";
                    visitMetadataAnnotation(metadataAnnotationCtx);
                }
            }
        }
        popParent();
    }
    
    qDebug() << "[ModelBuilderVisitor] visitConnectStatement: finished, annotations count =" << connectStmt->ownedAnnotations().size();
    return connectStmt;
}

std::any ModelBuilderVisitor::visitAllocateStatement(SysML::SysMLParser::AllocateStatementContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取分配部分
    auto allocationCtx = ctx->allocationPart();
    if (!allocationCtx) {
        return nullptr;
    }
    
    QStringList sourceEnds;
    QString targetEnd;
    bool isBinary = false;
    
    // 处理二元分配
    auto binaryCtx = allocationCtx->binaryAllocationPart();
    if (binaryCtx) {
        isBinary = true;
        auto endMembers = binaryCtx->connectorEndMember();
        if (endMembers.size() >= 2) {
            sourceEnds.append(getConnectorEndText(endMembers[0]));
            targetEnd = getConnectorEndText(endMembers[1]);
        }
    } else {
        // 处理 n元分配
        auto naryCtx = allocationCtx->naryAllocationPart();
        if (naryCtx) {
            isBinary = false;
            // n元分配语法：'(' connectorEndMember (',' connectorEndMember)+ ')' 'to' connectorEndMember
            // 获取括号内的源端点
            auto sourceEndMembers = naryCtx->connectorEndMember();
            // 最后一个成员是目标（'to' 后面的），前面的都是源
            if (sourceEndMembers.size() >= 2) {
                // 所有成员都在一个列表中，最后一个（索引 size-1）是目标
                for (size_t i = 0; i < sourceEndMembers.size() - 1; ++i) {
                    sourceEnds.append(getConnectorEndText(sourceEndMembers[i]));
                }
                targetEnd = getConnectorEndText(sourceEndMembers[sourceEndMembers.size() - 1]);
            }
        }
    }
    
    QString displayName = "allocate<" + sourceEnds.join(",") + "," + targetEnd + ">";
    qDebug() << "[ModelBuilderVisitor] Creating AllocateStatement:" << displayName;
    
    AllocateStatement* allocateStmt = new AllocateStatement();
    allocateStmt->setSourceEnds(sourceEnds);
    allocateStmt->setTargetEnd(targetEnd);
    allocateStmt->setIsBinary(isBinary);
    allocateStmt->setDeclaredName(displayName);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(allocateStmt);
    } else {
        if (m_resource) {
            m_resource->addContent(allocateStmt);
        } else {
            reportError("Resource is null, cannot add AllocateStatement", ctx);
            delete allocateStmt;
            return nullptr;
        }
    }
    
    // 如果 allocate 语句有体，访问定义体项
    auto bodyItems = ctx->definitionBodyItem();
    if (!bodyItems.empty()) {
        pushParent(allocateStmt);
        for (auto itemCtx : bodyItems) {
            if (itemCtx) {
                // 处理定义成员
                auto defMemberCtx = itemCtx->definitionMember();
                if (defMemberCtx) {
                    auto defCtx = defMemberCtx->definition();
                    if (defCtx) {
                        visitDefinition(defCtx);
                    }
                }
                
                // 处理使用成员
                auto usageMemberCtx = itemCtx->usageMember();
                if (usageMemberCtx) {
                    auto usageCtx = usageMemberCtx->usage();
                    if (usageCtx) {
                        visitUsage(usageCtx);
                    }
                }
                
                // 处理属性
                auto attributeCtx = itemCtx->attribute();
                if (attributeCtx) {
                    visitAttribute(attributeCtx);
                }
                
                // 处理导入语句
                auto importCtx = itemCtx->importStatement();
                if (importCtx) {
                    visitImportStatement(importCtx);
                }
                
                // 处理关系
                auto relationshipCtx = itemCtx->relationship();
                if (relationshipCtx) {
                    visitRelationship(relationshipCtx);
                }
                
                // 处理语句
                auto statementCtx = itemCtx->statement();
                if (statementCtx) {
                    visitStatement(statementCtx);
                }
            }
        }
        popParent();
    }
    
    return allocateStmt;
}

std::any ModelBuilderVisitor::visitEndStatement(SysML::SysMLParser::EndStatementContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取标签（#identifier）
    QString tag;
    auto tagCtx = ctx->identifier();
    if (tagCtx) {
        tag = getText(tagCtx->IDENTIFIER());
    }
    
    QString name;
    QString typeName;
    bool isNegated = false;
    
    // 处理新语法：'end' ('#' identifier)? (name referencesKeyword? ownedReferenceSubsetting | referencesKeyword ownedReferenceSubsetting) ';'
    auto refSubsetCtx = ctx->ownedReferenceSubsetting();
    bool usesReferences = false;
    if (refSubsetCtx) {
        // 有 ownedReferenceSubsetting，使用新语法
        // 检查是否有 referencesKeyword（::> 或 references）
        auto refKeywordCtx = ctx->referencesKeyword();
        if (refKeywordCtx) {
            usesReferences = true;
        } else {
            // 检查上下文文本是否包含 ::>
            QString ctxText = QString::fromStdString(ctx->getText());
            usesReferences = ctxText.contains("::>");
        }
        
        auto nameCtx = ctx->name();
        if (nameCtx) {
            name = getText(nameCtx->identifier()->IDENTIFIER());
        }
        
        // 从 ownedReferenceSubsetting 提取引用路径
        if (refSubsetCtx->qualifiedName()) {
            QStringList typeParts = getQualifiedNameParts(refSubsetCtx->qualifiedName());
            typeName = typeParts.join("::");
        } else if (refSubsetCtx->memberAccessExpression()) {
            // 处理 memberAccessExpression（如 vehicleSpecification.vehicleMassRequirement）
            QString refText = QString::fromStdString(refSubsetCtx->memberAccessExpression()->getText());
            typeName = refText;
        } else if (refSubsetCtx->identifier()) {
            typeName = getText(refSubsetCtx->identifier()->IDENTIFIER());
        }
    } else {
        // 处理旧语法：'end' ('#' identifier)? name ':' ('~')? qualifiedName ';'
        auto nameCtx = ctx->name();
        if (nameCtx) {
            name = getText(nameCtx->identifier()->IDENTIFIER());
        }
        
        auto qnameCtx = ctx->qualifiedName();
        if (qnameCtx) {
            QStringList typeParts = getQualifiedNameParts(qnameCtx);
            typeName = typeParts.join("::");
        }
        
        // 检查是否否定
        QString ctxText = QString::fromStdString(ctx->getText());
        isNegated = ctxText.contains(": ~") || ctxText.contains(":~");
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating EndStatement:" << name << ":" << typeName << "tag:" << tag;
    
    EndStatement* endStmt = new EndStatement();
    endStmt->setDeclaredName(name);
    endStmt->setTag(tag);
    endStmt->setTypeName(typeName);
    endStmt->setIsNegated(isNegated);
    endStmt->setUsesReferences(usesReferences);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(endStmt);
    } else {
        if (m_resource) {
            m_resource->addContent(endStmt);
        } else {
            reportError("Resource is null, cannot add EndStatement", ctx);
            delete endStmt;
            return nullptr;
        }
    }
    
    return endStmt;
}

// 辅助方法：获取连接器端点文本
QString ModelBuilderVisitor::getConnectorEndText(SysML::SysMLParser::ConnectorEndMemberContext* ctx)
{
    if (!ctx) {
        return QString();
    }
    
    QString result;
    
    // 获取多重性
    auto multCtx = ctx->ownedMultiplicity();
    if (multCtx) {
        QString multText = QString::fromStdString(multCtx->getText());
        if (!multText.isEmpty()) {
            result += multText;
        }
    }
    
    // 获取名称（优先使用名称）
    auto nameCtx = ctx->name();
    if (nameCtx) {
        QString name = getText(nameCtx->identifier()->IDENTIFIER());
        if (!name.isEmpty()) {
            if (!result.isEmpty()) {
                result += " ";
            }
            result += name;
            
            // 如果有引用关键字和引用子集化，也添加
            auto refKeywordCtx = ctx->referencesKeyword();
            if (refKeywordCtx) {
                QString refKeyword = QString::fromStdString(refKeywordCtx->getText());
                if (!refKeyword.isEmpty()) {
                    result += " " + refKeyword;
                }
            }
            
            auto refSubsetCtx = ctx->ownedReferenceSubsetting();
            if (refSubsetCtx) {
                QString refText;
                // 优先检查 memberAccessExpression（点号访问）
                auto memberAccessCtx = refSubsetCtx->memberAccessExpression();
                if (memberAccessCtx) {
                    // 直接获取点号访问表达式的文本（如 transferTorque.driveshaftTorque）
                    refText = QString::fromStdString(memberAccessCtx->getText());
                } else {
                    auto qnameCtx = refSubsetCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList parts = getQualifiedNameParts(qnameCtx);
                        refText = parts.join("::");
                    } else {
                        auto idCtx = refSubsetCtx->identifier();
                        if (idCtx) {
                            refText = getText(idCtx->IDENTIFIER());
                        }
                    }
                }
                if (!refText.isEmpty()) {
                    result += " " + refText;
                }
            }
            
            return result;
        }
    }
    
    // 如果没有名称，使用引用子集化（这是点号访问的主要情况）
    auto refSubsetCtx = ctx->ownedReferenceSubsetting();
    if (refSubsetCtx) {
        QString refText;
        qDebug() << "[ModelBuilderVisitor] getConnectorEndText: ownedReferenceSubsetting found, text:" << QString::fromStdString(refSubsetCtx->getText());
        
        // 注意：memberAccessExpression 可能是 qualifiedName 的一部分，也可能是 ownedReferenceSubsetting 的直接子节点
        // 先检查 ownedReferenceSubsetting 的直接 memberAccessExpression
        auto memberAccessCtx = refSubsetCtx->memberAccessExpression();
        if (memberAccessCtx) {
            // 直接获取点号访问表达式的文本（如 transferTorque.driveshaftTorque）
            refText = QString::fromStdString(memberAccessCtx->getText());
            qDebug() << "[ModelBuilderVisitor] getConnectorEndText: found memberAccessExpression (direct):" << refText;
        } else {
            // 检查 qualifiedName（它可能包含 memberAccessExpression）
            auto qnameCtx = refSubsetCtx->qualifiedName();
            if (qnameCtx) {
                // qualifiedName 可能包含 memberAccessExpression（点号访问）
                auto qnameMemberAccessCtx = qnameCtx->memberAccessExpression();
                if (qnameMemberAccessCtx) {
                    // 直接获取点号访问表达式的文本
                    refText = QString::fromStdString(qnameMemberAccessCtx->getText());
                    qDebug() << "[ModelBuilderVisitor] getConnectorEndText: found memberAccessExpression (in qualifiedName):" << refText;
                } else {
                    // qualifiedName 是普通的命名空间限定名（使用 ::）
                    QStringList parts = getQualifiedNameParts(qnameCtx);
                    refText = parts.join("::");
                    qDebug() << "[ModelBuilderVisitor] getConnectorEndText: found qualifiedName:" << refText;
                }
            } else {
                // 检查 identifier（简单标识符）
                auto idCtx = refSubsetCtx->identifier();
                if (idCtx) {
                    refText = getText(idCtx->IDENTIFIER());
                    qDebug() << "[ModelBuilderVisitor] getConnectorEndText: found identifier:" << refText;
                } else {
                    qDebug() << "[ModelBuilderVisitor] getConnectorEndText: no qualifiedName or identifier found in ownedReferenceSubsetting";
                }
            }
        }
        if (!refText.isEmpty()) {
            if (!result.isEmpty()) {
                result += " ";
            }
            result += refText;
        } else {
            qDebug() << "[ModelBuilderVisitor] getConnectorEndText: refText is empty, ctx text:" << QString::fromStdString(ctx->getText());
            qDebug() << "[ModelBuilderVisitor] getConnectorEndText: ownedReferenceSubsetting text:" << QString::fromStdString(refSubsetCtx->getText());
        }
    } else {
        qDebug() << "[ModelBuilderVisitor] getConnectorEndText: no ownedReferenceSubsetting, ctx text:" << QString::fromStdString(ctx->getText());
        qDebug() << "[ModelBuilderVisitor] getConnectorEndText: has name:" << (ctx->name() != nullptr);
        qDebug() << "[ModelBuilderVisitor] getConnectorEndText: has ownedMultiplicity:" << (ctx->ownedMultiplicity() != nullptr);
    }
    
    QString finalResult = result.isEmpty() ? "?" : result;
    qDebug() << "[ModelBuilderVisitor] getConnectorEndText: returning:" << finalResult;
    return finalResult;
}

std::any ModelBuilderVisitor::visitFlowDefinition(SysML::SysMLParser::FlowDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("FlowDefinition context is null");
        return nullptr;
    }
    
    // 获取定义前缀
    auto prefixCtx = ctx->occurrenceDefinitionPrefix();
    bool isAbstract = false;
    bool isVariation = false;
    bool isIndividual = false;
    
    if (prefixCtx) {
        auto basicPrefixCtx = prefixCtx->basicDefinitionPrefix();
        if (basicPrefixCtx) {
            QString prefixText = QString::fromStdString(basicPrefixCtx->getText());
            if (prefixText == "abstract") {
                isAbstract = true;
            } else if (prefixText == "variation") {
                isVariation = true;
            }
        }
        if (prefixCtx->emptyMultiplicityMember()) {
            isIndividual = true;
        }
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("FlowDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedFlow";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating FlowDefinition:" << name;
    
    FlowDefinition* flowDef = new FlowDefinition();
    flowDef->setDeclaredName(name);
    
    if (isAbstract) {
        flowDef->setIsAbstract(true);
    }
    if (isVariation) {
        flowDef->setIsVariation(true);
    }
    if (isIndividual) {
        flowDef->setIsIndividual(true);
    }
    
    checkDuplicateName(flowDef, name, ctx);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(flowDef);
    } else {
        if (m_resource) {
            m_resource->addContent(flowDef);
        } else {
            reportError("Resource is null, cannot add FlowDefinition", ctx);
            delete flowDef;
            return nullptr;
        }
    }
    
    // 处理子类化部分
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(flowDef, supertypeParts);
                }
            }
        }
    }
    
    pushParent(flowDef);
    
    auto bodyCtx = ctx->definitionBody();
    if (bodyCtx) {
        visitDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return flowDef;
}

std::any ModelBuilderVisitor::visitFlowStatement(SysML::SysMLParser::FlowStatementContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    auto flowDeclCtx = ctx->flowDeclaration();
    if (!flowDeclCtx) {
        return nullptr;
    }
    
    QString fromEnd;
    QString toEnd;
    QString payload;
    QString flowName;
    QString flowType;
    bool hasFromKeyword = false;
    
    // 处理流声明
    // 形式1: usageDeclaration? valuePart? ('of' payloadFeatureMember)? ('from' flowEndMember 'to' flowEndMember)?
    // 形式2: flowEndMember 'to' flowEndMember
    auto usageDeclCtx = flowDeclCtx->usageDeclaration();
    auto payloadCtx = flowDeclCtx->payloadFeatureMember();
    auto flowEndMembers = flowDeclCtx->flowEndMember();
    
    qDebug() << "[ModelBuilderVisitor] visitFlowStatement: flowEndMembers.size() =" << flowEndMembers.size();
    qDebug() << "[ModelBuilderVisitor] visitFlowStatement: flowDeclaration text:" << QString::fromStdString(flowDeclCtx->getText());
    
    // 处理使用声明（flowName : FlowType）
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    flowName = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    flowName = getText(identifiers[0]->IDENTIFIER());
                }
            }
            
            // 处理类型
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        flowType = typeParts.join("::");
                    }
                }
            }
        }
    }
    
    // 处理端点：形式1和形式2都可能有 flowEndMembers
    // 形式1: ('from' flowEndMember 'to' flowEndMember)? - 可选，如果有则包含两个端点
    // 形式2: flowEndMember 'to' flowEndMember - 必须包含两个端点
    if (flowEndMembers.size() >= 2) {
        // 有 'from' 关键字的情况（形式1）或直接 'to' 的情况（形式2）
        // 通过检查是否有 usageDeclaration 来判断是否是形式1
        if (usageDeclCtx) {
            hasFromKeyword = true;  // 形式1，有 'from' 关键字
        }
        
        auto connectorEnd0 = flowEndMembers[0]->connectorEndMember();
        auto connectorEnd1 = flowEndMembers[1]->connectorEndMember();
        if (connectorEnd0) {
            fromEnd = getConnectorEndText(connectorEnd0);
            qDebug() << "[ModelBuilderVisitor] FlowStatement fromEnd:" << fromEnd;
        }
        if (connectorEnd1) {
            toEnd = getConnectorEndText(connectorEnd1);
            qDebug() << "[ModelBuilderVisitor] FlowStatement toEnd:" << toEnd;
        }
    } else if (flowEndMembers.size() == 1) {
        // 如果只有一个端点，可能是形式1中的 'from' 部分（但这种情况应该不会发生）
        hasFromKeyword = true;
        auto connectorEnd0 = flowEndMembers[0]->connectorEndMember();
        if (connectorEnd0) {
            fromEnd = getConnectorEndText(connectorEnd0);
            qDebug() << "[ModelBuilderVisitor] FlowStatement fromEnd (form1):" << fromEnd;
        }
    }
    
    // 处理载荷
    if (payloadCtx) {
        // 简化处理：提取载荷文本
        payload = QString::fromStdString(payloadCtx->getText());
    }
    
    QString displayName = "flow<" + fromEnd + "," + toEnd + ">";
    if (!payload.isEmpty()) {
        displayName += " of " + payload;
    }
    qDebug() << "[ModelBuilderVisitor] Creating FlowStatement:" << displayName;
    
    FlowStatement* flowStmt = new FlowStatement();
    flowStmt->setFromEnd(fromEnd);
    flowStmt->setToEnd(toEnd);
    flowStmt->setPayload(payload);
    flowStmt->setFlowName(flowName);
    flowStmt->setFlowType(flowType);
    flowStmt->setHasFromKeyword(hasFromKeyword);
    flowStmt->setDeclaredName(displayName);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(flowStmt);
    } else {
        if (m_resource) {
            m_resource->addContent(flowStmt);
        } else {
            reportError("Resource is null, cannot add FlowStatement", ctx);
            delete flowStmt;
            return nullptr;
        }
    }
    
    return flowStmt;
}

std::any ModelBuilderVisitor::visitPerformStatement(SysML::SysMLParser::PerformStatementContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    QString actionRef;
    bool isActionKeyword = false;
    QString actionName;
    QString actionType;
    QString specializationType;
    
    // 处理 perform 语句
    // 形式1: 'perform' ownedReferenceSubsetting featureSpecializationPart? valuePart? usageBody
    auto refSubsetCtx = ctx->ownedReferenceSubsetting();
    if (refSubsetCtx) {
        // 提取引用子集化文本
        actionRef = QString::fromStdString(refSubsetCtx->getText());
        
        // 处理特征特殊化部分（:> BaseAction）
        auto specPartCtx = ctx->featureSpecializationPart();
        if (specPartCtx) {
            auto typingCtx = specPartCtx->typingExpression();
            if (typingCtx) {
                auto qnameCtx = typingCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList typeParts = getQualifiedNameParts(qnameCtx);
                    specializationType = typeParts.join("::");
                }
            }
        }
    }
    
    // 形式2: 'perform' 'action' usageDeclaration? valuePart? usageBody
    // 检查是否有 'action' 关键字（通过检查上下文文本）
    QString ctxText = QString::fromStdString(ctx->getText());
    if (ctxText.contains("action")) {
        isActionKeyword = true;
        auto usageDeclCtx = ctx->usageDeclaration();
        if (usageDeclCtx) {
            auto featureDeclCtx = usageDeclCtx->featureDeclaration();
            if (featureDeclCtx) {
                // 提取动作名称
                auto identCtx = featureDeclCtx->identification();
                if (identCtx) {
                    auto identifiers = identCtx->identifier();
                    if (identifiers.size() >= 2) {
                        actionName = getText(identifiers[1]->IDENTIFIER());
                    } else if (identifiers.size() == 1) {
                        actionName = getText(identifiers[0]->IDENTIFIER());
                    }
                }
                
                // 提取动作类型
                auto specCtx = featureDeclCtx->featureSpecializationPart();
                if (specCtx) {
                    auto typingCtx = specCtx->typingExpression();
                    if (typingCtx) {
                        auto qnameCtx = typingCtx->qualifiedName();
                        if (qnameCtx) {
                            QStringList typeParts = getQualifiedNameParts(qnameCtx);
                            actionType = typeParts.join("::");
                        }
                    }
                }
            }
        }
    }
    
    QString displayName = "perform<" + actionRef + ">";
    qDebug() << "[ModelBuilderVisitor] Creating PerformStatement:" << displayName;
    
    PerformStatement* performStmt = new PerformStatement();
    performStmt->setActionReference(actionRef);
    performStmt->setIsActionKeyword(isActionKeyword);
    performStmt->setActionName(actionName);
    performStmt->setActionType(actionType);
    performStmt->setSpecializationType(specializationType);
    performStmt->setDeclaredName(displayName);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(performStmt);
    } else {
        if (m_resource) {
            m_resource->addContent(performStmt);
        } else {
            reportError("Resource is null, cannot add PerformStatement", ctx);
            delete performStmt;
            return nullptr;
        }
    }
    
    // 如果 perform 语句有体，访问使用体
    auto bodyCtx = ctx->usageBody();
    if (bodyCtx) {
        pushParent(performStmt);
        visitUsageBody(bodyCtx);
        popParent();
    }
    
    return performStmt;
}

std::any ModelBuilderVisitor::visitAssertStatement(SysML::SysMLParser::AssertStatementContext* ctx)
{
    if (!ctx) {
        reportError("AssertStatement context is null");
        return nullptr;
    }
    
    // 获取表达式
    auto exprCtx = ctx->expression();
    QString expression = exprCtx ? QString::fromStdString(exprCtx->getText()) : QString();
    
    qDebug() << "[ModelBuilderVisitor] Creating AssertStatement:" << expression;
    
    AssertStatement* assertStmt = new AssertStatement();
    assertStmt->setExpression(expression);
    assertStmt->setDeclaredName("assert " + expression);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(assertStmt);
    } else {
        if (m_resource) {
            m_resource->addContent(assertStmt);
        } else {
            reportError("Resource is null, cannot add AssertStatement", ctx);
            delete assertStmt;
            return nullptr;
        }
    }
    
    return assertStmt;
}

std::any ModelBuilderVisitor::visitRequireStatement(SysML::SysMLParser::RequireStatementContext* ctx)
{
    if (!ctx) {
        reportError("RequireStatement context is null");
        return nullptr;
    }
    
    QString expression;
    
    // 检查是否是 require constraint {expression} 形式
    // 注意：ANTLR4 生成的代码中，如果有多个选择，会为每个选择生成单独的方法
    // 我们需要检查是否有 expression() 方法（第一种形式）或检查上下文文本
    auto exprCtx = ctx->expression();
    if (exprCtx) {
        // 第一种形式：require expression;
        expression = QString::fromStdString(exprCtx->getText());
    } else {
        // 第二种形式：require constraint {expression}
        // 从上下文文本中提取表达式（在大括号中）
        QString ctxText = QString::fromStdString(ctx->getText());
        // 查找大括号中的内容
        int startIdx = ctxText.indexOf('{');
        int endIdx = ctxText.lastIndexOf('}');
        if (startIdx >= 0 && endIdx > startIdx) {
            expression = ctxText.mid(startIdx + 1, endIdx - startIdx - 1).trimmed();
        } else {
            expression = ctxText; // 如果找不到大括号，使用整个文本
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating RequireStatement:" << expression;
    
    RequireStatement* requireStmt = new RequireStatement();
    requireStmt->setExpression(expression);
    requireStmt->setDeclaredName("require " + expression);
    
    // 检查是否包含 constraint 关键字
    QString ctxText = QString::fromStdString(ctx->getText());
    bool hasConstraintKeyword = ctxText.contains("constraint") && 
                                 ctxText.contains("{") && 
                                 ctxText.contains("}");
    requireStmt->setHasConstraintKeyword(hasConstraintKeyword);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(requireStmt);
    } else {
        if (m_resource) {
            m_resource->addContent(requireStmt);
        } else {
            reportError("Resource is null, cannot add RequireStatement", ctx);
            delete requireStmt;
            return nullptr;
        }
    }
    
    return requireStmt;
}

std::any ModelBuilderVisitor::visitAssumeStatement(SysML::SysMLParser::AssumeStatementContext* ctx)
{
    if (!ctx) {
        reportError("AssumeStatement context is null");
        return nullptr;
    }
    
    QString expression;
    
    // 检查是否是 assume constraint {expression} 形式
    auto exprCtx = ctx->expression();
    if (exprCtx) {
        // 第一种形式：assume expression;
        expression = QString::fromStdString(exprCtx->getText());
    } else {
        // 第二种形式：assume constraint {expression}
        // 从上下文文本中提取表达式（在大括号中）
        QString ctxText = QString::fromStdString(ctx->getText());
        // 查找大括号中的内容
        int startIdx = ctxText.indexOf('{');
        int endIdx = ctxText.lastIndexOf('}');
        if (startIdx >= 0 && endIdx > startIdx) {
            expression = ctxText.mid(startIdx + 1, endIdx - startIdx - 1).trimmed();
        } else {
            expression = ctxText; // 如果找不到大括号，使用整个文本
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating AssumeStatement:" << expression;
    
    AssumeStatement* assumeStmt = new AssumeStatement();
    assumeStmt->setExpression(expression);
    assumeStmt->setDeclaredName("assume " + expression);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(assumeStmt);
    } else {
        if (m_resource) {
            m_resource->addContent(assumeStmt);
        } else {
            reportError("Resource is null, cannot add AssumeStatement", ctx);
            delete assumeStmt;
            return nullptr;
        }
    }
    
    return assumeStmt;
}

std::any ModelBuilderVisitor::visitRedefinesStatement(SysML::SysMLParser::RedefinesStatementContext* ctx)
{
    if (!ctx) {
        reportError("RedefinesStatement context is null");
        return nullptr;
    }
    
    // 获取被重定义的名称
    auto qualifiedNameCtx = ctx->qualifiedName();
    if (!qualifiedNameCtx) {
        reportError("RedefinesStatement qualifiedName is null", ctx);
        return nullptr;
    }
    
    QStringList nameParts = getQualifiedNameParts(qualifiedNameCtx);
    QString redefinesName = nameParts.join("::");
    
    // 获取表达式
    // 注意：生成的parser中RedefinesStatementContext只有expression()方法，没有valuePart()方法
    // 虽然语法文件中有ruleValuePart?，但生成的parser可能还没有更新
    QString expression;
    auto exprCtx = ctx->expression();
    if (exprCtx) {
        expression = QString::fromStdString(exprCtx->getText());
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating RedefinesStatement: redefines" << redefinesName << "=" << expression;
    
    RedefinesStatement* redefinesStmt = new RedefinesStatement();
    redefinesStmt->setRedefinesName(redefinesName);
    redefinesStmt->setExpression(expression);
    redefinesStmt->setDeclaredName("redefines " + redefinesName + " = " + expression);
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(redefinesStmt);
    } else {
        if (m_resource) {
            m_resource->addContent(redefinesStmt);
        } else {
            reportError("Resource is null, cannot add RedefinesStatement", ctx);
            delete redefinesStmt;
            return nullptr;
        }
    }
    
    return redefinesStmt;
}

std::any ModelBuilderVisitor::visitActionInputOutput(SysML::SysMLParser::ActionInputOutputContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    QString ctxText = QString::fromStdString(ctx->getText());
    qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: full context text:" << ctxText;
    
    bool isInput = false;
    bool hasItemKeyword = false;
    QString itemName;
    QString itemType;
    QString defaultValue;
    QString redefinesName;
    QString multiplicity;
    
    // 检查是 'in' 还是 'out'
    if (ctxText.startsWith("in")) {
        isInput = true;
    }
    
    // 检查是否有 'item' 关键字
    // 注意：语法规则是 ('in' | 'out') 'item'?，所以需要检查文本中是否包含 "item"
    // 由于 'item' 是可选的，我们需要通过检查文本来判断
    // 格式应该是 "in item" 或 "out item"，而不是 "in" 或 "out" 后面直接跟其他内容
    if (ctxText.contains(" item ")) {
        hasItemKeyword = true;
        qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: found 'item' keyword in text";
    } else if (ctxText.startsWith("in item") || ctxText.startsWith("out item")) {
        // 检查是否以 "in item" 或 "out item" 开头（后面可能没有空格，直接跟其他内容）
        hasItemKeyword = true;
        qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: found 'item' keyword at start";
    } else {
        qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: no 'item' keyword found";
    }
    
    qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: isInput:" << isInput << ", hasItemKeyword:" << hasItemKeyword;
    
    // 处理使用声明（itemName : ItemType）
    auto usageDeclCtx = ctx->usageDeclaration();
    qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: usageDeclCtx =" << (usageDeclCtx != nullptr);
    if (usageDeclCtx) {
        qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: usageDeclaration text:" << QString::fromStdString(usageDeclCtx->getText());
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            // 提取项名称
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    itemName = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    itemName = getText(identifiers[0]->IDENTIFIER());
                }
            }
            
            // 提取项类型、重定义和多重性
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                // 提取类型
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        itemType = typeParts.join("::");
                    }
                } else if (specCtx->subsettingExpression()) {
                    auto qnameCtx = specCtx->subsettingExpression()->qualifiedName();
                    if (qnameCtx) itemType = getQualifiedNameParts(qnameCtx).join("::");
                }
                
                // 提取重定义表达式（redefines qualifiedName ownedMultiplicity?）
                auto redefExprCtx = specCtx->redefinitionExpression();
                if (redefExprCtx) {
                    auto redefQnameCtx = redefExprCtx->qualifiedName();
                    if (redefQnameCtx) {
                        QStringList redefParts = getQualifiedNameParts(redefQnameCtx);
                        redefinesName = redefParts.join("::");
                    }
                    
                    // 提取多重性
                    auto multCtx = redefExprCtx->ownedMultiplicity();
                    if (multCtx) {
                        multiplicity = QString::fromStdString(multCtx->getText());
                    }
                }
            }
        }
    }
    
    // 处理值部分（= defaultValue）
    auto valuePartCtx = ctx->valuePart();
    qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: valuePartCtx =" << (valuePartCtx != nullptr);
    if (valuePartCtx) {
        qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: valuePart text:" << QString::fromStdString(valuePartCtx->getText());
        auto featureValueCtx = valuePartCtx->featureValue();
        qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: featureValueCtx =" << (featureValueCtx != nullptr);
        if (featureValueCtx) {
            qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: featureValue text:" << QString::fromStdString(featureValueCtx->getText());
            auto exprCtx = featureValueCtx->expression();
            qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: expressionCtx =" << (exprCtx != nullptr);
            if (exprCtx) {
                defaultValue = QString::fromStdString(exprCtx->getText());
                qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: extracted defaultValue:" << defaultValue;
            } else {
                qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: expressionCtx is null";
            }
        } else {
            qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: featureValueCtx is null";
        }
    } else {
        qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: valuePartCtx is null";
    }
    
    QString displayName = (isInput ? "in" : "out");
    if (hasItemKeyword) {
        displayName += " item";
    }
    if (!itemName.isEmpty()) {
        displayName += " " + itemName;
    }
    qDebug() << "[ModelBuilderVisitor] Creating ActionInputOutput:" << displayName;
    
    ActionInputOutput* actionIO = new ActionInputOutput();
    actionIO->setIsInput(isInput);
    actionIO->setHasItemKeyword(hasItemKeyword);
    actionIO->setItemName(itemName);
    actionIO->setItemType(itemType);
    actionIO->setDefaultValue(defaultValue);
    actionIO->setRedefinesName(redefinesName);
    actionIO->setMultiplicity(multiplicity);
    actionIO->setDeclaredName(displayName);
    
    qDebug() << "[ModelBuilderVisitor] visitActionInputOutput: final values - itemName:" << itemName 
             << ", itemType:" << itemType << ", defaultValue:" << defaultValue 
             << ", redefinesName:" << redefinesName << ", multiplicity:" << multiplicity;
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(actionIO);
    } else {
        if (m_resource) {
            m_resource->addContent(actionIO);
        } else {
            reportError("Resource is null, cannot add ActionInputOutput", ctx);
            delete actionIO;
            return nullptr;
        }
    }
    
    // 如果 actionInputOutput 有体，访问使用体
    auto bodyCtx = ctx->usageBody();
    if (bodyCtx) {
        pushParent(actionIO);
        visitUsageBody(bodyCtx);
        popParent();
    }
    
    return actionIO;
}

std::any ModelBuilderVisitor::visitExpression(SysML::SysMLParser::ExpressionContext* ctx)
{
    if (!ctx) {
        return QString();
    }
    
    // 按照运算符优先级访问表达式
    // expression -> logicalOrExpression
    auto logicalOrExpr = ctx->logicalOrExpression();
    if (logicalOrExpr) {
        auto result = visitLogicalOrExpression(logicalOrExpr);
        if (result.has_value()) {
            return result;
        }
    }
    
    // 如果无法解析，返回表达式文本
    QString exprText = QString::fromStdString(ctx->getText());
    qDebug() << "[ModelBuilderVisitor] visitExpression fallback to text:" << exprText;
    return exprText;
}

std::any ModelBuilderVisitor::visitPrimaryExpression(SysML::SysMLParser::PrimaryExpressionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    QString units;
    
    // 通过语法树正确访问单位表达式
    // 语法规则：primaryExpression : (literalExpression | featureReferenceExpression | '(' expression ')' | 'null') ('[' unitExpression ']')?
    auto unitExpr = ctx->unitExpression();
    if (unitExpr) {
        auto unitResult = visitUnitExpression(unitExpr);
        if (unitResult.has_value()) {
            units = std::any_cast<QString>(unitResult);
            qDebug() << "[ModelBuilderVisitor] PrimaryExpression found unit expression:" << units;
        }
    }
    
    // 如果通过语法树访问失败，回退到文本解析方法
    if (units.isEmpty()) {
        QString expressionText = QString::fromStdString(ctx->getText());
        int bracketStart = expressionText.indexOf('[');
        int bracketEnd = expressionText.lastIndexOf(']');
        if (bracketStart >= 0 && bracketEnd > bracketStart) {
            QString unitText = expressionText.mid(bracketStart + 1, bracketEnd - bracketStart - 1).trimmed();
            if (!unitText.isEmpty()) {
                units = unitText;
                qDebug() << "[ModelBuilderVisitor] PrimaryExpression extracted unit from text:" << units;
            }
        }
    }
    
    // 注意：单位表达式的存储应该在调用 visitPrimaryExpression 的上层方法中处理
    // 这里只负责提取单位表达式文本，不直接存储到模型对象
    
    // 访问字面量表达式
    auto literalExpr = ctx->literalExpression();
    if (literalExpr) {
        auto result = visitLiteralExpression(literalExpr);
        if (result.has_value()) {
            QString value = std::any_cast<QString>(result);
            // 返回的值不包含单位，单位信息已在上层方法中处理
            return value;
        }
    }
    
    // 访问特征引用表达式
    auto featureRefExpr = ctx->featureReferenceExpression();
    if (featureRefExpr) {
        return visitFeatureReferenceExpression(featureRefExpr);
    }
    
    // 访问嵌套表达式
    auto nestedExpr = ctx->expression();
    if (nestedExpr) {
        return visitExpression(nestedExpr);
    }
    
    // 检查是否是 null
    QString ctxText = QString::fromStdString(ctx->getText());
    if (ctxText.trimmed() == "null") {
        return QString("null");
    }
    
    // 如果没有匹配的子表达式，返回表达式文本（不包含单位，单位已单独存储）
    QString expressionText = QString::fromStdString(ctx->getText());
    // 移除单位表达式部分
    if (expressionText.contains('[') && expressionText.contains(']')) {
        int bracketStart = expressionText.indexOf('[');
        expressionText = expressionText.left(bracketStart).trimmed();
    }
    return expressionText;
}

std::any ModelBuilderVisitor::visitLogicalOrExpression(SysML::SysMLParser::LogicalOrExpressionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问逻辑与表达式
    auto logicalAndExprs = ctx->logicalAndExpression();
    QStringList exprTexts;
    
    for (auto expr : logicalAndExprs) {
        if (expr) {
            auto result = visitLogicalAndExpression(expr);
            if (result.has_value()) {
                exprTexts.append(std::any_cast<QString>(result));
            } else {
                // 如果访问失败，使用文本
                exprTexts.append(QString::fromStdString(expr->getText()));
            }
        }
    }
    
    if (!exprTexts.isEmpty()) {
        QString combined = exprTexts.join(" or ");
        qDebug() << "[ModelBuilderVisitor] LogicalOrExpression:" << combined;
        return combined;
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitLiteralExpression(SysML::SysMLParser::LiteralExpressionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问字面量值
    if (ctx->Integer()) {
        QString value = QString::fromStdString(ctx->Integer()->getText());
        qDebug() << "[ModelBuilderVisitor] LiteralExpression (Integer):" << value;
        return value;
    } else if (ctx->Real()) {
        QString value = QString::fromStdString(ctx->Real()->getText());
        qDebug() << "[ModelBuilderVisitor] LiteralExpression (Real):" << value;
        return value;
    } else if (ctx->Boolean()) {
        QString value = QString::fromStdString(ctx->Boolean()->getText());
        qDebug() << "[ModelBuilderVisitor] LiteralExpression (Boolean):" << value;
        return value;
    } else if (ctx->String()) {
        QString value = QString::fromStdString(ctx->String()->getText());
        qDebug() << "[ModelBuilderVisitor] LiteralExpression (String):" << value;
        return value;
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitFeatureReferenceExpression(SysML::SysMLParser::FeatureReferenceExpressionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问限定名称
    auto qualifiedName = ctx->qualifiedName();
    if (qualifiedName) {
        QStringList nameParts = getQualifiedNameParts(qualifiedName);
        QString name = nameParts.join("::");
        qDebug() << "[ModelBuilderVisitor] FeatureReferenceExpression:" << name;
        return name;
    }
    
    // 访问成员访问表达式
    auto memberAccess = ctx->memberAccessExpression();
    if (memberAccess) {
        QString exprText = QString::fromStdString(memberAccess->getText());
        qDebug() << "[ModelBuilderVisitor] FeatureReferenceExpression (memberAccess):" << exprText;
        return exprText;
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitDocStatement(SysML::SysMLParser::DocStatementContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取块注释内容
    QString content;
    auto blockCommentCtx = ctx->blockComment();
    if (blockCommentCtx) {
        // 从块注释中提取内容（去掉 /* 和 */）
        QString commentText = QString::fromStdString(blockCommentCtx->getText());
        // 移除 /* 和 */
        if (commentText.startsWith("/*") && commentText.endsWith("*/")) {
            content = commentText.mid(2, commentText.length() - 4).trimmed();
        } else {
            content = commentText;
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating DocStatement:" << content;
    
    // 创建 DocStatement（需要包含头文件）
    // 注意：需要先包含 DocStatement.h
    DocStatement* docStmt = new DocStatement();
    docStmt->setContent(content);
    docStmt->setDeclaredName("doc");
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(docStmt);
    } else {
        if (m_resource) {
            m_resource->addContent(docStmt);
        } else {
            reportError("Resource is null, cannot add DocStatement", ctx);
            delete docStmt;
            return nullptr;
        }
    }
    
    return docStmt;
}

std::any ModelBuilderVisitor::visitCommentStatement(SysML::SysMLParser::CommentStatementContext* ctx)
{
    // comment 语句通常不需要创建元素，只是注释
    // 如果需要，可以类似实现
    if (!ctx) {
        return nullptr;
    }
    
    // 暂时不处理 comment 语句，因为它们通常只是注释
    return nullptr;
}

// ============================================
// 表达式访问者方法实现（阶段二）
// ============================================

std::any ModelBuilderVisitor::visitLogicalAndExpression(SysML::SysMLParser::LogicalAndExpressionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问比较表达式
    auto comparisonExprs = ctx->comparisonExpression();
    QStringList exprTexts;
    
    for (auto expr : comparisonExprs) {
        if (expr) {
            auto result = visitComparisonExpression(expr);
            if (result.has_value()) {
                QString exprText = std::any_cast<QString>(result);
                exprTexts.append(exprText);
            } else {
                // 如果访问失败，使用文本
                exprTexts.append(QString::fromStdString(expr->getText()));
            }
        }
    }
    
    if (!exprTexts.isEmpty()) {
        QString combined = exprTexts.join(" and ");
        qDebug() << "[ModelBuilderVisitor] LogicalAndExpression:" << combined;
        return combined;
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitComparisonExpression(SysML::SysMLParser::ComparisonExpressionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问第一个加法表达式
    auto additiveExprs = ctx->additiveExpression();
    if (additiveExprs.empty()) {
        return nullptr;
    }
    
    auto leftResult = visitAdditiveExpression(additiveExprs[0]);
    QString leftText;
    if (leftResult.has_value()) {
        leftText = std::any_cast<QString>(leftResult);
    } else {
        leftText = QString::fromStdString(additiveExprs[0]->getText());
    }
    
    // 检查是否有比较运算符和右侧表达式
    auto comparisonOpCtx = ctx->comparisonOp();
    if (comparisonOpCtx && additiveExprs.size() > 1) {
        // 获取运算符
        QString op;
        if (comparisonOpCtx->getText() == ">") {
            op = ">";
        } else if (comparisonOpCtx->getText() == ">=") {
            op = ">=";
        } else if (comparisonOpCtx->getText() == "<") {
            op = "<";
        } else if (comparisonOpCtx->getText() == "<=") {
            op = "<=";
        } else if (comparisonOpCtx->getText() == "==") {
            op = "==";
        } else if (comparisonOpCtx->getText() == "!=") {
            op = "!=";
        } else {
            op = QString::fromStdString(comparisonOpCtx->getText());
        }
        
        // 访问右侧表达式
        auto rightResult = visitAdditiveExpression(additiveExprs[1]);
        QString rightText;
        if (rightResult.has_value()) {
            rightText = std::any_cast<QString>(rightResult);
        } else {
            rightText = QString::fromStdString(additiveExprs[1]->getText());
        }
        
        QString combined = leftText + " " + op + " " + rightText;
        qDebug() << "[ModelBuilderVisitor] ComparisonExpression:" << combined;
        return combined;
    }
    
    // 没有比较运算符，只返回左侧表达式
    return leftText;
}

std::any ModelBuilderVisitor::visitAdditiveExpression(SysML::SysMLParser::AdditiveExpressionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问第一个乘法表达式
    auto multiplicativeExprs = ctx->multiplicativeExpression();
    if (multiplicativeExprs.empty()) {
        return nullptr;
    }
    
    QStringList exprTexts;
    QStringList operators;
    
    // 访问第一个表达式
    auto firstResult = visitMultiplicativeExpression(multiplicativeExprs[0]);
    if (firstResult.has_value()) {
        exprTexts.append(std::any_cast<QString>(firstResult));
    } else {
        exprTexts.append(QString::fromStdString(multiplicativeExprs[0]->getText()));
    }
    
    // 访问后续的运算符和表达式
    // 注意：语法规则是 multiplicativeExpression (('+' | '-') multiplicativeExpression)*
    // 运算符在语法树中可能作为单独的 token 节点存在
    // 我们需要从上下文中提取运算符信息
    QString ctxText = QString::fromStdString(ctx->getText());
    
    // 简单实现：如果有多个表达式，使用文本解析
    if (multiplicativeExprs.size() > 1) {
        // 访问所有表达式
        for (size_t i = 1; i < multiplicativeExprs.size(); ++i) {
            auto result = visitMultiplicativeExpression(multiplicativeExprs[i]);
            if (result.has_value()) {
                exprTexts.append(std::any_cast<QString>(result));
            } else {
                exprTexts.append(QString::fromStdString(multiplicativeExprs[i]->getText()));
            }
        }
        
        // 从文本中提取运算符（简化实现）
        // 更好的方法是访问语法树中的运算符节点
        QString combined = exprTexts[0];
        for (size_t i = 1; i < exprTexts.size(); ++i) {
            // 查找运算符（在表达式之间）
            QString before = exprTexts[i-1];
            QString after = exprTexts[i];
            int beforeEnd = ctxText.indexOf(before) + before.length();
            int afterStart = ctxText.indexOf(after, beforeEnd);
            if (afterStart > beforeEnd) {
                QString op = ctxText.mid(beforeEnd, afterStart - beforeEnd).trimmed();
                combined += " " + op + " " + exprTexts[i];
            } else {
                combined += " + " + exprTexts[i]; // 默认使用 +
            }
        }
        qDebug() << "[ModelBuilderVisitor] AdditiveExpression:" << combined;
        return combined;
    }
    
    // 只有一个表达式
    return exprTexts[0];
}

std::any ModelBuilderVisitor::visitMultiplicativeExpression(SysML::SysMLParser::MultiplicativeExpressionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问第一个一元表达式
    auto unaryExprs = ctx->unaryExpression();
    if (unaryExprs.empty()) {
        return nullptr;
    }
    
    QStringList exprTexts;
    
    // 访问所有表达式
    for (auto expr : unaryExprs) {
        if (expr) {
            auto result = visitUnaryExpression(expr);
            if (result.has_value()) {
                exprTexts.append(std::any_cast<QString>(result));
            } else {
                exprTexts.append(QString::fromStdString(expr->getText()));
            }
        }
    }
    
    if (!exprTexts.isEmpty()) {
        // 如果有多个表达式，需要添加运算符
        if (exprTexts.size() > 1) {
            QString ctxText = QString::fromStdString(ctx->getText());
            QString combined = exprTexts[0];
            for (size_t i = 1; i < exprTexts.size(); ++i) {
                // 查找运算符（在表达式之间）
                QString before = exprTexts[i-1];
                QString after = exprTexts[i];
                int beforeEnd = ctxText.indexOf(before) + before.length();
                int afterStart = ctxText.indexOf(after, beforeEnd);
                if (afterStart > beforeEnd) {
                    QString op = ctxText.mid(beforeEnd, afterStart - beforeEnd).trimmed();
                    combined += " " + op + " " + exprTexts[i];
                } else {
                    combined += " * " + exprTexts[i]; // 默认使用 *
                }
            }
            qDebug() << "[ModelBuilderVisitor] MultiplicativeExpression:" << combined;
            return combined;
        }
        return exprTexts[0];
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitUnaryExpression(SysML::SysMLParser::UnaryExpressionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    QString unaryOp;
    
    // 检查一元运算符
    // 语法规则：('-' | '+' | 'not')? primaryExpression
    QString ctxText = QString::fromStdString(ctx->getText());
    
    // 检查是否有 'not' 关键字
    if (ctxText.startsWith("not ")) {
        unaryOp = "not";
    } else if (ctxText.startsWith("-")) {
        unaryOp = "-";
    } else if (ctxText.startsWith("+")) {
        unaryOp = "+";
    }
    
    // 访问主表达式
    auto primaryExpr = ctx->primaryExpression();
    if (!primaryExpr) {
        return nullptr;
    }
    
    auto result = visitPrimaryExpression(primaryExpr);
    QString exprText;
    if (result.has_value()) {
        exprText = std::any_cast<QString>(result);
    } else {
        exprText = QString::fromStdString(primaryExpr->getText());
    }
    
    // 如果有运算符，添加到表达式前
    if (!unaryOp.isEmpty()) {
        QString combined = unaryOp + " " + exprText;
        qDebug() << "[ModelBuilderVisitor] UnaryExpression:" << combined;
        return combined;
    }
    
    return exprText;
}

std::any ModelBuilderVisitor::visitUnitExpression(SysML::SysMLParser::UnitExpressionContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问单位项
    auto unitTerms = ctx->unitTerm();
    QStringList unitParts;
    
    for (auto term : unitTerms) {
        if (term) {
            auto result = visitUnitTerm(term);
            if (result.has_value()) {
                unitParts.append(std::any_cast<QString>(result));
            } else {
                unitParts.append(QString::fromStdString(term->getText()));
            }
        }
    }
    
    if (!unitParts.isEmpty()) {
        // 如果有多个单位项，需要添加运算符
        if (unitParts.size() > 1) {
            QString ctxText = QString::fromStdString(ctx->getText());
            QString combined = unitParts[0];
            for (size_t i = 1; i < unitParts.size(); ++i) {
                // 查找运算符（在单位项之间）
                QString before = unitParts[i-1];
                QString after = unitParts[i];
                int beforeEnd = ctxText.indexOf(before) + before.length();
                int afterStart = ctxText.indexOf(after, beforeEnd);
                if (afterStart > beforeEnd) {
                    QString op = ctxText.mid(beforeEnd, afterStart - beforeEnd).trimmed();
                    combined += " " + op + " " + unitParts[i];
                } else {
                    combined += " / " + unitParts[i]; // 默认使用 /
                }
            }
            qDebug() << "[ModelBuilderVisitor] UnitExpression:" << combined;
            return combined;
        }
        return unitParts[0];
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitUnitTerm(SysML::SysMLParser::UnitTermContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问标识符
    auto identifier = ctx->IDENTIFIER();
    if (!identifier) {
        return nullptr;
    }
    
    QString unitName = QString::fromStdString(identifier->getText());
    
    // 检查是否有指数
    auto integer = ctx->Integer();
    if (integer) {
        QString exponent = QString::fromStdString(integer->getText());
        QString combined = unitName + "^" + exponent;
        qDebug() << "[ModelBuilderVisitor] UnitTerm:" << combined;
        return combined;
    }
    
    qDebug() << "[ModelBuilderVisitor] UnitTerm:" << unitName;
    return unitName;
}

// ============================================
// 状态机相关 Visitor 方法实现（阶段一）
// ============================================

std::any ModelBuilderVisitor::visitExhibitStatement(SysML::SysMLParser::ExhibitStatementContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取使用声明（可选）
    QString name = "UnnamedExhibit";
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ExhibitStatement:" << name;
    
    ExhibitStatement* exhibitStmt = new ExhibitStatement();
    exhibitStmt->setDeclaredName(name);
    
    // 获取状态机类型（parallel 或空）
    auto stateMachineKindCtx = ctx->stateMachineKind();
    if (stateMachineKindCtx) {
        QString kind = QString::fromStdString(stateMachineKindCtx->getText());
        exhibitStmt->setStateMachineKind(kind);
        qDebug() << "[ModelBuilderVisitor] ExhibitStatement stateMachineKind:" << kind;
    }
    
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(exhibitStmt);
    } else {
        if (m_resource) {
            m_resource->addContent(exhibitStmt);
        } else {
            reportError("Resource is null, cannot add ExhibitStatement", ctx);
            delete exhibitStmt;
            return nullptr;
        }
    }
    
    pushParent(exhibitStmt);
    
    // 访问状态体
    auto stateBodyCtx = ctx->stateBody();
    if (stateBodyCtx) {
        visitStateBody(stateBodyCtx);
    }
    
    popParent();
    
    return exhibitStmt;
}

std::any ModelBuilderVisitor::visitStateDefinitionBody(SysML::SysMLParser::StateDefinitionBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问状态体项
    auto stateBodyItems = ctx->stateBodyItem();
    for (auto itemCtx : stateBodyItems) {
        if (itemCtx) {
            visitStateBodyItem(itemCtx);
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitStateBody(SysML::SysMLParser::StateBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问状态体项
    auto stateBodyItems = ctx->stateBodyItem();
    QString stateBodyText = QString::fromStdString(ctx->getText());
    qDebug() << "[ModelBuilderVisitor] visitStateBody: text:" << stateBodyText << "has" << stateBodyItems.size() << "stateBodyItems";
    
    for (auto itemCtx : stateBodyItems) {
        if (itemCtx) {
            visitStateBodyItem(itemCtx);
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitStateBodyItem(SysML::SysMLParser::StateBodyItemContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    QString itemText = QString::fromStdString(ctx->getText());
    qDebug() << "[ModelBuilderVisitor] visitStateBodyItem: processing item, text:" << itemText;
    
    // 分发到相应的访问者方法
    // 注意：优先级很重要，stateMember 必须在 definitionBodyItem 之前检查
    // 根据语法规则：stateBodyItem 的优先级是：
    // 1. stateMember (状态成员) - 必须在 definitionBodyItem 之前
    // 2. transitionMember (转换成员)
    // 3. stateActionMember (状态动作成员)
    // 4. refStatement (ref 语句)
    // 5. constraintStatement (constraint 语句)
    // 6. definitionBodyItem (基础定义体项) - 放在最后
    
    auto stateMemberCtx = ctx->stateMember();
    if (stateMemberCtx) {
        qDebug() << "[ModelBuilderVisitor] visitStateBodyItem: found stateMember";
        return visitStateMember(stateMemberCtx);
    }
    
    auto transitionMemberCtx = ctx->transitionMember();
    if (transitionMemberCtx) {
        qDebug() << "[ModelBuilderVisitor] visitStateBodyItem: found transitionMember";
        return visitTransitionMember(transitionMemberCtx);
    }
    
    auto stateActionMemberCtx = ctx->stateActionMember();
    if (stateActionMemberCtx) {
        qDebug() << "[ModelBuilderVisitor] visitStateBodyItem: found stateActionMember";
        return visitStateActionMember(stateActionMemberCtx);
    }
    
    auto refStmtCtx = ctx->refStatement();
    if (refStmtCtx) {
        qDebug() << "[ModelBuilderVisitor] visitStateBodyItem: found refStatement";
        return visitRefStatement(refStmtCtx);
    }
    
    auto constraintStmtCtx = ctx->constraintStatement();
    if (constraintStmtCtx) {
        qDebug() << "[ModelBuilderVisitor] visitStateBodyItem: found constraintStatement";
        return visitConstraintStatement(constraintStmtCtx);
    }
    
    auto defBodyItemCtx = ctx->definitionBodyItem();
    if (defBodyItemCtx) {
        qDebug() << "[ModelBuilderVisitor] visitStateBodyItem: found definitionBodyItem";
        // 处理基础定义体项
        // 根据 definitionBodyItem 的类型进行分发
        auto defMemberCtx = defBodyItemCtx->definitionMember();
        if (defMemberCtx) {
            return visitDefinition(defMemberCtx->definition());
        }
        auto usageMemberCtx = defBodyItemCtx->usageMember();
        if (usageMemberCtx) {
            return visitUsage(usageMemberCtx->usage());
        }
        auto attributeCtx = defBodyItemCtx->attribute();
        if (attributeCtx) {
            return visitAttribute(attributeCtx);
        }
        auto statementCtx = defBodyItemCtx->statement();
        if (statementCtx) {
            return visitStatement(statementCtx);
        }
        auto docStmtCtx = defBodyItemCtx->docStatement();
        if (docStmtCtx) {
            return visitDocStatement(docStmtCtx);
        }
        auto exhibitStmtCtx = defBodyItemCtx->exhibitStatement();
        if (exhibitStmtCtx) {
            return visitExhibitStatement(exhibitStmtCtx);
        }
        auto metadataAnnotationCtx = defBodyItemCtx->metadataAnnotation();
        if (metadataAnnotationCtx) {
            qDebug() << "[ModelBuilderVisitor] visitStateBodyItem: found metadataAnnotation in definitionBodyItem";
            return visitMetadataAnnotation(metadataAnnotationCtx);
        }
        // 其他类型（importStatement, relationship等）...
        return nullptr;
    }
    
    qDebug() << "[ModelBuilderVisitor] visitStateBodyItem: no matching item type found";
    return nullptr;
}

std::any ModelBuilderVisitor::visitStateMember(SysML::SysMLParser::StateMemberContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问状态使用或状态定义
    auto stateUsageCtx = ctx->stateUsage();
    if (stateUsageCtx) {
        QString stateMemberText = QString::fromStdString(ctx->getText());
        qDebug() << "[ModelBuilderVisitor] visitStateMember: found stateUsage, text:" << stateMemberText;
        return visitStateUsage(stateUsageCtx);
    }
    
    auto stateDefCtx = ctx->stateDefinition();
    if (stateDefCtx) {
        QString stateMemberText = QString::fromStdString(ctx->getText());
        qDebug() << "[ModelBuilderVisitor] visitStateMember: found stateDefinition, text:" << stateMemberText;
        return visitStateDefinition(stateDefCtx);
    }
    
    qDebug() << "[ModelBuilderVisitor] visitStateMember: no stateUsage or stateDefinition found";
    return nullptr;
}

std::any ModelBuilderVisitor::visitTransitionMember(SysML::SysMLParser::TransitionMemberContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 访问转换
    auto transitionCtx = ctx->transition();
    if (transitionCtx) {
        return visitTransition(transitionCtx);
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitRefStatement(SysML::SysMLParser::RefStatementContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // ref 语句：ref usageDeclaration? usageCompletion
    QString refText = QString::fromStdString(ctx->getText());
    qDebug() << "[ModelBuilderVisitor] Found ref statement:" << refText;
    
    // 创建 PartUsage 来表示 ref（临时方案，后续可以创建专门的 RefStatement 类）
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            // 获取名称
            QString name = "UnnamedRef";
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
            
            PartUsage* refPart = new PartUsage();
            refPart->setDeclaredName(name);
            
            // 处理类型（通过 featureSpecializationPart）
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        QString typeName = typeParts.join("::");
                        refPart->setTypeName(typeName);
                    }
                }
            }
            
            Element* parent = getCurrentParent();
            if (parent) {
                parent->addOwnedElement(refPart);
                qDebug() << "[ModelBuilderVisitor] Created ref PartUsage:" << name;
            } else {
                delete refPart;
                return nullptr;
            }
            
            // 访问使用完成部分
            auto usageCompletionCtx = ctx->usageCompletion();
            if (usageCompletionCtx) {
                pushParent(refPart);
                // 访问值部分和使用体
                auto valuePartCtx = usageCompletionCtx->valuePart();
                if (valuePartCtx) {
                    // TODO: 处理值部分
                }
                auto usageBodyCtx = usageCompletionCtx->usageBody();
                if (usageBodyCtx) {
                    visitUsageBody(usageBodyCtx);
                }
                popParent();
            }
            
            return refPart;
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitConstraintStatement(SysML::SysMLParser::ConstraintStatementContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    QString expression;
    
    // constraint 语句有两种形式：
    // 1. constraint {expression};
    // 2. constraint;
    auto exprCtx = ctx->expression();
    if (exprCtx) {
        // Preserve the complete parsed expression. The expression visitor is
        // intentionally partial and previously dropped qualified RHS paths.
        expression = QString::fromStdString(exprCtx->getText());
    } else {
        // 第二种形式：constraint; (无表达式)
        expression = "";
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ConstraintStatement:" << expression;
    
    ConstraintStatement* constraintStmt = new ConstraintStatement();
    constraintStmt->setExpression(expression);
    if (ctx->identifier()) {
        constraintStmt->setDeclaredName(getText(ctx->identifier()->IDENTIFIER()));
    } else if (expression.isEmpty()) {
        constraintStmt->setDeclaredName("constraint");
    } else {
        constraintStmt->setDeclaredName("constraint {" + expression + "}");
    }
    
    // 添加到父元素（通常是 StateUsage 或 StateDefinition）
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(constraintStmt);
    } else {
        if (m_resource) {
            m_resource->addContent(constraintStmt);
        } else {
            reportError("Resource is null, cannot add ConstraintStatement", ctx);
            delete constraintStmt;
            return nullptr;
        }
    }
    
    return constraintStmt;
}

// ============================================
// 视图相关访问方法
// ============================================

std::any ModelBuilderVisitor::visitViewpointDefinition(SysML::SysMLParser::ViewpointDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("ViewpointDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("ViewpointDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedViewpoint";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ViewpointDefinition:" << name;
    
    ViewpointDefinition* viewpointDef = new ViewpointDefinition();
    viewpointDef->setDeclaredName(name);
    
    checkDuplicateName(viewpointDef, name, ctx);
    
    // 处理前缀
    auto prefixCtx = ctx->occurrenceDefinitionPrefix();
    if (prefixCtx) {
        auto basicPrefixCtx = prefixCtx->basicDefinitionPrefix();
        if (basicPrefixCtx) {
            if (basicPrefixCtx->getText() == "abstract") {
                viewpointDef->setIsAbstract(true);
            } else if (basicPrefixCtx->getText() == "variation") {
                viewpointDef->setIsVariation(true);
            }
        }
    }
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(viewpointDef, supertypeParts);
                }
            }
        }
    }
    
    // 添加到当前包或资源
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(viewpointDef);
    } else {
        if (m_resource) {
            m_resource->addContent(viewpointDef);
        } else {
            reportError("Resource is null, cannot add ViewpointDefinition", ctx);
            delete viewpointDef;
            return nullptr;
        }
    }
    
    pushParent(viewpointDef);
    
    // 处理 viewpointBody
    auto bodyCtx = ctx->viewpointBody();
    if (bodyCtx) {
        visitViewpointBody(bodyCtx);
    }
    
    popParent();
    
    return viewpointDef;
}

std::any ModelBuilderVisitor::visitViewDefinition(SysML::SysMLParser::ViewDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("ViewDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("ViewDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedView";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ViewDefinition:" << name;
    
    ViewDefinition* viewDef = new ViewDefinition();
    viewDef->setDeclaredName(name);
    
    checkDuplicateName(viewDef, name, ctx);
    
    // 处理前缀
    auto prefixCtx = ctx->occurrenceDefinitionPrefix();
    if (prefixCtx) {
        auto basicPrefixCtx = prefixCtx->basicDefinitionPrefix();
        if (basicPrefixCtx) {
            if (basicPrefixCtx->getText() == "abstract") {
                viewDef->setIsAbstract(true);
            } else if (basicPrefixCtx->getText() == "variation") {
                viewDef->setIsVariation(true);
            }
        }
    }
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    if (viewDef->viewType().isEmpty()) viewDef->setViewType(supertypeParts.join("::"));
                    createSubclassification(viewDef, supertypeParts);
                }
            }
        }
    }
    
    // 添加到当前包或资源
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(viewDef);
    } else {
        if (m_resource) {
            m_resource->addContent(viewDef);
        } else {
            reportError("Resource is null, cannot add ViewDefinition", ctx);
            delete viewDef;
            return nullptr;
        }
    }
    
    pushParent(viewDef);
    
    // 处理 viewDefinitionBody
    auto bodyCtx = ctx->viewDefinitionBody();
    if (bodyCtx) {
        visitViewDefinitionBody(bodyCtx);
    }
    
    popParent();
    
    return viewDef;
}

std::any ModelBuilderVisitor::visitConcernDefinition(SysML::SysMLParser::ConcernDefinitionContext* ctx)
{
    if (!ctx) {
        reportError("ConcernDefinition context is null");
        return nullptr;
    }
    
    // 获取定义声明
    auto declCtx = ctx->definitionDeclaration();
    if (!declCtx) {
        reportError("ConcernDefinition definitionDeclaration is null", ctx);
        return nullptr;
    }
    
    // 获取标识符
    QString name = "UnnamedConcern";
    auto identCtx = declCtx->identification();
    if (identCtx) {
        auto identifiers = identCtx->identifier();
        if (identifiers.size() >= 2) {
            name = getText(identifiers[1]->IDENTIFIER());
        } else if (identifiers.size() == 1) {
            name = getText(identifiers[0]->IDENTIFIER());
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ConcernDefinition:" << name;
    
    ConcernDefinition* concernDef = new ConcernDefinition();
    concernDef->setDeclaredName(name);
    
    checkDuplicateName(concernDef, name, ctx);
    
    // 处理前缀
    auto prefixCtx = ctx->occurrenceDefinitionPrefix();
    if (prefixCtx) {
        auto basicPrefixCtx = prefixCtx->basicDefinitionPrefix();
        if (basicPrefixCtx) {
            if (basicPrefixCtx->getText() == "abstract") {
                concernDef->setIsAbstract(true);
            } else if (basicPrefixCtx->getText() == "variation") {
                concernDef->setIsVariation(true);
            }
        }
    }
    
    // 处理子类化部分（如果有）
    auto subclassCtx = declCtx->subclassificationPart();
    if (subclassCtx) {
        auto supertypeCtxs = subclassCtx->ownedSubclassification();
        for (auto supertypeCtx : supertypeCtxs) {
            if (supertypeCtx) {
                auto qnameCtx = supertypeCtx->qualifiedName();
                if (qnameCtx) {
                    QStringList supertypeParts = getQualifiedNameParts(qnameCtx);
                    createSubclassification(concernDef, supertypeParts);
                }
            }
        }
    }
    
    // 添加到当前包或资源
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(concernDef);
    } else {
        if (m_resource) {
            m_resource->addContent(concernDef);
        } else {
            reportError("Resource is null, cannot add ConcernDefinition", ctx);
            delete concernDef;
            return nullptr;
        }
    }
    
    pushParent(concernDef);
    
    // 处理 concernBody
    auto bodyCtx = ctx->concernBody();
    if (bodyCtx) {
        visitConcernBody(bodyCtx);
    }
    
    popParent();
    
    // 解析完成后，尝试解析前向引用
    // 查找所有 FrameConcernMember，如果它们的 concern 为 null 但 concernQualifiedName 匹配，则设置 concern
    if (m_resource) {
        // 递归收集所有元素
        QList<Element*> allElements;
        std::function<void(Element*)> collectElements = [&](Element* elem) {
            if (!elem) return;
            allElements.append(elem);
            for (Element* child : elem->ownedElements()) {
                collectElements(child);
            }
        };
        for (Element* content : m_resource->contents()) {
            collectElements(content);
        }
        
        for (Element* elem : allElements) {
            if (FrameConcernMember* frameMember = qobject_cast<FrameConcernMember*>(elem)) {
                if (!frameMember->concern() && !frameMember->concernQualifiedName().isEmpty()) {
                    QStringList nameParts = frameMember->concernQualifiedName().split("::", Qt::SkipEmptyParts);
                    Element* foundConcern = findElementByQualifiedName(nameParts);
                    if (foundConcern) {
                        frameMember->setConcern(foundConcern);
                        qDebug() << "[ModelBuilderVisitor] Resolved forward reference: FrameConcernMember ->" << foundConcern->declaredName();
                    }
                }
            }
        }
    }
    
    return concernDef;
}

std::any ModelBuilderVisitor::visitViewUsage(SysML::SysMLParser::ViewUsageContext* ctx)
{
    if (!ctx) {
        reportError("ViewUsage context is null");
        return nullptr;
    }
    
    // 获取使用声明
    QString name = "UnnamedView";
    QString typeName;
    auto usageDeclCtx = ctx->usageDeclaration();
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                if (identifiers.size() >= 2) {
                    name = getText(identifiers[1]->IDENTIFIER());
                } else if (identifiers.size() == 1) {
                    name = getText(identifiers[0]->IDENTIFIER());
                }
            }
            
            // 获取类型（从特征特殊化部分）
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    if (qnameCtx) {
                        QStringList typeParts = getQualifiedNameParts(qnameCtx);
                        typeName = typeParts.join("::");
                    }
                } else if (specCtx->redefinitionExpression()) {
                    auto qnameCtx = specCtx->redefinitionExpression()->qualifiedName();
                    if (qnameCtx) typeName = getQualifiedNameParts(qnameCtx).join("::");
                } else if (specCtx->subsettingExpression()) {
                    auto qnameCtx = specCtx->subsettingExpression()->qualifiedName();
                    if (qnameCtx) typeName = getQualifiedNameParts(qnameCtx).join("::");
                }
            }
        }
    }
    
    qDebug() << "[ModelBuilderVisitor] Creating ViewUsage:" << name << "type:" << typeName;
    
    ViewUsage* viewUsage = new ViewUsage();
    viewUsage->setDeclaredName(name);
    if (!typeName.isEmpty()) {
        viewUsage->setTypeName(typeName);
        // TODO: 查找 ViewDefinition 并设置
    }
    
    checkDuplicateName(viewUsage, name, ctx);
    
    // 添加到当前包或资源
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(viewUsage);
    } else {
        if (m_resource) {
            m_resource->addContent(viewUsage);
        } else {
            reportError("Resource is null, cannot add ViewUsage", ctx);
            delete viewUsage;
            return nullptr;
        }
    }
    
    pushParent(viewUsage);
    
    // 处理 viewBody
    auto bodyCtx = ctx->viewBody();
    if (bodyCtx) {
        visitViewBody(bodyCtx);
    }
    
    popParent();
    
    return viewUsage;
}

std::any ModelBuilderVisitor::visitViewpointBody(SysML::SysMLParser::ViewpointBodyContext* ctx)
{
    if (!ctx) {
        qDebug() << "[ModelBuilderVisitor] visitViewpointBody: ctx is null";
        return nullptr;
    }
    
    qDebug() << "[ModelBuilderVisitor] visitViewpointBody: text =" << QString::fromStdString(ctx->getText());
    qDebug() << "[ModelBuilderVisitor] visitViewpointBody: processing" << ctx->viewpointBodyItem().size() << "body items";
    
    // 处理 viewpointBodyItem
    for (auto itemCtx : ctx->viewpointBodyItem()) {
        if (itemCtx) {
            if (itemCtx->definitionBodyItem()) {
                qDebug() << "[ModelBuilderVisitor] visitViewpointBody: found definitionBodyItem";
                visitDefinitionBodyItem(itemCtx->definitionBodyItem());
            } else if (itemCtx->requirementMember()) {
                qDebug() << "[ModelBuilderVisitor] visitViewpointBody: found requirementMember";
                visitRequirementMember(itemCtx->requirementMember());
            } else if (itemCtx->requirementConstraintMember()) {
                qDebug() << "[ModelBuilderVisitor] visitViewpointBody: found requirementConstraintMember";
                visitRequirementConstraintMember(itemCtx->requirementConstraintMember());
            } else if (itemCtx->frameConcernMember()) {
                qDebug() << "[ModelBuilderVisitor] visitViewpointBody: found frameConcernMember";
                visitFrameConcernMember(itemCtx->frameConcernMember());
            } else {
                qDebug() << "[ModelBuilderVisitor] visitViewpointBody: unknown body item type";
            }
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitViewDefinitionBody(SysML::SysMLParser::ViewDefinitionBodyContext* ctx)
{
    if (!ctx) {
        qDebug() << "[ModelBuilderVisitor] visitViewDefinitionBody: ctx is null";
        return nullptr;
    }
    
    qDebug() << "[ModelBuilderVisitor] visitViewDefinitionBody: text =" << QString::fromStdString(ctx->getText());
    qDebug() << "[ModelBuilderVisitor] visitViewDefinitionBody: processing" << ctx->viewDefinitionBodyItem().size() << "body items";
    
    // 处理 viewDefinitionBodyItem
    for (auto itemCtx : ctx->viewDefinitionBodyItem()) {
        if (itemCtx) {
            if (itemCtx->definitionBodyItem()) {
                qDebug() << "[ModelBuilderVisitor] visitViewDefinitionBody: found definitionBodyItem";
                visitDefinitionBodyItem(itemCtx->definitionBodyItem());
            } else if (itemCtx->elementFilterMember()) {
                qDebug() << "[ModelBuilderVisitor] visitViewDefinitionBody: found elementFilterMember";
                visitElementFilterMember(itemCtx->elementFilterMember());
            } else if (itemCtx->viewRenderingMember()) {
                qDebug() << "[ModelBuilderVisitor] visitViewDefinitionBody: found viewRenderingMember";
                visitViewRenderingMember(itemCtx->viewRenderingMember());
            } else {
                qDebug() << "[ModelBuilderVisitor] visitViewDefinitionBody: unknown body item type";
            }
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitViewBody(SysML::SysMLParser::ViewBodyContext* ctx)
{
    if (!ctx) {
        qDebug() << "[ModelBuilderVisitor] visitViewBody: ctx is null";
        return nullptr;
    }
    
    qDebug() << "[ModelBuilderVisitor] visitViewBody: text =" << QString::fromStdString(ctx->getText());
    qDebug() << "[ModelBuilderVisitor] visitViewBody: processing" << ctx->viewBodyItem().size() << "body items";
    
    // 处理 viewBodyItem
    // 注意：检查顺序必须与语法规则中的顺序一致（satisfyStatement 和 exposeStatement 在前）
    for (auto itemCtx : ctx->viewBodyItem()) {
        if (itemCtx) {
            if (itemCtx->satisfyStatement()) {
                qDebug() << "[ModelBuilderVisitor] visitViewBody: found satisfyStatement";
                visitSatisfyStatement(itemCtx->satisfyStatement());
            } else if (itemCtx->exposeStatement()) {
                qDebug() << "[ModelBuilderVisitor] visitViewBody: found exposeStatement";
                visitExposeStatement(itemCtx->exposeStatement());
            } else if (itemCtx->elementFilterMember()) {
                qDebug() << "[ModelBuilderVisitor] visitViewBody: found elementFilterMember";
                visitElementFilterMember(itemCtx->elementFilterMember());
            } else if (itemCtx->viewRenderingMember()) {
                qDebug() << "[ModelBuilderVisitor] visitViewBody: found viewRenderingMember";
                visitViewRenderingMember(itemCtx->viewRenderingMember());
            } else if (itemCtx->definitionBodyItem()) {
                qDebug() << "[ModelBuilderVisitor] visitViewBody: found definitionBodyItem";
                visitDefinitionBodyItem(itemCtx->definitionBodyItem());
            } else {
                qDebug() << "[ModelBuilderVisitor] visitViewBody: unknown body item type";
            }
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitConcernBody(SysML::SysMLParser::ConcernBodyContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 处理 concernBodyItem
    for (auto itemCtx : ctx->concernBodyItem()) {
        if (itemCtx) {
            if (itemCtx->definitionBodyItem()) {
                visitDefinitionBodyItem(itemCtx->definitionBodyItem());
            } else if (itemCtx->requirementMember()) {
                visitRequirementMember(itemCtx->requirementMember());
            } else if (itemCtx->docStatement()) {
                visitDocStatement(itemCtx->docStatement());
            }
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitExposeStatement(SysML::SysMLParser::ExposeStatementContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    ExposeStatement* exposeStmt = new ExposeStatement();
    
    // 处理 membershipExpose 或 namespaceExpose
    if (ctx->membershipExpose()) {
        auto membershipCtx = ctx->membershipExpose();
        exposeStmt->setExposeType(ExposeStatement::MembershipExpose);
        
        auto qnameCtx = membershipCtx->qualifiedName();
        if (qnameCtx) {
            QStringList nameParts = getQualifiedNameParts(qnameCtx);
            QString qualifiedName = nameParts.join("::");
            
            // 检查是否有递归标记 (::**)
            QString text = QString::fromStdString(membershipCtx->getText());
            if (text.contains("::**")) {
                exposeStmt->setRecursive(true);
                // 如果 qualifiedName 不包含 ::**，添加它
                if (!qualifiedName.endsWith("::**")) {
                    qualifiedName += "::**";
                }
            }
            
            exposeStmt->setExposedQualifiedName(qualifiedName);
        }
    } else if (ctx->namespaceExpose()) {
        auto namespaceCtx = ctx->namespaceExpose();
        exposeStmt->setExposeType(ExposeStatement::NamespaceExpose);
        
        auto qnameCtx = namespaceCtx->qualifiedName();
        if (qnameCtx) {
            QStringList nameParts = getQualifiedNameParts(qnameCtx);
            QString qualifiedName = nameParts.join("::");
            
            // 检查是否有递归标记 (::**)
            QString text = QString::fromStdString(namespaceCtx->getText());
            if (text.contains("::**")) {
                exposeStmt->setRecursive(true);
                // 如果 qualifiedName 不包含 ::**，添加它
                if (!qualifiedName.endsWith("::**")) {
                    qualifiedName += "::**";
                }
            }
            
            exposeStmt->setExposedQualifiedName(qualifiedName);
        }
    }
    
    // 添加到当前父元素（通常是 ViewUsage）
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(exposeStmt);
        
        // 如果是 ViewUsage，添加到 exposedElements
        if (ViewUsage* viewUsage = qobject_cast<ViewUsage*>(parent)) {
            // 查找暴露的元素
            QString qualifiedName = exposeStmt->exposedQualifiedName();
            qDebug() << "[ModelBuilderVisitor] visitExposeStatement: Processing expose for ViewUsage"
                     << viewUsage->declaredName() << "qualifiedName:" << qualifiedName;
            
            // 处理递归标记 ::**，去掉它后再查找元素
            QString nameForLookup = qualifiedName;
            if (nameForLookup.endsWith("::**")) {
                nameForLookup = nameForLookup.left(nameForLookup.length() - 4); // 去掉 "::**"
            } else if (nameForLookup.endsWith("**")) {
                nameForLookup = nameForLookup.left(nameForLookup.length() - 2); // 去掉 "**"
            }
            
            QStringList nameParts = nameForLookup.split("::", Qt::SkipEmptyParts);
            qDebug() << "[ModelBuilderVisitor] visitExposeStatement: nameParts:" << nameParts;
            
            // 修复：从 ViewUsage 的 owner 开始查找，而不是从 ViewUsage 本身
            // 因为 expose 语句中的 qualifiedName 是相对于 ViewUsage 所在的 Package 的
            Element* searchScope = viewUsage->owner();
            qDebug() << "[ModelBuilderVisitor] visitExposeStatement: ViewUsage owner:"
                     << (searchScope ? searchScope->declaredName() : "null");
            
            if (!searchScope) {
                // 如果没有 owner，回退到当前父元素
                searchScope = getCurrentParent();
                qDebug() << "[ModelBuilderVisitor] visitExposeStatement: Using current parent as searchScope:"
                         << (searchScope ? searchScope->declaredName() : "null");
            }
            
            // 如果 qualifiedName 的第一个部分与 searchScope 的名称匹配，说明是绝对路径
            // 但我们应该跳过第一个部分，直接从当前 scope 查找剩余部分
            Element* element = nullptr;
            if (!nameParts.isEmpty() && searchScope && 
                nameParts.first() == searchScope->declaredName()) {
                // 如果第一个部分匹配当前 scope，跳过第一个部分，从当前 scope 查找剩余部分
                qDebug() << "[ModelBuilderVisitor] visitExposeStatement: First part matches scope, skipping first part";
                if (nameParts.size() > 1) {
                    QStringList remainingParts = nameParts.mid(1);
                    qDebug() << "[ModelBuilderVisitor] visitExposeStatement: Remaining parts:" << remainingParts;
                    element = findElementByQualifiedNameFromScope(remainingParts, searchScope);
                } else {
                    // 如果只有一个部分且匹配，说明就是当前 scope 本身
                    element = searchScope;
                }
            } else {
                // 相对路径，从当前 scope 开始查找
                qDebug() << "[ModelBuilderVisitor] visitExposeStatement: Using relative path lookup";
                element = findElementByQualifiedNameFromScope(nameParts, searchScope);
            }
            
            // 如果还是找不到，尝试从资源根开始查找（绝对路径）
            if (!element && m_resource && !m_resource->contents().isEmpty()) {
                qDebug() << "[ModelBuilderVisitor] visitExposeStatement: Retrying with root scope as absolute path";
                Element* rootScope = m_resource->contents().first();
                element = findElementByQualifiedNameFromScope(nameParts, rootScope);
            }
            
            if (element) {
                viewUsage->addExposedElement(element);
                qDebug() << "[ModelBuilderVisitor] visitExposeStatement: Added exposed element" 
                         << element->declaredName() << "(" << element->qualifiedName() << ")"
                         << "to ViewUsage" << viewUsage->declaredName();
            } else {
                // 如果找不到元素，仍然添加 ExposeStatement，以便序列化时能够保留原始文本
                // 这种情况下，exposedElements 可能为空，但 ExposeStatement 会保留 qualifiedName
                qWarning() << "[ModelBuilderVisitor] visitExposeStatement: Element not found:" 
                           << qualifiedName << "in scope" << (searchScope ? searchScope->declaredName() : "null")
                           << "nameParts:" << nameParts;
            }
        }
    }
    
    return exposeStmt;
}

std::any ModelBuilderVisitor::visitElementFilterMember(SysML::SysMLParser::ElementFilterMemberContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    ElementFilterMember* filterMember = new ElementFilterMember();
    
    // 获取过滤表达式
    auto filterExprCtx = ctx->filterExpression();
    if (filterExprCtx) {
        QString filterExpr = QString::fromStdString(filterExprCtx->getText());
        filterMember->setFilterExpression(filterExpr);
    }
    
    // 添加到当前父元素
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(filterMember);
        
        // 如果是 ViewDefinition 或 ViewUsage，设置 filterExpression
        if (ViewDefinition* viewDef = qobject_cast<ViewDefinition*>(parent)) {
            viewDef->setFilterExpression(filterMember->filterExpression());
        } else if (ViewUsage* viewUsage = qobject_cast<ViewUsage*>(parent)) {
            viewUsage->setFilterExpression(filterMember->filterExpression());
        }
    }
    
    return filterMember;
}

std::any ModelBuilderVisitor::visitViewRenderingMember(SysML::SysMLParser::ViewRenderingMemberContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    ViewRenderingMember* renderMember = new ViewRenderingMember();
    
    // 处理 viewRenderingUsage，提取 renderMethod
    QString renderMethod;
    auto renderUsageCtx = ctx->viewRenderingUsage();
    if (renderUsageCtx) {
        // viewRenderingUsage: ownedReferenceSubsetting featureSpecializationPart?
        auto ownedRefCtx = renderUsageCtx->ownedReferenceSubsetting();
        if (ownedRefCtx) {
            // ownedReferenceSubsetting 可以是：
            // 1. qualifiedName
            // 2. memberAccessExpression
            // 3. identifier (简化：支持简单的标识符)
            
            // 对于 "render asRequirementTable;"，应该是简单的 identifier
            if (ownedRefCtx->identifier()) {
                // 直接提取标识符
                renderMethod = getText(ownedRefCtx->identifier()->IDENTIFIER());
                qDebug() << "[ModelBuilderVisitor] visitViewRenderingMember: extracted renderMethod from identifier:" << renderMethod;
            } else if (ownedRefCtx->qualifiedName()) {
                // 如果是 qualifiedName，提取最后一个部分作为 renderMethod
                QStringList parts = getQualifiedNameParts(ownedRefCtx->qualifiedName());
                if (!parts.isEmpty()) {
                    renderMethod = parts.last();
                    qDebug() << "[ModelBuilderVisitor] visitViewRenderingMember: extracted renderMethod from qualifiedName:" << renderMethod;
                }
            } else {
                // 如果都不匹配，尝试从上下文中提取文本（备用方案）
                QString text = QString::fromStdString(renderUsageCtx->getText());
                text = text.trimmed();
                // 移除可能的空格和分号
                if (text.endsWith(';')) {
                    text = text.left(text.length() - 1);
                }
                renderMethod = text;
                qDebug() << "[ModelBuilderVisitor] visitViewRenderingMember: extracted renderMethod from text:" << renderMethod;
            }
        }
    }
    renderMember->setDeclaredName(renderMethod);
    
    // 添加到当前父元素
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(renderMember);
        
        // 如果是 ViewDefinition，设置 renderingUsage
        if (ViewDefinition* viewDef = qobject_cast<ViewDefinition*>(parent)) {
            viewDef->setRenderingUsage(renderMember);
        }
        // 如果是 ViewUsage，设置 renderMethod（关键修复）
        else if (ViewUsage* viewUsage = qobject_cast<ViewUsage*>(parent)) {
            if (!renderMethod.isEmpty()) {
                viewUsage->setRenderMethod(renderMethod);
                qDebug() << "[ModelBuilderVisitor] visitViewRenderingMember: Set renderMethod for ViewUsage:" 
                         << viewUsage->declaredName() << "->" << renderMethod;
            } else {
                qDebug() << "[ModelBuilderVisitor] visitViewRenderingMember: renderMethod is empty for ViewUsage:" 
                         << viewUsage->declaredName();
            }
        }
    }
    
    return renderMember;
}

std::any ModelBuilderVisitor::visitFrameConcernMember(SysML::SysMLParser::FrameConcernMemberContext* ctx)
{
    if (!ctx) {
        qDebug() << "[ModelBuilderVisitor] visitFrameConcernMember: ctx is null";
        return nullptr;
    }
    
    qDebug() << "[ModelBuilderVisitor] visitFrameConcernMember: text =" << QString::fromStdString(ctx->getText());
    
    FrameConcernMember* frameMember = new FrameConcernMember();
    
    // 处理 usageDeclaration（如果有）
    // 例如：frame concern vs:VehicleSafety;
    auto usageDeclCtx = ctx->usageDeclaration();
    qDebug() << "[ModelBuilderVisitor] visitFrameConcernMember: usageDeclCtx is" << (usageDeclCtx ? "not null" : "null");
    if (usageDeclCtx) {
        auto featureDeclCtx = usageDeclCtx->featureDeclaration();
        if (featureDeclCtx) {
            // 获取标识符（如果有，如 vs）
            // featureDeclaration 可能是 identification? featureSpecializationPart?
            // 对于 vs:VehicleSafety，identification 应该只包含一个标识符 vs
            auto identCtx = featureDeclCtx->identification();
            if (identCtx) {
                auto identifiers = identCtx->identifier();
                // identification 通常只有一个标识符（名称），除非是命名空间路径
                // 对于 frame concern vs:VehicleSafety，应该只有一个标识符 vs
                if (!identifiers.empty()) {
                    // 取最后一个标识符作为名称（如果是命名空间路径，最后一个才是实际名称）
                    frameMember->setIdentifier(getText(identifiers.back()->IDENTIFIER()));
                }
            }
            
            // 获取类型（从 featureSpecializationPart）
            auto specCtx = featureDeclCtx->featureSpecializationPart();
            qDebug() << "[ModelBuilderVisitor] visitFrameConcernMember: specCtx is" << (specCtx ? "not null" : "null");
            if (specCtx) {
                auto typingCtx = specCtx->typingExpression();
                qDebug() << "[ModelBuilderVisitor] visitFrameConcernMember: typingCtx is" << (typingCtx ? "not null" : "null");
                if (typingCtx) {
                    auto qnameCtx = typingCtx->qualifiedName();
                    qDebug() << "[ModelBuilderVisitor] visitFrameConcernMember: qnameCtx is" << (qnameCtx ? "not null" : "null");
                    if (qnameCtx) {
                        QStringList concernNameParts = getQualifiedNameParts(qnameCtx);
                        qDebug() << "[ModelBuilderVisitor] visitFrameConcernMember: concernNameParts =" << concernNameParts;
                        Element* concern = findElementByQualifiedName(concernNameParts);
                        qDebug() << "[ModelBuilderVisitor] visitFrameConcernMember: concern is" << (concern ? concern->declaredName() : "null");
                        if (concern) {
                            frameMember->setConcern(concern);
                        } else {
                            // 前向引用：如果找不到，保存 qualifiedName，稍后解析
                            QString concernQualifiedName = concernNameParts.join("::");
                            frameMember->setConcernQualifiedName(concernQualifiedName);
                            qDebug() << "[ModelBuilderVisitor] visitFrameConcernMember: concern not found, saved qualifiedName for later resolution:" << concernQualifiedName;
                        }
                    }
                }
            }
        } else {
            qDebug() << "[ModelBuilderVisitor] visitFrameConcernMember: featureDeclCtx is null";
        }
    } else {
        qDebug() << "[ModelBuilderVisitor] visitFrameConcernMember: usageDeclCtx is null";
        // 如果 featureDeclaration 不存在，可能是 featureSpecializationPart 直接存在
        // 这种情况应该不会发生，但为了安全起见
    }
    
    qDebug() << "[ModelBuilderVisitor] visitFrameConcernMember: identifier =" << frameMember->identifier() << ", concern =" << (frameMember->concern() ? frameMember->concern()->declaredName() : "null");
    
    // 添加到当前父元素（通常是 ViewpointDefinition）
    Element* parent = getCurrentParent();
    if (parent) {
        parent->addOwnedElement(frameMember);
        
        // 如果是 ViewpointDefinition，添加到 framedConcerns
        if (ViewpointDefinition* viewpointDef = qobject_cast<ViewpointDefinition*>(parent)) {
            Element* concern = frameMember->concern();
            if (concern) {
                viewpointDef->addFramedConcern(concern);
            }
        }
    }
    
    return frameMember;
}

std::any ModelBuilderVisitor::visitSatisfyStatement(SysML::SysMLParser::SatisfyStatementContext* ctx)
{
    if (!ctx) {
        qDebug() << "[ModelBuilderVisitor] visitSatisfyStatement: ctx is null";
        return nullptr;
    }
    
    qDebug() << "[ModelBuilderVisitor] visitSatisfyStatement: text =" << QString::fromStdString(ctx->getText());
    
    // 处理两种形式的 satisfy 语句
    // 形式1: satisfy requirement sv:SafetyViewpoint;
    // 形式2: satisfy requirement by subject { ... }
    
    QString text = QString::fromStdString(ctx->getText());
    if (text.contains(" by ")) {
        // 形式2：复杂形式
        // TODO: 实现复杂形式的 satisfy 语句
    } else {
        // 形式1：简单形式 - satisfy requirement sv:SafetyViewpoint;
        // ownedReferenceSubsetting 可能是 sv（标识符）或 sv:SafetyViewpoint（qualifiedName）
        // featureSpecializationPart 可能包含 :SafetyViewpoint（typingExpression）
        
        QString identifier;
        QStringList viewpointNameParts;
        
        // 先检查 featureSpecializationPart（优先，因为 sv:SafetyViewpoint 中的 :SafetyViewpoint 在这里）
        auto specCtx = ctx->featureSpecializationPart();
        if (specCtx) {
            auto typingCtx = specCtx->typingExpression();
            if (typingCtx) {
                auto qnameCtx = typingCtx->qualifiedName();
                if (qnameCtx) {
                    viewpointNameParts = getQualifiedNameParts(qnameCtx);
                    qDebug() << "[ModelBuilderVisitor] visitSatisfyStatement: found type in featureSpecializationPart:" << viewpointNameParts;
                }
            }
        }
        
        // 然后检查 ownedReferenceSubsetting
        auto refCtxs = ctx->ownedReferenceSubsetting();
        if (!refCtxs.empty()) {
            auto refCtx = refCtxs[0];
            if (refCtx) {
                if (refCtx->identifier()) {
                    // 如果是标识符（如 sv），这就是标识符
                    identifier = getText(refCtx->identifier()->IDENTIFIER());
                    qDebug() << "[ModelBuilderVisitor] visitSatisfyStatement: found identifier in ownedReferenceSubsetting:" << identifier;
                } else if (refCtx->qualifiedName()) {
                    // 如果是 qualifiedName，可能是完整的名称（如 SafetyViewpoint）或者包含标识符
                    QStringList parts = getQualifiedNameParts(refCtx->qualifiedName());
                    qDebug() << "[ModelBuilderVisitor] visitSatisfyStatement: found qualifiedName in ownedReferenceSubsetting:" << parts;
                    // 如果还没有从 featureSpecializationPart 获取到类型，使用这个
                    if (viewpointNameParts.isEmpty()) {
                        viewpointNameParts = parts;
                    } else {
                        // 如果已经有类型，这个应该是标识符
                        // 对于 sv:SafetyViewpoint，sv 是标识符，SafetyViewpoint 是类型
                        if (parts.size() == 1) {
                            identifier = parts[0];  // 修复：设置标识符
                            qDebug() << "[ModelBuilderVisitor] visitSatisfyStatement: set identifier from qualifiedName:" << identifier;
                        }
                    }
                }
            }
        }
        
        // 查找 ViewpointDefinition
        if (!viewpointNameParts.isEmpty()) {
            qDebug() << "[ModelBuilderVisitor] visitSatisfyStatement: viewpointNameParts =" << viewpointNameParts << ", identifier =" << identifier;
            Element* viewpoint = findElementByQualifiedName(viewpointNameParts);
            qDebug() << "[ModelBuilderVisitor] visitSatisfyStatement: viewpoint is" << (viewpoint ? viewpoint->declaredName() : "null");
            
            // 如果是 ViewUsage，设置 satisfiedViewpoint
            Element* parent = getCurrentParent();
            qDebug() << "[ModelBuilderVisitor] visitSatisfyStatement: parent is" << (parent ? parent->elementType() : "null");
            if (ViewUsage* viewUsage = qobject_cast<ViewUsage*>(parent)) {
                viewUsage->setSatisfiedViewpoint(viewpoint);
                if (!identifier.isEmpty()) {
                    viewUsage->setSatisfiedViewpointIdentifier(identifier);
                }
                qDebug() << "[ModelBuilderVisitor] visitSatisfyStatement: set satisfiedViewpoint to" << (viewpoint ? viewpoint->declaredName() : "null");
            } else {
                qDebug() << "[ModelBuilderVisitor] visitSatisfyStatement: parent is not ViewUsage";
            }
        } else {
            qDebug() << "[ModelBuilderVisitor] visitSatisfyStatement: viewpointNameParts is empty";
        }
    }
    
    return nullptr;
}

std::any ModelBuilderVisitor::visitMetadataAnnotation(
    SysML::SysMLParser::MetadataAnnotationContext* ctx)
{
    if (!ctx) {
        return nullptr;
    }
    
    // 获取当前父元素（注解所属的元素）
    Element* parent = getCurrentParent();
    if (!parent) {
        reportError("No current element for annotation", ctx);
        return nullptr;
    }
    
    // 获取注解名称（identifier）
    auto identifierCtx = ctx->identifier();
    if (!identifierCtx) {
        reportError("MetadataAnnotation missing identifier", ctx);
        return nullptr;
    }
    QString annotationName = getText(identifierCtx->IDENTIFIER());
    if (annotationName.isEmpty()) {
        reportError("MetadataAnnotation identifier is empty", ctx);
        return nullptr;
    }
    
    qDebug() << "[ModelBuilderVisitor] visitMetadataAnnotation: annotationName =" << annotationName;
    
    // 创建Annotation对象
    Annotation* annotation = new Annotation();
    annotation->setAnnotationName(annotationName);
    
    // 解析metadataAssignment列表
    auto assignments = ctx->metadataAssignment();
    for (auto* assignmentCtx : assignments) {
        QString key = getMetadataAssignmentKey(assignmentCtx);
        QVariant value = getMetadataAssignmentValue(assignmentCtx);
        if (!key.isEmpty()) {
            annotation->setMetadataAssignment(key, value);
            qDebug() << "[ModelBuilderVisitor] visitMetadataAnnotation: assignment" << key << "=" << value;
        }
    }
    
    // 将注解添加到当前元素
    parent->addAnnotation(annotation);
    
    qDebug() << "[ModelBuilderVisitor] visitMetadataAnnotation: added annotation to" 
             << parent->elementType() << parent->declaredName();
    
    return annotation;
}

QString ModelBuilderVisitor::getMetadataAssignmentKey(
    SysML::SysMLParser::MetadataAssignmentContext* ctx) const
{
    if (!ctx) {
        return QString();
    }
    
    // 从metadataAssignment中提取key
    // 格式：identifier ':' expression
    auto identifierCtx = ctx->identifier();
    if (identifierCtx) {
        return getText(identifierCtx->IDENTIFIER());
    }
    return QString();
}

QVariant ModelBuilderVisitor::getMetadataAssignmentValue(
    SysML::SysMLParser::MetadataAssignmentContext* ctx) const
{
    if (!ctx) {
        return QVariant();
    }
    
    // 从metadataAssignment中提取value
    // 可能是字符串、数字、布尔值等
    auto expressionCtx = ctx->expression();
    if (!expressionCtx) {
        return QVariant();
    }
    
    // 通过表达式层次结构访问：expression -> logicalOrExpression -> ... -> unaryExpression -> primaryExpression
    // 注意：logicalOrExpression() 返回指针，而 logicalAndExpression() 等返回 vector
    auto logicalOrExprCtx = expressionCtx->logicalOrExpression();
    if (logicalOrExprCtx) {
        auto logicalAndExprCtxs = logicalOrExprCtx->logicalAndExpression();
        if (!logicalAndExprCtxs.empty()) {
            auto logicalAndExprCtx = logicalAndExprCtxs[0];
            auto comparisonExprCtxs = logicalAndExprCtx->comparisonExpression();
            if (!comparisonExprCtxs.empty()) {
                auto comparisonExprCtx = comparisonExprCtxs[0];
                auto additiveExprCtxs = comparisonExprCtx->additiveExpression();
                if (!additiveExprCtxs.empty()) {
                    auto additiveExprCtx = additiveExprCtxs[0];
                    auto multiplicativeExprCtxs = additiveExprCtx->multiplicativeExpression();
                    if (!multiplicativeExprCtxs.empty()) {
                        auto multiplicativeExprCtx = multiplicativeExprCtxs[0];
                        auto unaryExprCtxs = multiplicativeExprCtx->unaryExpression();
                        if (!unaryExprCtxs.empty()) {
                            auto unaryExprCtx = unaryExprCtxs[0];
                            auto primaryExprCtx = unaryExprCtx->primaryExpression();
                            if (primaryExprCtx) {
                                auto literalExprCtx = primaryExprCtx->literalExpression();
                                if (literalExprCtx) {
                                    // LiteralExpressionContext 直接有 String(), Integer(), Real(), Boolean() 方法
                                    // 字符串字面量
                                    if (literalExprCtx->String()) {
                                        QString text = QString::fromStdString(literalExprCtx->String()->getText());
                                        // 移除引号
                                        if (text.startsWith('"') && text.endsWith('"')) {
                                            text = text.mid(1, text.length() - 2);
                                        } else if (text.startsWith('\'') && text.endsWith('\'')) {
                                            text = text.mid(1, text.length() - 2);
                                        }
                                        return text;
                                    }
                                    
                                    // 整数字面量
                                    if (literalExprCtx->Integer()) {
                                        bool ok;
                                        qint64 value = QString::fromStdString(literalExprCtx->Integer()->getText()).toLongLong(&ok);
                                        if (ok) {
                                            return value;
                                        }
                                    }
                                    
                                    // 实数字面量
                                    if (literalExprCtx->Real()) {
                                        bool ok;
                                        double value = QString::fromStdString(literalExprCtx->Real()->getText()).toDouble(&ok);
                                        if (ok) {
                                            return value;
                                        }
                                    }
                                    
                                    // 布尔字面量
                                    if (literalExprCtx->Boolean()) {
                                        QString text = QString::fromStdString(literalExprCtx->Boolean()->getText());
                                        return text.toLower() == "true";
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 如果不是字面量，返回表达式文本
    QString exprText = QString::fromStdString(expressionCtx->getText());
    return exprText;
}

SYSML_NAMESPACE_END
