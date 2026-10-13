#include "ModelSnapshotAdapter.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonValue>
#include <QMetaProperty>
#include <QRegularExpression>
#include <QSet>
#include <QVariant>

#include <algorithm>

#include "parser/SysMLParser.h"
#include "core/infrastructure/Resource.h"
#include "core/model/Element.h"
#include "core/model/Relationship.h"
#include "core/model/Feature.h"
#include "core/model/Type.h"
#include "core/model/Namespace.h"
#include "core/model/FeatureTyping.h"
#include "core/model/Redefinition.h"
#include "core/model/Subsetting.h"
#include "core/model/Subclassification.h"
#include "core/model/Conjugation.h"
#include "core/model/Dependency.h"
#include "core/model/Membership.h"
#include "core/model/Import.h"
#include "core/model/MembershipImport.h"
#include "core/model/NamespaceImport.h"
#include "core/model/RequirementUsage.h"
#include "core/model/PartUsage.h"
#include "core/model/PortUsage.h"
#include "core/model/ConnectStatement.h"
#include "core/model/RequireStatement.h"
#include "core/model/AssertStatement.h"
#include "core/model/AssumeStatement.h"
#include "core/model/ConstraintStatement.h"
#include "core/model/ActionInputOutput.h"
#include "core/model/CalculationDefinition.h"
#include "core/model/AnalysisCase.h"
#include "core/model/AnalysisSubject.h"
#include "core/model/RenderingUsage.h"
#include "core/model/ReturnParameter.h"
#include "core/model/ViewDefinition.h"
#include "core/model/ViewUsage.h"
#include "core/model/ExposeStatement.h"
#include "core/model/ViewRenderingMember.h"
#include "StandardLibraryIndex.h"

using namespace SysML;

namespace Aidc {
namespace {

struct SourceSpan {
    bool valid = false;
    int startUtf16 = -1;
    int endUtf16 = -1;
    QJsonObject json;
};

struct Record {
    Element* element = nullptr;
    Element* owner = nullptr;
    QString id;
    QString kind;
    QString name;
    QString qualifiedName;
    SourceSpan source;
    int ordinal = 0;
};

struct PackageScope {
    QString name;
    int nameStart = -1;
    int parent = -1;
};

QString sha256(const QByteArray& value)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(value, QCryptographicHash::Sha256).toHex());
}

QJsonObject positionFor(const QString& text, int utf16Offset)
{
    const QString prefix = text.left(utf16Offset);
    const int lastNewline = prefix.lastIndexOf('\n');
    const int line = prefix.count('\n') + 1;
    const int column = utf16Offset - lastNewline;
    return {
        {"line", line},
        {"column", column},
        {"offset", text.left(utf16Offset).toUtf8().size()}
    };
}

SourceSpan spanFor(const QString& uri, const QString& text, int start, int length)
{
    SourceSpan span;
    if (start < 0 || length <= 0) return span;
    span.valid = true;
    span.startUtf16 = start;
    span.endUtf16 = start + length;
    span.json = {
        {"uri", uri},
        {"start", positionFor(text, span.startUtf16)},
        {"end", positionFor(text, span.endUtf16)}
    };
    return span;
}

bool isIdentifierCharacter(QChar value)
{
    return value.isLetterOrNumber() || value == '_';
}

QList<PackageScope> parsePackageScopes(const QString& text)
{
    QList<PackageScope> scopes;
    QHash<int, int> packageAtBrace;
    QVector<int> bracePackages;
    bool lineComment = false;
    bool blockComment = false;
    QChar quote;

    auto currentPackage = [&]() {
        for (int i = bracePackages.size() - 1; i >= 0; --i) {
            if (bracePackages.at(i) >= 0) return bracePackages.at(i);
        }
        return -1;
    };

    for (int i = 0; i < text.size(); ++i) {
        const QChar value = text.at(i);
        const QChar next = i + 1 < text.size() ? text.at(i + 1) : QChar();
        if (lineComment) {
            if (value == '\n') lineComment = false;
            continue;
        }
        if (blockComment) {
            if (value == '*' && next == '/') { blockComment = false; ++i; }
            continue;
        }
        if (!quote.isNull()) {
            if (value == '\\') { ++i; continue; }
            if (value == quote) quote = QChar();
            continue;
        }
        if (value == '/' && next == '/') { lineComment = true; ++i; continue; }
        if (value == '/' && next == '*') { blockComment = true; ++i; continue; }
        if (value == '"' || value == '\'') { quote = value; continue; }

        if (text.mid(i, 7) == "package" &&
            (i == 0 || !isIdentifierCharacter(text.at(i - 1))) &&
            (i + 7 == text.size() || !isIdentifierCharacter(text.at(i + 7)))) {
            int cursor = i + 7;
            while (cursor < text.size() && text.at(cursor).isSpace()) ++cursor;
            const int nameStart = cursor;
            while (cursor < text.size() && isIdentifierCharacter(text.at(cursor))) ++cursor;
            const QString name = text.mid(nameStart, cursor - nameStart);
            while (cursor < text.size() && text.at(cursor).isSpace()) ++cursor;
            if (!name.isEmpty() && cursor < text.size() && text.at(cursor) == '{') {
                const int index = scopes.size();
                scopes.append(PackageScope{name, nameStart, currentPackage()});
                packageAtBrace.insert(cursor, index);
            }
        }
        if (value == '{') {
            bracePackages.append(packageAtBrace.value(i, -1));
        } else if (value == '}' && !bracePackages.isEmpty()) {
            bracePackages.removeLast();
        }
    }
    return scopes;
}

QString declaredShortNameFor(const QString& text, const QString& name)
{
    if (name.isEmpty()) return {};
    const QRegularExpression expression(
        QStringLiteral("\\brequirement\\s*<\\s*(?:'([^']+)'|([^>\\s]+))\\s*>\\s*%1\\b")
            .arg(QRegularExpression::escape(name)));
    const QRegularExpressionMatch match = expression.match(text);
    if (!match.hasMatch()) return {};
    return !match.captured(1).isEmpty() ? match.captured(1) : match.captured(2);
}

QString stringLiteralValue(const QString& expression)
{
    const QString value = expression.trimmed();
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        QString result = value.mid(1, value.size() - 2);
        result.replace(QStringLiteral("\\\""), QStringLiteral("\""));
        result.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
        result.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));
        return result;
    }
    return value;
}

class SourceLocator {
public:
    SourceLocator(const QString& uri, const QString& text)
        : m_uri(uri), m_text(text)
    {
    }

    SourceSpan locate(const QString& name)
    {
        SourceSpan span;
        if (name.isEmpty()) {
            return span;
        }

        if (!m_occurrences.contains(name)) {
            QVector<int> offsets;
            const QString pattern = QStringLiteral("(?<![A-Za-z0-9_])%1(?![A-Za-z0-9_])")
                                        .arg(QRegularExpression::escape(name));
            QRegularExpression expression(pattern);
            auto matches = expression.globalMatch(m_text);
            while (matches.hasNext()) {
                offsets.append(matches.next().capturedStart());
            }
            m_occurrences.insert(name, offsets);
        }

        const QVector<int>& offsets = m_occurrences[name];
        const int index = m_nextOccurrence.value(name, 0);
        if (index >= offsets.size()) {
            return span;
        }
        m_nextOccurrence[name] = index + 1;

        span.valid = true;
        span.startUtf16 = offsets.at(index);
        span.endUtf16 = span.startUtf16 + name.size();
        span.json = {
            {"uri", m_uri},
            {"start", positionFor(m_text, span.startUtf16)},
            {"end", positionFor(m_text, span.endUtf16)}
        };
        return span;
    }

