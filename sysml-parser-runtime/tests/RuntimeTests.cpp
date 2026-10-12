#include <QtTest>

#include "ModelSnapshotAdapter.h"
#include "StandardLibraryIndex.h"
#include "AnalysisEngine.h"

using namespace Aidc;

class RuntimeTests : public QObject {
    Q_OBJECT

private slots:
    void healthAndCapabilities();
    void parseBasicRequirement();
    void deterministicSnapshot();
    void structuredSyntaxDiagnostic();
    void architectureConnectionEndpoints();
    void typedPartConnectionEndpoints();
    void officialStandardLibraryTypes();
    void requirementConstraintRelationship();
    void packageHierarchyFromSourceScopes();
    void calculationSemanticProjection();
    void tableViewSemanticProjection();
    void unitAwareAnalysisAndFailures();
};

void RuntimeTests::healthAndCapabilities()
{
    QCOMPARE(ModelSnapshotAdapter::healthResponse().value("status").toString(), QString("ok"));
    QCOMPARE(ModelSnapshotAdapter::capabilitiesResponse().value("schemaVersion").toString(),
             QString("0.1"));
}

void RuntimeTests::parseBasicRequirement()
{
    const QString text = R"(package Robot {
    requirement def SafeStop;
    requirement <'R1'> stop : SafeStop;
})";
    ModelSnapshotAdapter adapter;
    const ParseOutcome outcome = adapter.parse("memory://robot.sysml", text);
    QVERIFY(!outcome.canceled);
    QCOMPARE(outcome.snapshot.value("schemaVersion").toString(), QString("0.1"));
    QVERIFY(outcome.snapshot.value("elements").toArray().size() >= 3);

    bool foundRequirement = false;
    bool foundShortName = false;
    for (const QJsonValue& value : outcome.snapshot.value("elements").toArray()) {
        const QJsonObject element = value.toObject();
        if (element.value("kind") == "RequirementDefinition") {
            foundRequirement = true;
            QVERIFY(!element.value("id").toString().isEmpty());
            QVERIFY(element.value("source").isObject());
        }
        if (element.value("kind") == "RequirementUsage" && element.value("name") == "stop") {
            foundShortName = element.value("properties").toObject()
                                 .value("declaredShortName").toString() == "R1";
        }
    }
    QVERIFY(foundRequirement);
    QVERIFY(foundShortName);
}

void RuntimeTests::deterministicSnapshot()
{
    const QString text = "package Stable { part def Robot; }";
    ModelSnapshotAdapter adapter;
    const QJsonObject first = adapter.parse("memory://stable.sysml", text).snapshot;
    const QJsonObject second = adapter.parse("memory://stable.sysml", text).snapshot;
    QCOMPARE(first, second);
}

void RuntimeTests::structuredSyntaxDiagnostic()
{
    ModelSnapshotAdapter adapter;
    const QJsonObject snapshot = adapter.parse(
        "memory://invalid.sysml", "package Broken { this is invalid @@@ }").snapshot;
    const QJsonArray diagnostics = snapshot.value("diagnostics").toArray();
    QVERIFY(!diagnostics.isEmpty());
    const QJsonObject first = diagnostics.first().toObject();
    QCOMPARE(first.value("severity").toString(), QString("error"));
    QVERIFY(first.value("source").isObject());
    QCOMPARE(first.value("source").toObject().value("start").toObject().value("line").toInt(), 1);
}

void RuntimeTests::architectureConnectionEndpoints()
{
    const QString text = R"(package Robot {
    part def System {
        port commandIn;
        part motor { port command; }
        connect commandIn to motor.command;
    }
})";
    ModelSnapshotAdapter adapter;
    const QJsonObject snapshot = adapter.parse("memory://architecture.sysml", text).snapshot;
    const QJsonArray relationships = snapshot.value("relationships").toArray();
    bool foundConnection = false;
    QHash<QString, QJsonObject> elementsById;
    for (const QJsonValue& value : snapshot.value("elements").toArray()) {
        const QJsonObject element = value.toObject();
        elementsById.insert(element.value("id").toString(), element);
    }
    for (const QJsonValue& value : relationships) {
        const QJsonObject relationship = value.toObject();
        if (relationship.value("kind") == "Connection") {
            foundConnection = true;
            QCOMPARE(relationship.value("sourceIds").toArray().size(), 1);
            QCOMPARE(relationship.value("targetIds").toArray().size(), 1);
            QCOMPARE(relationship.value("properties").toObject().value("ends").toArray().size(), 2);
            const QString targetId = relationship.value("targetIds").toArray().first().toString();
            QCOMPARE(elementsById.value(targetId).value("kind").toString(), QString("PortUsage"));
            QCOMPARE(elementsById.value(targetId).value("name").toString(), QString("command"));
        }
    }
    QVERIFY(foundConnection);
}

