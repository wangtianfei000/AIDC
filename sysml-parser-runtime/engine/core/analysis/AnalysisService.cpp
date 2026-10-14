#include "AnalysisService.h"
#include "../semantic/UnitSystem.h"

#include <QHash>
#include <QJsonArray>
#include <QSet>
#include <QStringList>

#include <cmath>
#include <functional>

namespace Aidc {
namespace {

using Semantic::Dimensions;
using Semantic::UnitDefinition;
using Semantic::UnitSystem;

struct Quantity {
    bool valid = false;
    bool boolean = false;
    bool boolValue = false;
    double siValue = 0.0;
    QString unit;
    Dimensions dimensions;
    QString status = "computed";
    QString message;
    QSet<QString> sourceIds;
};

Dimensions expectedDimensions(const QString& typeName)
{
    return UnitSystem::standard().dimensionsForType(typeName);
}

QString dimensionsText(const Dimensions& dimensions)
{
    return UnitSystem::standard().dimensionsText(dimensions);
}

bool sameDimensions(const Dimensions& left, const Dimensions& right)
{
    return UnitSystem::sameDimensions(left, right);
}

double displayValue(const Quantity& value, const QString& unit = {})
{
    const QString selected = unit.isEmpty() ? value.unit : unit;
    UnitDefinition definition;
    return UnitSystem::standard().resolve(selected, &definition)
        ? value.siValue / definition.factor
        : value.siValue;
}

Quantity failure(const QString& status, const QString& message)
{
    Quantity value; value.status = status; value.message = message; return value;
}

QString normalizedReference(QString value)
{
    value.replace("::", ".");
    while (value.startsWith('.')) value.remove(0, 1);
    return value;
}

QStringList referenceSuffixes(const QString& qualifiedName)
{
    const QStringList parts = normalizedReference(qualifiedName).split('.', Qt::SkipEmptyParts);
    QStringList result;
    for (int start = 0; start < parts.size(); ++start) {
        result.append(parts.mid(start).join('.'));
    }
    return result;
}

QString preferredProductUnit(const Quantity& left, const Quantity& right, const Dimensions& result)
{
    const QSet<QString> pair{left.unit, right.unit};
    if (pair == QSet<QString>{"V", "Ah"}) return "Wh";
    if (pair == QSet<QString>{"V", "A"}) return "W";
    if (left.dimensions.isEmpty()) return right.unit;
    if (right.dimensions.isEmpty()) return left.unit;
    return UnitSystem::standard().preferredUnit(result);
}

QString preferredDivisionUnit(const Quantity& left, const Quantity& right, const Dimensions& result)
{
    if (left.unit == "Wh" && right.unit == "W") return "h";
    if (right.dimensions.isEmpty()) return left.unit;
    return UnitSystem::standard().preferredUnit(result);
}

class Evaluator {
public:
    Evaluator(const QJsonObject& snapshot, const std::atomic_bool* canceled)
        : m_snapshot(snapshot), m_canceled(canceled)
    {
        const QJsonObject model = snapshot.value("calculationModel").toObject();
        for (const QJsonValue& value : model.value("parameters").toArray()) {
            const QJsonObject object = value.toObject();
            m_parameters.insert(object.value("id").toString(), object);
        }
        for (const QJsonValue& value : model.value("definitions").toArray()) {
            const QJsonObject object = value.toObject();
            m_definitions.insert(object.value("id").toString(), object);
            m_definitionByName.insert(object.value("name").toString(), object.value("id").toString());
        }
        for (const QJsonValue& value : model.value("usages").toArray()) {
            const QJsonObject object = value.toObject();
            m_usageByResult.insert(object.value("resultParameterId").toString(), object);
        }
        for (const QJsonValue& value : model.value("constraints").toArray()) {
            const QJsonObject object = value.toObject();
            m_constraintsByAnalysis[object.value("analysisId").toString()].append(object);
        }
        for (const QJsonValue& value : model.value("analyses").toArray()) {
            const QJsonObject analysis = value.toObject();
            const QString id = analysis.value("id").toString();
            m_analyses.insert(id, analysis);
            m_analysisOrder.append(id);
            for (const QJsonValue& parameterId : analysis.value("resultParameterIds").toArray()) {
                const QString pid = parameterId.toString();
                m_analysisParameterByName[id].insert(m_parameters.value(pid).value("name").toString(), pid);
            }
        }
        QSet<QString> semanticOwners;
        for (const QJsonObject& definition : m_definitions) semanticOwners.insert(definition.value("id").toString());
        for (const QJsonObject& analysis : m_analyses) semanticOwners.insert(analysis.value("id").toString());
        for (auto it = m_parameters.begin(); it != m_parameters.end(); ++it) {
            const QJsonObject parameter = it.value();
            if (!semanticOwners.contains(parameter.value("ownerId").toString()) && parameter.value("expressionAst").isObject()) {
                QStringList keys = referenceSuffixes(parameter.value("qualifiedName").toString());
                if (keys.isEmpty()) keys.append(parameter.value("name").toString());
                for (const QString& key : keys) {
                    if (!key.isEmpty()) m_globalParameterIdsByReference[key].append(it.key());
                }
            }
        }
    }