    int occurrenceCount(const QString& name) const
    {
        return m_occurrences.value(name).size();
    }

private:
    QString m_uri;
    QString m_text;
    QHash<QString, QVector<int>> m_occurrences;
    QHash<QString, int> m_nextOccurrence;
};

void collectElements(Element* element,
                     Element* owner,
                     QList<Record>& records,
                     QSet<const Element*>& visited,
                     const std::atomic_bool* canceled)
{
    if (!element || visited.contains(element) || (canceled && canceled->load())) {
        return;
    }
    visited.insert(element);
    Record record;
    record.element = element;
    record.owner = owner;
    records.append(record);

    for (Relationship* relationship : element->ownedRelationships()) {
        collectElements(relationship, element, records, visited, canceled);
    }
    for (Element* annotation : element->ownedAnnotations()) {
        collectElements(annotation, element, records, visited, canceled);
    }
    for (Element* child : element->ownedElements()) {
        collectElements(child, element, records, visited, canceled);
    }
}

QJsonValue scalarProperty(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return QJsonValue::Null;
    }
    if (value.metaType().flags().testFlag(QMetaType::PointerToQObject)) {
        return QJsonValue::Undefined;
    }
    if (value.metaType().id() == QMetaType::QStringList) {
        QJsonArray array;
        for (const QString& item : value.toStringList()) {
            array.append(item);
        }
        return array;
    }
    const QJsonValue json = QJsonValue::fromVariant(value);
    if (json.isObject() || json.isArray() || json.isString() || json.isBool() || json.isDouble()) {
        return json;
    }
    return QJsonValue::Undefined;
}

QJsonObject elementProperties(Element* element)
{
    QJsonObject result;
    const QMetaObject* meta = element->metaObject();
    static const QSet<QString> excluded = {
        "objectName", "declaredName", "name", "qualifiedName"
    };
    for (int i = 0; i < meta->propertyCount(); ++i) {
        const QMetaProperty property = meta->property(i);
        const QString propertyName = QString::fromLatin1(property.name());
        if (excluded.contains(propertyName) || !property.isReadable()) {
            continue;
        }
        const QJsonValue value = scalarProperty(property.read(element));
        if (!value.isUndefined()) {
            result.insert(propertyName, value);
        }
    }
    if (auto* value = qobject_cast<RequirementUsage*>(element); value && !value->typeName().isEmpty()) {
        result.insert("typeName", value->typeName());
    } else if (auto* value = qobject_cast<PartUsage*>(element); value && !value->typeName().isEmpty()) {
        result.insert("typeName", value->typeName());
    } else if (auto* value = qobject_cast<PortUsage*>(element); value && !value->typeName().isEmpty()) {
        result.insert("typeName", value->typeName());
    } else if (auto* value = qobject_cast<Feature*>(element)) {
        result.insert("isAbstract", value->isAbstract());
        result.insert("isConst", value->isConst());
        result.insert("isDerived", value->isDerived());
        result.insert("direction", static_cast<int>(value->direction()));
        result.insert("isRef", value->isRef());
        if (!value->unitExpression().isEmpty()) result.insert("unitExpression", value->unitExpression());
        if (!value->valueExpression().isEmpty()) result.insert("valueExpression", value->valueExpression());
        if (!value->defaultValue().isEmpty()) result.insert("defaultValue", value->defaultValue());
        result.insert("isSimplifiedAttribute", value->isSimplifiedAttribute());
        result.insert("isRedefines", value->isRedefines());
    } else if (auto* value = qobject_cast<FeatureTyping*>(element); value && !value->typeName().isEmpty()) {
        result.insert("typeName", value->typeName());
    } else if (auto* value = qobject_cast<Redefinition*>(element); value && !value->redefinedFeatureName().isEmpty()) {
        result.insert("redefinedFeatureName", value->redefinedFeatureName());
    } else if (auto* value = qobject_cast<Subsetting*>(element); value && !value->subsettedFeatureName().isEmpty()) {
        result.insert("subsettedFeatureName", value->subsettedFeatureName());
    } else if (auto* value = qobject_cast<Conjugation*>(element); value && !value->originalTypeName().isEmpty()) {
        result.insert("originalTypeName", value->originalTypeName());
    } else if (auto* value = qobject_cast<Dependency*>(element); value && !value->supplierElementName().isEmpty()) {
        result.insert("supplierElementName", value->supplierElementName());
    } else if (auto* value = qobject_cast<NamespaceImport*>(element); value && !value->importedNamespaceName().isEmpty()) {
        result.insert("importedNamespaceName", value->importedNamespaceName());
    } else if (auto* value = qobject_cast<RequireStatement*>(element)) {
        result.insert("expression", value->expression());
        result.insert("hasConstraintKeyword", value->hasConstraintKeyword());
    } else if (auto* value = qobject_cast<AssertStatement*>(element)) {
        result.insert("expression", value->expression());
    } else if (auto* value = qobject_cast<AssumeStatement*>(element)) {
        result.insert("expression", value->expression());
    } else if (auto* value = qobject_cast<ConstraintStatement*>(element)) {
        result.insert("expression", value->expression());
    } else if (auto* value = qobject_cast<ViewUsage*>(element)) {
        if (!value->typeName().isEmpty()) result.insert("typeName", value->typeName());
        if (!value->renderMethod().isEmpty()) result.insert("renderMethod", value->renderMethod());
        if (!value->filterExpression().isEmpty()) result.insert("filterExpression", value->filterExpression());
    } else if (auto* value = qobject_cast<ExposeStatement*>(element)) {
        result.insert("exposedQualifiedName", value->exposedQualifiedName());
        result.insert("recursive", value->isRecursive());
        result.insert("exposeType", static_cast<int>(value->exposeType()));
    } else if (auto* value = qobject_cast<ViewDefinition*>(element)) {
        if (!value->filterExpression().isEmpty()) result.insert("filterExpression", value->filterExpression());
    } else if (auto* value = qobject_cast<ViewRenderingMember*>(element)) {
        if (!value->declaredName().isEmpty()) result.insert("renderingReference", value->declaredName());
    }
    return result;
}

QString constraintExpression(Element* element)
{
    if (auto* value = qobject_cast<RequireStatement*>(element)) return value->expression();
    if (auto* value = qobject_cast<AssertStatement*>(element)) return value->expression();
    if (auto* value = qobject_cast<AssumeStatement*>(element)) return value->expression();
    if (auto* value = qobject_cast<ConstraintStatement*>(element)) return value->expression();
    return {};
}

QString constraintRelationshipKind(Element* element)
{
    if (qobject_cast<RequireStatement*>(element)) return "RequirementConstraint";
    if (qobject_cast<AssertStatement*>(element)) return "AssertConstraint";
    if (qobject_cast<AssumeStatement*>(element)) return "AssumeConstraint";
    if (qobject_cast<ConstraintStatement*>(element)) return "Constraint";
    return {};
}

QJsonObject diagnostic(const QString& code,
                       const QString& severity,
                       const QString& message,
                       const QJsonValue& source = QJsonValue::Null)
{
    return {
        {"code", code},
        {"severity", severity},
        {"message", message},
        {"source", source},
        {"relatedElementIds", QJsonArray()}
    };
}

