#include <QtTest>

#include "parser/SysMLParser.h"
#include "core/infrastructure/Resource.h"

using namespace SysML;

class P2EmbeddedParserTests : public QObject {
    Q_OBJECT

private slots:
    void parsesMinimalPackage()
    {
        SysMLTextParser parser;
        Resource* resource = parser.parseText(
            QStringLiteral("package MinimalPackage { }"),
            QStringLiteral("memory://p2-minimal.sysml"));

        QVERIFY2(resource != nullptr, qPrintable(parser.errors().join('\n')));
        QVERIFY2(!parser.hasErrors(), qPrintable(parser.errors().join('\n')));
        QCOMPARE(resource->uri(), QStringLiteral("file://memory://p2-minimal.sysml"));
        QVERIFY(!resource->contents().isEmpty());
        delete resource;
    }

    void reportsInvalidSyntax()
    {
        SysMLTextParser parser;
        Resource* resource = parser.parseText(
            QStringLiteral("package Broken { part def ;"),
            QStringLiteral("memory://p2-invalid.sysml"));

        QVERIFY(resource != nullptr);
        QVERIFY(parser.hasErrors());
        delete resource;
    }
};

QTEST_MAIN(P2EmbeddedParserTests)
#include "P2EmbeddedParserTests.moc"