    QJsonObject run()
    {
        QJsonArray analyses;
        for (const QString& analysisId : m_analysisOrder) {
            const QJsonObject analysis = m_analyses.value(analysisId);
            if (isCanceled()) break;
            m_currentAnalysisId = analysis.value("id").toString();
            m_subjectReference = analysis.value("subjectReference").toString();
            QJsonArray results;
            for (const QJsonValue& value : analysis.value("resultParameterIds").toArray()) {
                const QString id = value.toString();
                const Quantity quantity = evaluateParameter(id);
                results.append(resultObject(id, quantity));
            }
            QJsonArray constraints;
            for (const QJsonObject& constraint : m_constraintsByAnalysis.value(m_currentAnalysisId)) {
                constraints.append(evaluateConstraint(constraint));
            }
            QJsonArray order;
            for (const QString& id : m_evaluationOrder) if (m_analysisParameterByName.value(m_currentAnalysisId).values().contains(id)) order.append(id);
            analyses.append(QJsonObject{
                {"analysisId", m_currentAnalysisId}, {"name", analysis.value("name")},
                {"subjectId", analysis.value("subjectId")}, {"subjectReference", m_subjectReference},
                {"results", results}, {"constraints", constraints},
                {"evaluationOrder", order}, {"dependencies", dependencyArray(m_currentAnalysisId)}
            });
        }
        return {
            {"schemaVersion", "0.1"},
            {"document", m_snapshot.value("document")},
            {"engine", QJsonObject{{"name", "AIDC Unit-aware Calculation Engine"},
                                     {"version", AnalysisService::EngineVersion}}},
            {"analyses", analyses}, {"diagnostics", m_diagnostics},
            {"capabilities", QJsonObject{{"scalar-arithmetic", "supported"},
                                           {"unit-conversion", "supported"},
                                           {"dimension-check", "supported"},
                                           {"dependency-cycle", "supported"},
                                           {"constraint-margin", "supported"},
                                           {"advanced-functions", "unsupported"}}}
        };
    }

private:
    bool isCanceled() const { return m_canceled && m_canceled->load(); }

