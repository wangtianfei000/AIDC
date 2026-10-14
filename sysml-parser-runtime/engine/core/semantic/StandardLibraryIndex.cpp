#include "StandardLibraryIndex.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QRegularExpression>

namespace Aidc {
namespace {

QString withoutComments(const QString& text)
{
    QString result = text;
    bool lineComment = false;
    bool blockComment = false;
    QChar quote;
    for (int i = 0; i < result.size(); ++i) {
        const QChar value = result.at(i);
        const QChar next = i + 1 < result.size() ? result.at(i + 1) : QChar();
        if (lineComment) {
            if (value == '\n') lineComment = false;
            else result[i] = ' ';
            continue;
        }
        if (blockComment) {
            if (value == '*' && next == '/') {
                result[i] = result[i + 1] = ' ';
                blockComment = false;
                ++i;
            } else if (value != '\n') {
                result[i] = ' ';
            }
            continue;
        }
        if (!quote.isNull()) {
            if (value == '\\') { ++i; continue; }
            if (value == quote) quote = QChar();
            continue;
        }
        if (value == '/' && next == '/') {
            result[i] = result[i + 1] = ' ';
            lineComment = true;
            ++i;
        } else if (value == '/' && next == '*') {
            result[i] = result[i + 1] = ' ';
            blockComment = true;
            ++i;
        } else if (value == '\'' || value == '"') {
            quote = value;
        }
    }
    return result;
}

QVector<int> braceDepths(const QString& text)
{
    QVector<int> result(text.size() + 1);
    int depth = 0;
    QChar quote;
    for (int i = 0; i < text.size(); ++i) {
        result[i] = depth;
        const QChar value = text.at(i);
        if (!quote.isNull()) {
            if (value == '\\') { ++i; continue; }
            if (value == quote) quote = QChar();
            continue;
        }
        if (value == '\'' || value == '"') quote = value;
        else if (value == '{') ++depth;
        else if (value == '}') depth = qMax(0, depth - 1);
    }
    result[text.size()] = depth;
    return result;
}

QString normalizedIdentifier(QString value)
{
    value = value.trimmed();
    if (value.size() >= 2 && value.startsWith('\'') && value.endsWith('\'')) {
        value = value.mid(1, value.size() - 2);
    }
    return value;
}

} // namespace

const StandardLibraryIndex& StandardLibraryIndex::instance()
{
    static const StandardLibraryIndex value;
    return value;
}

StandardLibraryIndex::StandardLibraryIndex()
{
    const QString configured = QString::fromLocal8Bit(qgetenv("AIDC_SYSML_LIBRARY_ROOT"));
    m_rootPath = configured;
    if (m_rootPath.isEmpty()) {
        const QString packaged = QDir(QCoreApplication::applicationDirPath())
                                     .filePath("../library/sysml.library");
        if (QDir(packaged).exists()) m_rootPath = packaged;
    }
#ifdef AIDC_DEFAULT_SYSML_LIBRARY_ROOT
    if (m_rootPath.isEmpty()) m_rootPath = QString::fromUtf8(AIDC_DEFAULT_SYSML_LIBRARY_ROOT);
#endif
    m_rootPath = QDir::cleanPath(m_rootPath);
    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) return;

    const QRegularExpression packageExpression(
        QStringLiteral("(?:standard\\s+library\\s+)?package\\s+('(?:[^']|'')+'|[A-Za-z_][A-Za-z0-9_]*)\\s*\\{"));
    const QRegularExpression importExpression(
        QStringLiteral("\\bpublic\\s+import\\s+([A-Za-z_][A-Za-z0-9_:]*)::\\*"));
    const QRegularExpression declarationExpression(
        QStringLiteral("\\b(?:abstract\\s+)?(datatype|class|struct|association|behavior|function|predicate|interaction|metaclass|attribute(?:\\s+def)?|part\\s+def|port\\s+def|interface\\s+def|item\\s+def|connection\\s+def|action\\s+def|constraint\\s+def|requirement\\s+def|view\\s+def|rendering|metadata\\s+def)\\s+(?:<[^>]+>\\s*)?('(?:[^']|'')+'|[A-Za-z_][A-Za-z0-9_]*)"));