void RuntimeTests::typedPartConnectionEndpoints()
{
    const QString text = R"(package Robot {
    part def Actuator {
        port powerIn;
        port commandIn;
    }
    part def Leg {
        port powerIn;
        port controlIn;
        part hip : Actuator;
        connect powerIn to hip.powerIn;
        connect controlIn to hip.commandIn;
    }
})";
    ModelSnapshotAdapter adapter;
    const QJsonObject snapshot = adapter.parse("memory://typed-endpoints.sysml", text).snapshot;
    QHash<QString, QJsonObject> elementsById;
    QString actuatorId;
    for (const QJsonValue& value : snapshot.value("elements").toArray()) {
        const QJsonObject element = value.toObject();
        elementsById.insert(element.value("id").toString(), element);
        if (element.value("kind") == "PartDefinition" && element.value("name") == "Actuator") {
            actuatorId = element.value("id").toString();
        }
    }
    QVERIFY(!actuatorId.isEmpty());

    int connectionCount = 0;
    for (const QJsonValue& value : snapshot.value("relationships").toArray()) {
        const QJsonObject relationship = value.toObject();
        if (relationship.value("kind") != "Connection") continue;
        ++connectionCount;
        QCOMPARE(relationship.value("sourceIds").toArray().size(), 1);
        QCOMPARE(relationship.value("targetIds").toArray().size(), 1);
        const QString targetId = relationship.value("targetIds").toArray().first().toString();
        const QJsonObject target = elementsById.value(targetId);
        QCOMPARE(target.value("kind").toString(), QString("PortUsage"));
        QCOMPARE(target.value("ownerId").toString(), actuatorId);
    }
    QCOMPARE(connectionCount, 2);
    for (const QJsonValue& value : snapshot.value("diagnostics").toArray()) {
        QVERIFY(value.toObject().value("code") != "AIDC_UNRESOLVED_ENDPOINT");
    }
}

void RuntimeTests::officialStandardLibraryTypes()
{
    QVERIFY(StandardLibraryIndex::instance().resolve("ISQBase::mass"));
    QVERIFY(StandardLibraryIndex::instance().resolve("ISQ::mass"));
    const QString text = R"(package Robot {
        part def Battery {
            attribute componentId : String = "BAT-001";
            attribute mass :> ISQ::mass = 1.2[kg];
            attribute capacity : Real = 10.0;
        }
    })";
    ModelSnapshotAdapter adapter;
    const QJsonObject snapshot = adapter.parse("memory://library.sysml", text).snapshot;
    QSet<QString> libraryNames;
    for (const QJsonValue& value : snapshot.value("elements").toArray()) {
        const QJsonObject element = value.toObject();
        if (element.value("kind") == "LibraryElement") {
            libraryNames.insert(element.value("qualifiedName").toString());
            QCOMPARE(element.value("properties").toObject().value("libraryRelease").toString(),
                     QString("2026-08"));
        }
    }
    QVERIFY(libraryNames.contains("String"));
    QVERIFY(libraryNames.contains("Real"));
    QVERIFY(libraryNames.contains("ISQ::mass"));
    for (const QJsonValue& value : snapshot.value("diagnostics").toArray()) {
        QVERIFY2(!value.toObject().value("message").toString().contains("Type not found"),
                 qPrintable(value.toObject().value("message").toString()));
    }
}

void RuntimeTests::requirementConstraintRelationship()
{
    const QString text = R"(package Robot {
    requirement def SafeStop {
        attribute stopTime;
        require constraint {stopTime <= 500}
    }
})";
    ModelSnapshotAdapter adapter;
    const QJsonObject snapshot = adapter.parse("memory://constraint.sysml", text).snapshot;
    const QJsonArray relationships = snapshot.value("relationships").toArray();
    bool foundConstraint = false;
    for (const QJsonValue& value : relationships) {
        const QJsonObject relationship = value.toObject();
        if (relationship.value("kind") == "RequirementConstraint") {
            foundConstraint = true;
            QCOMPARE(relationship.value("sourceIds").toArray().size(), 1);
            QCOMPARE(relationship.value("targetIds").toArray().size(), 1);
            QCOMPARE(relationship.value("properties").toObject().value("expression").toString(),
                     QString("stopTime <= 500"));
            QVERIFY(relationship.value("source").isObject());
        }
    }
    QVERIFY(foundConstraint);
}