    Quantity evaluateParameter(const QString& id)
    {
        if (m_values.contains(id)) return m_values.value(id);
        if (m_visiting.contains(id)) {
            const Quantity value = failure("cycle", QStringLiteral("Circular dependency at %1").arg(m_parameters.value(id).value("name").toString()));
            m_values.insert(id, value); addDiagnostic("AIDC_ANALYSIS_CYCLE", value.message, id); return value;
        }
        if (!m_parameters.contains(id)) return failure("missing-input", QStringLiteral("Unknown parameter %1").arg(id));
        m_visiting.insert(id);
        m_stack.append(id);
        const QJsonObject parameter = m_parameters.value(id);
        Quantity value;
        if (m_usageByResult.contains(id)) value = invoke(m_usageByResult.value(id));
        else value = evaluateNode(parameter.value("expressionAst").toObject(), {});
        if (value.valid) {
            const Dimensions expected = expectedDimensions(parameter.value("typeName").toString());
            if (!expected.isEmpty() && !sameDimensions(expected, value.dimensions)) {
                value = failure("unit-error", QStringLiteral("%1 expects %2 but expression has %3")
                    .arg(parameter.value("name").toString(), dimensionsText(expected), dimensionsText(value.dimensions)));
                addDiagnostic("AIDC_ANALYSIS_UNIT_ERROR", value.message, id);
            }
        } else if (value.status != "cycle") {
            addDiagnostic("AIDC_ANALYSIS_" + value.status.toUpper().replace('-', '_'), value.message, id);
        }
        if (value.valid) value.sourceIds.insert(id);
        m_stack.removeLast(); m_visiting.remove(id); m_values.insert(id, value); m_evaluationOrder.append(id);
        return value;
    }

    Quantity invoke(const QJsonObject& usage)
    {
        QString definitionId = usage.value("definitionId").toString();
        if (definitionId.isEmpty()) definitionId = m_definitionByName.value(usage.value("definitionName").toString());
        if (!m_definitions.contains(definitionId)) return failure("unsupported", "Calculation definition is unresolved");
        const QJsonObject definition = m_definitions.value(definitionId);
        const QJsonArray inputIds = definition.value("inputParameterIds").toArray();
        const QJsonArray arguments = usage.value("arguments").toArray();
        if (inputIds.size() != arguments.size()) return failure("missing-input", "Calculation argument count does not match definition");
        QHash<QString, Quantity> locals;
        for (int i = 0; i < inputIds.size(); ++i) {
            const QString inputId = inputIds.at(i).toString();
            const QString name = m_parameters.value(inputId).value("name").toString();
            Quantity argument;
            const QJsonArray bindings = usage.value("bindings").toArray();
            const QString actualId = i < bindings.size()
                ? bindings.at(i).toObject().value("actualParameterId").toString()
                : QString();
            if (!actualId.isEmpty() && m_parameters.contains(actualId)) {
                addDependency(actualId);
                argument = evaluateParameter(actualId);
            } else {
                argument = evaluateNode(arguments.at(i).toObject(), {});
            }
            if (!argument.valid) return argument;
            const Dimensions expected = expectedDimensions(m_parameters.value(inputId).value("typeName").toString());
            if (!expected.isEmpty() && !sameDimensions(expected, argument.dimensions)) {
                return failure("unit-error", QStringLiteral("Argument %1 has incompatible dimension").arg(name));
            }
            locals.insert(name, argument);
        }
        std::function<Quantity(const QString&)> localResolver;
        QSet<QString> localVisiting;
        localResolver = [&](const QString& name) -> Quantity {
            if (locals.contains(name)) return locals.value(name);
            QString parameterId;
            for (const QJsonValue& localId : definition.value("localParameterIds").toArray()) {
                if (m_parameters.value(localId.toString()).value("name") == name) { parameterId = localId.toString(); break; }
            }
            if (parameterId.isEmpty()) return resolveReference(name);
            if (localVisiting.contains(name)) return failure("cycle", "Circular local calculation dependency");
            localVisiting.insert(name);
            Quantity value = evaluateNode(m_parameters.value(parameterId).value("expressionAst").toObject(), locals, localResolver);
            localVisiting.remove(name); if (value.valid) value.sourceIds.insert(parameterId);
            locals.insert(name, value); return value;
        };
        for (const QJsonValue& localId : definition.value("localParameterIds").toArray()) {
            localResolver(m_parameters.value(localId.toString()).value("name").toString());
        }
        const QJsonArray outputIds = definition.value("outputParameterIds").toArray();
        if (outputIds.isEmpty()) return failure("unsupported", "Calculation definition has no return parameter");
        const QJsonObject output = m_parameters.value(outputIds.first().toString());
        Quantity result = evaluateNode(output.value("expressionAst").toObject(), locals, localResolver);
        if (result.valid) result.sourceIds.insert(output.value("id").toString());
        return result;
    }

