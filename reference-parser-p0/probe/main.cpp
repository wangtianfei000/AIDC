#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QTextStream>

#include "parser/SysMLParser.h"
#include "core/infrastructure/Resource.h"
#include "core/model/Element.h"
#include "core/model/Relationship.h"

using namespace SysML;

namespace {

void visitElement(Element* element,
                  QSet<const Element*>& visited,
                  QMap<QString, int>& elementTypes,
                  QMap<QString, int>& relationshipTypes,
                  int& relationshipCount,
                  int& annotationCount)
{
    if (!element || visited.contains(element)) {
        return;
    }
    visited.insert(element);
    elementTypes[element->elementType()] += 1;

    const auto relationships = element->ownedRelationships();
    relationshipCount += relationships.size();
    for (Relationship* relationship : relationships) {
        if (relationship) {
            relationshipTypes[relationship->elementType()] += 1;
            visitElement(relationship, visited, elementTypes, relationshipTypes,
                         relationshipCount, annotationCount);
        }
    }

    const auto annotations = element->ownedAnnotations();
    annotationCount += annotations.size();
    for (Element* annotation : annotations) {
        visitElement(annotation, visited, elementTypes, relationshipTypes,
                     relationshipCount, annotationCount);
    }

    for (Element* child : element->ownedElements()) {
        visitElement(child, visited, elementTypes, relationshipTypes,
                     relationshipCount, annotationCount);
    }
}

QJsonObject countsToJson(const QMap<QString, int>& counts)
{
    QJsonObject result;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        result.insert(it.key(), it.value());
    }
    return result;
}

QJsonArray stringsToJson(const QStringList& strings)
{
    QJsonArray result;
    for (const QString& value : strings) {
        result.append(value);
    }
    return result;
}

QJsonObject inspectFile(const QString& filePath)
{
    QJsonObject result;
    QFileInfo info(filePath);
    result.insert("file", info.absoluteFilePath());

    SysMLTextParser parser;
    Resource* resource = parser.parseFile(filePath);
    result.insert("resourceCreated", resource != nullptr);
    result.insert("errors", stringsToJson(parser.errors()));
    result.insert("warnings", stringsToJson(parser.warnings()));
    result.insert("errorCount", parser.errors().size());
    result.insert("warningCount", parser.warnings().size());

    QSet<const Element*> visited;
    QMap<QString, int> elementTypes;
    QMap<QString, int> relationshipTypes;
    int relationshipCount = 0;
    int annotationCount = 0;
    int rootCount = 0;

    if (resource) {
        const auto roots = resource->contents();
        rootCount = roots.size();
        for (Element* root : roots) {
            visitElement(root, visited, elementTypes, relationshipTypes,
                         relationshipCount, annotationCount);
        }
    }

    result.insert("rootCount", rootCount);
    result.insert("elementCount", visited.size());
    result.insert("relationshipCount", relationshipCount);
    result.insert("annotationCount", annotationCount);
    result.insert("elementTypes", countsToJson(elementTypes));
    result.insert("relationshipTypes", countsToJson(relationshipTypes));
    delete resource;
    return result;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.size() < 2) {
        QTextStream(stderr) << "Usage: reference_parser_probe <file> [file ...]\n";
        return 2;
    }

    QJsonArray files;
    bool allParsed = true;
    for (int i = 1; i < args.size(); ++i) {
        QJsonObject result = inspectFile(args.at(i));
        if (result.value("errorCount").toInt() > 0 ||
            !result.value("resourceCreated").toBool()) {
            allParsed = false;
        }
        files.append(result);
    }

    QJsonObject root;
    root.insert("probeVersion", "0.1.0");
    root.insert("files", files);
    QTextStream(stdout) << QJsonDocument(root).toJson(QJsonDocument::Indented);
    return allParsed ? 0 : 1;
}

