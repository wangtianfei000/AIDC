"""Publish a FreeCAD assembly as an AIDC Model3D GLB + manifest package."""

from __future__ import annotations

import hashlib
import json
import math
import os
import re
import struct
import zipfile
from collections import deque
from pathlib import Path
from xml.etree import ElementTree

import FreeCAD as App


PUBLISHER_VERSION = "0.1.0"
SCHEMA_VERSION = "aidc.model3d.package/0.1"
GLTF_JSON_CHUNK = 0x4E4F534A
GLTF_BIN_CHUNK = 0x004E4942


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rel_uri(path: Path, base: Path) -> str:
    return os.path.relpath(path, base).replace("\\", "/")


def finite_geometry(obj) -> bool:
    if obj.TypeId == "App::Part":
        return False
    if hasattr(obj, "Mesh") and obj.Mesh.CountFacets:
        bb = obj.Mesh.BoundBox
    elif hasattr(obj, "Shape") and not obj.Shape.isNull():
        bb = obj.Shape.BoundBox
    else:
        return False
    values = [bb.XMin, bb.YMin, bb.ZMin, bb.XLength, bb.YLength, bb.ZLength]
    return bool(values) and all(math.isfinite(v) and abs(v) < 1e6 for v in values)


def collect_parts(root):
    result = []

    def visit(part):
        if part in result:
            return
        result.append(part)
        for child in getattr(part, "Group", []):
            if child.TypeId == "App::Part":
                visit(child)

    visit(root)
    return result


def nearest_part(obj, allowed_parts):
    queue = deque(getattr(obj, "InList", []))
    seen = set()
    while queue:
        candidate = queue.popleft()
        if candidate.Name in seen:
            continue
        seen.add(candidate.Name)
        if candidate.TypeId == "App::Part" and candidate in allowed_parts:
            return candidate
        queue.extend(getattr(candidate, "InList", []))
    return None


def mesh_topology(obj):
    if hasattr(obj, "Mesh") and obj.Mesh.CountFacets:
        points, faces = obj.Mesh.Topology
        return [App.Vector(p.x, p.y, p.z) for p in points], [tuple(f) for f in faces]
    if hasattr(obj, "Shape") and not obj.Shape.isNull():
        points, faces = obj.Shape.tessellate(0.15)
        return [App.Vector(p.x, p.y, p.z) for p in points], [tuple(f) for f in faces]
    return [], []


def split_vertices_for_hard_edges(positions, faces, smoothing_angle_degrees=38.0):
    """Build glTF normals without smoothing across mechanical hard edges."""
    cosine_limit = math.cos(math.radians(smoothing_angle_degrees))
    face_data = []
    for a, b, c in faces:
        pa, pb, pc = positions[a], positions[b], positions[c]
        ux, uy, uz = pb[0] - pa[0], pb[1] - pa[1], pb[2] - pa[2]
        vx, vy, vz = pc[0] - pa[0], pc[1] - pa[1], pc[2] - pa[2]
        cross = (uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx)
        length = math.sqrt(sum(value * value for value in cross)) or 1.0
        face_data.append((cross, tuple(value / length for value in cross)))

    groups_by_vertex = [[] for _ in positions]
    remapped_faces = []
    new_positions = []
    accumulated_normals = []
    for face_index, face in enumerate(faces):
        cross, unit = face_data[face_index]
        remapped = []
        for original_index in face:
            selected = None
            for group in groups_by_vertex[original_index]:
                normal = accumulated_normals[group]
                length = math.sqrt(sum(value * value for value in normal)) or 1.0
                if sum((normal[axis] / length) * unit[axis] for axis in range(3)) >= cosine_limit:
                    selected = group
                    break
            if selected is None:
                selected = len(new_positions)
                new_positions.append(positions[original_index])
                accumulated_normals.append([0.0, 0.0, 0.0])
                groups_by_vertex[original_index].append(selected)
            for axis in range(3):
                accumulated_normals[selected][axis] += cross[axis]
            remapped.append(selected)
        remapped_faces.append(tuple(remapped))

    normals = []
    for normal in accumulated_normals:
        length = math.sqrt(sum(value * value for value in normal)) or 1.0
        normals.append(tuple(value / length for value in normal))
    return new_positions, normals, remapped_faces


