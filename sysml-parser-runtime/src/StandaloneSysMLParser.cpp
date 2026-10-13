#include "StandaloneSysMLParser.h"

#include <QRegularExpression>

namespace {

struct Token {
    QString text;
    int start = 0;
    int end = 0;
};

QString normalized(QString value)
{
    if (value.size() >= 2 && value.startsWith('\'') && value.endsWith('\'')) {
        value = value.mid(1, value.size() - 2).replace("''", "'");
    }
    return value;
}

QList<Token> tokenize(const QString& source, QStringList* errors)
{
    QList<Token> result;
    int i = 0;
    while (i < source.size()) {
        const QChar ch = source.at(i);
        if (ch.isSpace()) { ++i; continue; }
        if (ch == '/' && i + 1 < source.size() && source.at(i + 1) == '/') {
            i += 2; const int contentBegin = i;
            while (i < source.size() && source.at(i) != '\n') ++i;
            if (!result.isEmpty() && result.last().text == "doc") {
                result.append({"__AIDC_DOC__" + source.mid(contentBegin, i - contentBegin).trimmed(), contentBegin, i});
            }
            continue;
        }
        if (ch == '/' && i + 1 < source.size() && source.at(i + 1) == '*') {
            const int begin = i; i += 2; const int contentBegin = i;
            while (i + 1 < source.size() && !(source.at(i) == '*' && source.at(i + 1) == '/')) ++i;
            if (i + 1 >= source.size()) {
                errors->append(QStringLiteral("[AIDC_UNTERMINATED_COMMENT] Line 1, Column %1: Unterminated block comment").arg(begin));
                break;
            }
            if (!result.isEmpty() && result.last().text == "doc") {
                result.append({"__AIDC_DOC__" + source.mid(contentBegin, i - contentBegin).trimmed(), contentBegin, i});
            }
            i += 2; continue;
        }
        const int begin = i;
        if (ch == '"' || ch == '\'') {
            const QChar quote = ch; ++i;
            while (i < source.size()) {
                if (source.at(i) == '\\') { i += 2; continue; }
                if (source.at(i) == quote) { ++i; break; }
                ++i;
            }
            if (i > source.size() || source.at(qMin(i, source.size()) - 1) != quote) {
                errors->append(QStringLiteral("[AIDC_UNTERMINATED_STRING] Line 1, Column %1: Unterminated quoted value").arg(begin));
            }
            result.append({source.mid(begin, i - begin), begin, i});
            continue;
        }
        if (ch.isLetter() || ch == '_') {
            ++i; while (i < source.size() && (source.at(i).isLetterOrNumber() || source.at(i) == '_')) ++i;
            result.append({source.mid(begin, i - begin), begin, i}); continue;
        }
        if (ch.isDigit() || (ch == '.' && i + 1 < source.size() && source.at(i + 1).isDigit())) {
            ++i; while (i < source.size() && (source.at(i).isDigit() || source.at(i) == '.' || source.at(i).toLower() == 'e' || source.at(i) == '+' || source.at(i) == '-')) ++i;
            result.append({source.mid(begin, i - begin), begin, i}); continue;
        }
        static const QStringList operators = {":>>", ":>", "::", "<=", ">=", "==", "!=", "->"};
        bool matched = false;
        for (const QString& op : operators) {
            if (source.mid(i, op.size()) == op) { i += op.size(); result.append({op, begin, i}); matched = true; break; }
        }
        if (matched) continue;
        ++i;
        result.append({QString(ch), begin, i});
        if (ch == '@') {
            int line = source.left(begin).count('\n') + 1;
            int column = begin - source.lastIndexOf('\n', begin - 1) - 1;
            errors->append(QStringLiteral("[AIDC_SYNTAX] Line %1, Column %2: Unexpected character '@'").arg(line).arg(column));
        }
    }
    return result;
}

class Parser {
public:
    Parser(QString source, QList<Token> tokens, QStringList* errors, QStringList* warnings)
        : m_source(std::move(source)), m_tokens(std::move(tokens)), m_errors(errors), m_warnings(warnings) {}

    Resource* parse()
    {
        auto* resource = new Resource;
        parseScope(nullptr, resource, false);
        return resource;
    }

private:
    bool atEnd() const { return m_index >= m_tokens.size(); }
    QString peek(int offset = 0) const { return m_index + offset < m_tokens.size() ? m_tokens.at(m_index + offset).text : QString(); }
    bool take(const QString& value) { if (peek() == value) { ++m_index; return true; } return false; }
    Token next() { return atEnd() ? Token{} : m_tokens.at(m_index++); }
    QString name() { return normalized(next().text); }