    Quantity evaluateNode(const QJsonObject& node,
                          const QHash<QString, Quantity>& locals,
                          const std::function<Quantity(const QString&)>& localResolver = {})
    {
        if (node.isEmpty()) return failure("missing-input", "Expression is missing");
        const QString kind = node.value("kind").toString();
        if (kind == "literal") {
            Quantity result; result.valid = true;
            if (node.value("value").isBool()) { result.boolean = true; result.boolValue = node.value("value").toBool(); return result; }
            const QString unit = node.value("unit").toString();
            UnitDefinition definition;
            if (!UnitSystem::standard().resolve(unit, &definition)) {
                return failure("unit-error", QStringLiteral("Unsupported unit %1").arg(unit));
            }
            result.siValue = node.value("value").toDouble() * definition.factor;
            result.unit = unit; result.dimensions = definition.dimensions; return result;
        }
        if (kind == "reference") {
            const QString path = node.value("path").toString();
            const QString simple = path.section('.', -1);
            if (locals.contains(path)) return locals.value(path);
            if (locals.contains(simple)) return locals.value(simple);
            if (localResolver) { Quantity value = localResolver(simple); if (value.valid || value.status != "missing-input") return value; }
            return resolveReference(path);
        }
        if (kind == "call") {
            QJsonObject usage;
            usage.insert("definitionName", node.value("callee"));
            usage.insert("arguments", node.value("arguments"));
            return invoke(usage);
        }
        if (kind == "unary") {
            Quantity value = evaluateNode(node.value("operand").toObject(), locals, localResolver);
            if (!value.valid) return value;
            if (node.value("operator") == "-") value.siValue = -value.siValue;
            return value;
        }
        if (kind == "binary") {
            Quantity left = evaluateNode(node.value("left").toObject(), locals, localResolver);
            if (!left.valid) return left;
            Quantity right = evaluateNode(node.value("right").toObject(), locals, localResolver);
            if (!right.valid) return right;
            const QString op = node.value("operator").toString();
            Quantity result; result.valid = true; result.sourceIds = left.sourceIds | right.sourceIds;
            if (op == "+" || op == "-") {
                if (!sameDimensions(left.dimensions, right.dimensions)) return failure("unit-error", "Addition or subtraction requires matching dimensions");
                result.siValue = op == "+" ? left.siValue + right.siValue : left.siValue - right.siValue;
                result.dimensions = left.dimensions; result.unit = left.unit.isEmpty() ? right.unit : left.unit;
            } else if (op == "*") {
                result.siValue = left.siValue * right.siValue;
                result.dimensions = UnitSystem::combine(left.dimensions, right.dimensions, 1);
                result.unit = preferredProductUnit(left, right, result.dimensions);
            } else if (op == "/") {
                if (std::abs(right.siValue) < 1e-15) return failure("runtime-error", "Division by zero");
                result.siValue = left.siValue / right.siValue;
                result.dimensions = UnitSystem::combine(left.dimensions, right.dimensions, -1);
                result.unit = preferredDivisionUnit(left, right, result.dimensions);
            } else if (op == "<" || op == "<=" || op == ">" || op == ">=" || op == "==" || op == "!=") {
                if (!sameDimensions(left.dimensions, right.dimensions)) return failure("unit-error", "Comparison requires matching dimensions");
                result.boolean = true;
                if (op == "<") result.boolValue = left.siValue < right.siValue;
                else if (op == "<=") result.boolValue = left.siValue <= right.siValue;
                else if (op == ">") result.boolValue = left.siValue > right.siValue;
                else if (op == ">=") result.boolValue = left.siValue >= right.siValue;
                else if (op == "==") result.boolValue = left.siValue == right.siValue;
                else result.boolValue = left.siValue != right.siValue;
            } else return failure("unsupported", QStringLiteral("Unsupported operator %1").arg(op));
            if (!result.boolean && !std::isfinite(result.siValue)) return failure("runtime-error", "Numeric overflow");
            return result;
        }
        return failure("unsupported", QStringLiteral("Unsupported expression kind %1").arg(kind));
    }

