#!/usr/bin/env python3
"""Build the second P5 conformance robot: a fixed-base four-axis inspection arm."""
from __future__ import annotations

import json
from pathlib import Path

OUT = Path(__file__).resolve().parent
MODEL = "aidc.reference.arm5"
REVISION = "p5-r1"
JOINTS = ["base_yaw", "shoulder_pitch", "elbow_pitch", "wrist_roll"]
BODIES = ["pedestal", "shoulder", "upper_arm", "forearm", "tool"]


def write(name: str, value: dict) -> None:
    (OUT / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


coordinate = {"lengthUnit": "m", "angleUnit": "rad", "forward": "+X", "left": "+Y", "up": "+Z", "geometryQuaternionOrder": "xyzw"}
bodies = []
for index, body_id in enumerate(BODIES):
    body = {
        "bodyId": body_id,
        "translation_m": [0, 0, 0], "rotation_xyzw": [0, 0, 0, 1],
        "mass_kg": [4.0, 2.0, 1.6, 1.1, 0.6][index],
        "centerOfMass_m": [0.08 if index else 0, 0, 0],
        "inertia_kg_m2": {"ixx": 0.02, "iyy": 0.025, "izz": 0.018, "ixy": 0, "ixz": 0, "iyz": 0},
        "collisionAssetIds": [],
    }
    if index: body["parentBodyId"] = BODIES[index - 1]
    bodies.append(body)

joints = []
for index, joint_id in enumerate(JOINTS):
    joints.append({
        "jointId": joint_id, "type": "revolute", "parentBodyId": BODIES[index], "childBodyId": BODIES[index + 1],
        "translation_m": [0, 0, 0.18 if index == 0 else 0], "rotation_xyzw": [0, 0, 0, 1],
        "axis": [0, 0, 1] if index in (0, 3) else [0, 1, 0], "zeroOffset_rad": 0, "initialPosition_rad": 0,
        "limit": {"lower": -2.5, "upper": 2.5, "velocity": 3.0, "effort": 20.0},
        "damping_Nm_s_rad": 0.05, "friction_Nm": 0.01, "armature_kg_m2": 0.001,
    })

physics = {
    "schemaVersion": "aidc.robot.physics/0.1", "modelId": MODEL, "modelRevision": REVISION,
    "coordinateSystem": coordinate, "rootBodyId": "pedestal", "rootType": "fixed",
    "bodies": bodies, "joints": joints,
    "namedPoses": [
        {"name": "home", "jointPositions": {joint: 0 for joint in JOINTS}},
        {"name": "inspection", "jointPositions": {"base_yaw": 0.4, "shoulder_pitch": -0.5, "elbow_pitch": 0.8, "wrist_roll": 0.2}},
    ],
    "contactGeometries": [], "contactDefaults": {"friction": 0.7, "contactMargin_m": 0.001, "excludedBodyPairs": []},
}

devices = [{"deviceId": "dc_bus", "deviceType": "power", "model": "AIDCRobotics.Electrical.BatteryBus", "nominalVoltage_V": 48, "capacity_Ah": 10, "internalResistance_Ohm": 0.08, "currentLimit_A": 40}]
for joint in JOINTS:
    devices.append({"deviceId": f"actuator-{joint}", "deviceType": "actuator", "model": "AIDCRobotics.Actuators.IdealTorque", "jointId": joint, "commandKind": "torque", "gearRatio": 80, "rotorInertia_kg_m2": 0.001, "viscousDamping_Nm_s_rad": 0.002, "effortLimit_Nm": 20, "speedLimit_rad_s": 3, "powerDeviceId": "dc_bus"})

def channel(name: str, index: int, semantic: str, unit: str, joint: str, device: bool = False) -> dict:
    value = {"name": name, "index": index, "semantic": semantic, "unit": unit, "direction": 1, "scale": 1, "samplePeriod_s": 0.001, "jointId": joint}
    if device: value["deviceId"] = f"actuator-{joint}"
    return value

interfaces = {
    "schemaVersion": "aidc.robot.interfaces/0.1", "modelId": MODEL, "modelRevision": REVISION,
    "interfaceId": "aidc.reference.arm5.control", "interfaceVersion": "1.0.0", "publicQuaternionOrder": "wxyz",
    "commands": [channel(f"{joint}TorqueCommand", index, "jointTorqueCommand", "N.m", joint, True) for index, joint in enumerate(JOINTS, 1)],
    "states": [channel(f"{joint}Position", index, "jointPosition", "rad", joint) for index, joint in enumerate(JOINTS, 1)],
    "diagnostics": [{"name": "modelValid", "index": 1, "semantic": "valid", "unit": "1", "direction": 1, "scale": 1, "samplePeriod_s": 0}],
}

write("physics.json", physics); write("devices.json", {"schemaVersion": "aidc.robot.devices/0.1", "modelId": MODEL, "modelRevision": REVISION, "devices": devices}); write("interfaces.json", interfaces)