def transform_geometry_to_body(obj, body):
    points, faces = mesh_topology(obj)
    if not points or not faces:
        return [], [], [], []
    # FreeCAD Mesh.Topology and Shape.tessellate() already include the feature's
    # own Placement. Their points are expressed in the containing App::Part's
    # coordinates. Applying obj.getGlobalPlacement() here would apply the
    # feature placement a second time and visually explode the assembly.
    body_global = body.getGlobalPlacement() if hasattr(body, "getGlobalPlacement") else body.Placement
    transformed = []
    world_points = []
    for point in points:
        transformed.append((point.x * 0.001, point.y * 0.001, point.z * 0.001))
        world = body_global.multVec(point)
        world_points.append((world.x * 0.001, world.y * 0.001, world.z * 0.001))

    valid_faces = []
    for face in faces:
        if len(face) < 3:
            continue
        a = int(face[0])
        for i in range(1, len(face) - 1):
            valid_faces.append((a, int(face[i]), int(face[i + 1])))

    split_positions, normals, split_faces = split_vertices_for_hard_edges(transformed, valid_faces)
    return split_positions, normals, split_faces, world_points


def _unpack_rgba(value):
    return [
        ((value >> 24) & 0xFF) / 255.0,
        ((value >> 16) & 0xFF) / 255.0,
        ((value >> 8) & 0xFF) / 255.0,
        (value & 0xFF) / 255.0,
    ]


def load_fcstd_appearance(path: Path):
    """Read persisted FreeCAD view materials without requiring a GUI process.

    FreeCAD stores ShapeAppearance in sidecar binary entries referenced by
    GuiDocument.xml.  FreeCADCmd deliberately does not restore ViewObjects, so
    reading obj.ViewObject here would silently discard the CAD author's colors.
    """
    if not path.is_file():
        return {}
    with zipfile.ZipFile(path) as archive:
        if "GuiDocument.xml" not in archive.namelist():
            return {}
        root = ElementTree.fromstring(archive.read("GuiDocument.xml"))
        available = set(archive.namelist())
        result = {}
        for provider in root.iter("ViewProvider"):
            object_name = provider.attrib.get("name", "")
            for prop in provider.iter("Property"):
                if prop.attrib.get("name") != "ShapeAppearance":
                    continue
                material_list = prop.find("MaterialList")
                filename = material_list.attrib.get("file", "") if material_list is not None else ""
                if not filename or filename not in available:
                    continue
                payload = archive.read(filename)
                if len(payload) < 28:
                    continue
                count = struct.unpack_from("<I", payload, 0)[0]
                if not count or len(payload) < 4 + count * 24:
                    continue
                # Object-level color is sufficient for the current source.  If
                # the list contains per-face entries, retain all colors so a
                # later publisher revision can split tessellated primitives.
                materials = []
                for index in range(count):
                    _, diffuse, _, _, shininess, transparency = struct.unpack_from(
                        "<IIIIff", payload, 4 + index * 24
                    )
                    color = _unpack_rgba(diffuse)
                    color[3] = max(0.0, min(1.0, 1.0 - transparency))
                    materials.append({"color": color, "shininess": shininess})
                result[object_name] = materials
        return result