    void attach(Element* owner, Resource* resource, Element* element)
    {
        const QString ownerName = owner ? owner->qualifiedName() : QString();
        element->setQualifiedName(ownerName.isEmpty() ? element->declaredName()
                                                       : ownerName + "::" + element->declaredName());
        if (owner) owner->addElement(element); else resource->add(element);
    }

    QString collectUntil(const QSet<QString>& terminators)
    {
        if (atEnd()) return {};
        const int begin = m_tokens.at(m_index).start;
        int end = begin;
        int round = 0, square = 0;
        while (!atEnd()) {
            const QString value = peek();
            if (round == 0 && square == 0 && terminators.contains(value)) break;
            const Token token = next(); end = token.end;
            if (value == "(") ++round; else if (value == ")") --round;
            else if (value == "[") ++square; else if (value == "]") --square;
        }
        return m_source.mid(begin, end - begin).trimmed();
    }

    QString parseType()
    {
        if (!(take(":") || take(":>") || take(":>>"))) return {};
        QString value;
        while (!atEnd() && peek() != "=" && peek() != ";" && peek() != "{") {
            const QString part = next().text;
            if (part == "::") value += part; else value += (value.isEmpty() || value.endsWith("::") ? "" : " ") + normalized(part);
        }
        return value.trimmed();
    }

    void parseBody(Element* element, Resource* resource)
    {
        if (take("{")) parseScope(element, resource, true); else take(";");
    }

    void parseFeature(Element* owner, Resource* resource)
    {
        const QString featureName = name();
        const QString typeName = parseType();
        QString expression;
        if (take("=")) expression = collectUntil({";", "}"});
        auto* feature = new Feature("AttributeUsage", featureName);
        feature->setValueExpression(expression);
        attach(owner, resource, feature);
        if (!typeName.isEmpty()) feature->addRelationship(new FeatureTyping(feature, typeName));
        take(";");
    }

    void parseConstraint(Element* owner, Resource* resource, const QString& keyword)
    {
        if (peek() == "constraint") next();
        QString constraintName = keyword;
        if (peek() != "{") constraintName = name();
        if (!take("{")) { skipStatement(); return; }
        const QString expression = collectUntil({"}"});
        take("}"); take(";");
        Element* statement = nullptr;
        if (keyword == "require") statement = new RequireStatement(constraintName, expression);
        else if (keyword == "assert") statement = new AssertStatement(constraintName, expression);
        else if (keyword == "assume") statement = new AssumeStatement(constraintName, expression);
        else statement = new ConstraintStatement(constraintName, expression);
        attach(owner, resource, statement);
    }

    void parseConnect(Element* owner, Resource* resource)
    {
        const QString first = collectUntil({"to", ";"});
        if (!take("to")) { skipStatement(); return; }
        const QString second = collectUntil({";", "}"});
        auto* connection = new ConnectStatement({first.simplified().remove(' '), second.simplified().remove(' ')});
        attach(owner, resource, connection); take(";");
    }