QJsonObject parseDiagnostic(const QString& raw,
                            const QString& defaultSeverity,
                            const QString& uri,
                            const QString& text)
{
    static const QRegularExpression expression(
        QStringLiteral("^\\[([A-Z_]+)\\] Line (\\d+), Column (\\d+): (.*)$"));
    const QRegularExpressionMatch match = expression.match(raw);
    if (!match.hasMatch()) {
        return diagnostic(defaultSeverity == "error" ? "PARSER_ERROR" : "PARSER_WARNING",
                          defaultSeverity, raw);
    }

    const int line = match.captured(2).toInt();
    const int antlrColumn = match.captured(3).toInt();
    const QStringList lines = text.split('\n');
    int utf16Offset = 0;
    for (int i = 1; i < line && i <= lines.size(); ++i) {
        utf16Offset += lines.at(i - 1).size() + 1;
    }
    utf16Offset += antlrColumn;
    const int textSize = static_cast<int>(text.size());
    const int safeOffset = std::clamp(utf16Offset, 0, textSize);
    const int endOffset = std::min(safeOffset + 1, textSize);
    const QJsonObject source = {
        {"uri", uri},
        {"start", positionFor(text, safeOffset)},
        {"end", positionFor(text, endOffset)}
    };
    return diagnostic(match.captured(1), defaultSeverity, match.captured(4), source);
}

QStringList propertyNamesForDirection(bool source)
{
    if (source) {
        return {"source", "sourceElement", "client", "specific", "subsettingFeature",
                "redefiningFeature", "sourceEnd", "sourceState"};
    }
    return {"target", "targetElement", "supplier", "general", "subsettedFeature",
            "redefinedFeature", "targetEnd", "targetState", "type"};
}

Element* resolveByName(const QString& name, const QList<Record>& records)
{
    if (name.isEmpty()) {
        return nullptr;
    }
    QList<Element*> matches;
    const QString simpleName = name.section("::", -1);
    const bool qualified = name.contains("::");
    for (const Record& record : records) {
        if (record.qualifiedName == name || record.name == name ||
            (!qualified && record.name == simpleName)) {
            matches.append(record.element);
        }
    }
    QSet<Element*> unique(matches.begin(), matches.end());
    return unique.size() == 1 ? *unique.constBegin() : nullptr;
}

Element* resolveConnectEnd(const QString& path, Element* owner, const QList<Record>& records)
{
    Element* currentOwner = owner;
    Element* current = nullptr;
    const QStringList segments = path.split('.', Qt::SkipEmptyParts);
    for (const QString& segment : segments) {
        QList<Element*> direct;
        for (const Record& record : records) {
            if (record.owner == currentOwner && record.name == segment) {
                direct.append(record.element);
            }
        }
        if (direct.isEmpty() && currentOwner) {
            const QString typeName = elementProperties(currentOwner).value("typeName").toString();
            Element* type = resolveByName(typeName, records);
            if (type) {
                for (const Record& record : records) {
                    if (record.owner == type && record.name == segment) {
                        direct.append(record.element);
                    }
                }
            }
        }
        QSet<Element*> unique(direct.begin(), direct.end());
        if (unique.size() != 1) {
            current = nullptr;
            break;
        }
        current = *unique.constBegin();
        currentOwner = current;
    }
    return current ? current : resolveByName(path, records);
}

