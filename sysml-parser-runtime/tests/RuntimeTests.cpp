#include <QtTest>

#include "services/ModelSnapshotAdapter.h"
#include "semantic/StandardLibraryIndex.h"
#include "semantic/UnitSystem.h"
#include "analysis/AnalysisService.h"

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
    void kernelUnitSystem();
};

void RuntimeTests::healthAndCapabilities()
{
    const QJsonObject health = ModelSnapshotAdapter::healthResponse();
    QCOMPARE(health.value("status").toString(), QString("ok"));
    QCOMPARE(health.value("runtimeVersion").toString(), QString("1.0.0"));
    QCOMPARE(health.value("engineVersion").toString(), QString(ModelSnapshotAdapter::EngineVersion));
    QCOMPARE(health.value("parserMode").toString(), QString(ModelSnapshotAdapter::ParserMode));
    QCOMPARE(health.value("qtDependency").toBool(), true);
    const QJsonObject capabilities = ModelSnapshotAdapter::capabilitiesResponse();
    QCOMPARE(capabilities.value("schemaVersion").toString(), QString("0.1"));
    QCOMPARE(capabilities.value("engineVersion").toString(), QString(ModelSnapshotAdapter::EngineVersion));
    QCOMPARE(capabilities.value("parserMode").toString(), QString(ModelSnapshotAdapter::ParserMode));
    QCOMPARE(capabilities.value("qtDependency").toBool(), true);
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
            attribute distance :> ISQ::length = 0.25[m];
            attribute speed :> ISQ::speed = 0.96[m/s];
            attribute gravity :> ISQ::acceleration = 9.81[m/s^2];
            attribute powerLimit :> ISQ::power = 1[kW];
        }
        part inputs : Inputs;
        part otherInputs { attribute grams :> ISQ::mass = 250[g]; }
        analysis check {
            subject = inputs;
            attribute total :> ISQ::duration = AddDuration(inputs.lhs, inputs.second);
            attribute mixedMass :> ISQ::mass = AddMass(inputs.kilograms, inputs.grams);
            attribute qualifiedMass :> ISQ::mass = AddMass(inputs.kilograms, otherInputs.grams);
            attribute badResult :> ISQ::mass = inputs.badMass;
            attribute missingResult : Real = inputs.notDeclared;
            attribute divideResult : Real = 1 / inputs.zero;
            attribute cycleA : Real = cycleB + 1;
            attribute cycleB : Real = cycleA + 1;
            attribute lengthResult :> ISQ::length = inputs.distance;
            attribute speedResult :> ISQ::speed = inputs.speed;
            attribute accelerationResult :> ISQ::acceleration = inputs.gravity;
            attribute forceResult :> ISQ::force = inputs.kilograms * inputs.gravity;
            attribute torqueResult :> ISQ::torque = inputs.kilograms * inputs.gravity * inputs.distance;
            attribute powerResult :> ISQ::power = inputs.powerLimit;
            constraint durationTarget { total >= 1500[ms] }
        }
    })";
    ModelSnapshotAdapter adapter;
    const QJsonObject model = adapter.parse("memory://analysis-test.sysml", text).snapshot;
    AnalysisService engine;
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
    QCOMPARE(results.value("qualifiedMass").value("status").toString(), QString("computed"));
    QCOMPARE(results.value("qualifiedMass").value("value").toDouble(), 1.25);
    QCOMPARE(results.value("qualifiedMass").value("unit").toString(), QString("kg"));
    QCOMPARE(results.value("badResult").value("status").toString(), QString("unit-error"));
    QCOMPARE(results.value("missingResult").value("status").toString(), QString("missing-input"));
    QCOMPARE(results.value("divideResult").value("status").toString(), QString("runtime-error"));
    QCOMPARE(results.value("cycleA").value("status").toString(), QString("cycle"));
    QCOMPARE(results.value("cycleB").value("status").toString(), QString("cycle"));
    QCOMPARE(results.value("lengthResult").value("status").toString(), QString("computed"));
    QCOMPARE(results.value("lengthResult").value("unit").toString(), QString("m"));
    QCOMPARE(results.value("speedResult").value("status").toString(), QString("computed"));
    QCOMPARE(results.value("speedResult").value("unit").toString(), QString("m/s"));
    QCOMPARE(results.value("accelerationResult").value("status").toString(), QString("computed"));
    QCOMPARE(results.value("accelerationResult").value("unit").toString(), QString("m/s^2"));
    QCOMPARE(results.value("forceResult").value("status").toString(), QString("computed"));
    QCOMPARE(results.value("forceResult").value("unit").toString(), QString("N"));
    QCOMPARE(results.value("torqueResult").value("status").toString(), QString("computed"));
    QCOMPARE(results.value("torqueResult").value("unit").toString(), QString("N.m"));
    QCOMPARE(results.value("powerResult").value("status").toString(), QString("computed"));
    QCOMPARE(results.value("powerResult").value("value").toDouble(), 1.0);
    QCOMPARE(results.value("powerResult").value("unit").toString(), QString("kW"));
    const QJsonObject constraint = analysis.value("constraints").toArray().first().toObject();
    QCOMPARE(constraint.value("status").toString(), QString("pass"));
    QCOMPARE(constraint.value("margin").toDouble(), 0.0);
}

void RuntimeTests::kernelUnitSystem()
{
    const Semantic::UnitSystem& units = Semantic::UnitSystem::standard();
    Semantic::UnitDefinition acceleration;
    QVERIFY(units.resolve("m/s^2", &acceleration));
    QCOMPARE(units.dimensionsText(acceleration.dimensions), QString("L^1 T^-2"));

    Semantic::UnitDefinition compositeForce;
    QVERIFY(units.resolve("kg*m/s^2", &compositeForce));
    QCOMPARE(units.dimensionsText(compositeForce.dimensions), QString("L^1 M^1 T^-2"));
    QCOMPARE(units.preferredUnit(compositeForce.dimensions), QString("N"));

    Semantic::UnitDefinition compositeTorque;
    QVERIFY(units.resolve("kg*m^2/s^2", &compositeTorque));
    QCOMPARE(units.preferredUnit(compositeTorque.dimensions), QString("N.m"));
    QVERIFY(!units.resolve("notAUnit"));
}

QTEST_MAIN(RuntimeTests)
#include "RuntimeTests.moc"