    void parseScope(Element* owner, Resource* resource, bool closingBrace)
    {
        while (!atEnd()) {
            if (peek() == "}") { if (closingBrace) next(); return; }
            if (take(";")) continue;
            const QString keyword = next().text;
            if (keyword == "package") {
                auto* value = new Element("Package", name()); attach(owner, resource, value); parseBody(value, resource);
            } else if (keyword == "requirement") {
                if (take("def")) { auto* value = new Element("RequirementDefinition", name()); attach(owner, resource, value); parseBody(value, resource); }
                else {
                    if (take("<")) { while (!atEnd() && !take(">")) next(); }
                    const QString n = name(); const QString t = parseType();
                    auto* value = new RequirementUsage(n, t); attach(owner, resource, value); parseBody(value, resource);
                }
            } else if (keyword == "part") {
                if (take("def")) { auto* value = new Element("PartDefinition", name()); attach(owner, resource, value); parseBody(value, resource); }
                else { const QString n = name(); const QString t = parseType(); auto* value = new PartUsage(n, t); attach(owner, resource, value); parseBody(value, resource); }
            } else if (keyword == "port") {
                if (take("def")) { auto* value = new Element("PortDefinition", name()); attach(owner, resource, value); parseBody(value, resource); }
                else { const QString n = name(); const QString t = parseType(); auto* value = new PortUsage(n, t); attach(owner, resource, value); parseBody(value, resource); }
            } else if (keyword == "attribute") {
                parseFeature(owner, resource);
            } else if (keyword == "calc") {
                if (take("def")) { auto* value = new CalculationDefinition(name()); attach(owner, resource, value); parseBody(value, resource); }
                else skipStatement();
            } else if (keyword == "in" || keyword == "out") {
                const QString n = name(); const QString t = parseType(); QString expression;
                if (take("=")) expression = collectUntil({";", "}"});
                auto* value = new ActionInputOutput(keyword == "in", n, t, expression); attach(owner, resource, value); take(";");
            } else if (keyword == "return") {
                const QString n = name(); const QString t = parseType(); QString expression;
                if (take("=")) expression = collectUntil({";", "}"});
                auto* value = new ReturnParameter(n, t, expression); attach(owner, resource, value); take(";");
            } else if (keyword == "analysis") {
                auto* value = new AnalysisCase(name()); attach(owner, resource, value); parseBody(value, resource);
            } else if (keyword == "subject") {
                take("="); const QString reference = collectUntil({";", "}"}); take(";");
                if (auto* analysis = dynamic_cast<AnalysisCase*>(owner)) {
                    analysis->setSubjectReference(reference.simplified());
                    auto* subject = new AnalysisSubject("AnalysisSubject", "subject");
                    attach(owner, resource, subject);
                }
            } else if (keyword == "constraint" || keyword == "require" || keyword == "assert" || keyword == "assume") {
                parseConstraint(owner, resource, keyword);
            } else if (keyword == "connect") {
                parseConnect(owner, resource);
            } else if (keyword == "rendering") {
                const QString n = name(); const QString t = parseType(); auto* value = new RenderingUsage(n, t); attach(owner, resource, value); parseBody(value, resource);
            } else if (keyword == "view") {
                if (take("def")) {
                    const QString n = name(); const QString t = parseType();
                    auto* value = new ViewDefinition(n, t); attach(owner, resource, value);
                    parseBody(value, resource);
                }
                else { const QString n = name(); const QString t = parseType(); auto* value = new ViewUsage(n, t); attach(owner, resource, value); parseBody(value, resource); }
            } else if (keyword == "render") {
                const QString reference = collectUntil({";", "}"}).simplified(); take(";");
                if (auto* view = dynamic_cast<ViewUsage*>(owner)) view->setRenderMethod(reference);
                auto* value = new ViewRenderingMember(reference); attach(owner, resource, value);
            } else if (keyword == "expose") {
                const QString reference = collectUntil({";", "}"}).simplified().remove(' '); take(";");
                auto* value = new ExposeStatement(reference); attach(owner, resource, value);
            } else if ((keyword == "public" || keyword == "private") && take("import")) {
                QString imported = collectUntil({";", "::"}).simplified();
                while (!atEnd() && !take(";")) next();
                if (!imported.isEmpty() && owner) owner->addRelationship(new NamespaceImport(imported));
            } else if (keyword == "doc") {
                QString content;
                if (peek().startsWith("__AIDC_DOC__")) content = next().text.mid(12);
                auto* value = new DocStatement(content); attach(owner, resource, value);
            } else {
                skipStatement();
            }
        }
        if (closingBrace) m_errors->append("[AIDC_MISSING_BRACE] Line 1, Column 0: Missing closing brace");
    }

    void skipStatement()
    {
        int depth = 0;
        while (!atEnd()) {
            if (peek() == "{" ) { ++depth; next(); continue; }
            if (peek() == "}") { if (depth == 0) return; --depth; next(); continue; }
            if (peek() == ";" && depth == 0) { next(); return; }
            next();
        }
    }

    QString m_source;
    QList<Token> m_tokens;
    int m_index = 0;
    QStringList* m_errors;
    QStringList* m_warnings;
};

} // namespace

Resource* SysMLTextParser::parseText(const QString& text, const QString&)
{
    m_errors.clear();
    m_warnings.clear();
    const QList<Token> tokens = tokenize(text, &m_errors);
    Parser parser(text, tokens, &m_errors, &m_warnings);
    return parser.parse();
}