QString unresolvedTypeName(const QString& warning)
{
    static const QRegularExpression expression(
        QStringLiteral("(?:Type not found(?: for [^:]+)?|Supertype not found for Subclassification):\\s*([^\\s]+)\\s*\\(may be defined in another resource\\)"));
    const QRegularExpressionMatch match = expression.match(warning);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString unresolvedNamespaceName(const QString& warning)
{
    static const QRegularExpression expression(
        QStringLiteral("Imported namespace not found:\\s*([^\\s]+)\\s*\\(may be defined in another resource\\)"));
    const QRegularExpressionMatch match = expression.match(warning);
    return match.hasMatch() ? match.captured(1) : QString();
}

QJsonArray connectEndpointIds(ConnectStatement* statement,
                              Element* owner,
                              const QList<Record>& records,
                              const QHash<const Element*, QString>& ids,
                              bool source)
{
    QJsonArray result;
    const QStringList ends = statement->ends();
    if (ends.isEmpty()) {
        return result;
    }
    const int begin = source ? 0 : 1;
    const int end = source ? std::min(1, static_cast<int>(ends.size()))
                           : static_cast<int>(ends.size());
    for (int index = begin; index < end; ++index) {
        Element* endpoint = resolveConnectEnd(ends.at(index), owner, records);
        if (endpoint && ids.contains(endpoint) && !result.contains(ids.value(endpoint))) {
            result.append(ids.value(endpoint));
        }
    }
    return result;
}

void appendEndpoint(QJsonArray& result,
                    Element* endpoint,
                    const QHash<const Element*, QString>& ids)
{
    if (endpoint && ids.contains(endpoint) && !result.contains(ids.value(endpoint))) {
        result.append(ids.value(endpoint));
    }
}

QJsonArray endpointIds(Element* relationship,
                       Element* owner,
                       const QList<Record>& records,
                       const QHash<const Element*, QString>& ids,
                       bool source)
{
    QJsonArray result;
    if (auto* value = qobject_cast<FeatureTyping*>(relationship)) {
        appendEndpoint(result, source ? static_cast<Element*>(value->typedFeature())
                                      : static_cast<Element*>(value->type()), ids);
        if (!source && result.isEmpty()) {
            appendEndpoint(result, resolveByName(value->typeName(), records), ids);
        }
    } else if (auto* value = qobject_cast<Redefinition*>(relationship)) {
        appendEndpoint(result, source ? static_cast<Element*>(value->redefiningFeature())
                                      : static_cast<Element*>(value->redefinedFeature()), ids);
        if (!source && result.isEmpty()) {
            appendEndpoint(result, resolveByName(value->redefinedFeatureName(), records), ids);
        }
    } else if (auto* value = qobject_cast<Subsetting*>(relationship)) {
        appendEndpoint(result, source ? static_cast<Element*>(value->subsettingFeature())
                                      : static_cast<Element*>(value->subsettedFeature()), ids);
        if (!source && result.isEmpty()) {
            appendEndpoint(result, resolveByName(value->subsettedFeatureName(), records), ids);
        }
    } else if (auto* value = qobject_cast<Subclassification*>(relationship)) {
        appendEndpoint(result, source ? static_cast<Element*>(value->subtype())
                                      : static_cast<Element*>(value->supertype()), ids);
    } else if (auto* value = qobject_cast<Conjugation*>(relationship)) {
        appendEndpoint(result, source ? static_cast<Element*>(value->conjugatedType())
                                      : static_cast<Element*>(value->originalType()), ids);
        if (!source && result.isEmpty()) {
            appendEndpoint(result, resolveByName(value->originalTypeName(), records), ids);
        }
    } else if (auto* value = qobject_cast<Dependency*>(relationship)) {
        appendEndpoint(result, source ? value->clientElement() : value->supplierElement(), ids);
        if (!source && result.isEmpty()) {
            appendEndpoint(result, resolveByName(value->supplierElementName(), records), ids);
        }
    } else if (auto* value = qobject_cast<Membership*>(relationship)) {
        appendEndpoint(result, source ? owner : value->memberElement(), ids);
    } else if (auto* value = qobject_cast<MembershipImport*>(relationship)) {
        appendEndpoint(result, source ? owner : static_cast<Element*>(value->importedMembership()), ids);
    } else if (auto* value = qobject_cast<Import*>(relationship)) {
        appendEndpoint(result, source ? owner : dynamic_cast<Element*>(value->importedNamespace()), ids);
        if (!source && result.isEmpty()) {
            if (auto* namespaceImport = qobject_cast<NamespaceImport*>(relationship)) {
                appendEndpoint(result, resolveByName(namespaceImport->importedNamespaceName(), records), ids);
            }
        }
    }

    const QStringList candidates = propertyNamesForDirection(source);
    const QMetaObject* meta = relationship->metaObject();
    for (const QString& candidate : candidates) {
        const int index = meta->indexOfProperty(candidate.toLatin1().constData());
        if (index < 0) {
            continue;
        }
        const QVariant value = meta->property(index).read(relationship);
        if (value.canConvert<QObject*>()) {
            QObject* object = value.value<QObject*>();
            Element* endpoint = qobject_cast<Element*>(object);
            if (endpoint && ids.contains(endpoint)) {
                appendEndpoint(result, endpoint, ids);
            }
        }
    }
    return result;
}

class ExpressionProjector {
public:
    explicit ExpressionProjector(QString text) : m_text(std::move(text)) { next(); }
    QJsonObject parse() { return parseComparison(); }

private:
    enum class TokenKind { End, Identifier, Number, String, Operator, LParen, RParen, Comma, LBracket, RBracket };
    struct Token { TokenKind kind = TokenKind::End; QString text; };

    void next()
    {
        while (m_pos < m_text.size() && m_text.at(m_pos).isSpace()) ++m_pos;
        if (m_pos >= m_text.size()) { m_token = {}; return; }
        const QChar ch = m_text.at(m_pos);
        if (ch.isLetter() || ch == '_') {
            const int start = m_pos++;
            while (m_pos < m_text.size()) {
                const QChar c = m_text.at(m_pos);
                if (c.isLetterOrNumber() || c == '_' || c == '.' || c == ':') ++m_pos;
                else break;
            }
            m_token = {TokenKind::Identifier, m_text.mid(start, m_pos - start)};
            return;
        }
        if (ch.isDigit() || (ch == '.' && m_pos + 1 < m_text.size() && m_text.at(m_pos + 1).isDigit())) {
            const int start = m_pos++;
            while (m_pos < m_text.size() && (m_text.at(m_pos).isDigit() || m_text.at(m_pos) == '.' ||
                   m_text.at(m_pos) == 'e' || m_text.at(m_pos) == 'E' || m_text.at(m_pos) == '+' ||
                   m_text.at(m_pos) == '-')) {
                const QChar c = m_text.at(m_pos);
                if ((c == '+' || c == '-') && m_pos > start && m_text.at(m_pos - 1).toLower() != 'e') break;
                ++m_pos;
            }
            m_token = {TokenKind::Number, m_text.mid(start, m_pos - start)};
            return;
        }
        if (ch == '"' || ch == '\'') {
            const QChar quote = ch; const int start = m_pos++;
            while (m_pos < m_text.size() && m_text.at(m_pos) != quote) ++m_pos;
            if (m_pos < m_text.size()) ++m_pos;
            m_token = {TokenKind::String, m_text.mid(start, m_pos - start)};
            return;
        }
        if (ch == '(') { ++m_pos; m_token = {TokenKind::LParen, "("}; return; }
        if (ch == ')') { ++m_pos; m_token = {TokenKind::RParen, ")"}; return; }
        if (ch == ',') { ++m_pos; m_token = {TokenKind::Comma, ","}; return; }
        if (ch == '[') { ++m_pos; m_token = {TokenKind::LBracket, "["}; return; }
        if (ch == ']') { ++m_pos; m_token = {TokenKind::RBracket, "]"}; return; }
        QString op(ch); ++m_pos;
        if (m_pos < m_text.size() && (m_text.at(m_pos) == '=' || (ch == '<' && m_text.at(m_pos) == '>'))) op += m_text.at(m_pos++);
        m_token = {TokenKind::Operator, op};
    }

    QJsonObject binary(QStringList operators, QJsonObject (ExpressionProjector::*nextLevel)())
    {
        QJsonObject left = (this->*nextLevel)();
        while (m_token.kind == TokenKind::Operator && operators.contains(m_token.text)) {
            const QString op = m_token.text; next();
            QJsonObject right = (this->*nextLevel)();
            left = {{"kind", "binary"}, {"operator", op}, {"left", left}, {"right", right}};
        }
        return left;
    }
    QJsonObject parseComparison() { return binary({"<", "<=", ">", ">=", "==", "!="}, &ExpressionProjector::parseAdditive); }
    QJsonObject parseAdditive() { return binary({"+", "-"}, &ExpressionProjector::parseMultiplicative); }
    QJsonObject parseMultiplicative() { return binary({"*", "/"}, &ExpressionProjector::parseUnary); }
    QJsonObject parseUnary()
    {
        if (m_token.kind == TokenKind::Operator && (m_token.text == "+" || m_token.text == "-")) {
            const QString op = m_token.text; next();
            return {{"kind", "unary"}, {"operator", op}, {"operand", parseUnary()}};
        }
        return parsePrimary();
    }
    QJsonObject parsePrimary()
    {
        if (m_token.kind == TokenKind::LParen) { next(); QJsonObject value = parseComparison(); if (m_token.kind == TokenKind::RParen) next(); return value; }
        if (m_token.kind == TokenKind::Number) {
            const QString raw = m_token.text; next(); QString unit;
            if (m_token.kind == TokenKind::LBracket) {
                next(); QStringList parts;
                while (m_token.kind != TokenKind::RBracket && m_token.kind != TokenKind::End) { parts.append(m_token.text); next(); }
                if (m_token.kind == TokenKind::RBracket) next(); unit = parts.join("");
            }
            QJsonObject value{{"kind", "literal"}, {"value", raw.toDouble()}, {"raw", raw}};
            if (!unit.isEmpty()) value.insert("unit", unit);
            return value;
        }
        if (m_token.kind == TokenKind::String) { const QString raw = m_token.text; next(); return {{"kind", "literal"}, {"value", raw.mid(1, raw.size() - 2)}, {"raw", raw}}; }
        if (m_token.kind == TokenKind::Identifier) {
            const QString name = m_token.text; next();
            if (m_token.kind == TokenKind::LParen) {
                next(); QJsonArray arguments;
                while (m_token.kind != TokenKind::RParen && m_token.kind != TokenKind::End) {
                    arguments.append(parseComparison());
                    if (m_token.kind == TokenKind::Comma) next(); else break;
                }
                if (m_token.kind == TokenKind::RParen) next();
                return {{"kind", "call"}, {"callee", name}, {"arguments", arguments}};
            }
            if (name == "true" || name == "false") return {{"kind", "literal"}, {"value", name == "true"}, {"raw", name}};
            return {{"kind", "reference"}, {"path", name}};
        }
        const QString unsupported = m_token.text; next();
        return {{"kind", "unsupported"}, {"text", unsupported}};
    }

    QString m_text;
    int m_pos = 0;
    Token m_token;
};

QJsonObject expressionAst(const QString& expression)
{
    if (expression.trimmed().isEmpty()) return {};
    return ExpressionProjector(expression).parse();
}

QString ownedFeatureType(Element* element)
{
    if (!element) return {};
    for (Relationship* relationship : element->ownedRelationships()) {
        if (auto* typing = qobject_cast<FeatureTyping*>(relationship); typing && !typing->typeName().isEmpty()) return typing->typeName();
    }
    return {};
}

} // namespace