void RuntimeTests::packageHierarchyFromSourceScopes()
{
    const QString text = R"(package Root {
        package Definitions {
            package Ports { port def CommandPort; }
        }
        package Configurations { }
    })";
    ModelSnapshotAdapter adapter;
    const QJsonObject snapshot = adapter.parse("memory://packages.sysml", text).snapshot;
    const QJsonArray elements = snapshot.value("elements").toArray();
    QHash<QString, QJsonObject> packages;
    for (const QJsonValue& value : elements) {
        const QJsonObject element = value.toObject();
        if (element.value("kind") == "Package") packages.insert(element.value("name").toString(), element);
    }
    QCOMPARE(packages.value("Definitions").value("ownerId").toString(), packages.value("Root").value("id").toString());
    QCOMPARE(packages.value("Ports").value("ownerId").toString(), packages.value("Definitions").value("id").toString());
    QCOMPARE(packages.value("Configurations").value("ownerId").toString(), packages.value("Root").value("id").toString());
    QCOMPARE(packages.value("Ports").value("source").toObject().value("start").toObject().value("line").toInt(), 3);
}

void RuntimeTests::calculationSemanticProjection()
{
    const QString text = R"(package AnalysisContract {
        calc def SumMass {
            in count : Integer;
            in unitMass :> ISQ::mass;
            return total :> ISQ::mass = count * unitMass;
        }
        part def Inputs { attribute count : Integer = 10; attribute unitMass :> ISQ::mass = 2[g]; }
        part inputs : Inputs;
        analysis massAnalysis {
            subject = inputs;
            attribute totalMass :> ISQ::mass = SumMass(inputs.count, inputs.unitMass);
            constraint massPositive { totalMass > 0[g] }
        }
    })";
    ModelSnapshotAdapter adapter;
    const QJsonObject snapshot = adapter.parse("memory://calculation.sysml", text).snapshot;
    for (const QJsonValue& value : snapshot.value("diagnostics").toArray()) {
        QVERIFY(value.toObject().value("severity") != "error");
    }
    const QJsonObject model = snapshot.value("calculationModel").toObject();
    QCOMPARE(model.value("definitions").toArray().size(), 1);
    QCOMPARE(model.value("analyses").toArray().size(), 1);
    QCOMPARE(model.value("usages").toArray().size(), 1);
    QCOMPARE(model.value("constraints").toArray().size(), 1);
    const QJsonObject usage = model.value("usages").toArray().first().toObject();
    QCOMPARE(usage.value("definitionName").toString(), QString("SumMass"));
    QCOMPARE(usage.value("arguments").toArray().size(), 2);
    QCOMPARE(usage.value("bindings").toArray().size(), 2);
    const QJsonObject firstBinding = usage.value("bindings").toArray().first().toObject();
    QCOMPARE(firstBinding.value("formalParameterName").toString(), QString("count"));
    QVERIFY(!firstBinding.value("formalParameterId").toString().isEmpty());
    QVERIFY(!firstBinding.value("actualParameterId").toString().isEmpty());
    QCOMPARE(firstBinding.value("actualReference").toString(), QString("inputs.count"));
    QCOMPARE(model.value("usages").toArray().first().toObject().value("actualParameterIds").toArray().size(), 2);
    QCOMPARE(snapshot.value("capabilities").toObject().value("calculation.definition").toString(),
             QString("supported"));
}

