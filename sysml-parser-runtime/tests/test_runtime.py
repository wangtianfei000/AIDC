from pathlib import Path
import json
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "sysml-parser-runtime" / "src"))

from aidc_sysml_runtime.analysis import analyze
from aidc_sysml_runtime.model import make_snapshot
from aidc_sysml_runtime.server import capabilities, health


class RuntimeTests(unittest.TestCase):
    def test_health_declares_no_qt(self):
        self.assertEqual("1.0.0", health()["runtimeVersion"])
        self.assertEqual("stdlib", health()["parserMode"])
        self.assertFalse(health()["qtDependency"])
        self.assertEqual("0.1", capabilities()["schemaVersion"])

    def test_deterministic_parse_and_structured_error(self):
        text = "package Robot { requirement def SafeStop; requirement stop : SafeStop; }"
        first, _ = make_snapshot("memory://robot.sysml", text)
        second, _ = make_snapshot("memory://robot.sysml", text)
        self.assertEqual(first, second)
        self.assertGreaterEqual(len(first["elements"]), 3)
        broken, _ = make_snapshot("memory://broken.sysml", "package Broken { @@@ }")
        self.assertTrue(any(x["severity"] == "error" and x["source"] for x in broken["diagnostics"]))

    def test_connection_endpoints_and_views(self):
        text = '''package Robot {
          port def Bus; part def Device { port command; }
          part a : Device; part b : Device;
          connect a.command to b.command;
          rendering table :> asElementTable {
            view nameColumn :>> columnView { attribute label : String = "名称"; }
          }
          view tableView { expose Robot::a; render table; }
        }'''
        snapshot, _ = make_snapshot("memory://view.sysml", text)
        connection = next(x for x in snapshot["relationships"] if x["kind"] == "Connection")
        self.assertEqual(1, len(connection["sourceIds"]))
        self.assertEqual(1, len(connection["targetIds"]))
        self.assertEqual("名称", snapshot["viewModel"]["renderings"][0]["columns"][0]["label"])

    def test_unit_analysis(self):
        text = '''package AnalysisTest {
          calc def Sum { in a :> ISQ::duration; in b :> ISQ::duration; return total :> ISQ::duration = a + b; }
          part def Inputs { attribute first :> ISQ::duration = 500[ms]; attribute second :> ISQ::duration = 1[s]; }
          part inputs : Inputs;
          analysis run { subject = inputs;
            attribute total :> ISQ::duration = Sum(inputs.first, inputs.second);
            attribute bad :> ISQ::mass = 2[h];
            attribute missing : Real = inputs.unknown;
            attribute divide : Real = 1 / 0;
            attribute cycleA : Real = cycleB + 1;
            attribute cycleB : Real = cycleA + 1;
            constraint target { total >= 1.5[s] }
          }
        }'''
        result = analyze("memory://analysis.sysml", text)
        values = {x["name"]: x for x in result["analyses"][0]["results"]}
        self.assertEqual(("computed", 1500.0, "ms"), (values["total"]["status"], values["total"]["value"], values["total"]["unit"]))
        self.assertEqual("unit-error", values["bad"]["status"])
        self.assertEqual("missing-input", values["missing"]["status"])
        self.assertEqual("runtime-error", values["divide"]["status"])
        self.assertEqual("cycle", values["cycleA"]["status"])
        self.assertEqual("pass", result["analyses"][0]["constraints"][0]["status"])

    def test_robot_acceptance_fixture(self):
        source = (ROOT / "samples" / "jingang-10axis-system-design" / "architecture.sysml").read_text(encoding="utf-8")
        snapshot, _ = make_snapshot("fixture:///architecture.sysml", source)
        rendering = next(x for x in snapshot["viewModel"]["renderings"] if x["renderingKind"] == "element-table")
        view = next(x for x in snapshot["viewModel"]["views"] if x["renderingId"] == rendering["id"])
        self.assertEqual(8, len(rendering["columns"]))
        self.assertEqual(4, len(view["exposedElementIds"]))
        result = analyze("fixture:///architecture.sysml", source)
        core = next(x for x in result["analyses"] if x["name"] == "coreParameterAnalysis")
        values = {x["name"]: x for x in core["results"]}
        self.assertAlmostEqual(816.8, values["totalMassResult"]["value"])
        self.assertAlmostEqual(1.92, values["enduranceResult"]["value"])
        self.assertAlmostEqual(19.84, values["peakElectricalPowerResult"]["value"])
        self.assertAlmostEqual(128, values["stopResponseTimeResult"]["value"])


if __name__ == "__main__": unittest.main()