def object_material(obj, component_id, appearance, source_appearance):
    """Resolve model-driven engineering materials with safe geometry fallbacks."""
    source_materials = source_appearance.get(getattr(obj, "Name", ""), [])
    if source_materials:
        source = source_materials[0]
        # FreeCAD's source file uses a high specular/shininess setting.  A low
        # metallic value and bounded PBR roughness preserve that appearance
        # under the browser's physically based renderer.
        roughness = max(0.22, min(0.72, 0.58 - 0.32 * source.get("shininess", 0.5)))
        return source["color"], 0.06, roughness
    component_class = appearance.get("componentClasses", {}).get(component_id)
    configured = appearance.get("palette", {}).get(component_class, {})
    if configured:
        return configured["color"], configured.get("metallic", 0.05), configured.get("roughness", 0.55)
    identity = f"{getattr(obj, 'Name', '')} {getattr(obj, 'Label', '')}".lower()
    if "bearing" in identity:
        return [0.54, 0.59, 0.64, 1.0], 0.72, 0.24
    if any(token in identity for token in ("pcb", "robot_hat", "imu")):
        return [0.07, 0.27, 0.22, 1.0], 0.08, 0.42
    if any(token in identity for token in ("lens", "camera")):
        return [0.035, 0.075, 0.11, 1.0], 0.16, 0.16
    if "battery" in identity:
        return [0.13, 0.28, 0.48, 1.0], 0.18, 0.38
    if any(token in identity for token in ("xl33", "xl34", "servo")):
        return [0.20, 0.24, 0.28, 1.0], 0.32, 0.34
    if any(token in identity for token in ("sole", "foot")):
        return [0.24, 0.27, 0.30, 1.0], 0.08, 0.72
    return [0.66, 0.70, 0.74, 1.0], 0.28, 0.38


def placement_values(placement):
    x, y, z, w = placement.Rotation.Q
    return {
        "translation": [placement.Base.x * 0.001, placement.Base.y * 0.001, placement.Base.z * 0.001],
        "rotation": [x, y, z, w],
    }


def box_geometry(minimum, maximum):
    x0, y0, z0 = minimum
    x1, y1, z1 = maximum
    positions = [
        (x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
        (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1),
    ]
    faces = [
        (0, 2, 1), (0, 3, 2), (4, 5, 6), (4, 6, 7),
        (0, 1, 5), (0, 5, 4), (1, 2, 6), (1, 6, 5),
        (2, 3, 7), (2, 7, 6), (3, 0, 4), (3, 4, 7),
    ]
    center = ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2)
    normals = []
    for x, y, z in positions:
        dx, dy, dz = x - center[0], y - center[1], z - center[2]
        length = math.sqrt(dx * dx + dy * dy + dz * dz) or 1.0
        normals.append((dx / length, dy / length, dz / length))
    return positions, normals, faces


