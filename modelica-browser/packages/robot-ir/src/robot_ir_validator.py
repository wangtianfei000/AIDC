#!/usr/bin/env python3
"""Schema and cross-document validation for AIDC Robot IR 0.1."""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any

from jsonschema import Draft202012Validator


ROOT = Path(__file__).resolve().parents[1]
SCHEMAS = ROOT / "schemas"


class RobotIRError(RuntimeError):
    pass


def read_json(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
    except Exception as exc:
        raise RobotIRError(f"Cannot read {path}: {exc}") from exc


def validate_schema(document: dict[str, Any], name: str, path: Path) -> None:
    schema = read_json(SCHEMAS / f"{name}.schema.json")
    errors = sorted(Draft202012Validator(schema).iter_errors(document), key=lambda item: list(item.absolute_path))
    if errors:
        details = "\n".join(
            f"{path}#/{'/'.join(map(str, error.absolute_path))}: {error.message}" for error in errors
        )
        raise RobotIRError(details)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RobotIRError(message)


def unique(items: list[dict[str, Any]], key: str, label: str) -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    for item in items:
        value = item[key]
        require(value not in result, f"Duplicate {label}: {value}")
        result[value] = item
    return result


def normalized(values: list[float], label: str, tolerance: float = 1e-8) -> None:
    norm = math.sqrt(sum(value * value for value in values))
    require(abs(norm - 1.0) <= tolerance, f"{label} must be normalized, norm={norm}")


def positive_definite(inertia: dict[str, float], label: str) -> None:
    ixx, iyy, izz = inertia["ixx"], inertia["iyy"], inertia["izz"]
    ixy, ixz, iyz = inertia["ixy"], inertia["ixz"], inertia["iyz"]
    minor2 = ixx * iyy - ixy * ixy
    determinant = ixx * (iyy * izz - iyz * iyz) - ixy * (ixy * izz - iyz * ixz) + ixz * (ixy * iyz - iyy * ixz)
    require(ixx > 0 and minor2 > 0 and determinant > 0, f"{label} inertia tensor must be positive definite")
    require(ixx <= iyy + izz and iyy <= ixx + izz and izz <= ixx + iyy, f"{label} inertia violates triangle inequalities")


def contiguous_channels(channels: list[dict[str, Any]], label: str) -> None:
    indices = [item["index"] for item in channels]
    require(indices == list(range(1, len(indices) + 1)), f"{label} indices must be ordered and contiguous from 1")
    require(len({item["name"] for item in channels}) == len(channels), f"{label} names must be unique")


def validate_bundle(root: Path) -> dict[str, Any]:
    root = root.resolve()
    paths = {name: root / f"{name}.json" for name in ("physics", "devices", "interfaces")}
    documents = {name: read_json(path) for name, path in paths.items()}
    for name, document in documents.items():
        validate_schema(document, name, paths[name])

    physics, devices, interfaces = (documents[name] for name in ("physics", "devices", "interfaces"))
    identity = {(document["modelId"], document["modelRevision"]) for document in documents.values()}
    require(len(identity) == 1, "physics/devices/interfaces modelId and modelRevision must match")

    bodies = unique(physics["bodies"], "bodyId", "bodyId")
    joints = unique(physics["joints"], "jointId", "jointId")
    assets = unique(physics.get("geometryAssets", []), "assetId", "assetId")
    require(physics["rootBodyId"] in bodies, "rootBodyId does not reference a body")
    require("parentBodyId" not in bodies[physics["rootBodyId"]], "root body must not have parentBodyId")
    if physics.get("rootType", "fixed") == "floating":
        require("rootInitialTranslation_m" in physics and "rootInitialRotation_xyzw" in physics, "floating root requires initial translation and rotation")
    if "rootInitialRotation_xyzw" in physics:
        normalized(physics["rootInitialRotation_xyzw"], "root initial rotation")
    for body_id, body in bodies.items():
        normalized(body["rotation_xyzw"], f"body {body_id} rotation")
        positive_definite(body["inertia_kg_m2"], f"body {body_id}")
        if body_id != physics["rootBodyId"]:
            require(body.get("parentBodyId") in bodies, f"body {body_id} has unknown parent")
        if "visualAssetId" in body:
            require(body["visualAssetId"] in assets, f"body {body_id} references unknown visual asset")
        for asset_id in body.get("collisionAssetIds", []):
            require(asset_id in assets, f"body {body_id} references unknown collision asset")

    joint_children: set[str] = set()
    for joint_id, joint in joints.items():
        require(joint["parentBodyId"] in bodies and joint["childBodyId"] in bodies, f"joint {joint_id} references unknown body")
        require(joint["parentBodyId"] != joint["childBodyId"], f"joint {joint_id} connects a body to itself")
        require(joint["childBodyId"] not in joint_children, f"body {joint['childBodyId']} has multiple parent joints")
        joint_children.add(joint["childBodyId"])
        require(bodies[joint["childBodyId"]].get("parentBodyId") == joint["parentBodyId"], f"joint {joint_id} disagrees with body parent")
        normalized(joint["rotation_xyzw"], f"joint {joint_id} rotation")
        if joint["type"] != "fixed":
            normalized(joint["axis"], f"joint {joint_id} axis")
            limit = joint["limit"]
            require(limit["lower"] < limit["upper"], f"joint {joint_id} lower limit must be less than upper")
            require(limit["lower"] <= joint["initialPosition_rad"] <= limit["upper"], f"joint {joint_id} initial position is outside limits")
    require(joint_children == set(bodies) - {physics["rootBodyId"]}, "every non-root body must have exactly one parent joint")

    movable_joint_ids = {joint_id for joint_id, joint in joints.items() if joint["type"] != "fixed"}
    pose_names: set[str] = set()
    for pose in physics.get("namedPoses", []):
        require(pose["name"] not in pose_names, f"duplicate named pose: {pose['name']}")
        pose_names.add(pose["name"])
        require(set(pose["jointPositions"]) == movable_joint_ids, f"named pose {pose['name']} must define every movable joint exactly once")
        for joint_id, coordinate in pose["jointPositions"].items():
            limit = joints[joint_id]["limit"]
            require(limit["lower"] <= coordinate <= limit["upper"], f"named pose {pose['name']} joint {joint_id} is outside limits")

    for body_a, body_b in physics["contactDefaults"].get("excludedBodyPairs", []):
        require(body_a in bodies and body_b in bodies and body_a != body_b, "excludedBodyPairs contains an invalid body reference")

    contact_ids: set[str] = set()
    for contact in physics.get("contactGeometries", []):
        require(contact["contactId"] not in contact_ids, f"duplicate contactId: {contact['contactId']}")
        contact_ids.add(contact["contactId"])
        require(contact["bodyId"] in bodies, f"contact {contact['contactId']} references unknown body")
        normalized(contact["rotation_xyzw"], f"contact {contact['contactId']} rotation")
        shape = contact["shape"]
        if shape == "sphere":
            require("radius_m" in contact, f"sphere contact {contact['contactId']} requires radius_m")
        elif shape == "box":
            require("halfExtents_m" in contact and all(value > 0 for value in contact["halfExtents_m"]), f"box contact {contact['contactId']} requires positive halfExtents_m")
        elif shape == "capsule":
            require("radius_m" in contact and "halfLength_m" in contact and "axis" in contact, f"capsule contact {contact['contactId']} requires radius_m, halfLength_m and axis")
            normalized(contact["axis"], f"contact {contact['contactId']} axis")
        elif shape == "footPatch":
            require(len(contact.get("points_m", [])) >= 4, f"footPatch contact {contact['contactId']} requires at least four points")
        elif shape == "mesh":
            require("assetId" in contact, f"mesh contact {contact['contactId']} requires assetId")
            require(contact["assetId"] in assets, f"mesh contact {contact['contactId']} references unknown asset")

    device_map = unique(devices["devices"], "deviceId", "deviceId")
    for device_id, device in device_map.items():
        if device["deviceType"] == "actuator":
            require(device["jointId"] in joints, f"actuator {device_id} references unknown joint")
            if "powerDeviceId" in device:
                require(device["powerDeviceId"] in device_map, f"actuator {device_id} references unknown power device")
                require(device_map[device["powerDeviceId"]]["deviceType"] == "power", f"actuator {device_id} powerDeviceId is not a power device")
        elif device["deviceType"] == "sensor":
            require(device["bodyId"] in bodies, f"sensor {device_id} references unknown body")
            if "jointId" in device:
                require(device["jointId"] in joints, f"sensor {device_id} references unknown joint")
            normalized(device["rotation_xyzw"], f"sensor {device_id} rotation")

    expected_units = {
        "jointTorqueCommand": "N.m", "jointPositionCommand": "rad", "jointVelocityCommand": "rad/s",
        "jointPosition": "rad", "jointVelocity": "rad/s", "jointAcceleration": "rad/s2", "jointTorque": "N.m",
        "valid": "1", "limited": "1", "statusCode": "1",
    }
    all_channel_names: set[str] = set()
    for group in ("commands", "states", "diagnostics"):
        channels = interfaces[group]
        contiguous_channels(channels, group)
        for channel in channels:
            require(channel["name"] not in all_channel_names, f"public channel name repeated across groups: {channel['name']}")
            all_channel_names.add(channel["name"])
            require(channel["unit"] == expected_units[channel["semantic"]], f"channel {channel['name']} unit does not match semantic")
            if "jointId" in channel:
                require(channel["jointId"] in joints, f"channel {channel['name']} references unknown joint")
            if "deviceId" in channel:
                require(channel["deviceId"] in device_map, f"channel {channel['name']} references unknown device")

    return {
        "valid": True,
        "modelId": physics["modelId"],
        "modelRevision": physics["modelRevision"],
        "bodyCount": len(bodies),
        "jointCount": len(joints),
        "deviceCount": len(device_map),
        "commandCount": len(interfaces["commands"]),
        "stateCount": len(interfaces["states"]),
        "diagnosticCount": len(interfaces["diagnostics"]),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("bundle", type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(validate_bundle(args.bundle), ensure_ascii=False, indent=2))
        return 0
    except Exception as exc:
        print(json.dumps({"valid": False, "error": str(exc)}, ensure_ascii=False, indent=2))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
