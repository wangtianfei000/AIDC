"""Validate the V0 Jingang robot model package without FreeCAD dependencies."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
from pathlib import Path


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def multiply_quaternion(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return [
        aw * bx + ax * bw + ay * bz - az * by,
        aw * by - ax * bz + ay * bw + az * bx,
        aw * bz + ax * by - ay * bx + az * bw,
        aw * bw - ax * bx - ay * by - az * bz,
    ]


def rotate_vector(q, v):
    rotated = multiply_quaternion(multiply_quaternion(q, [*v, 0.0]), [-q[0], -q[1], -q[2], q[3]])
    return rotated[:3]


def world_poses(manifest):
    bodies = {body["bodyId"]: body for body in manifest["bodies"]}
    result = {}

    def resolve(body_id, stack=()):
        if body_id in result:
            return result[body_id]
        if body_id in stack:
            raise ValueError(f"刚体树存在环: {' -> '.join((*stack, body_id))}")
        body = bodies[body_id]
        translation = body["translation"]
        rotation = body["rotation"]
        parent_id = body.get("parentBodyId")
        if parent_id:
            if parent_id not in bodies:
                raise ValueError(f"{body_id} 引用了不存在的父刚体 {parent_id}")
            parent_t, parent_q = resolve(parent_id, (*stack, body_id))
            local_t = rotate_vector(parent_q, translation)
            translation = [parent_t[index] + local_t[index] for index in range(3)]
            rotation = multiply_quaternion(parent_q, rotation)
        result[body_id] = (translation, rotation)
        return result[body_id]

    for body_id in bodies:
        resolve(body_id)
    return result


def read_glb_json(path: Path):
    data = path.read_bytes()
    if len(data) < 20:
        raise ValueError("GLB 文件过短")
    magic, version, total_length = struct.unpack_from("<4sII", data, 0)
    if magic != b"glTF" or version != 2 or total_length != len(data):
        raise ValueError("GLB 头或总长度不合法")
    json_length, json_type = struct.unpack_from("<II", data, 12)
    if json_type != 0x4E4F534A:
        raise ValueError("GLB 首块不是 JSON")
    return json.loads(data[20 : 20 + json_length].decode("utf-8").rstrip(" \x00"))


def close_vector(actual, expected, tolerance):
    return len(actual) == len(expected) and max(abs(a - b) for a, b in zip(actual, expected)) <= tolerance


def validate(manifest_path: Path, config_path: Path, expected_path: Path, neutral_frame_path: Path, assembly_path: Path, report_path: Path):
    manifest = read_json(manifest_path)
    config = read_json(config_path)
    checks = []
    errors = []

    def check(name, condition, detail=""):
        checks.append({"name": name, "passed": bool(condition), "detail": detail})
        if not condition:
            errors.append(f"{name}: {detail}")

    check("schema-version", manifest.get("schemaVersion") == "aidc.model3d.package/0.1", str(manifest.get("schemaVersion")))
    coordinate = manifest.get("coordinateSystem", {})
    check(
        "coordinate-contract",
        coordinate == {
            "lengthUnit": "m",
            "angleUnit": "rad",
            "forward": "+X",
            "left": "+Y",
            "up": "+Z",
            "quaternionOrder": "xyzw",
        },
        json.dumps(coordinate, ensure_ascii=False),
    )

    visual = next((asset for asset in manifest.get("assets", []) if asset.get("role") == "visual"), None)
    check("visual-asset-declared", visual is not None)
    glb_path = manifest_path.parent / (visual["uri"] if visual else "visual.glb")
    check("visual-asset-exists", glb_path.is_file(), str(glb_path))
    if glb_path.is_file() and visual:
        check("visual-byte-length", glb_path.stat().st_size == visual.get("byteLength"), str(glb_path.stat().st_size))
        check("visual-sha256", sha256(glb_path) == visual.get("sha256"), sha256(glb_path))
        gltf = read_glb_json(glb_path)
        node_names = {node.get("name") for node in gltf.get("nodes", [])}
        check("glb-version", gltf.get("asset", {}).get("version") == "2.0", str(gltf.get("asset")))
        check("glb-has-meshes", len(gltf.get("meshes", [])) > 0, str(len(gltf.get("meshes", []))))
    else:
        node_names = set()

    kinematics_asset = next((asset for asset in manifest.get("assets", []) if asset.get("role") == "kinematics"), None)
    check("kinematics-asset-declared", kinematics_asset is not None)
    kinematics_path = manifest_path.parent / (kinematics_asset["uri"] if kinematics_asset else "kinematics.json")
    check("kinematics-asset-exists", kinematics_path.is_file(), str(kinematics_path))
    if kinematics_path.is_file() and kinematics_asset:
        kinematics = read_json(kinematics_path)
        check("kinematics-byte-length", kinematics_path.stat().st_size == kinematics_asset.get("byteLength"), str(kinematics_path.stat().st_size))
        check("kinematics-sha256", sha256(kinematics_path) == kinematics_asset.get("sha256"), sha256(kinematics_path))
        check("kinematics-schema-version", kinematics.get("schemaVersion") == "aidc.model3d.kinematics/0.1", str(kinematics.get("schemaVersion")))
        check("kinematics-model-id", kinematics.get("modelId") == manifest.get("modelId"), str(kinematics.get("modelId")))
        check("kinematics-model-revision", kinematics.get("modelRevision") == manifest.get("revision"), str(kinematics.get("modelRevision")))
        check("kinematics-root-body", kinematics.get("rootBodyId") == manifest.get("rootBodyId"), str(kinematics.get("rootBodyId")))
        check("kinematics-bodies-match", kinematics.get("bodies") == manifest.get("bodies"), "独立运动学文件必须与兼容清单一致")
        check("kinematics-joints-match", kinematics.get("joints") == manifest.get("joints"), "独立运动学文件必须与兼容清单一致")

    bodies = manifest.get("bodies", [])
    body_ids = [body.get("bodyId") for body in bodies]
    check("body-id-unique", len(body_ids) == len(set(body_ids)), str(body_ids))
    check("root-body-present", manifest.get("rootBodyId") in body_ids, str(manifest.get("rootBodyId")))
    check("body-nodes-in-glb", all(body.get("nodeName") in node_names for body in bodies), "每个刚体必须对应 GLB 节点")
    for body in bodies:
        q = body.get("rotation", [])
        check(f"quaternion-normalized:{body.get('bodyId')}", len(q) == 4 and abs(math.sqrt(sum(v * v for v in q)) - 1.0) < 1e-8)

    poses = world_poses(manifest)
    max_extent = max(abs(value) for translation, _ in poses.values() for value in translation)
    check("meter-scale-sanity", 0.1 <= max_extent <= 2.0, f"最大刚体原点距离 {max_extent:.6f} m")
    visual_bounds = manifest.get("statistics", {}).get("visualBounds", {})
    bounds_min = visual_bounds.get("minimum", [])
    bounds_max = visual_bounds.get("maximum", [])
    bounds_size = [high - low for low, high in zip(bounds_min, bounds_max)]
    check("visual-bounds-shape", len(bounds_min) == 3 and len(bounds_max) == 3 and all(size > 0 for size in bounds_size), str(visual_bounds))
    check("visual-bounds-meter-scale", len(bounds_size) == 3 and all(0.05 <= size <= 1.0 for size in bounds_size), str(bounds_size))

    components = manifest.get("components", [])
    expected_components = config.get("componentMap", {})
    component_ids = {component.get("componentId") for component in components}
    check("component-count-19", len(components) == 19, str(len(components)))
    check("component-id-contract", component_ids == set(expected_components), f"actual={sorted(component_ids)}")
    check("component-nodes-in-glb", all(component.get("nodeName") in node_names for component in components), "每个部件必须对应 GLB 节点")
    product_nodes = manifest.get("productStructure", [])
    product_by_id = {node.get("nodeId"): node for node in product_nodes}
    product_ids = [node.get("nodeId") for node in product_nodes]
    roots = [node for node in product_nodes if not node.get("parentNodeId")]
    exported_count = manifest.get("statistics", {}).get("exportedGeometryCount")
    parts_in_tree = [node for node in product_nodes if node.get("nodeType") == "part"]
    check("product-node-id-unique", len(product_ids) == len(set(product_ids)), str(product_ids))
    check("product-structure-single-root", len(roots) == 1 and roots[0].get("nodeType") == "collection", str([item.get("nodeId") for item in roots]))
    check("product-node-types", all(node.get("nodeType") in {"collection", "part"} for node in product_nodes))
    check("product-parts-complete", len(parts_in_tree) == exported_count, f"parts={len(parts_in_tree)}, exported={exported_count}")
    check("product-nodes-in-glb", all(node.get("nodeName") in node_names for node in product_nodes), "每个产品树节点必须对应 GLB 节点")
    check(
        "product-parents-are-collections",
        all(
            not node.get("parentNodeId")
            or node.get("parentNodeId") in product_by_id
            and product_by_id[node["parentNodeId"]].get("nodeType") == "collection"
            for node in product_nodes
        ),
    )

    collision_asset = next((asset for asset in manifest.get("assets", []) if asset.get("role") == "collision"), None)
    collision_path = manifest_path.parent / (collision_asset["uri"] if collision_asset else "collision.glb")
    check("collision-asset-declared", collision_asset is not None)
    check("collision-asset-exists", collision_path.is_file(), str(collision_path))
    if collision_path.is_file() and collision_asset:
        check("collision-byte-length", collision_path.stat().st_size == collision_asset.get("byteLength"), str(collision_path.stat().st_size))
        check("collision-sha256", sha256(collision_path) == collision_asset.get("sha256"), sha256(collision_path))
        collision_gltf = read_glb_json(collision_path)
        proxy_ids = {node.get("extras", {}).get("componentId") for node in collision_gltf.get("nodes", []) if node.get("extras", {}).get("kind") == "collisionProxy"}
    else:
        proxy_ids = set()
    collision = manifest.get("collision", {})
    enabled_ids = set(collision.get("enabledComponentIds", []))
    check("collision-components-known", enabled_ids <= component_ids, str(sorted(enabled_ids - component_ids)))
    check("collision-proxies-complete", enabled_ids == proxy_ids, f"enabled={len(enabled_ids)}, proxies={len(proxy_ids)}")
    contact_pairs = [frozenset((rule.get("componentA"), rule.get("componentB"))) for rule in collision.get("allowedContacts", [])]
    check("allowed-contacts-known", all(pair <= component_ids and len(pair) == 2 for pair in contact_pairs), str(contact_pairs))

    joints = manifest.get("joints", [])
    expected_joint_ids = {joint["jointId"] for joint in config.get("joints", [])}
    joint_ids = {joint.get("jointId") for joint in joints}
    check("joint-count-10", len(joints) == 10, str(len(joints)))
    check("joint-id-contract", joint_ids == expected_joint_ids, f"actual={sorted(joint_ids)}")
    for joint in joints:
        joint_id = joint.get("jointId")
        axis = joint.get("axis", [])
        check(f"joint-bodies:{joint_id}", joint.get("parentBodyId") in body_ids and joint.get("childBodyId") in body_ids)
        check(f"joint-axis:{joint_id}", len(axis) == 3 and abs(math.sqrt(sum(v * v for v in axis)) - 1.0) < 1e-8, str(axis))
        limits = joint.get("limit", {})
        check(f"joint-limit:{joint_id}", limits.get("lower", 0) < limits.get("upper", 0), str(limits))
        check(f"joint-neutral:{joint_id}", limits.get("lower", 0) <= joint.get("neutralValue", math.inf) <= limits.get("upper", 0), str(joint.get("neutralValue")))
        check(f"joint-drive:{joint_id}", joint.get("drive", {}).get("sign") in (-1, 1) and joint.get("drive", {}).get("scale") == 1)

    neutral_frame = read_json(neutral_frame_path)
    neutral_values = neutral_frame.get("joints", {})
    check("neutral-frame-version", neutral_frame.get("schemaVersion") == "aidc.model3d.simulation-frame/0.1")
    check("neutral-frame-time", neutral_frame.get("time") == 0, str(neutral_frame.get("time")))
    check("neutral-frame-joint-set", set(neutral_values) == joint_ids, f"actual={sorted(neutral_values)}")
    for joint in joints:
        check(
            f"neutral-frame-value:{joint['jointId']}",
            abs(neutral_values.get(joint["jointId"], math.inf) - joint["neutralValue"]) <= 1e-12,
            str(neutral_values.get(joint["jointId"])),
        )

    assembly = read_json(assembly_path)
    check("assembly-version", assembly.get("schemaVersion") == "aidc.model3d.assembly/0.1")
    check("assembly-model", assembly.get("modelId") == manifest.get("modelId") and assembly.get("modelRevision") == manifest.get("revision"))
    check("assembly-steps-array", isinstance(assembly.get("steps"), list), str(type(assembly.get("steps"))))
    steps = assembly.get("steps", [])
    step_ids = [step.get("stepId") for step in steps]
    check("assembly-has-demo", len(steps) >= 1, str(len(steps)))
    check("assembly-step-id-unique", len(step_ids) == len(set(step_ids)), str(step_ids))
    seen_steps = set()
    for step in steps:
        step_id = step.get("stepId")
        check(f"assembly-components:{step_id}", bool(step.get("componentIds")) and set(step.get("componentIds", [])) <= component_ids, str(step.get("componentIds")))
        check(f"assembly-dependencies:{step_id}", set(step.get("dependsOn", [])) <= seen_steps, str(step.get("dependsOn")))
        frames = step.get("keyframes", [])
        times = [frame.get("time") for frame in frames]
        check(f"assembly-keyframes:{step_id}", len(frames) >= 2 and times == sorted(times) and times[0] == 0 and times[-1] > 0, str(times))
        for index, frame in enumerate(frames):
            position = frame.get("position", [])
            quaternion = frame.get("quaternion", [])
            check(f"assembly-position:{step_id}:{index}", len(position) == 3 and all(math.isfinite(value) for value in position), str(position))
            check(f"assembly-quaternion:{step_id}:{index}", len(quaternion) == 4 and abs(math.sqrt(sum(value * value for value in quaternion)) - 1.0) < 1e-8, str(quaternion))
        tool = step.get("tool")
        if tool:
            direction = tool.get("direction", [])
            check(f"assembly-tool-direction:{step_id}", len(direction) == 3 and sum(value * value for value in direction) > 0, str(direction))
        seen_steps.add(step_id)

    if expected_path.is_file():
        expected = read_json(expected_path)
        expected_poses = expected.get("poses", {})
        check("frozen-body-set", set(expected_poses) == set(poses), f"expected={len(expected_poses)}, actual={len(poses)}")
        for body_id, (translation, rotation) in poses.items():
            target = expected_poses.get(body_id, {})
            check(f"frozen-translation:{body_id}", close_vector(translation, target.get("translation", []), 1e-9))
            expected_q = target.get("rotation", [])
            same_q = close_vector(rotation, expected_q, 1e-9)
            negated_q = close_vector(rotation, [-value for value in expected_q], 1e-9) if expected_q else False
            check(f"frozen-rotation:{body_id}", same_q or negated_q)
        expected_joints = {joint["jointId"]: joint for joint in expected.get("joints", [])}
        actual_joints = {joint["jointId"]: joint for joint in joints}
        check("frozen-joint-set", set(expected_joints) == set(actual_joints), f"expected={len(expected_joints)}, actual={len(actual_joints)}")
        for joint_id, expected_joint in expected_joints.items():
            actual_joint = actual_joints.get(joint_id)
            check(f"frozen-joint:{joint_id}", actual_joint == expected_joint, json.dumps(actual_joint, ensure_ascii=False))
        expected_bounds = expected.get("visualBounds", {})
        check("frozen-visual-bounds-min", close_vector(bounds_min, expected_bounds.get("minimum", []), 1e-9), str(bounds_min))
        check("frozen-visual-bounds-max", close_vector(bounds_max, expected_bounds.get("maximum", []), 1e-9), str(bounds_max))
    else:
        errors.append(f"缺少冻结姿态基线: {expected_path}")

    report = {
        "schemaVersion": "aidc.model3d.validation/0.1",
        "passed": not errors,
        "summary": {"checks": len(checks), "passed": sum(item["passed"] for item in checks), "failed": len(errors)},
        "errors": errors,
        "checks": checks,
    }
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report["summary"], ensure_ascii=False))
    if errors:
        raise SystemExit("\n".join(errors))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--expected", type=Path, required=True)
    parser.add_argument("--neutral-frame", type=Path, required=True)
    parser.add_argument("--assembly", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    validate(
        args.manifest.resolve(),
        args.config.resolve(),
        args.expected.resolve(),
        args.neutral_frame.resolve(),
        args.assembly.resolve(),
        args.report.resolve(),
    )


if __name__ == "__main__":
    main()