    Quantity resolveReference(const QString& path)
    {
        const QString normalized = normalizedReference(path);
        const QString simple = normalized.section('.', -1);
        const QString analysisId = m_analysisParameterByName.value(m_currentAnalysisId).value(simple);
        if (!analysisId.isEmpty()) {
            addDependency(analysisId);
            return evaluateParameter(analysisId);
        }
        const QStringList exactIds = m_globalParameterIdsByReference.value(normalized);
        if (exactIds.size() == 1) {
            addDependency(exactIds.first());
            return evaluateParameter(exactIds.first());
        }
        const QStringList simpleIds = m_globalParameterIdsByReference.value(simple);
        if (simpleIds.size() == 1) {
            addDependency(simpleIds.first());
            return evaluateParameter(simpleIds.first());
        }
        if (!exactIds.isEmpty() || !simpleIds.isEmpty()) {
            return failure("ambiguous-reference", QStringLiteral("Reference %1 is ambiguous").arg(path));
        }
        return failure("missing-input", QStringLiteral("Reference %1 is unresolved").arg(path));
    }

    QJsonObject resultObject(const QString& id, const Quantity& quantity) const
    {
        const QJsonObject parameter = m_parameters.value(id);
        QJsonObject result = {
            {"parameterId", id}, {"name", parameter.value("name")}, {"typeName", parameter.value("typeName")},
            {"status", quantity.valid ? QJsonValue("computed") : QJsonValue(quantity.status)},
            {"formula", parameter.value("expression")}, {"formulaAst", parameter.value("expressionAst")},
            {"dimension", dimensionsText(quantity.dimensions)}, {"source", parameter.value("source")}
        };
        if (quantity.valid && !quantity.boolean) {
            result.insert("value", displayValue(quantity));
            result.insert("unit", quantity.unit.isEmpty() ? QJsonValue::Null : QJsonValue(quantity.unit));
            result.insert("siValue", quantity.siValue);
        } else if (quantity.boolean) result.insert("value", quantity.boolValue);
        else result.insert("message", quantity.message);
        QStringList sortedSources(quantity.sourceIds.begin(), quantity.sourceIds.end());
        sortedSources.sort();
        QJsonArray sources; for (const QString& source : sortedSources) sources.append(source);
        result.insert("inputParameterIds", sources);
        if (m_usageByResult.contains(id)) {
            const QJsonObject usage = m_usageByResult.value(id);
            result.insert("calculationUsageId", usage.value("id"));
            result.insert("calculationDefinitionId", usage.value("definitionId"));
        }
        return result;
    }

