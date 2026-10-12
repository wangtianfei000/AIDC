#pragma once

#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QSet>

namespace Aidc {

struct LibrarySymbol {
    QString requestedName;
    QString qualifiedName;
    QString simpleName;
    QString kind;
    QString sourcePath;
    int line = 1;
};

class StandardLibraryIndex {
public:
    static constexpr const char* Release = "2026-08";
    static constexpr const char* Commit = "fb97b754f29588b8e9c7a35f370880cd15eb29e7";

    static const StandardLibraryIndex& instance();

    bool isLoaded() const;
    QString rootPath() const;
    int fileCount() const;
    int symbolCount() const;
    bool containsPackage(const QString& name) const;
    bool resolve(const QString& name, LibrarySymbol* result = nullptr) const;
    QJsonObject status() const;

private:
    StandardLibraryIndex();
    bool resolveQualified(const QString& packageName,
                          const QString& simpleName,
                          QSet<QString>* visited,
                          LibrarySymbol* result) const;

    QString m_rootPath;
    int m_fileCount = 0;
    QHash<QString, LibrarySymbol> m_symbols;
    QHash<QString, QStringList> m_publicImports;
    QMultiHash<QString, QString> m_qualifiedNamesBySimpleName;
    QSet<QString> m_packages;
};

} // namespace Aidc