ParseOutcome ModelSnapshotAdapter::parse(const QString& uri,
                                         const QString& text,
                                         const std::atomic_bool* canceled) const
{
    ParseOutcome outcome;
    if (canceled && canceled->load()) {
        outcome.canceled = true;
        return outcome;
    }

    SysMLTextParser parser;
    Resource* resource = parser.parseText(text, uri);
    if (canceled && canceled->load()) {
        delete resource;
        outcome.canceled = true;
        return outcome;
    }

    QList<Record> records;
    QSet<const Element*> visited;
    if (resource) {
        for (Element* root : resource->contents()) {
            collectElements(root, nullptr, records, visited, canceled);
        }
    }
    if (canceled && canceled->load()) {
        delete resource;
        outcome.canceled = true;
        return outcome;
    }

    SourceLocator locator(uri, text);
    const QList<PackageScope> packageScopes = parsePackageScopes(text);
    QHash<QString, QVector<int>> scopesByName;
    for (int i = 0; i < packageScopes.size(); ++i) {
        scopesByName[packageScopes.at(i).name].append(i);
    }
    QHash<QString, int> nextScopeByName;
    QHash<const Element*, int> scopeByElement;
    QHash<int, Element*> elementByScope;
    for (Record& record : records) {
        if (record.element->elementType() != "Package") continue;
        const QString name = record.element->declaredName();
        const QVector<int>& candidates = scopesByName[name];
        const int occurrence = nextScopeByName.value(name, 0);
        if (occurrence >= candidates.size()) continue;
        const int scopeIndex = candidates.at(occurrence);
        nextScopeByName[name] = occurrence + 1;
        scopeByElement.insert(record.element, scopeIndex);
        elementByScope.insert(scopeIndex, record.element);
    }
    for (Record& record : records) {
        const int scopeIndex = scopeByElement.value(record.element, -1);
        if (scopeIndex < 0) continue;
        const int parentScope = packageScopes.at(scopeIndex).parent;
        record.owner = parentScope >= 0 ? elementByScope.value(parentScope, record.owner) : nullptr;
    }
    QHash<const Element*, QString> ids;
    QHash<QString, int> keyOccurrences;
    int missingSourceCount = 0;
    int ambiguousSourceCount = 0;

    for (int i = 0; i < records.size(); ++i) {
        Record& record = records[i];
        record.ordinal = i;
        record.kind = record.element->elementType();
        record.name = record.element->declaredName();
        record.qualifiedName = record.element->qualifiedName();
        const int scopeIndex = scopeByElement.value(record.element, -1);
        if (scopeIndex >= 0) {
            const PackageScope& scope = packageScopes.at(scopeIndex);
            record.source = spanFor(uri, text, scope.nameStart, scope.name.size());
        } else {
            record.source = locator.locate(record.name);
        }
        if (!record.source.valid) {
            const QString expression = constraintExpression(record.element);
            if (!expression.isEmpty()) record.source = locator.locate(expression);
        }
        if (!record.source.valid) {
            ++missingSourceCount;
        } else if (locator.occurrenceCount(record.name) > 1) {
            ++ambiguousSourceCount;
        }

        const QString ownerId = ids.value(record.owner);
        const QString identityName = !record.qualifiedName.isEmpty()
                                         ? record.qualifiedName
                                         : record.name;
        const QString baseKey = QStringLiteral("%1|%2|%3|%4|%5")
                                    .arg(uri, record.kind, identityName, ownerId)
                                    .arg(record.source.valid ? record.source.startUtf16 : -1);
        const int collision = keyOccurrences.value(baseKey, 0);
        keyOccurrences[baseKey] = collision + 1;
        record.id = QStringLiteral("e-%1").arg(
            sha256(QStringLiteral("%1|%2").arg(baseKey).arg(collision).toUtf8()).left(24));
        ids.insert(record.element, record.id);
    }

    QJsonArray elements;
    QJsonArray relationships;
    QJsonArray diagnostics;
    const StandardLibraryIndex& standardLibrary = StandardLibraryIndex::instance();
    QHash<QString, QString> libraryElementIds;
    QHash<QString, QString> libraryPackageIds;
    QJsonArray libraryElements;
    auto ensureLibraryElement = [&](const QString& typeName) -> QString {
        LibrarySymbol symbol;
        if (!standardLibrary.resolve(typeName, &symbol)) return {};
        if (libraryElementIds.contains(typeName)) return libraryElementIds.value(typeName);
        const QString id = QStringLiteral("lib-%1").arg(
            sha256((QString::fromUtf8(StandardLibraryIndex::Release) + "|" + typeName)
                       .toUtf8()).left(24));
        libraryElementIds.insert(typeName, id);
        libraryElements.append(QJsonObject{
            {"id", id},
            {"kind", "LibraryElement"},
            {"name", symbol.simpleName},
            {"qualifiedName", typeName},
            {"ownerId", QJsonValue::Null},
            {"source", QJsonObject{
                {"uri", QStringLiteral("sysml-library://%1/%2")
                            .arg(StandardLibraryIndex::Release, symbol.sourcePath)},
                {"start", QJsonObject{{"line", symbol.line}, {"column", 1}, {"offset", 0}}},
                {"end", QJsonObject{{"line", symbol.line}, {"column", 1}, {"offset", 0}}}
            }},
            {"properties", QJsonObject{
                {"declarationKind", symbol.kind},
                {"canonicalQualifiedName", symbol.qualifiedName},
                {"librarySource", "Systems-Modeling/SysML-v2-Release"},
                {"libraryRelease", StandardLibraryIndex::Release},
                {"libraryCommit", StandardLibraryIndex::Commit},
                {"sourcePath", symbol.sourcePath},
                {"resolutionStatus", "official-library"}
            }}
        });
        return id;
    };
    auto ensureLibraryPackage = [&](const QString& packageName) -> QString {
        if (!standardLibrary.containsPackage(packageName)) return {};
        if (libraryPackageIds.contains(packageName)) return libraryPackageIds.value(packageName);
        const QString id = QStringLiteral("libpkg-%1").arg(
            sha256((QString::fromUtf8(StandardLibraryIndex::Release) + "|package|" + packageName)
                       .toUtf8()).left(24));
        libraryPackageIds.insert(packageName, id);
        libraryElements.append(QJsonObject{
            {"id", id}, {"kind", "LibraryPackage"}, {"name", packageName},
            {"qualifiedName", packageName}, {"ownerId", QJsonValue::Null},
            {"source", QJsonValue::Null},
            {"properties", QJsonObject{
                {"librarySource", "Systems-Modeling/SysML-v2-Release"},
                {"libraryRelease", StandardLibraryIndex::Release},
                {"libraryCommit", StandardLibraryIndex::Commit},
                {"resolutionStatus", "official-library"}
            }}
        });
        return id;
    };
    auto localTypeId = [&](const QString& typeName) -> QString {
        Element* local = resolveByName(typeName, records);
        return local ? ids.value(local) : QString();
    };
    for (const QString& error : parser.errors()) {
        diagnostics.append(parseDiagnostic(error, "error", uri, text));
    }
    for (const QString& warning : parser.warnings()) {
        const QString typeName = unresolvedTypeName(warning);
        const QString namespaceName = unresolvedNamespaceName(warning);
        if (!typeName.isEmpty() &&
            (!localTypeId(typeName).isEmpty() || standardLibrary.resolve(typeName))) {
            continue;
        }
        if (!namespaceName.isEmpty() && standardLibrary.containsPackage(namespaceName)) continue;
        diagnostics.append(parseDiagnostic(warning, "warning", uri, text));
    }

    for (const Record& record : records) {
        const QJsonValue source = record.source.valid
                                      ? QJsonValue(record.source.json)
                                      : QJsonValue(QJsonValue::Null);
        auto* connectStatement = qobject_cast<ConnectStatement*>(record.element);
        const bool isRelationship = qobject_cast<Relationship*>(record.element) != nullptr || connectStatement;
        if (isRelationship) {
            QJsonArray sources = connectStatement
                                           ? connectEndpointIds(connectStatement, record.owner, records, ids, true)
                                           : endpointIds(record.element, record.owner, records, ids, true);
            QJsonArray targets = connectStatement
                                           ? connectEndpointIds(connectStatement, record.owner, records, ids, false)
                                           : endpointIds(record.element, record.owner, records, ids, false);
            if (auto* typing = qobject_cast<FeatureTyping*>(record.element);
                typing && targets.isEmpty() && !typing->typeName().isEmpty()) {
                QString targetId = localTypeId(typing->typeName());
                if (targetId.isEmpty()) targetId = ensureLibraryElement(typing->typeName());
                if (!targetId.isEmpty()) targets.append(targetId);
            }
            if (auto* namespaceImport = qobject_cast<NamespaceImport*>(record.element);
                namespaceImport && targets.isEmpty()) {
                const QString packageId = ensureLibraryPackage(namespaceImport->importedNamespaceName());
                if (!packageId.isEmpty()) targets.append(packageId);
            }
            relationships.append(QJsonObject{
                {"id", record.id},
                {"kind", connectStatement ? QJsonValue("Connection") : QJsonValue(record.kind)},
                {"sourceIds", sources},
                {"targetIds", targets},
                {"ownerId", record.owner ? QJsonValue(ids.value(record.owner)) : QJsonValue::Null},
                {"source", source},
                {"properties", elementProperties(record.element)}
            });
            if (sources.isEmpty() || targets.isEmpty()) {
                QJsonObject item = diagnostic(
                    "AIDC_UNRESOLVED_ENDPOINT", "warning",
                    QStringLiteral("Relationship %1 has unresolved source or target endpoints")
                        .arg(record.kind), source);
                item["relatedElementIds"] = QJsonArray{record.id};
                diagnostics.append(item);
            }
        } else {
            QJsonObject properties = elementProperties(record.element);
            const QString typeName = properties.value("typeName").toString();
            if (!typeName.isEmpty()) {
                QString typeId = localTypeId(typeName);
                QString typeResolution = "local";
                if (typeId.isEmpty()) {
                    typeId = ensureLibraryElement(typeName);
                    typeResolution = typeId.isEmpty() ? "unresolved" : "official-library";
                }
                if (!typeId.isEmpty()) properties.insert("typeId", typeId);
                properties.insert("typeResolution", typeResolution);
            }
            if (record.source.valid) {
                const QString shortName = declaredShortNameFor(text, record.name);
                if (!shortName.isEmpty()) properties.insert("declaredShortName", shortName);
            }
            elements.append(QJsonObject{
                {"id", record.id},
                {"kind", record.kind},
                {"name", record.name.isEmpty() ? QJsonValue::Null : QJsonValue(record.name)},
                {"qualifiedName", record.qualifiedName.isEmpty()
                                      ? QJsonValue::Null
                                      : QJsonValue(record.qualifiedName)},
                {"ownerId", record.owner ? QJsonValue(ids.value(record.owner)) : QJsonValue::Null},
                {"source", source},
                {"properties", properties}
            });
        }
    }

    for (const QJsonValue& libraryElement : libraryElements) elements.append(libraryElement);

    for (const Record& record : records) {
        const QString relationKind = constraintRelationshipKind(record.element);
        if (relationKind.isEmpty() || !record.owner || !ids.contains(record.owner)) {
            continue;
        }
        const QJsonValue source = record.source.valid
                                      ? QJsonValue(record.source.json)
                                      : QJsonValue(QJsonValue::Null);
        relationships.append(QJsonObject{
            {"id", QStringLiteral("r-%1").arg(sha256((record.id + "|constraint").toUtf8()).left(24))},
            {"kind", relationKind},
            {"sourceIds", QJsonArray{ids.value(record.owner)}},
            {"targetIds", QJsonArray{record.id}},
            {"ownerId", ids.value(record.owner)},
            {"source", source},
            {"properties", QJsonObject{
                {"expression", constraintExpression(record.element)},
                {"statementKind", record.kind}
            }}
        });
    }

    // C1 semantic projections. These objects reference the canonical element IDs
    // above; they do not duplicate ownership or source-of-truth model data.
    QJsonArray calculationDefinitions;
    QJsonArray calculationUsages;
    QJsonArray analysisCases;
    QJsonArray parameters;
    QJsonArray calculationConstraints;
    QHash<const Element*, QStringList> parameterIdsByOwner;
    QHash<const Element*, QStringList> constraintIdsByOwner;
    QHash<const Element*, QStringList> usageIdsByOwner;

    auto sourceValue = [](const Record& record) -> QJsonValue {
        return record.source.valid ? QJsonValue(record.source.json) : QJsonValue(QJsonValue::Null);
    };
    auto appendParameter = [&](const Record& record, const QString& role,
                               const QString& name, const QString& typeName,
                               const QString& expression, const QString& unit) {
        QJsonObject parameter = {
            {"id", record.id}, {"elementId", record.id}, {"name", name},
            {"role", role}, {"typeName", typeName.isEmpty() ? QJsonValue::Null : QJsonValue(typeName)},
            {"expression", expression.isEmpty() ? QJsonValue::Null : QJsonValue(expression)},
            {"expressionAst", expression.isEmpty() ? QJsonValue::Null : QJsonValue(expressionAst(expression))},
            {"unit", unit.isEmpty() ? QJsonValue::Null : QJsonValue(unit)},
            {"ownerId", record.owner ? QJsonValue(ids.value(record.owner)) : QJsonValue::Null},
            {"source", sourceValue(record)}
        };
        parameters.append(parameter);
        if (record.owner) parameterIdsByOwner[record.owner].append(record.id);
    };

    for (const Record& record : records) {
        if (auto* input = qobject_cast<ActionInputOutput*>(record.element)) {
            appendParameter(record, input->isInput() ? "input" : "output",
                            input->itemName(), input->itemType(), input->defaultValue(), {});
        } else if (auto* output = qobject_cast<ReturnParameter*>(record.element)) {
            appendParameter(record, "output", record.name, output->typeName(), output->expression(), {});
        } else if (auto* feature = qobject_cast<Feature*>(record.element)) {
            QString role = qobject_cast<AnalysisCase*>(record.owner) ? "result" : "local";
            appendParameter(record, role, record.name, ownedFeatureType(feature),
                            feature->valueExpression(), feature->unitExpression());
            const QJsonObject ast = expressionAst(feature->valueExpression());
            if (ast.value("kind") == "call") {
                const QString usageId = QStringLiteral("cu-%1").arg(
                    sha256((record.id + "|" + ast.value("callee").toString()).toUtf8()).left(24));
                Element* definition = resolveByName(ast.value("callee").toString(), records);
                QJsonObject usage = {
                    {"id", usageId}, {"resultParameterId", record.id},
                    {"definitionName", ast.value("callee")},
                    {"definitionId", definition ? QJsonValue(ids.value(definition)) : QJsonValue::Null},
                    {"analysisId", record.owner && qobject_cast<AnalysisCase*>(record.owner)
                                       ? QJsonValue(ids.value(record.owner)) : QJsonValue::Null},
                    {"arguments", ast.value("arguments")}, {"source", sourceValue(record)}
                };
                calculationUsages.append(usage);
                if (record.owner) usageIdsByOwner[record.owner].append(usageId);
            }
        }

        if (qobject_cast<ConstraintStatement*>(record.element) && record.owner &&
            qobject_cast<AnalysisCase*>(record.owner)) {
            const QString expression = constraintExpression(record.element);
            calculationConstraints.append(QJsonObject{
                {"id", record.id}, {"elementId", record.id}, {"name", record.name},
                {"analysisId", ids.value(record.owner)}, {"expression", expression},
                {"expressionAst", expressionAst(expression)}, {"source", sourceValue(record)}
            });
            constraintIdsByOwner[record.owner].append(record.id);
        }
    }

    for (const Record& record : records) {
        if (qobject_cast<CalculationDefinition*>(record.element)) {
            QJsonArray inputs, outputs, locals;
            for (const QString& parameterId : parameterIdsByOwner.value(record.element)) {
                for (const QJsonValue& parameterValue : parameters) {
                    const QJsonObject parameter = parameterValue.toObject();
                    if (parameter.value("id") != parameterId) continue;
                    const QString role = parameter.value("role").toString();
                    if (role == "input") inputs.append(parameterId);
                    else if (role == "output") outputs.append(parameterId);
                    else locals.append(parameterId);
                    break;
                }
            }
            calculationDefinitions.append(QJsonObject{
                {"id", record.id}, {"elementId", record.id}, {"name", record.name},
                {"inputParameterIds", inputs}, {"outputParameterIds", outputs},
                {"localParameterIds", locals}, {"source", sourceValue(record)}
            });
        } else if (auto* analysis = qobject_cast<AnalysisCase*>(record.element)) {
            QJsonArray resultIds, constraintIds, usageIds;
            for (const QString& id : parameterIdsByOwner.value(record.element)) resultIds.append(id);
            for (const QString& id : constraintIdsByOwner.value(record.element)) constraintIds.append(id);
            for (const QString& id : usageIdsByOwner.value(record.element)) usageIds.append(id);
            Element* subject = resolveByName(analysis->subjectReference(), records);
            analysisCases.append(QJsonObject{
                {"id", record.id}, {"elementId", record.id}, {"name", record.name},
                {"subjectReference", analysis->subjectReference()},
                {"subjectId", subject ? QJsonValue(ids.value(subject)) : QJsonValue::Null},
                {"resultParameterIds", resultIds}, {"constraintIds", constraintIds},
                {"calculationUsageIds", usageIds}, {"source", sourceValue(record)}
            });
        }
    }

    // Resolve every call argument to its canonical model element and pair it
    // with the definition's formal input. Consumers can render bindings
    // without repeating name/path resolution in the UI.
    QHash<QString, QJsonObject> definitionById;
    QHash<QString, QJsonObject> parameterById;
    for (const QJsonValue& value : calculationDefinitions) {
        const QJsonObject definition = value.toObject();
        definitionById.insert(definition.value("id").toString(), definition);
    }
    for (const QJsonValue& value : parameters) {
        const QJsonObject parameter = value.toObject();
        parameterById.insert(parameter.value("id").toString(), parameter);
    }
    for (qsizetype usageIndex = 0; usageIndex < calculationUsages.size(); ++usageIndex) {
        QJsonObject usage = calculationUsages.at(usageIndex).toObject();
        const QJsonObject definition = definitionById.value(usage.value("definitionId").toString());
        const QJsonArray formalIds = definition.value("inputParameterIds").toArray();
        const QJsonArray arguments = usage.value("arguments").toArray();
        Element* analysisOwner = nullptr;
        const QString analysisId = usage.value("analysisId").toString();
        for (const Record& record : records) {
            if (record.id == analysisId) { analysisOwner = record.owner; break; }
        }
        QJsonArray bindings;
        QJsonArray actualParameterIds;
        const qsizetype bindingCount = std::max(formalIds.size(), arguments.size());
        for (qsizetype bindingIndex = 0; bindingIndex < bindingCount; ++bindingIndex) {
            const QString formalId = bindingIndex < formalIds.size()
                                         ? formalIds.at(bindingIndex).toString() : QString();
            const QJsonObject formal = parameterById.value(formalId);
            const QJsonObject argument = bindingIndex < arguments.size()
                                             ? arguments.at(bindingIndex).toObject() : QJsonObject();
            const QString reference = argument.value("kind") == "reference"
                                          ? argument.value("path").toString() : QString();
            Element* actual = reference.isEmpty()
                                  ? nullptr : resolveConnectEnd(reference, analysisOwner, records);
            const QString actualId = actual ? ids.value(actual) : QString();
            if (!actualId.isEmpty()) actualParameterIds.append(actualId);
            bindings.append(QJsonObject{
                {"index", static_cast<int>(bindingIndex)},
                {"formalParameterId", formalId.isEmpty() ? QJsonValue::Null : QJsonValue(formalId)},
                {"formalParameterName", formal.value("name")},
                {"actualParameterId", actualId.isEmpty() ? QJsonValue::Null : QJsonValue(actualId)},
                {"actualReference", reference.isEmpty() ? QJsonValue::Null : QJsonValue(reference)},
                {"argument", argument}
            });
        }
        usage.insert("bindings", bindings);
        usage.insert("actualParameterIds", actualParameterIds);
        calculationUsages.replace(usageIndex, usage);
    }

    QJsonArray renderingDefinitions;
    QJsonArray viewDefinitions;
    QJsonArray viewUsages;
    QHash<QString, const Record*> renderingByName;
    for (const Record& record : records) {
        if (auto* rendering = qobject_cast<RenderingUsage*>(record.element)) {
            renderingByName.insert(record.name, &record);
            QJsonArray columns;
            for (const Record& candidate : records) {
                auto* column = qobject_cast<ViewUsage*>(candidate.element);
                if (!column || candidate.owner != record.element) continue;
                QString label = candidate.name;
                for (const Record& member : records) {
                    auto* feature = qobject_cast<Feature*>(member.element);
                    if (!feature || member.owner != candidate.element || member.name != "label") continue;
                    const QString declaredLabel = stringLiteralValue(feature->valueExpression());
                    if (!declaredLabel.isEmpty()) label = declaredLabel;
                    break;
                }
                columns.append(QJsonObject{
                    {"id", candidate.id}, {"name", candidate.name},
                    {"key", candidate.name}, {"label", label},
                    {"columnView", column->typeName()}, {"source", sourceValue(candidate)}
                });
            }
            const QString normalizedType = rendering->renderingType().section("::", -1);
            renderingDefinitions.append(QJsonObject{
                {"id", record.id}, {"elementId", record.id}, {"name", record.name},
                {"renderingType", rendering->renderingType()},
                {"renderingKind", normalizedType == "asElementTable" ? "element-table" : "unknown"},
                {"columns", columns}, {"source", sourceValue(record)}
            });
        }
    }
    for (const Record& record : records) {
        if (auto* definition = qobject_cast<ViewDefinition*>(record.element)) {
            QString renderingReference;
            for (const Record& child : records) {
                if (child.owner == record.element && qobject_cast<ViewRenderingMember*>(child.element)) {
                    renderingReference = child.name; break;
                }
            }
            const Record* rendering = renderingByName.value(renderingReference, nullptr);
            viewDefinitions.append(QJsonObject{
                {"id", record.id}, {"elementId", record.id}, {"name", record.name},
                {"viewType", definition->viewType()},
                {"viewKind", definition->viewType().section("::", -1) == "GridView" ? "grid" : "general"},
                {"renderingReference", renderingReference},
                {"renderingId", rendering ? QJsonValue(rendering->id) : QJsonValue::Null},
                {"source", sourceValue(record)}
            });
        } else if (auto* view = qobject_cast<ViewUsage*>(record.element)) {
            if (qobject_cast<RenderingUsage*>(record.owner)) continue;
            QJsonArray exposedReferences, exposedElementIds;
            for (const Record& child : records) {
                auto* expose = qobject_cast<ExposeStatement*>(child.element);
                if (!expose || child.owner != record.element) continue;
                exposedReferences.append(expose->exposedQualifiedName());
                if (Element* exposed = resolveByName(expose->exposedQualifiedName(), records)) {
                    exposedElementIds.append(ids.value(exposed));
                }
            }
            QString renderingReference = view->renderMethod();
            const Record* rendering = renderingByName.value(renderingReference, nullptr);
            Element* viewDefinition = resolveByName(view->typeName(), records);
            viewUsages.append(QJsonObject{
                {"id", record.id}, {"elementId", record.id}, {"name", record.name},
                {"definitionReference", view->typeName()},
                {"definitionId", viewDefinition ? QJsonValue(ids.value(viewDefinition)) : QJsonValue::Null},
                {"renderingReference", renderingReference},
                {"renderingId", rendering ? QJsonValue(rendering->id) : QJsonValue::Null},
                {"exposedReferences", exposedReferences}, {"exposedElementIds", exposedElementIds},
                {"source", sourceValue(record)}
            });
        }
    }

    QJsonObject calculationModel = {
        {"version", "0.1"}, {"definitions", calculationDefinitions},
        {"usages", calculationUsages}, {"analyses", analysisCases},
        {"parameters", parameters}, {"constraints", calculationConstraints}
    };
    QJsonObject viewModel = {
        {"version", "0.1"}, {"definitions", viewDefinitions},
        {"views", viewUsages}, {"renderings", renderingDefinitions}
    };

    if (missingSourceCount > 0 || ambiguousSourceCount > 0) {
        diagnostics.append(diagnostic(
            "AIDC_SOURCE_SPAN_PARTIAL", "warning",
            QStringLiteral("Conservative source mapping left %1 elements without a span; "
                           "%2 named elements had multiple textual candidates")
                .arg(missingSourceCount)
                .arg(ambiguousSourceCount)));
    }

    QJsonObject capabilities = {
        {"parse.file", "supported"},
        {"diagnostic.structured", "supported"},
        {"model.package-ownership", "supported"},
        {"model.stable-id", "supported"},
        {"model.source-span", "partial"},
        {"requirement.definition", "partial"},
        {"requirement.usage", "partial"},
        {"requirement.constraint", "supported"},
        {"trace.satisfy", "partial"},
        {"trace.verify", "unsupported"},
        {"trace.derive", "unsupported"},
        {"trace.refine", "unsupported"},
        {"reference.cross-file", "partial"},
        {"library.standard-models", standardLibrary.isLoaded() ? "supported" : "unavailable"},
        {"model.type-reference", standardLibrary.isLoaded() ? "supported" : "partial"},
        {"architecture.part", "partial"},
        {"architecture.port", "partial"},
        {"architecture.connection", "partial"},
        {"architecture.typed-instance-port", "supported"}
        ,{"calculation.definition", "supported"}
        ,{"calculation.analysis", "supported"}
        ,{"calculation.expression-ast", "partial"}
        ,{"calculation.argument-binding", "supported"}
        ,{"view.definition", "supported"}
        ,{"view.expose", "supported"}
        ,{"view.table-rendering", "supported"}
        ,{"analysis.evaluate", "supported"}
        ,{"analysis.units", "supported"}
        ,{"analysis.constraints", "supported"}
        ,{"analysis.advanced-functions", "unsupported"}
    };

    outcome.snapshot = {
        {"schemaVersion", SchemaVersion},
        {"document", QJsonObject{
             {"uri", uri},
             {"contentHash", sha256(text.toUtf8())},
             {"parser", QJsonObject{
                  {"name", "AIDC Reference C++ Parser Runtime"},
                  {"baseline", ParserBaseline},
                  {"capabilityVersion", RuntimeVersion}
              }}
         }},
        {"elements", elements},
        {"relationships", relationships},
        {"calculationModel", calculationModel},
        {"viewModel", viewModel},
        {"diagnostics", diagnostics},
        {"capabilities", capabilities}
    };

    delete resource;
    return outcome;
}