    QDirIterator files(m_rootPath, {"*.sysml", "*.kerml"}, QDir::Files,
                       QDirIterator::Subdirectories);
    while (files.hasNext()) {
        const QString filePath = files.next();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        ++m_fileCount;
        const QString source = QString::fromUtf8(file.readAll());
        const QString text = withoutComments(source);
        const QVector<int> depths = braceDepths(text);
        const QRegularExpressionMatch packageMatch = packageExpression.match(text);
        if (!packageMatch.hasMatch()) continue;
        const QString packageName = normalizedIdentifier(packageMatch.captured(1));
        m_packages.insert(packageName);
        const int packageDepth = depths.value(packageMatch.capturedStart()) + 1;
        const QString relativePath = QDir(m_rootPath).relativeFilePath(filePath).replace('\\', '/');

        auto imports = importExpression.globalMatch(text);
        while (imports.hasNext()) {
            const QRegularExpressionMatch match = imports.next();
            if (depths.value(match.capturedStart()) == packageDepth) {
                m_publicImports[packageName].append(match.captured(1));
            }
        }

        auto declarations = declarationExpression.globalMatch(text);
        while (declarations.hasNext()) {
            const QRegularExpressionMatch match = declarations.next();
            if (depths.value(match.capturedStart()) != packageDepth) continue;
            const QString simpleName = normalizedIdentifier(match.captured(2));
            if (simpleName.isEmpty()) continue;
            LibrarySymbol symbol;
            symbol.requestedName = packageName + "::" + simpleName;
            symbol.qualifiedName = symbol.requestedName;
            symbol.simpleName = simpleName;
            symbol.kind = match.captured(1).simplified();
            symbol.sourcePath = relativePath;
            symbol.line = text.left(match.capturedStart(2)).count('\n') + 1;
            m_symbols.insert(symbol.qualifiedName, symbol);
            m_qualifiedNamesBySimpleName.insert(simpleName, symbol.qualifiedName);
        }
    }
}

bool StandardLibraryIndex::isLoaded() const { return m_fileCount > 0 && !m_symbols.isEmpty(); }
QString StandardLibraryIndex::rootPath() const { return m_rootPath; }
int StandardLibraryIndex::fileCount() const { return m_fileCount; }
int StandardLibraryIndex::symbolCount() const { return m_symbols.size(); }
bool StandardLibraryIndex::containsPackage(const QString& name) const { return m_packages.contains(name); }

bool StandardLibraryIndex::resolveQualified(const QString& packageName,
                                            const QString& simpleName,
                                            QSet<QString>* visited,
                                            LibrarySymbol* result) const
{
    if (visited->contains(packageName)) return false;
    visited->insert(packageName);
    const QString qualifiedName = packageName + "::" + simpleName;
    if (m_symbols.contains(qualifiedName)) {
        if (result) *result = m_symbols.value(qualifiedName);
        return true;
    }
    for (const QString& importedPackage : m_publicImports.value(packageName)) {
        if (resolveQualified(importedPackage, simpleName, visited, result)) return true;
    }
    return false;
}

bool StandardLibraryIndex::resolve(const QString& name, LibrarySymbol* result) const
{
    if (!isLoaded() || name.trimmed().isEmpty()) return false;
    const QString requestedName = name.trimmed();
    LibrarySymbol resolved;
    bool found = false;
    if (requestedName.contains("::")) {
        QSet<QString> visited;
        found = resolveQualified(requestedName.section("::", 0, -2),
                                 requestedName.section("::", -1), &visited, &resolved);
    } else {
        const QList<QString> candidates = m_qualifiedNamesBySimpleName.values(requestedName);
        if (candidates.size() == 1) {
            resolved = m_symbols.value(candidates.first());
            found = true;
        } else {
            const QString scalarName = "ScalarValues::" + requestedName;
            if (m_symbols.contains(scalarName)) {
                resolved = m_symbols.value(scalarName);
                found = true;
            }
        }
    }
    if (found && result) {
        resolved.requestedName = requestedName;
        *result = resolved;
    }
    return found;
}

QJsonObject StandardLibraryIndex::status() const
{
    return {
        {"status", isLoaded() ? "loaded" : "unavailable"},
        {"source", "Systems-Modeling/SysML-v2-Release"},
        {"release", Release},
        {"commit", Commit},
        {"rootPath", m_rootPath},
        {"fileCount", m_fileCount},
        {"symbolCount", m_symbols.size()}
    };
}

} // namespace Aidc