class GlbBuilder:
    def __init__(self):
        self.binary = bytearray()
        self.buffer_views = []
        self.accessors = []
        self.meshes = []
        self.materials = []
        self.material_by_color = {}

    def _align(self):
        while len(self.binary) % 4:
            self.binary.append(0)

    def _append(self, payload: bytes, target: int | None = None) -> int:
        self._align()
        offset = len(self.binary)
        self.binary.extend(payload)
        view = {"buffer": 0, "byteOffset": offset, "byteLength": len(payload)}
        if target is not None:
            view["target"] = target
        self.buffer_views.append(view)
        return len(self.buffer_views) - 1

    def _accessor(self, view: int, component_type: int, count: int, value_type: str, minimum=None, maximum=None) -> int:
        item = {"bufferView": view, "componentType": component_type, "count": count, "type": value_type}
        if minimum is not None:
            item["min"] = minimum
        if maximum is not None:
            item["max"] = maximum
        self.accessors.append(item)
        return len(self.accessors) - 1

    def material(self, color, metallic=0.05, roughness=0.68, double_sided=False):
        key = tuple(round(v, 4) for v in color) + (round(metallic, 4), round(roughness, 4), double_sided)
        if key in self.material_by_color:
            return self.material_by_color[key]
        alpha = color[3]
        material = {
            "name": f"material-{len(self.materials)}",
            "pbrMetallicRoughness": {
                "baseColorFactor": color,
                "metallicFactor": metallic,
                "roughnessFactor": roughness,
            },
            "doubleSided": double_sided,
        }
        if alpha < 0.999:
            material["alphaMode"] = "BLEND"
        self.materials.append(material)
        index = len(self.materials) - 1
        self.material_by_color[key] = index
        return index

    def add_mesh(self, name, positions, normals, faces, color, metallic=0.05, roughness=0.68, double_sided=False):
        flat_positions = [value for point in positions for value in point]
        flat_normals = [value for point in normals for value in point]
        flat_indices = [index for face in faces for index in face]
        position_view = self._append(struct.pack(f"<{len(flat_positions)}f", *flat_positions), 34962)
        normal_view = self._append(struct.pack(f"<{len(flat_normals)}f", *flat_normals), 34962)
        index_view = self._append(struct.pack(f"<{len(flat_indices)}I", *flat_indices), 34963)
        minimum = [min(p[i] for p in positions) for i in range(3)]
        maximum = [max(p[i] for p in positions) for i in range(3)]
        position_accessor = self._accessor(position_view, 5126, len(positions), "VEC3", minimum, maximum)
        normal_accessor = self._accessor(normal_view, 5126, len(normals), "VEC3")
        index_accessor = self._accessor(index_view, 5125, len(flat_indices), "SCALAR")
        self.meshes.append({
            "name": name,
            "primitives": [{
                "attributes": {"POSITION": position_accessor, "NORMAL": normal_accessor},
                "indices": index_accessor,
                "material": self.material(color, metallic, roughness, double_sided),
                "mode": 4,
            }],
        })
        return len(self.meshes) - 1

    def write(self, path: Path, nodes, scene_roots, extras):
        document = {
            "asset": {"version": "2.0", "generator": f"AIDC FreeCAD Publisher {PUBLISHER_VERSION}"},
            "scene": 0,
            "scenes": [{"name": "Robot", "nodes": scene_roots}],
            "nodes": nodes,
            "meshes": self.meshes,
            "materials": self.materials,
            "buffers": [{"byteLength": len(self.binary)}],
            "bufferViews": self.buffer_views,
            "accessors": self.accessors,
            "extras": extras,
        }
        json_bytes = json.dumps(document, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
        while len(json_bytes) % 4:
            json_bytes += b" "
        self._align()
        binary = bytes(self.binary)
        total = 12 + 8 + len(json_bytes) + 8 + len(binary)
        with path.open("wb") as handle:
            handle.write(struct.pack("<III", 0x46546C67, 2, total))
            handle.write(struct.pack("<II", len(json_bytes), GLTF_JSON_CHUNK))
            handle.write(json_bytes)
            handle.write(struct.pack("<II", len(binary), GLTF_BIN_CHUNK))
            handle.write(binary)


def sysml_component_ids(path: Path):
    text = path.read_text(encoding="utf-8")
    return set(re.findall(r'componentId\s*:>\s*String\s*=\s*"([^"]+)"', text))


def main():
    config_path = Path(os.environ["AIDC_MODEL3D_CONFIG"]).resolve()
    config = json.loads(config_path.read_text(encoding="utf-8"))
    base = config_path.parent
    cad_path = (base / config["sourceCad"]).resolve()
    sysml_path = (base / config["sourceSysml"]).resolve()
    output = (base / config["outputDirectory"]).resolve()
    output.mkdir(parents=True, exist_ok=True)

    doc = App.openDocument(str(cad_path))
    root = doc.getObject(config["rootBody"])
    if root is None or root.TypeId != "App::Part":
        raise RuntimeError(f"root App::Part not found: {config['rootBody']}")
    parts = collect_parts(root)
    part_set = set(parts)
    body_parent = {part.Name: nearest_part(part, part_set - {part}) for part in parts}
    for part in parts:
        if not str(part.Label).strip():
            raise RuntimeError(f"physical App::Part has no model label: {part.Name}")

    nodes = []
    node_index = {}
    bodies = []
    for part in parts:
        transform = placement_values(part.Placement)
        node = {
            "name": part.Name,
            "translation": transform["translation"],
            "rotation": transform["rotation"],
            "children": [],
            "extras": {"kind": "rigidBody", "bodyId": part.Name},
        }
        node_index[part.Name] = len(nodes)
        nodes.append(node)
        parent = body_parent[part.Name]
        body = {
            "bodyId": part.Name,
            "label": part.Label,
            "nodeName": part.Name,
            "translation": transform["translation"],
            "rotation": transform["rotation"],
        }
        if parent:
            body["parentBodyId"] = parent.Name
        bodies.append(body)
    for part in parts:
        parent = body_parent[part.Name]
        if parent:
            nodes[node_index[parent.Name]]["children"].append(node_index[part.Name])

    builder = GlbBuilder()
    exported = []
    triangle_count = 0
    world_bounds_min = [math.inf, math.inf, math.inf]
    world_bounds_max = [-math.inf, -math.inf, -math.inf]
    component_map = config["componentMap"]
    reverse_component_map = {name: cid for cid, name in component_map.items()}
    appearance = config.get("appearance", {})
    appearance_cad = (base / appearance.get("sourceCad", config["sourceCad"])).resolve()
    source_appearance = load_fcstd_appearance(appearance_cad)
    for obj in doc.Objects:
        if not finite_geometry(obj) or obj.TypeId == "PartDesign::Body":
            continue
        body = nearest_part(obj, part_set)
        if body is None:
            continue
        positions, normals, faces, world_points = transform_geometry_to_body(obj, body)
        if not faces:
            continue
        for point in world_points:
            for axis in range(3):
                world_bounds_min[axis] = min(world_bounds_min[axis], point[axis])
                world_bounds_max[axis] = max(world_bounds_max[axis], point[axis])
        component_id = getattr(obj, "ComponentId", "") or reverse_component_map.get(obj.Name, "") or reverse_component_map.get(body.Name, "")
        color, metallic, roughness = object_material(obj, component_id, appearance, source_appearance)
        mesh_index = builder.add_mesh(obj.Name, positions, normals, faces, color, metallic, roughness)
        geometry_node = {
            "name": obj.Name,
            "mesh": mesh_index,
            "extras": {
                "kind": "geometry",
                "componentId": component_id,
                "bodyId": body.Name,
                "sourceObject": obj.Name,
                "geometryStatus": getattr(obj, "GeometryStatus", "SOURCE_MODEL_GEOMETRY"),
            },
        }
        geom_index = len(nodes)
        nodes.append(geometry_node)
        nodes[node_index[body.Name]]["children"].append(geom_index)
        exported.append({"object": obj.Name, "label": obj.Label, "bodyId": body.Name, "triangles": len(faces)})
        triangle_count += len(faces)

    glb_path = output / "visual.glb"
    builder.write(glb_path, nodes, [node_index[root.Name]], {
        "modelId": config["modelId"],
        "revision": config["revision"],
        "coordinateSystem": "+X forward, +Y left, +Z up; metres",
    })

    by_name = {obj.Name: obj for obj in doc.Objects}
    sysml_ids = sysml_component_ids(sysml_path)
    configured_ids = set(component_map)
    if sysml_ids != configured_ids:
        raise RuntimeError(f"component ID mismatch: missing={sorted(sysml_ids-configured_ids)}, extra={sorted(configured_ids-sysml_ids)}")

    components = []
    for component_id, object_name in component_map.items():
        obj = by_name.get(object_name)
        if obj is None:
            raise RuntimeError(f"component source object missing: {component_id} -> {object_name}")
        body = obj if obj.TypeId == "App::Part" else nearest_part(obj, part_set)
        if body is None:
            raise RuntimeError(f"component has no rigid body: {component_id} -> {object_name}")
        components.append({
            "componentId": component_id,
            "label": obj.Label,
            "nodeName": obj.Name,
            "bodyId": body.Name,
            "geometryStatus": getattr(obj, "GeometryStatus", "SOURCE_MODEL_GEOMETRY"),
            "sourceObject": obj.Name,
        })

    collection_defs = config["productStructure"]["collections"]
    collection_ids = {item["nodeId"] for item in collection_defs}
    if len(collection_ids) != len(collection_defs):
        raise RuntimeError("productStructure collection nodeId must be unique")
    roots = [item["nodeId"] for item in collection_defs if not item.get("parentNodeId")]
    if len(roots) != 1:
        raise RuntimeError(f"productStructure must have one root collection: {roots}")
    body_ids = {body["bodyId"] for body in bodies}
    for item in collection_defs:
        if item["bodyId"] not in body_ids:
            raise RuntimeError(f"product collection body missing: {item['nodeId']} -> {item['bodyId']}")
        if item.get("parentNodeId") and item["parentNodeId"] not in collection_ids:
            raise RuntimeError(f"product collection parent missing: {item['nodeId']} -> {item['parentNodeId']}")
    collection_by_body = {item["bodyId"]: item["nodeId"] for item in collection_defs}

    def owning_collection(body_id):
        current = next(part for part in parts if part.Name == body_id)
        while current is not None:
            if current.Name in collection_by_body:
                return collection_by_body[current.Name]
            current = body_parent[current.Name]
        raise RuntimeError(f"published geometry has no product collection: {body_id}")

    product_structure = [{
        "nodeId": item["nodeId"],
        "label": item["label"],
        "nodeType": "collection",
        **({"parentNodeId": item["parentNodeId"]} if item.get("parentNodeId") else {}),
        "nodeName": item["bodyId"],
        "bodyId": item["bodyId"],
    } for item in collection_defs]
    for item in exported:
        product_structure.append({
            "nodeId": f"PART:{item['object']}",
            "label": item["label"],
            "nodeType": "part",
            "parentNodeId": owning_collection(item["bodyId"]),
            "nodeName": item["object"],
            "bodyId": item["bodyId"],
        })

    collision_path = output / "collision.glb"
    collision_builder = GlbBuilder()
    collision_nodes = []
    collision_node_index = {}
    for body in bodies:
        node = {
            "name": body["nodeName"], "translation": body["translation"], "rotation": body["rotation"], "children": [],
            "extras": {"kind": "rigidBody", "bodyId": body["bodyId"]},
        }
        collision_node_index[body["bodyId"]] = len(collision_nodes)
        collision_nodes.append(node)
    for body in bodies:
        if body.get("parentBodyId"):
            collision_nodes[collision_node_index[body["parentBodyId"]]]["children"].append(collision_node_index[body["bodyId"]])
    enabled_collision = set(config.get("collision", {}).get("enabledComponentIds", []))
    collision_proxy_count = 0
    for component in components:
        if component["componentId"] not in enabled_collision:
            continue
        obj = by_name[component["sourceObject"]]
        body = next(part for part in parts if part.Name == component["bodyId"])
        positions, _, faces, _ = transform_geometry_to_body(obj, body)
        if not positions or not faces:
            continue
        minimum = [min(point[axis] for point in positions) for axis in range(3)]
        maximum = [max(point[axis] for point in positions) for axis in range(3)]
        box_positions, box_normals, box_faces = box_geometry(minimum, maximum)
        mesh_index = collision_builder.add_mesh(component["componentId"], box_positions, box_normals, box_faces, [0.85, 0.25, 0.18, 0.32])
        proxy_node = {
            "name": component["componentId"], "mesh": mesh_index,
            "extras": {"kind": "collisionProxy", "componentId": component["componentId"], "bodyId": component["bodyId"]},
        }
        proxy_index = len(collision_nodes)
        collision_nodes.append(proxy_node)
        collision_nodes[collision_node_index[component["bodyId"]]]["children"].append(proxy_index)
        collision_proxy_count += 1
    collision_builder.write(collision_path, collision_nodes, [collision_node_index[root.Name]], {
        "modelId": config["modelId"], "revision": config["revision"], "role": "collision",
        "precision": "oriented bounding-box proxy; not B-Rep exact",
    })

    body_names = {part.Name for part in parts}
    joints = []
    for item in config["joints"]:
        if item["parentBodyId"] not in body_names or item["childBodyId"] not in body_names:
            raise RuntimeError(f"joint body missing: {item['jointId']}")
        actual_parent = body_parent[item["childBodyId"]]
        if actual_parent is None or actual_parent.Name != item["parentBodyId"]:
            raise RuntimeError(
                f"joint hierarchy mismatch {item['jointId']}: expected {item['parentBodyId']} -> {item['childBodyId']}"
            )
        joints.append({
            "jointId": item["jointId"],
            "type": "revolute",
            "parentBodyId": item["parentBodyId"],
            "childBodyId": item["childBodyId"],
            "axis": item["axis"],
            "neutralValue": math.radians(item["neutralDeg"]),
            "zeroOffset": 0.0,
            "limit": {
                "lower": math.radians(item["lowerDeg"]),
                "upper": math.radians(item["upperDeg"]),
                "velocity": math.radians(item["velocityDegPerSec"]),
            },
            "drive": {"channel": f"{item['jointId']}.position", "sign": 1, "scale": 1},
        })

    kinematics = {
        "schemaVersion": "aidc.model3d.kinematics/0.1",
        "modelId": config["modelId"],
        "modelRevision": config["revision"],
        "rootBodyId": root.Name,
        "bodies": bodies,
        "joints": joints,
    }
    kinematics_path = output / "kinematics.json"
    kinematics_path.write_text(json.dumps(kinematics, ensure_ascii=False, indent=2), encoding="utf-8")

    manifest = {
        "schemaVersion": SCHEMA_VERSION,
        "modelId": config["modelId"],
        "revision": config["revision"],
        "name": config["name"],
        "manufacturingStatus": config["manufacturingStatus"],
        "coordinateSystem": {
            "lengthUnit": "m",
            "angleUnit": "rad",
            "forward": "+X",
            "left": "+Y",
            "up": "+Z",
            "quaternionOrder": "xyzw",
        },
        "source": {
            "cadUri": rel_uri(cad_path, base),
            "cadSha256": sha256(cad_path),
            "sysmlUri": rel_uri(sysml_path, base),
            "sysmlSha256": sha256(sysml_path),
            "publisherVersion": PUBLISHER_VERSION,
        },
        "assets": [{
            "role": "visual",
            "uri": glb_path.name,
            "mediaType": "model/gltf-binary",
            "sha256": sha256(glb_path),
            "byteLength": glb_path.stat().st_size,
        }, {
            "role": "collision",
            "uri": collision_path.name,
            "mediaType": "model/gltf-binary",
            "sha256": sha256(collision_path),
            "byteLength": collision_path.stat().st_size,
        }, {
            "role": "kinematics",
            "uri": kinematics_path.name,
            "mediaType": "application/vnd.aidc.model3d.kinematics+json",
            "sha256": sha256(kinematics_path),
            "byteLength": kinematics_path.stat().st_size,
        }],
        "rootBodyId": root.Name,
        "bodies": bodies,
        "components": components,
        "productStructure": product_structure,
        "joints": joints,
        "collision": config.get("collision", {
            "enabledComponentIds": [],
            "tolerance": 0.0002,
            "allowedContacts": [],
        }),
        "statistics": {
            "sourceObjectCount": len(doc.Objects),
            "exportedGeometryCount": len(exported),
            "triangleCount": triangle_count,
            "componentCount": len(components),
            "jointCount": len(joints),
            "visualBounds": {"minimum": world_bounds_min, "maximum": world_bounds_max},
        },
    }
    manifest_path = output / "model3d.manifest.json"
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    (output / "publish-report.json").write_text(json.dumps({
        "publisherVersion": PUBLISHER_VERSION,
        "source": str(cad_path),
        "manifest": manifest_path.name,
        "visual": glb_path.name,
        "collision": collision_path.name,
        "kinematics": kinematics_path.name,
        "collisionProxyCount": collision_proxy_count,
        "exportedGeometry": exported,
    }, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(manifest["statistics"], ensure_ascii=False))
    App.closeDocument(doc.Name)


if __name__ == "__main__":
    main()