    QJsonObject evaluateConstraint(const QJsonObject& constraint)
    {
        const QJsonObject ast = constraint.value("expressionAst").toObject();
        Quantity verdict = evaluateNode(ast, {});
        QJsonObject result = {
            {"constraintId", constraint.value("id")}, {"name", constraint.value("name")},
            {"expression", constraint.value("expression")}, {"expressionAst", ast},
            {"status", verdict.valid && verdict.boolean ? QJsonValue(verdict.boolValue ? "pass" : "fail") : QJsonValue(verdict.status)},
            {"source", constraint.value("source")}
        };
        QStringList constraintSources(verdict.sourceIds.begin(), verdict.sourceIds.end());
        constraintSources.sort();
        QJsonArray constraintInputIds;
        for (const QString& sourceId : constraintSources) constraintInputIds.append(sourceId);
        result.insert("inputParameterIds", constraintInputIds);
        if (ast.value("kind") == "binary") {
            Quantity left = evaluateNode(ast.value("left").toObject(), {});
            Quantity right = evaluateNode(ast.value("right").toObject(), {});
            const QString op = ast.value("operator").toString();
            if (left.valid && right.valid && sameDimensions(left.dimensions, right.dimensions)) {
                const QString unit = left.unit.isEmpty() ? right.unit : left.unit;
                result.insert("actual", displayValue(left, unit)); result.insert("target", displayValue(right, unit));
                result.insert("unit", unit.isEmpty() ? QJsonValue::Null : QJsonValue(unit)); result.insert("comparison", op);
                const double marginSi = (op == "<" || op == "<=") ? right.siValue - left.siValue : left.siValue - right.siValue;
                Quantity margin = left; margin.siValue = marginSi; margin.unit = unit;
                result.insert("margin", displayValue(margin, unit));
                if (ast.value("left").toObject().value("kind") == "reference") {
                    const QString name = ast.value("left").toObject().value("path").toString().section('.', -1);
                    result.insert("resultParameterId", m_analysisParameterByName.value(m_currentAnalysisId).value(name));
                }
                if (ast.value("right").toObject().value("kind") == "reference") {
                    const QString name = ast.value("right").toObject().value("path").toString().section('.', -1);
                    QString targetId = m_analysisParameterByName.value(m_currentAnalysisId).value(name);
                    if (targetId.isEmpty()) {
                        const QStringList ids = m_globalParameterIdsByReference.value(
                            normalizedReference(ast.value("right").toObject().value("path").toString()));
                        if (ids.size() == 1) targetId = ids.first();
                    }
                    if (!targetId.isEmpty()) result.insert("targetParameterId", targetId);
                }
            }
        }
        if (!verdict.valid) result.insert("message", verdict.message);
        return result;
    }

    void addDependency(const QString& target)
    {
        if (m_stack.isEmpty()) return;
        const QString source = m_stack.last();
        m_dependencies.insert(source + "\n" + target);
    }

    QJsonArray dependencyArray(const QString& analysisId) const
    {
        QJsonArray result;
        const QList<QString> ids = m_analysisParameterByName.value(analysisId).values();
        const QSet<QString> analysisIds(ids.begin(), ids.end());
        QStringList edges(m_dependencies.begin(), m_dependencies.end());
        edges.sort();
        for (const QString& edge : edges) {
            const QString source = edge.section('\n', 0, 0), target = edge.section('\n', 1);
            if (analysisIds.contains(source)) result.append(QJsonObject{{"sourceParameterId", source}, {"targetParameterId", target}});
        }
        return result;
    }

    void addDiagnostic(const QString& code, const QString& message, const QString& parameterId)
    {
        if (message.isEmpty()) return;
        m_diagnostics.append(QJsonObject{{"code", code}, {"severity", "error"}, {"message", message},
                                          {"parameterId", parameterId}});
    }

    QJsonObject m_snapshot;
    const std::atomic_bool* m_canceled = nullptr;
    QHash<QString, QJsonObject> m_parameters, m_definitions, m_analyses;
    QHash<QString, QString> m_definitionByName;
    QHash<QString, QStringList> m_globalParameterIdsByReference;
    QHash<QString, QJsonObject> m_usageByResult;
    QHash<QString, QList<QJsonObject>> m_constraintsByAnalysis;
    QHash<QString, QHash<QString, QString>> m_analysisParameterByName;
    QHash<QString, Quantity> m_values;
    QSet<QString> m_visiting, m_dependencies;
    QStringList m_analysisOrder, m_evaluationOrder, m_stack;
    QJsonArray m_diagnostics;
    QString m_currentAnalysisId, m_subjectReference;
};

} // namespace

AnalysisOutcome AnalysisService::analyze(const QJsonObject& modelSnapshot,
                                        const std::atomic_bool* canceled) const
{
    if (canceled && canceled->load()) return {{}, true};
    Evaluator evaluator(modelSnapshot, canceled);
    QJsonObject snapshot = evaluator.run();
    return {snapshot, canceled && canceled->load()};
}

} // namespace Aidc
