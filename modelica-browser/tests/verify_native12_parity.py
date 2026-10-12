"""Verify that the portable Modelica Native12 model stays aligned with its MuJoCo source baseline."""

from __future__ import annotations

import csv
import json
import math
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
AIDC = REPO / "AIDC"
MODEL = AIDC / "samples/modelica-t2-single-joint/modelica/AIDCT2/package.mo"
PROJECT = AIDC / "samples/modelica-t2-single-joint/model-project.json"
BASELINE = REPO / "Jingang/workspace/baselines/imu-locomotion-v1/B0-20261007/model/microduck_12axis_native"
ROBOT_XML = BASELINE / "mujoco/robot.xml"
ACTUATORS = BASELINE / "hd1910_actuators.json"
BATTERY = BASELINE / "battery_supply.json"
OMC = REPO / "tools/openmodelica/1.27.1/bin/omc.exe"
RESULT = AIDC / "modelica-browser/results/native12-parity.json"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


source = MODEL.read_text(encoding="utf-8")
project = json.loads(PROJECT.read_text(encoding="utf-8"))
actuator_spec = json.loads(ACTUATORS.read_text(encoding="utf-8"))
battery_spec = json.loads(BATTERY.read_text(encoding="utf-8"))
xml_root = ET.parse(ROBOT_XML).getroot()

joints = [node for node in xml_root.findall(".//joint") if node.get("type") == "hinge"]
joint_names = [node.get("name") for node in joints]
xml_masses = [float(node.get("mass", "nan")) for node in xml_root.findall(".//inertial")]

names_match = re.search(r"final parameter String jointNames\[12\]\s*=\s*\{([^}]+)\}", source, re.S)
require(names_match is not None, "Modelica Native12 jointNames declaration is missing")
modelica_joint_names = re.findall(r'"([^"]+)"', names_match.group(1))
require(modelica_joint_names == joint_names, "Native12 joint order differs from MuJoCo")
require(len(xml_masses) == 13, "MuJoCo baseline must contain 13 rigid-body inertials")
for mass in xml_masses:
    require(format(mass, ".12g") in source or repr(mass) in source, f"CAD mass {mass} is absent from Modelica")

evaluated = actuator_spec["evaluated_parameters"]
for key in ("stall_torque_nm", "stall_current_a", "rated_torque_nm", "rated_current_a", "no_load_speed_rad_s", "no_load_current_a", "static_current_a"):
    require(str(evaluated[key]) in source, f"Actuator parameter {key} differs from the frozen HD-1910 specification")
controller = actuator_spec["position_controller"]
require(str(controller["kp_nm_per_rad"]) in source, "HD-1910 Kp differs from MuJoCo runtime")
require(str(controller["kv_nm_per_rad_s"]) in source, "HD-1910 Kd differs from MuJoCo runtime")

for key in ("nominal_voltage_v", "capacity_ah", "internal_resistance_ohm", "max_output_current_a", "servo_bus_voltage_v", "converter_efficiency", "default_auxiliary_current_a"):
    require(str(battery_spec[key]) in source, f"Battery parameter {key} differs from MuJoCo runtime")
require('parameter Boolean enforceCurrentLimit = false' in source, "Training baseline requires telemetry-only current limiting")
require(project["modelica"]["defaultExperiment"] == "AIDCT2.MachineDuck", "MachineDuck is not the default experiment")

with tempfile.TemporaryDirectory(prefix="aidc-native12-") as temp_name:
    temp = Path(temp_name)
    package_uri = MODEL.as_posix()
    output_uri = temp.as_posix()
    mos = temp / "verify.mos"
    mos.write_text(
        '\n'.join([
            'setCommandLineOptions("--std=3.6");',
            'loadModel(Modelica, {"4.1.0"});',
            f'loadFile("{package_uri}");',
            'checkModel(AIDCT2.MachineDuck);',
            f'cd("{output_uri}");',
            'simulate(AIDCT2.MachineDuck, startTime=0, stopTime=0.02, numberOfIntervals=4, tolerance=1e-8, method="dassl", outputFormat="csv", variableFilter="time|basePosition\\\\[3\\\\]|batteryVoltage|jointPosition\\\\[[0-9]+\\\\]");',
            'getErrorString();',
        ]),
        encoding="utf-8",
    )
    run = subprocess.run([str(OMC), str(mos)], text=True, encoding="utf-8", errors="replace", capture_output=True, timeout=120)
    require(run.returncode == 0, f"OMC exited with {run.returncode}: {run.stderr}")
    require("completed successfully" in run.stdout, f"OMC checkModel failed: {run.stdout}")
    require("simulation finished successfully" in run.stdout, f"OMC simulation failed: {run.stdout}")
    csv_path = temp / "AIDCT2.MachineDuck_res.csv"
    rows = list(csv.DictReader(csv_path.open(encoding="utf-8")))
    require(rows, "OMC simulation produced no samples")
    require(all(math.isfinite(float(value)) for row in rows for value in row.values()), "Simulation contains non-finite values")
    require(sum(name.startswith("jointPosition[") for name in rows[-1]) == 12, "Simulation did not expose all 12 joint positions")

summary = {
    "passed": True,
    "modelClass": "AIDCT2.MachineDuck",
    "projectRevision": project["revision"],
    "mujocoBaseline": str(ROBOT_XML.relative_to(REPO)).replace("\\", "/"),
    "matched": {
        "jointOrder": joint_names,
        "jointCount": len(joints),
        "rigidBodyCount": len(xml_masses),
        "totalMassKg": sum(xml_masses),
        "actuatorModel": actuator_spec["actuator_model"],
        "batteryModel": battery_spec["battery_model"],
        "gravityMPerS2": 9.81,
        "stepSeconds": 0.005,
    },
    "verified": [
        "OMC structural balance",
        "OMC executable simulation",
        "12-channel Native12 protocol order",
        "13 CAD rigid-body masses",
        "HD-1910 PD, torque-speed and current anchors",
        "shared 6.4 V battery and 6.0 V servo bus",
    ],
    "knownDifference": "MuJoCo's 40 convex collision meshes are represented by two regularized foot contact points in portable MSL 4.1.0.",
}
RESULT.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(json.dumps(summary, ensure_ascii=False))
