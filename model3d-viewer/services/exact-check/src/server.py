"""Localhost-only FreeCAD geometry exact interference service for AIDC V3."""
from __future__ import annotations

import argparse
import hashlib
import json
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

import FreeCAD as App
import MeshPart
import Part

SCHEMA_VERSION = "aidc.model3d.interference/0.1"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def matrix_from_three(values):
    if not isinstance(values, list) or len(values) != 16:
        raise ValueError("matrixWorld must contain 16 values")
    matrix = App.Matrix()
    # THREE.Matrix4 elements are column-major. FreeCAD uses row-major fields.
    for row in range(4):
        for column in range(4):
            value = float(values[column * 4 + row])
            if column == 3 and row < 3:
                value *= 1000.0
            setattr(matrix, f"A{row + 1}{column + 1}", value)
    return matrix


def geometry_for_object(obj):
    if hasattr(obj, "Shape") and not obj.Shape.isNull():
        return "shape", obj.Shape.copy(), "source B-Rep"
    if hasattr(obj, "Mesh") and obj.Mesh is not None and obj.Mesh.CountFacets > 0:
        return "mesh", obj.Mesh.copy(), "source triangle mesh"
    descendants = getattr(obj, "Group", [])
    child_shapes = []
    sources = []
    for child in descendants:
        try:
            kind, child_geometry, source = geometry_for_object(child)
            if kind != "shape":
                continue
            child_shapes.append(child_geometry)
            sources.append(source)
        except ValueError:
            pass
    if child_shapes:
        return "shape", Part.makeCompound(child_shapes), "+".join(sorted(set(sources)))
    raise ValueError(f"{obj.Name} 没有可用于 OpenCascade 校核的实体或发布网格")


def mesh_from_shape(shape):
    return MeshPart.meshFromShape(Shape=shape, LinearDeflection=0.05, AngularDeflection=0.35, Relative=False)


def exact_check(cad_path: Path, request: dict) -> dict:
    now = datetime.now(timezone.utc).isoformat()
    base = {
        "pairId": "::".join(sorted([request["componentA"]["componentId"], request["componentB"]["componentId"]])),
        "componentA": request["componentA"]["componentId"],
        "componentB": request["componentB"]["componentId"],
        "precision": "BREP_EXACT",
        "tolerance": float(request.get("tolerance", 0.0)),
        "checkedAt": now,
        "pose": request.get("pose", {}),
    }
    if request.get("cadSha256") != sha256(cad_path):
        return {**base, "status": "UNVERIFIED", "message": "CAD 源文件哈希与模型包不一致"}
    doc = App.openDocument(str(cad_path))
    try:
        objects = []
        geometry_kinds = []
        geometry_sources = []
        for side in ("componentA", "componentB"):
            item = request[side]
            obj = doc.getObject(item["sourceObject"])
            if obj is None:
                return {**base, "status": "UNVERIFIED", "message": f"{item['componentId']} 在 CAD 源文件中不存在"}
            try:
                kind, geometry, source = geometry_for_object(obj)
            except ValueError as error:
                return {**base, "status": "UNVERIFIED", "message": str(error)}
            matrix = matrix_from_three(item["matrixWorld"])
            if kind == "shape":
                geometry.Placement = App.Placement(matrix)
            else:
                geometry.transform(matrix)
            objects.append(geometry)
            geometry_kinds.append(kind)
            geometry_sources.append(source)
        first, second = objects
        tolerance = float(request.get("tolerance", 0.0))
        if geometry_kinds == ["shape", "shape"]:
            common = first.common(second)
            volume_m3 = max(0.0, float(common.Volume)) * 1e-9
            distance_m = float(first.distToShape(second)[0]) * 0.001
            confirmed = volume_m3 > max(1e-12, tolerance ** 3) or distance_m <= tolerance
            precision = "BREP_EXACT"
            detail = "OpenCascade B-Rep"
        else:
            first_mesh = first if geometry_kinds[0] == "mesh" else mesh_from_shape(first)
            second_mesh = second if geometry_kinds[1] == "mesh" else mesh_from_shape(second)
            common = first_mesh.intersect(second_mesh)
            volume_m3 = max(0.0, float(common.Volume)) * 1e-9
            distance_m = None
            confirmed = common.CountFacets > 0 or volume_m3 > max(1e-12, tolerance ** 3)
            precision = "MESH_EXACT"
            detail = "发布三角网格"
        return {
            **base,
            "precision": precision,
            "status": "CONFIRMED" if confirmed else "CLEARED",
            "minimumDistance": distance_m,
            "commonVolume": volume_m3,
            "message": (f"{detail}已确认干涉" if confirmed else f"{detail}已排除实时预警")
            + f"（{geometry_sources[0]} × {geometry_sources[1]}）",
        }
    except Exception as error:
        return {**base, "status": "UNVERIFIED", "message": f"精确校核失败：{error}"}
    finally:
        App.closeDocument(doc.Name)


class Handler(BaseHTTPRequestHandler):
    cad_path: Path

    def send_json(self, status, value):
        payload = json.dumps(value, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Access-Control-Allow-Origin", "http://127.0.0.1:5174")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()
        self.wfile.write(payload)

    def do_OPTIONS(self):
        self.send_json(204, {})

    def do_GET(self):
        if self.path == "/health":
            self.send_json(200, {"status": "ok", "engine": "FreeCAD/OpenCascade", "cad": str(self.cad_path)})
        else:
            self.send_json(404, {"error": "not found"})

    def do_POST(self):
        if self.path != "/check":
            self.send_json(404, {"error": "not found"}); return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length <= 0 or length > 1_000_000:
                raise ValueError("invalid request size")
            request = json.loads(self.rfile.read(length))
            if request.get("schemaVersion") != SCHEMA_VERSION:
                raise ValueError("unsupported schemaVersion")
            self.send_json(200, exact_check(self.cad_path, request))
        except Exception as error:
            self.send_json(400, {"error": str(error)})

    def log_message(self, format, *args):
        print(f"[exact-check] {self.address_string()} {format % args}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cad", type=Path, required=True)
    parser.add_argument("--port", type=int, default=47331)
    args = parser.parse_args()
    Handler.cad_path = args.cad.resolve()
    server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    print(json.dumps({"status": "ready", "url": f"http://127.0.0.1:{args.port}", "cad": str(Handler.cad_path)}, ensure_ascii=False))
    server.serve_forever()


if __name__ == "__main__":
    main()