QJsonObject ModelSnapshotAdapter::capabilitiesResponse()
{
    const StandardLibraryIndex& standardLibrary = StandardLibraryIndex::instance();
    return {
        {"runtimeVersion", RuntimeVersion},
        {"schemaVersion", SchemaVersion},
        {"parserMode", "reference-antlr"},
        {"parserBaseline", ParserBaseline},
        {"qtDependency", true},
        {"capabilities", QJsonObject{
             {"parse.file", "supported"},
             {"cancel.cooperative", "partial"},
             {"diagnostic.structured", "supported"},
             {"model.stable-id", "supported"},
             {"model.source-span", "partial"},
             {"requirement.constraint", "supported"},
             {"trace.satisfy", "partial"},
             {"trace.verify", "unsupported"},
             {"trace.derive", "unsupported"},
             {"trace.refine", "unsupported"},
              {"reference.cross-file", "partial"},
              {"library.standard-models", standardLibrary.isLoaded() ? "supported" : "unavailable"},
              {"model.type-reference", standardLibrary.isLoaded() ? "supported" : "partial"},
             {"architecture.part", "partial"},
             {"architecture.port", "partial"},
             {"architecture.connection", "partial"},
             {"architecture.typed-instance-port", "supported"},
             {"calculation.definition", "supported"},
             {"calculation.analysis", "supported"},
             {"calculation.expression-ast", "partial"},
             {"view.definition", "supported"},
             {"view.expose", "supported"},
             {"view.table-rendering", "supported"}
             ,{"analysis.evaluate", "supported"}
             ,{"analysis.units", "supported"}
             ,{"analysis.constraints", "supported"}
             ,{"analysis.advanced-functions", "unsupported"}
         }},
        {"standardLibrary", standardLibrary.status()}
    };
}

QJsonObject ModelSnapshotAdapter::healthResponse()
{
    const StandardLibraryIndex& standardLibrary = StandardLibraryIndex::instance();
    return {
        {"status", standardLibrary.isLoaded() ? "ok" : "degraded"},
        {"runtimeVersion", RuntimeVersion},
        {"schemaVersion", SchemaVersion},
        {"parserMode", "reference-antlr"},
        {"parserBaseline", ParserBaseline},
        {"qtDependency", true},
        {"standardLibrary", standardLibrary.status()}
    };
}

} // namespace Aidc