void RuntimeTests::tableViewSemanticProjection()
{
    const QString text = R"(package ViewContract {
        rendering resultTable :> asElementTable {
            view nameColumn :>> columnView { attribute label : String = "名称"; }
            view valueColumn :>> columnView { attribute label : String = "数值"; }
        }
        view def ResultTable :> GridView { render resultTable; }
        view results : ResultTable { expose ViewContract::ResultTable; render resultTable; }
    })";
    ModelSnapshotAdapter adapter;
    const QJsonObject snapshot = adapter.parse("memory://view.sysml", text).snapshot;
    for (const QJsonValue& value : snapshot.value("diagnostics").toArray()) {
        QVERIFY(value.toObject().value("severity") != "error");
    }
    const QJsonObject model = snapshot.value("viewModel").toObject();
    QCOMPARE(model.value("definitions").toArray().size(), 1);
    QCOMPARE(model.value("views").toArray().size(), 1);
    QCOMPARE(model.value("renderings").toArray().size(), 1);
    const QJsonObject rendering = model.value("renderings").toArray().first().toObject();
    QCOMPARE(rendering.value("renderingKind").toString(), QString("element-table"));
    QCOMPARE(rendering.value("columns").toArray().size(), 2);
    const QJsonObject firstColumn = rendering.value("columns").toArray().first().toObject();
    QCOMPARE(firstColumn.value("key").toString(), QString("nameColumn"));
    QCOMPARE(firstColumn.value("label").toString(), QString("名称"));
    QCOMPARE(snapshot.value("capabilities").toObject().value("view.table-rendering").toString(),
             QString("supported"));
}

void RuntimeTests::unitAwareAnalysisAndFailures()
{
    const QString text = R"(package AnalysisTest {
        calc def AddDuration {
            in lhs :> ISQ::duration;
            in second :> ISQ::duration;
            return total :> ISQ::duration = lhs + second;
        }
        calc def AddMass {
            in leftMass :> ISQ::mass;
            in rightMass :> ISQ::mass;
            return totalMass :> ISQ::mass = leftMass + rightMass;
        }
        part def Inputs {
            attribute lhs :> ISQ::duration = 1[s];
            attribute second :> ISQ::duration = 500[ms];
            attribute badMass :> ISQ::mass = 2[h];
            attribute zero : Real = 0;
            attribute kilograms :> ISQ::mass = 1[kg];
            attribute grams :> ISQ::mass = 500[g];
        }
        part inputs : Inputs;
        analysis check {
            subject = inputs;
            attribute total :> ISQ::duration = AddDuration(inputs.lhs, inputs.second);
            attribute mixedMass :> ISQ::mass = AddMass(inputs.kilograms, inputs.grams);
            attribute badResult :> ISQ::mass = inputs.badMass;
            attribute missingResult : Real = inputs.notDeclared;
            attribute divideResult : Real = 1 / inputs.zero;
            attribute cycleA : Real = cycleB + 1;
            attribute cycleB : Real = cycleA + 1;
            constraint durationTarget { total >= 1500[ms] }
        }
    })";
    ModelSnapshotAdapter adapter;
    const QJsonObject model = adapter.parse("memory://analysis-test.sysml", text).snapshot;
    AnalysisEngine engine;
    const QJsonObject analysis = engine.analyze(model).snapshot.value("analyses").toArray().first().toObject();
    QHash<QString, QJsonObject> results;
    for (const QJsonValue& value : analysis.value("results").toArray()) {
        const QJsonObject result = value.toObject();
        results.insert(result.value("name").toString(), result);
    }
    QCOMPARE(results.value("total").value("status").toString(), QString("computed"));
    QCOMPARE(results.value("total").value("value").toDouble(), 1.5);
    QCOMPARE(results.value("total").value("unit").toString(), QString("s"));
    QCOMPARE(results.value("mixedMass").value("status").toString(), QString("computed"));
    QCOMPARE(results.value("mixedMass").value("value").toDouble(), 1.5);
    QCOMPARE(results.value("mixedMass").value("unit").toString(), QString("kg"));
    QCOMPARE(results.value("badResult").value("status").toString(), QString("unit-error"));
    QCOMPARE(results.value("missingResult").value("status").toString(), QString("missing-input"));
    QCOMPARE(results.value("divideResult").value("status").toString(), QString("runtime-error"));
    QCOMPARE(results.value("cycleA").value("status").toString(), QString("cycle"));
    QCOMPARE(results.value("cycleB").value("status").toString(), QString("cycle"));
    const QJsonObject constraint = analysis.value("constraints").toArray().first().toObject();
    QCOMPARE(constraint.value("status").toString(), QString("pass"));
    QCOMPARE(constraint.value("margin").toDouble(), 0.0);
}

QTEST_MAIN(RuntimeTests)
#include "RuntimeTests.moc"

