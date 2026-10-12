#!/usr/bin/env python3
"""Import one native STL as a passive rigid-body Robot IR bundle."""
from __future__ import annotations

import json
import re
import struct
from collections import Counter
from pathlib import Path
from typing import Any


class StlImportError(RuntimeError):
    pass


def _write(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def _identifier(stem: str) -> str:
    value = (re.sub(r"[^A-Za-z0-9_.:-]+", "-", stem).strip("-.") or "stl-body").lower()
    return value if value[0].isalpha() else "model-" + value


def _load_stl(path: Path, np: Any) -> Any:
    data = path.read_bytes()
    triangles: list[list[list[float]]] = []
    if len(data) >= 84:
        count = struct.unpack_from("<I", data, 80)[0]
        if 84 + count * 50 == len(data):
            for index in range(count):
                values = struct.unpack_from("<12fH", data, 84 + index * 50)
                triangles.append([list(values[3:6]), list(values[6:9]), list(values[9:12])])
    if not triangles:
        try:
            text = data.decode("utf-8", errors="strict")
        except UnicodeDecodeError as exc:
            raise StlImportError("STL 既不是有效二进制格式，也不是 UTF-8 ASCII 格式") from exc
        vertices = re.findall(r"\bvertex\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)", text, flags=re.IGNORECASE)
        if len(vertices) % 3:
            raise StlImportError("ASCII STL 的顶点数量不是 3 的倍数")
        triangles = [[list(map(float, vertices[i + offset])) for offset in range(3)] for i in range(0, len(vertices), 3)]
    if not triangles:
        raise StlImportError("STL 不包含有效三角面")
    return np.asarray(triangles, dtype=float)


def _is_watertight(triangles: Any) -> bool:
    edges: Counter[tuple[tuple[float, ...], tuple[float, ...]]] = Counter()
    for triangle in triangles:
        points = [tuple(round(float(value), 12) for value in point) for point in triangle]
        for left, right in ((0, 1), (1, 2), (2, 0)):
            edges[tuple(sorted((points[left], points[right])))] += 1
    return bool(edges) and all(count == 2 for count in edges.values())


def _closed_mass_properties(triangles: Any, density: float, np: Any) -> tuple[float, float, Any, Any]:
    volume = 0.0
    first = np.zeros(3)
    second = np.zeros(3)
    products = np.zeros(3)  # xy, xz, yz
    for a, b, c in triangles:
        tetra = float(np.dot(a, np.cross(b, c))) / 6.0
        volume += tetra
        first += tetra * (a + b + c) / 4.0
        for axis in range(3):
            values = (a[axis], b[axis], c[axis])
            second[axis] += tetra * (sum(value * value for value in values) + values[0]*values[1] + values[1]*values[2] + values[2]*values[0]) / 10.0
        for product_index, (left, right) in enumerate(((0, 1), (0, 2), (1, 2))):
            diagonal = a[left]*a[right] + b[left]*b[right] + c[left]*c[right]
            cross_terms = (a[left]*b[right] + b[left]*a[right] + a[left]*c[right] + c[left]*a[right]
                           + b[left]*c[right] + c[left]*b[right])
            products[product_index] += tetra * (2*diagonal + cross_terms) / 20.0
    if abs(volume) <= 1e-15:
        raise StlImportError("闭合 STL 的有向体积为零")
    orientation = 1.0 if volume > 0 else -1.0
    volume *= orientation; first *= orientation; second *= orientation; products *= orientation
    center = first / volume
    mass = volume * density
    inertia_origin = density * np.asarray([
        [second[1] + second[2], -products[0], -products[1]],
        [-products[0], second[0] + second[2], -products[2]],
        [-products[1], -products[2], second[0] + second[1]],
    ])
    inertia = inertia_origin - mass * ((float(np.dot(center, center)) * np.eye(3)) - np.outer(center, center))
    return volume, mass, center, inertia


def _write_binary_stl(path: Path, triangles: Any, np: Any) -> None:
    header = b"AIDC STL normalized to SI metres".ljust(80, b" ")
    with path.open("wb") as handle:
        handle.write(header); handle.write(struct.pack("<I", len(triangles)))
        for a, b, c in triangles:
            normal = np.cross(b - a, c - a)
            length = float(np.linalg.norm(normal))
            if length:
                normal = normal / length
            handle.write(struct.pack("<12fH", *(float(value) for value in (*normal, *a, *b, *c)), 0))


def import_stl(source: Path, output: Path, *, length_unit: str = "mm", density_kg_m3: float = 1000.0,
               root_type: str = "fixed") -> dict[str, Any]:
    source, output = source.resolve(), output.resolve()
    if source.suffix.lower() != ".stl" or not source.is_file():
        raise StlImportError(f"不是可读取的 STL 文件: {source}")
    if length_unit not in {"mm", "m"}:
        raise StlImportError("STL 长度单位必须是 mm 或 m")
    if not 0 < float(density_kg_m3) <= 50000:
        raise StlImportError("材料密度必须在 0 到 50000 kg/m³ 之间")
    if root_type not in {"fixed", "floating"}:
        raise StlImportError("根类型必须是 fixed 或 floating")

    try:
        import numpy as np
    except ImportError as exc:
        raise StlImportError("STL 导入需要后端基础依赖 numpy") from exc

    triangles = _load_stl(source, np)
    scale = 0.001 if length_unit == "mm" else 1.0
    triangles *= scale
    watertight = _is_watertight(triangles)

    warnings: list[str] = []
    if watertight:
        volume, mass, center, inertia = _closed_mass_properties(triangles, float(density_kg_m3), np)
        method = "closed-mesh"
    else:
        points = triangles.reshape((-1, 3))
        bounds = np.asarray([points.min(axis=0), points.max(axis=0)], dtype=float)
        extents = np.maximum(bounds[1] - bounds[0], 1e-6)
        volume = float(np.prod(extents))
        mass = volume * float(density_kg_m3)
        center = (bounds[0] + bounds[1]) / 2.0
        x, y, z = (float(item) for item in extents)
        inertia = np.diag([mass * (y*y + z*z) / 12, mass * (x*x + z*z) / 12, mass * (x*x + y*y) / 12])
        method = "bounding-box-fallback"
        warnings.append("STL 不是闭合实体，质量和惯量使用包围盒近似；建议修复网格后重新导入")

    minimum_inertia = max(mass * 1e-12, 1e-12)
    for axis in range(3):
        inertia[axis, axis] = max(float(inertia[axis, axis]), minimum_inertia)
    model_name = _identifier(source.stem)
    model_id = f"aidc.imported.{model_name}"
    revision = "stl-r1"
    asset_id = f"geometry:{model_name}"
    asset_relative = f"assets/{source.name}"
    common = {"modelId": model_id, "modelRevision": revision}

    physics: dict[str, Any] = {
        "schemaVersion": "aidc.robot.physics/0.1", **common,
        "coordinateSystem": {"lengthUnit": "m", "angleUnit": "rad", "forward": "+X", "left": "+Y", "up": "+Z", "geometryQuaternionOrder": "xyzw"},
        "rootBodyId": "body", "rootType": root_type,
        "bodies": [{
            "bodyId": "body", "translation_m": [0, 0, 0], "rotation_xyzw": [0, 0, 0, 1],
            "mass_kg": mass, "centerOfMass_m": center.tolist(),
            "inertia_kg_m2": {"ixx": float(inertia[0, 0]), "iyy": float(inertia[1, 1]), "izz": float(inertia[2, 2]),
                               "ixy": float(inertia[0, 1]), "ixz": float(inertia[0, 2]), "iyz": float(inertia[1, 2])},
            "visualAssetId": asset_id, "collisionAssetIds": [asset_id],
        }],
        "joints": [],
        "geometryAssets": [{"assetId": asset_id, "path": asset_relative, "format": "stl", "scale": [1, 1, 1]}],
        "contactGeometries": [{"contactId": "body-mesh", "bodyId": "body", "shape": "mesh", "translation_m": [0, 0, 0], "rotation_xyzw": [0, 0, 0, 1], "assetId": asset_id}],
        "contactDefaults": {"friction": 0.8, "contactMargin_m": 0.001, "excludedBodyPairs": []},
    }
    if root_type == "floating":
        physics.update({"rootInitialTranslation_m": [0, 0, 0], "rootInitialRotation_xyzw": [0, 0, 0, 1]})

    devices = {
        "schemaVersion": "aidc.robot.devices/0.1", **common,
        "devices": [{"deviceId": "body-imu", "deviceType": "sensor", "model": "ideal-body-imu", "sensorKind": "imu", "bodyId": "body",
                     "translation_m": [0, 0, 0], "rotation_xyzw": [0, 0, 0, 1], "samplePeriod_s": 0.001}],
    }
    interfaces = {
        "schemaVersion": "aidc.robot.interfaces/0.1", **common,
        "interfaceId": f"{model_id}.passive", "interfaceVersion": "1.0.0", "publicQuaternionOrder": "wxyz",
        "commands": [],
        "states": [{"name": "bodyValid", "index": 1, "semantic": "valid", "unit": "1", "direction": 1, "scale": 1, "samplePeriod_s": 0.001, "deviceId": "body-imu"}],
        "diagnostics": [{"name": "bodyStatusCode", "index": 1, "semantic": "statusCode", "unit": "1", "direction": 1, "scale": 1, "samplePeriod_s": 0, "deviceId": "body-imu"}],
    }

    output.mkdir(parents=True, exist_ok=True)
    _write(output / "physics.json", physics)
    _write(output / "devices.json", devices)
    _write(output / "interfaces.json", interfaces)
    asset_path = output / asset_relative
    asset_path.parent.mkdir(parents=True, exist_ok=True)
    # Normalize the generated asset to SI metres so Modelica visualization
    # and native contact queries consume exactly the same geometry.
    _write_binary_stl(asset_path, triangles, np)
    report = {
        "source": str(source), "sourceUnit": length_unit, "meterScale": scale, "density_kg_m3": float(density_kg_m3),
        "triangleCount": int(len(triangles)), "watertight": watertight, "massPropertyMethod": method,
        "volume_m3": volume, "mass_kg": mass, "centerOfMass_m": center.tolist(), "warnings": warnings,
    }
    _write(output / "stl-import-report.json", report)
    return report

