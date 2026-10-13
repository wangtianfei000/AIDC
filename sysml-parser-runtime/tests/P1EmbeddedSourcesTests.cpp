#include <QtTest>

#include "infrastructure/Resource.h"
#include "model/PartDefinition.h"
#include "antlr4-runtime.h"

class P1EmbeddedSourcesTests : public QObject {
    Q_OBJECT

private slots:
    void modelLayerCreatesOwnedResourceContent()
    {
        SysML::Resource resource;
        auto* part = new SysML::PartDefinition;
        part->setDeclaredName(QStringLiteral("Robot"));

        resource.addContent(part);

        QCOMPARE(resource.contents().size(), 1);
        QCOMPARE(resource.contents().constFirst(), part);
        QCOMPARE(part->parent(), &resource);
        QCOMPARE(part->elementType(), QStringLiteral("PartDefinition"));
        QCOMPARE(part->declaredName(), QStringLiteral("Robot"));
        QVERIFY(resource.isModified());
    }

    void antlrRuntimeReadsUtf8Input()
    {
        antlr4::ANTLRInputStream input("part def Robot;");
        QCOMPARE(input.size(), size_t(15));
        const antlr4::misc::Interval firstWord(size_t(0), size_t(3));
        QCOMPARE(QString::fromStdString(input.getText(firstWord)), QStringLiteral("part"));
        QCOMPARE(ANTLRCPP_VERSION_MAJOR, 4);
        QCOMPARE(ANTLRCPP_VERSION_MINOR, 13);
        QCOMPARE(ANTLRCPP_VERSION_PATCH, 2);
    }
};

QTEST_APPLESS_MAIN(P1EmbeddedSourcesTests)
#include "P1EmbeddedSourcesTests.moc"
