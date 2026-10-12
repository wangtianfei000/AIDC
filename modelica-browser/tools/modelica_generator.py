#!/usr/bin/env python3
"""P2 Robot IR to Modelica vertical-slice generator."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import shutil
import sys
from pathlib import Path
from typing import Any

HERE = Path(__file__).resolve()
MODEL_BROWSER = HERE.parents[1]
AIDC_ROOT = MODEL_BROWSER.parent
ROBOT_IR_PACKAGE = MODEL_BROWSER / "packages" / "robot-ir"
AIDC_ROBOTICS = MODEL_BROWSER / "libraries" / "AIDCRobotics"
sys.path.insert(0, str(ROBOT_IR_PACKAGE / "src"))
sys.path.insert(0, str(MODEL_BROWSER / "tools"))

from robot_ir_validator import validate_bundle
from project_tool import normalized_text_digest, source_fingerprint, write_json

GENERATOR_VERSION = "0.3.1"
TEMPLATE_VERSION = "p2-v1"
TREE_TEMPLATE_VERSION = "p3-v2"
OVERRIDE_SCHEMA = "aidc.modelica.overrides/0.1"


class GenerationError(RuntimeError):
    def __init__(self, message: str, document: str, pointer: str):
        super().__init__(message)
        self.document = document
        self.pointer = pointer


def read_json(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
    except Exception as exc:
        raise GenerationError(f"Cannot read JSON: {exc}", path.name, "/") from exc


def number(value: float) -> str:
    if not math.isfinite(float(value)):
        raise ValueError("Modelica literals must be finite")
    return format(float(value), ".17g")


def vector(values: list[float]) -> str:
    return "{" + ", ".join(number(value) for value in values) + "}"


def modelica_name(model_id: str) -> str:
    tokens = re.findall(r"[A-Za-z0-9]+", model_id)
    name = "".join(token[:1].upper() + token[1:] for token in tokens)
    if not name or name[0].isdigit():
        name = "Robot" + name
    return name


def axis_angle(quaternion: list[float]) -> tuple[list[float], float]:
    x, y, z, w = quaternion
    w = max(-1.0, min(1.0, w))
    angle = 2 * math.acos(w)
    scale = math.sqrt(max(0.0, 1 - w * w))
    return ([0.0, 0.0, 1.0], 0.0) if scale < 1e-12 else ([x / scale, y / scale, z / scale], angle)


def sha(path: Path) -> str:
    try:
        digest = normalized_text_digest(path)
    except UnicodeDecodeError:
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
    return "sha256:" + digest


def load_bundle(source: Path, profile: str) -> dict[str, Any]:
    validate_bundle(source)
    physics = read_json(source / "physics.json")
    devices = read_json(source / "devices.json")
    interfaces = read_json(source / "interfaces.json")
    bodies = {body["bodyId"]: body for body in physics["bodies"]}
    actuators = [device for device in devices["devices"] if device["deviceType"] == "actuator"]
    if profile == "p2":
        if len(physics["joints"]) != 1:
            raise GenerationError("P2 requires exactly one joint", "physics.json", "/joints")
        joint = physics["joints"][0]
        if joint["type"] != "revolute":
            raise GenerationError("P2 requires a revolute joint", "physics.json", "/joints/0/type")
        if len(bodies) != 2:
            raise GenerationError("P2 requires one fixed root and one moving rigid body", "physics.json", "/bodies")
        if len(actuators) != 1:
            raise GenerationError("P2 requires exactly one actuator", "devices.json", "/devices")
        if actuators[0]["jointId"] != joint["jointId"]:
            raise GenerationError("Actuator must drive the generated joint", "devices.json", "/devices")
    else:
        joint = physics["joints"][0] if physics["joints"] else None
        unsupported = [(index, item["type"]) for index, item in enumerate(physics["joints"]) if item["type"] not in {"fixed", "revolute", "prismatic"}]
        if unsupported:
            raise GenerationError(f"Unsupported joint type: {unsupported[0][1]}", "physics.json", f"/joints/{unsupported[0][0]}/type")
    return {
        "physics": physics,
        "devices": devices,
        "interfaces": interfaces,
        "joint": joint,
        "root": bodies[physics["rootBodyId"]],
        "body": bodies[joint["childBodyId"]] if joint else bodies[physics["rootBodyId"]],
        "actuator": actuators[0] if actuators else None,
        "bodies": bodies,
        "actuators": {item["jointId"]: item for item in actuators},
    }


def default_overrides(profile: str = "p2") -> dict[str, Any]:
    return {
        "schemaVersion": OVERRIDE_SCHEMA,
        "commandStepTorque_Nm": 0.1,
        "commandStartTime_s": 0.1,
        "stopTime_s": 2.0 if profile == "p2" else 0.02,
        "outputInterval_s": 0.001,
        "relativeTolerance": 1e-8,
    }


def validate_overrides(value: dict[str, Any]) -> None:
    allowed = set(default_overrides())
    if value.get("schemaVersion") != OVERRIDE_SCHEMA:
        raise GenerationError("Unsupported override schema", "user-overrides.json", "/schemaVersion")
    extra = set(value) - allowed
    if extra:
        raise GenerationError(f"Unknown override field: {sorted(extra)[0]}", "user-overrides.json", "/" + sorted(extra)[0])
    for name in allowed - {"schemaVersion"}:
        if name not in value or not isinstance(value[name], (int, float)) or not math.isfinite(float(value[name])):
            raise GenerationError("Override must be a finite number", "user-overrides.json", "/" + name)
    if value["stopTime_s"] <= 0 or value["outputInterval_s"] <= 0 or value["relativeTolerance"] <= 0:
        raise GenerationError("Simulation settings must be positive", "user-overrides.json", "/stopTime_s")


def render_package(package: str, bundle: dict[str, Any], overrides: dict[str, Any]) -> str:
    physics, joint, body, actuator = bundle["physics"], bundle["joint"], bundle["body"], bundle["actuator"]
    inertia = body["inertia_kg_m2"]
    rotation_axis, rotation_angle = axis_angle(joint["rotation_xyzw"])
    return f'''within ;
package {package} "Generated from Robot IR {physics["modelId"]}@{physics["modelRevision"]}"
  extends Modelica.Icons.Package;

  record SourceData "Generated parameters; edit Robot IR instead of this record"
    constant String modelId = "{physics["modelId"]}";
    constant String modelRevision = "{physics["modelRevision"]}";
    parameter Modelica.Units.SI.Mass bodyMass = {number(body["mass_kg"])};
    parameter Modelica.Units.SI.Position centerOfMass[3] = {vector(body["centerOfMass_m"])};
    parameter Modelica.Units.SI.Inertia Ixx = {number(inertia["ixx"])};
    parameter Modelica.Units.SI.Inertia Iyy = {number(inertia["iyy"])};
    parameter Modelica.Units.SI.Inertia Izz = {number(inertia["izz"])};
    parameter Modelica.Units.SI.RotationalDampingConstant damping = {number(joint.get("damping_Nm_s_rad", 0))};
    parameter Modelica.Units.SI.Torque friction = {number(joint.get("friction_Nm", 0))};
    parameter Modelica.Units.SI.Angle lowerLimit = {number(joint["limit"]["lower"])};
    parameter Modelica.Units.SI.Angle upperLimit = {number(joint["limit"]["upper"])};
  end SourceData;

  record UserOverrides "Values preserved outside generated sources"
    parameter Modelica.Units.SI.Torque commandStepTorque = {number(overrides["commandStepTorque_Nm"])};
    parameter Modelica.Units.SI.Time commandStartTime = {number(overrides["commandStartTime_s"])};
  end UserOverrides;

  model Robot "One rigid body, one revolute joint and one standard actuator"
    parameter SourceData data;
    parameter UserOverrides user;
    inner Modelica.Mechanics.MultiBody.World world(enableAnimation = false,
      gravityType = Modelica.Mechanics.MultiBody.Types.GravityTypes.NoGravity);
    AIDCRobotics.Mechanics.FixedBase3D base
      annotation(Placement(transformation(extent = {{{{-90, -10}}, {{-70, 10}}}})));
    AIDCRobotics.Mechanics.FixedJoint3D jointOrigin(
      translation = {vector(joint["translation_m"])}, rotationAxis = {vector(rotation_axis)}, rotationAngle = {number(rotation_angle)})
      annotation(Placement(transformation(extent = {{{{-60, -10}}, {{-40, 10}}}})));
    AIDCRobotics.Mechanics.RevoluteJoint3D revolute(
      axis = {vector(joint["axis"])}, initialPosition = {number(joint["initialPosition_rad"] + joint["zeroOffset_rad"])})
      annotation(Placement(transformation(extent = {{{{-20, -10}}, {{0, 10}}}})));
    AIDCRobotics.Mechanics.RigidBody3D link(
      mass = data.bodyMass, centerOfMass = data.centerOfMass,
      I_11 = data.Ixx, I_22 = data.Iyy, I_33 = data.Izz,
      I_21 = {number(inertia["ixy"])}, I_31 = {number(inertia["ixz"])}, I_32 = {number(inertia["iyz"])})
      annotation(Placement(transformation(extent = {{{{40, -10}}, {{60, 10}}}})));
    AIDCRobotics.Actuators.IdealTorque actuator
      annotation(Placement(transformation(extent = {{{{-20, 40}}, {{0, 60}}}})));
    AIDCRobotics.Mechanics.RotationalJointLoss jointLoss(
      damping = data.damping, coulombFriction = data.friction,
      lowerLimit = data.lowerLimit, upperLimit = data.upperLimit)
      annotation(Placement(transformation(extent = {{{{20, 40}}, {{40, 60}}}})));
    Modelica.Blocks.Interfaces.RealInput torqueCommand(unit = "N.m")
      annotation(Placement(transformation(extent = {{{{-120, 40}}, {{-80, 80}}}})));
    output Modelica.Units.SI.Angle jointPosition = revolute.joint.phi;
    output Modelica.Units.SI.AngularVelocity jointVelocity = der(revolute.joint.phi);
    output Modelica.Units.SI.Torque appliedTorque = torqueCommand;
  equation
    connect(base.frame_b, jointOrigin.frame_a);
    connect(jointOrigin.frame_b, revolute.frame_a);
    connect(revolute.frame_b, link.frame_a);
    connect(torqueCommand, actuator.command);
    connect(actuator.flange, revolute.drive);
    connect(revolute.drive, jointLoss.flange_a);
    connect(revolute.support, jointLoss.flange_b);
    annotation(
      Icon(graphics = {{Rectangle(extent = {{{{-90, 60}}, {{90, -60}}}}, radius = 12, lineColor = {{0, 120, 105}}, fillColor = {{230, 247, 244}}, fillPattern = FillPattern.Solid), Ellipse(extent = {{{{-20, 20}}, {{20, -20}}}}, lineColor = {{0, 90, 100}}), Text(extent = {{{{-80, 52}}, {{80, 28}}}}, textString = "{joint["jointId"]}")}}),
      Diagram(coordinateSystem(extent = {{{{-100, -100}}, {{100, 100}}}})));
  end Robot;

  model Experiment "Generated reproducible experiment"
    parameter UserOverrides user;
    Modelica.Blocks.Sources.Step command(height = user.commandStepTorque, startTime = user.commandStartTime);
    Robot robot(user = user);
    output Modelica.Units.SI.Angle jointPosition = robot.jointPosition;
    output Modelica.Units.SI.AngularVelocity jointVelocity = robot.jointVelocity;
    output Modelica.Units.SI.Torque appliedTorque = robot.appliedTorque;
  equation
    connect(command.y, robot.torqueCommand)
      annotation(Line(points = {{{{-59, 0}}, {{-20, 0}}}}, color = {{0, 0, 127}}));
    annotation(experiment(StartTime = 0, StopTime = {number(overrides["stopTime_s"])}, Interval = {number(overrides["outputInterval_s"])}, Tolerance = {number(overrides["relativeTolerance"])}));
  end Experiment;

  annotation(uses(Modelica(version = "4.1.0"), AIDCRobotics(version = "0.3.0")), version = "0.1.0");
end {package};
'''


def instance_name(identifier: str, prefix: str) -> str:
    tokens = re.findall(r"[A-Za-z0-9]+", identifier)
    suffix = "".join(token[:1].upper() + token[1:] for token in tokens)
    return prefix + (suffix or "Item")


def tree_layout(physics: dict[str, Any]) -> dict[str, tuple[float, float]]:
    children: dict[str, list[str]] = {body["bodyId"]: [] for body in physics["bodies"]}
    for joint in physics["joints"]:
        children[joint["parentBodyId"]].append(joint["childBodyId"])
    root = physics["rootBodyId"]
    first = children[root]
    # Keep every first-level branch inside the generated diagram and give the
    # mechanical chain enough horizontal room for mount + joint + body icons.
    if len(first) <= 1:
        lanes = {child: 0.0 for child in first}
    else:
        lanes = {child: 80.0 - index * 160.0 / (len(first) - 1) for index, child in enumerate(first)}
    result = {root: (-64.0, 0.0)}

    def visit(body_id: str, depth: int, lane: float) -> None:
        result[body_id] = (-64.0 + depth * 42.0, lane)
        for child in children[body_id]:
            visit(child, depth + 1, lane)

    for child in first:
        visit(child, 1, lanes[child])
    return result


def connect_line(left: str, right: str, points: list[tuple[float, float]], color: tuple[int, int, int] = (95, 115, 112)) -> str:
    rendered = ", ".join("{" + number(x) + ", " + number(y) + "}" for x, y in points)
    return f"    connect({left}, {right}) annotation(Line(points = {{{rendered}}}, color = {{{color[0]}, {color[1]}, {color[2]}}}));"


def orthogonal_route(start: tuple[float, float], end: tuple[float, float]) -> list[tuple[float, float]]:
    """Route a branch without drawing a diagonal through component icons."""
    if start[0] == end[0] or start[1] == end[1]:
        return [start, end]
    return [start, (end[0], start[1]), end]


def generated_structure(bundle: dict[str, Any]) -> dict[str, Any]:
    physics = bundle["physics"]
    layout = tree_layout(physics)
    movable = [joint for joint in physics["joints"] if joint["type"] != "fixed"]
    named_poses = physics.get("namedPoses") or [{
        "name": "initial",
        "jointPositions": {joint["jointId"]: joint["initialPosition_rad"] + joint.get("zeroOffset_rad", 0) for joint in movable},
    }]
    return {
        "schemaVersion": "aidc.modelica.generated-structure/0.1",
        "modelId": physics["modelId"],
        "modelRevision": physics["modelRevision"],
        "rootBodyId": physics["rootBodyId"],
        "rootType": physics.get("rootType", "fixed"),
        "rootInitialTranslation_m": physics.get("rootInitialTranslation_m", [0, 0, 0]),
        "rootInitialRotation_xyzw": physics.get("rootInitialRotation_xyzw", [0, 0, 0, 1]),
        "totalMass_kg": sum(body["mass_kg"] for body in physics["bodies"]),
        "bodies": [
            {
                **body,
                "component": instance_name(body["bodyId"], "body"),
                "diagramPosition": list(layout[body["bodyId"]]),
            }
            for body in physics["bodies"]
        ],
        "joints": [
            {
                **joint,
                "mountComponent": instance_name(joint["jointId"], "mount"),
                "jointComponent": instance_name(joint["jointId"], "joint"),
            }
            for joint in physics["joints"]
        ],
        "actuators": [
            {**actuator, "component": instance_name(actuator["jointId"], "actuator")}
            for actuator in bundle["actuators"].values()
        ],
        "namedPoses": named_poses,
        "contactGeometries": physics.get("contactGeometries", []),
        "geometryAssets": physics.get("geometryAssets", []),
    }


def render_tree_package(package: str, bundle: dict[str, Any], overrides: dict[str, Any]) -> str:
    physics = bundle["physics"]
    bodies = physics["bodies"]
    joints = physics["joints"]
    body_map = bundle["bodies"]
    actuator_map = bundle["actuators"]
    power_devices = [item for item in bundle["devices"]["devices"] if item["deviceType"] == "power"]
    battery = power_devices[0] if len(power_devices) == 1 else None
    asset_map = {item["assetId"]: item for item in physics.get("geometryAssets", [])}
    moving = [joint for joint in joints if joint["type"] != "fixed"]
    named_poses = physics.get("namedPoses") or [{
        "name": "initial",
        "jointPositions": {joint["jointId"]: joint["initialPosition_rad"] + joint.get("zeroOffset_rad", 0) for joint in moving},
    }]
    moving_index = {joint["jointId"]: index for index, joint in enumerate(moving, 1)}
    layout = tree_layout(physics)
    declarations: list[str] = []
    equations: list[str] = []
    root_id = physics["rootBodyId"]
    root_x, root_y = layout[root_id]
    root_axis, root_angle = axis_angle(physics.get("rootInitialRotation_xyzw", [0, 0, 0, 1]))
    declarations.append(
        f"    AIDCRobotics.Mechanics.FixedJoint3D rootPose(translation = {{0, 0, 0}}, rotationAxis = {vector(root_axis)}, rotationAngle = {number(root_angle)}) "
        f"annotation(Placement(transformation(extent = {{{{-86, -5}}, {{-76, 5}}}})));"
    )
    if physics.get("rootType", "fixed") == "floating":
        root_translation = physics.get("rootInitialTranslation_m", [0, 0, 0])
        declarations.append(f"    AIDCRobotics.Mechanics.FloatingBase3D rootBase(r_start = {vector(root_translation)}) annotation(Placement(transformation(extent = {{{{-104, -8}}, {{-88, 8}}}})));" )
        equations.append(connect_line("world.frame_b", "rootBase.frame_a", [(-116, 0), (-104, 0)]))
        equations.append(connect_line("rootBase.frame_b", "rootPose.frame_a", [(-88, 0), (-86, 0)]))
        equations.append(connect_line("rootPose.frame_b", f"{instance_name(root_id, 'body')}.frame_a", [(-76, 0), (root_x - 8, root_y)]))
    else:
        root_translation = physics.get("rootInitialTranslation_m", [0, 0, 0])
        declarations.append(f"    AIDCRobotics.Mechanics.FixedBase3D rootBase(origin = {vector(root_translation)}) annotation(Placement(transformation(extent = {{{{-104, -8}}, {{-88, 8}}}})));" )
        equations.append(connect_line("rootBase.frame_b", "rootPose.frame_a", [(-88, 0), (-86, 0)]))
        equations.append(connect_line("rootPose.frame_b", f"{instance_name(root_id, 'body')}.frame_a", [(-76, 0), (root_x - 8, root_y)]))

    for body_index, body in enumerate(bodies, 1):
        tensor = body["inertia_kg_m2"]
        x, y = layout[body["bodyId"]]
        name = instance_name(body["bodyId"], "body")
        declarations.append(
            f"    AIDCRobotics.Mechanics.RigidBody3D {name}(mass = {number(body['mass_kg'])}, centerOfMass = {vector(body['centerOfMass_m'])}, "
            f"I_11 = {number(tensor['ixx'])}, I_22 = {number(tensor['iyy'])}, I_33 = {number(tensor['izz'])}, "
            f"I_21 = {number(tensor['ixy'])}, I_31 = {number(tensor['ixz'])}, I_32 = {number(tensor['iyz'])}) "
            f"annotation(Placement(transformation(extent = {{{{{number(x-8)}, {number(y-8)}}}, {{{number(x+8)}, {number(y+8)}}}}})));"
        )
        visual_asset = asset_map.get(body.get("visualAssetId"))
        if visual_asset:
            visual_name = instance_name(body["bodyId"], "visual")
            visual_file = Path(visual_asset["path"]).name
            declarations.append(
                f'    Modelica.Mechanics.MultiBody.Visualizers.FixedShape {visual_name}('
                f'shapeType = "modelica://{package}/Resources/Geometry/{visual_file}", length = 1, width = 1, height = 1, extra = 0) '
                # FixedShape is a 3D rendering helper.  Keeping its placement
                # hidden prevents a duplicate CAD node and long URI label from
                # cluttering the component connection diagram.
                f'annotation(Placement(visible = false, transformation(extent = {{{{{number(x-8)}, {number(y-8)}}}, {{{number(x+8)}, {number(y+8)}}}}})));'
            )
            equations.append(connect_line(f"{name}.frame_b", f"{visual_name}.frame_a", [(x + 8, y), (x + 8, y - 10), (x, y - 10)]))
        for axis in range(1, 4):
            equations.append(f"    bodyPosition[{body_index}, {axis}] = {name}.frame_a.r_0[{axis}];")

    for joint in joints:
        child_x, child_y = layout[joint["childBodyId"]]
        parent_x, parent_y = layout[joint["parentBodyId"]]
        mount_x, mount_y = (parent_x + child_x) / 2 - 7, child_y
        joint_x = mount_x + 14
        axis, angle = axis_angle(joint["rotation_xyzw"])
        mount = instance_name(joint["jointId"], "mount")
        joint_name = instance_name(joint["jointId"], "joint")
        declarations.append(
            f"    AIDCRobotics.Mechanics.FixedJoint3D {mount}(translation = {vector(joint['translation_m'])}, rotationAxis = {vector(axis)}, rotationAngle = {number(angle)}) "
            f"annotation(Placement(transformation(extent = {{{{{number(mount_x-5)}, {number(mount_y-5)}}}, {{{number(mount_x+5)}, {number(mount_y+5)}}}}})));"
        )
        parent = instance_name(joint["parentBodyId"], "body")
        child = instance_name(joint["childBodyId"], "body")
        parent_port = (parent_x + 8, parent_y)
        mount_port = (mount_x - 5, mount_y)
        equations.append(connect_line(f"{parent}.frame_b", f"{mount}.frame_a", orthogonal_route(parent_port, mount_port)))
        if joint["type"] == "fixed":
            declarations.append(f"    AIDCRobotics.Mechanics.FixedJoint3D {joint_name} annotation(Placement(transformation(extent = {{{{{number(joint_x-5)}, {number(child_y-5)}}}, {{{number(joint_x+5)}, {number(child_y+5)}}}}})));" )
            equations.append(connect_line(f"{mount}.frame_b", f"{joint_name}.frame_a", [(mount_x + 5, mount_y), (joint_x - 5, child_y)]))
            equations.append(connect_line(f"{joint_name}.frame_b", f"{child}.frame_a", [(joint_x + 5, child_y), (child_x - 8, child_y)]))
            continue
        index = moving_index[joint["jointId"]]
        initial = joint["initialPosition_rad"] + joint.get("zeroOffset_rad", 0)
        if joint["type"] == "revolute":
            declarations.append(
                f"    AIDCRobotics.Mechanics.RevoluteJoint3D {joint_name}(axis = {vector(joint['axis'])}, initialPosition = {number(initial)}) "
                f"annotation(Placement(transformation(extent = {{{{{number(joint_x-5)}, {number(child_y-5)}}}, {{{number(joint_x+5)}, {number(child_y+5)}}}}})));"
            )
            actuator = instance_name(joint["jointId"], "actuator")
            loss = instance_name(joint["jointId"], "loss")
            declarations.append(f"    AIDCRobotics.Actuators.IdealTorque {actuator};")
            declarations.append(
                f"    AIDCRobotics.Mechanics.RotationalJointLoss {loss}(damping = {number(joint.get('damping_Nm_s_rad', 0))}, coulombFriction = {number(joint.get('friction_Nm', 0))}, "
                f"lowerLimit = {number(joint['limit']['lower'])}, upperLimit = {number(joint['limit']['upper'])});"
            )
            effort_limit = actuator_map.get(joint["jointId"], {}).get("effortLimit_Nm", joint["limit"]["effort"])
            equations.extend([
                connect_line(f"{mount}.frame_b", f"{joint_name}.frame_a", [(mount_x + 5, mount_y), (joint_x - 5, child_y)]),
                connect_line(f"{joint_name}.frame_b", f"{child}.frame_a", [(joint_x + 5, child_y), (child_x - 8, child_y)]),
                f"    rawTorque[{index}] = max(-{number(effort_limit)}, min({number(effort_limit)}, jointTorqueCommand[{index}]));",
                f"    {actuator}.command = actuatorTorque[{index}];",
                f"    connect({actuator}.flange, {joint_name}.drive);",
                f"    connect({joint_name}.drive, {loss}.flange_a);",
                f"    connect({joint_name}.support, {loss}.flange_b);",
                f"    jointPosition[{index}] = {joint_name}.joint.phi;",
                f"    jointVelocity[{index}] = der({joint_name}.joint.phi);",
                f"    appliedTorque[{index}] = actuatorTorque[{index}];",
            ])
        else:
            declarations.append(
                f"    AIDCRobotics.Mechanics.PrismaticJoint3D {joint_name}(axis = {vector(joint['axis'])}, initialPosition = {number(initial)}) "
                f"annotation(Placement(transformation(extent = {{{{{number(joint_x-5)}, {number(child_y-5)}}}, {{{number(joint_x+5)}, {number(child_y+5)}}}}})));"
            )
            force = instance_name(joint["jointId"], "force")
            declarations.append(f"    Modelica.Mechanics.Translational.Sources.ConstantForce {force}(f_constant = 0);")
            equations.extend([
                connect_line(f"{mount}.frame_b", f"{joint_name}.frame_a", [(mount_x + 5, mount_y), (joint_x - 5, child_y)]),
                connect_line(f"{joint_name}.frame_b", f"{child}.frame_a", [(joint_x + 5, child_y), (child_x - 8, child_y)]),
                f"    connect({force}.flange, {joint_name}.drive);",
                f"    jointPosition[{index}] = {joint_name}.joint.s;",
                f"    jointVelocity[{index}] = der({joint_name}.joint.s);",
                f"    appliedTorque[{index}] = 0;",
                f"    rawTorque[{index}] = 0;",
            ])

    contact_defaults = physics.get("contactDefaults", {})
    friction = contact_defaults.get("friction", 0.8)
    for contact in physics.get("contactGeometries", []):
        contact_id = contact["contactId"]
        body_name = instance_name(contact["bodyId"], "body")
        prefix = instance_name(contact_id, "contact")
        mount = prefix + "Mount"
        position = prefix + "Position"
        velocity = prefix + "Velocity"
        force = prefix + "Force"
        law = prefix + "Law"
        rotation_axis, rotation_angle = axis_angle(contact["rotation_xyzw"])
        declarations.extend([
            f"    AIDCRobotics.Mechanics.FixedJoint3D {mount}(translation = {vector(contact['translation_m'])}, rotationAxis = {vector(rotation_axis)}, rotationAngle = {number(rotation_angle)});",
            f"    Modelica.Mechanics.MultiBody.Sensors.AbsolutePosition {position}(resolveInFrame = Modelica.Mechanics.MultiBody.Types.ResolveInFrameA.world);",
            f"    Modelica.Mechanics.MultiBody.Sensors.AbsoluteVelocity {velocity}(resolveInFrame = Modelica.Mechanics.MultiBody.Types.ResolveInFrameA.world);",
            f"    Modelica.Mechanics.MultiBody.Forces.WorldForce {force}(animation = false);",
        ])
        common = f"frictionCoefficient = {number(friction)}"
        if contact["shape"] == "point":
            declaration = f"    AIDCRobotics.Contacts.PointPlaneContact {law}({common});"
        elif contact["shape"] == "sphere":
            declaration = f"    AIDCRobotics.Contacts.SpherePlaneContact {law}(radius = {number(contact['radius_m'])}, {common});"
        elif contact["shape"] == "box":
            declaration = f"    AIDCRobotics.Contacts.BoxPlaneContact {law}(halfExtents = {vector(contact['halfExtents_m'])}, {common});"
        elif contact["shape"] == "capsule":
            declaration = f"    AIDCRobotics.Contacts.CapsulePlaneContact {law}(radius = {number(contact['radius_m'])}, halfLength = {number(contact['halfLength_m'])}, capsuleAxis = {vector(contact['axis'])}, {common});"
        elif contact["shape"] == "footPatch":
            offsets = "{" + ", ".join(vector(point) for point in contact["points_m"]) + "}"
            declaration = f"    AIDCRobotics.Contacts.FootPatchPlaneContact {law}(offsets = {offsets}, {common});"
        else:
            raise GenerationError(f"Dynamic contact generation does not support {contact['shape']}", "physics.json", "/contactGeometries")
        declarations.append(declaration)
        equations.extend([
            f"    connect({body_name}.frame_b, {mount}.frame_a);",
            f"    connect({mount}.frame_b, {position}.frame_a);",
            f"    connect({mount}.frame_b, {velocity}.frame_a);",
            f"    connect({mount}.frame_b, {force}.frame_b);",
            f"    {law}.position = {position}.r;",
            f"    {law}.velocity = {velocity}.v;",
            f"    {force}.force = {law}.force;",
        ])

    if battery:
        battery_name = instance_name(battery["deviceId"], "power")
        declarations.append(
            f"    AIDCRobotics.Electrical.SimpleBatteryBus {battery_name}(nominalVoltage = {number(battery['nominalVoltage_V'])}, "
            f"internalResistance = {number(battery['internalResistance_Ohm'])}, capacity_Ah = {number(battery['capacity_Ah'])}, maximumCurrent = {number(battery['currentLimit_A'])});"
        )
        gains = []
        for joint in moving:
            actuator = actuator_map.get(joint["jointId"], {})
            effort = actuator.get("effortLimit_Nm", joint["limit"]["effort"])
            gains.append(battery["currentLimit_A"] / effort if actuator.get("powerDeviceId") == battery["deviceId"] else 0)
        declarations.extend([
            f"    final parameter Real currentPerTorque[movableJointCount](each unit = \"A/N.m\") = {vector(gains)};",
            "    Real requestedBusCurrent(unit = \"A\");",
            "    Real powerScale(min = 0, max = 1);",
        ])
        equations.extend([
            "    requestedBusCurrent = sum(abs(rawTorque[i]) * currentPerTorque[i] for i in 1:movableJointCount);",
            f"    {battery_name}.requestedCurrent = requestedBusCurrent;",
            f"    powerScale = if requestedBusCurrent > Modelica.Constants.small then {battery_name}.deliveredCurrent / requestedBusCurrent else 1;",
            "    actuatorTorque = powerScale * rawTorque;",
            f"    batteryVoltage = {battery_name}.terminalVoltage;",
            f"    batteryStateOfCharge = {battery_name}.stateOfCharge;",
        ])
    else:
        equations.extend([
            "    actuatorTorque = rawTorque;",
            "    batteryVoltage = 0;",
            "    batteryStateOfCharge = 1;",
        ])

    total_mass = sum(body["mass_kg"] for body in bodies)
    pose_matrix = ("{" + ", ".join(
        "{" + ", ".join(number(pose["jointPositions"][joint["jointId"]]) for joint in moving) + "}"
        for pose in named_poses
    ) + "}") if moving else "fill(0.0, namedPoseCount, movableJointCount)"
    joint_ids = "{" + ", ".join(json.dumps(item["jointId"]) for item in joints) + "}" if joints else 'fill("", 0)'
    declarations_text = "\n".join(declarations)
    equations_text = "\n".join(equations)
    return f'''within ;
package {package} "Generated tree robot {physics["modelId"]}@{physics["modelRevision"]}"
  extends Modelica.Icons.Package;
  constant Integer bodyCount = {len(bodies)};
  constant Integer jointCount = {len(joints)};
  constant Integer movableJointCount = {len(moving)};
  constant Integer namedPoseCount = {len(named_poses)};
  constant Modelica.Units.SI.Mass totalMass = {number(total_mass)};
  constant String bodyIds[bodyCount] = {{{", ".join(json.dumps(item["bodyId"]) for item in bodies)}}};
  constant String jointIds[jointCount] = {joint_ids};
  constant String namedPoseNames[namedPoseCount] = {{{", ".join(json.dumps(item["name"]) for item in named_poses)}}};
  constant Real namedPoseJointPosition[namedPoseCount, movableJointCount] = {pose_matrix};

  record UserOverrides
    parameter Modelica.Units.SI.Time stopTime = {number(overrides["stopTime_s"])};
  end UserOverrides;

  model Robot "Generated fixed/floating tree of standard components"
    inner Modelica.Mechanics.MultiBody.World world(enableAnimation = true,
      gravityType = Modelica.Mechanics.MultiBody.Types.GravityTypes.UniformGravity,
      g = 9.81, n = {{0, 0, -1}});
{declarations_text}
    Modelica.Blocks.Interfaces.RealInput jointTorqueCommand[movableJointCount](each unit = "N.m");
    Real rawTorque[movableJointCount](each unit = "N.m");
    Real actuatorTorque[movableJointCount](each unit = "N.m");
    output Modelica.Units.SI.Position bodyPosition[bodyCount, 3];
    output Real jointPosition[movableJointCount](each unit = "rad");
    output Real jointVelocity[movableJointCount](each unit = "rad/s");
    output Real appliedTorque[movableJointCount](each unit = "N.m");
    output Modelica.Units.SI.Voltage batteryVoltage;
    output Real batteryStateOfCharge(unit = "1");
  equation
{equations_text}
    annotation(
      Icon(graphics = {{Rectangle(extent = {{{{-94, 72}}, {{94, -72}}}}, radius = 12, lineColor = {{0, 110, 100}}, fillColor = {{231, 247, 244}}, fillPattern = FillPattern.Solid), Line(points = {{{{-64, 0}}, {{-20, 0}}, {{20, 46}}, {{64, 46}}}}, color = {{0, 90, 100}}, thickness = 2), Line(points = {{{{-20, 0}}, {{20, -46}}, {{64, -46}}}}, color = {{0, 90, 100}}, thickness = 2), Text(extent = {{{{-82, 66}}, {{82, 42}}}}, textString = "{physics['modelId']}")}}),
      Diagram(coordinateSystem(extent = {{{{-120, -100}}, {{170, 100}}}}, preserveAspectRatio = false)));
  end Robot;

  model Experiment
    parameter UserOverrides user;
    Modelica.Blocks.Sources.Constant commands[movableJointCount](each k = 0);
    Robot robot;
    output Modelica.Units.SI.Position bodyPosition[bodyCount, 3] = robot.bodyPosition;
    output Real jointPosition[movableJointCount](each unit = "rad") = robot.jointPosition;
  equation
    connect(commands.y, robot.jointTorqueCommand);
    annotation(experiment(StartTime = 0, StopTime = {number(overrides["stopTime_s"])}, Interval = {number(overrides["outputInterval_s"])}, Tolerance = {number(overrides["relativeTolerance"])}));
  end Experiment;
  annotation(uses(Modelica(version = "4.1.0"), AIDCRobotics(version = "0.3.0")), version = "0.2.0");
end {package};
'''


def clean_generated(output: Path) -> None:
    for name in ("modelica", "robot-ir", "results"):
        path = output / name
        if path.exists():
            shutil.rmtree(path)
    for name in ("model-project.json", "generation-diagnostics.json", "generation.request.json"):
        (output / name).unlink(missing_ok=True)


def generate(source: Path, output: Path, requested_package: str | None = None, profile: str = "p2") -> dict[str, Any]:
    source, output = source.resolve(), output.resolve()
    if profile not in {"p2", "p3"}:
        raise GenerationError("Profile must be p2 or p3", "command", "/profile")
    bundle = load_bundle(source, profile)
    package = requested_package or modelica_name(bundle["physics"]["modelId"])
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", package):
        raise GenerationError("Invalid Modelica package name", "command", "/package")
    override_path = output / "overrides" / "user-overrides.json"
    overrides = read_json(override_path) if override_path.is_file() else default_overrides(profile)
    validate_overrides(overrides)
    clean_generated(output)
    override_path.parent.mkdir(parents=True, exist_ok=True)
    write_json(override_path, overrides)
    package_file = output / "modelica" / package / "package.mo"
    package_file.parent.mkdir(parents=True, exist_ok=True)
    package_file.write_text((render_package if profile == "p2" else render_tree_package)(package, bundle, overrides), encoding="utf-8", newline="\n")
    copied_ir = output / "robot-ir" / "source"
    copied_ir.mkdir(parents=True, exist_ok=True)
    for name in ("physics.json", "devices.json", "interfaces.json"):
        shutil.copyfile(source / name, copied_ir / name)
    physics = bundle["physics"]
    for asset in physics.get("geometryAssets", []):
        relative = Path(asset["path"])
        source_asset = (source / relative).resolve()
        if not source_asset.is_file() or source.resolve() not in source_asset.parents:
            raise GenerationError(f"Geometry asset does not exist: {asset['path']}", "physics.json", "/geometryAssets")
        ir_asset = copied_ir / relative
        ir_asset.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source_asset, ir_asset)
        resource_asset = package_file.parent / "Resources" / "Geometry" / source_asset.name
        resource_asset.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source_asset, resource_asset)
    structure_file = output / "robot-ir" / "generated-structure.json"
    write_json(structure_file, generated_structure(bundle))
    dependency_file = AIDC_ROBOTICS / "modelica" / "AIDCRobotics" / "package.mo"
    dependency_path = Path(os.path.relpath(dependency_file, output)).as_posix()
    project = {
        "schemaVersion": "2.0",
        "projectId": bundle["physics"]["modelId"] + ".modelica",
        "name": f"{bundle['physics']['modelId']} generated Modelica",
        "revision": bundle["physics"]["modelRevision"] + "+" + profile,
        "description": "Generated P2 vertical slice: one rigid body, one revolute joint and one standard actuator." if profile == "p2" else "Generated P3 fixed/floating tree robot using AIDCRobotics standard components.",
        "modelica": {
            "rootPackageFile": f"modelica/{package}/package.mo", "rootPackage": package,
            "defaultExperiment": f"{package}.Experiment", "languageVersion": "3.6",
            "libraries": [
                {"name": "Modelica", "version": "4.1.0"},
                {"name": "AIDCRobotics", "version": "0.3.0", "path": dependency_path},
            ],
        },
    }
    write_json(output / "model-project.json", project)
    diagnostics = {"schemaVersion": "1.0", "diagnostics": []}
    write_json(output / "generation-diagnostics.json", diagnostics)
    dependency_digest, dependency_files = source_fingerprint(AIDC_ROBOTICS)
    inputs = [
        {"path": f"robot-ir/{name}", "sha256": sha(source / name)} for name in ("physics.json", "devices.json", "interfaces.json")
    ] + [{"path": "overrides/user-overrides.json", "sha256": sha(override_path)}]
    output_files = [output / "model-project.json", package_file, output / "generation-diagnostics.json", structure_file,
                    *sorted(path for path in copied_ir.rglob("*") if path.is_file()),
                    *sorted(path for path in (package_file.parent / "Resources").rglob("*") if path.is_file())]
    outputs = [{"path": path.relative_to(output).as_posix(), "sha256": sha(path)} for path in output_files]
    aggregate = hashlib.sha256()
    for item in inputs + outputs:
        aggregate.update(f"{item['path']}\0{item['sha256']}\n".encode())
    aggregate.update(f"dependency:AIDCRobotics\0{dependency_digest}\n".encode())
    lock = {
        "schemaVersion": "aidc.modelica.generation-lock/0.1",
        "modelId": bundle["physics"]["modelId"], "modelRevision": bundle["physics"]["modelRevision"],
        "generator": {"name": "aidc-modelica-generator", "version": GENERATOR_VERSION, "templateVersion": TEMPLATE_VERSION if profile == "p2" else TREE_TEMPLATE_VERSION},
        "package": package, "inputs": inputs,
        "dependencies": [{"name": "AIDCRobotics", "version": "0.3.0", "sourceDigest": dependency_digest, "sourceFileCount": len(dependency_files)}],
        "outputs": outputs,
        "structure": {"rootType": bundle["physics"].get("rootType", "fixed"), "bodyCount": len(bundle["physics"]["bodies"]), "jointCount": len(bundle["physics"]["joints"]), "actuatorCount": len(bundle["actuators"]), "totalMass_kg": sum(item["mass_kg"] for item in bundle["physics"]["bodies"])},
        "generationDigest": "sha256:" + aggregate.hexdigest(),
    }
    write_json(output / "robot-ir" / "generation.lock.json", lock)
    return lock


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--package")
    parser.add_argument("--profile", choices=("p2", "p3"), default="p2")
    args = parser.parse_args()
    try:
        lock = generate(args.source, args.output, args.package, args.profile)
        print(json.dumps({"passed": True, "output": str(args.output.resolve()), "generationDigest": lock["generationDigest"]}, ensure_ascii=False))
        return 0
    except GenerationError as exc:
        print(json.dumps({"passed": False, "diagnostic": {"message": str(exc), "document": exc.document, "jsonPointer": exc.pointer}}, ensure_ascii=False), file=sys.stderr)
        return 1
    except Exception as exc:
        print(json.dumps({"passed": False, "diagnostic": {"message": str(exc), "document": "generator", "jsonPointer": "/"}}, ensure_ascii=False), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
