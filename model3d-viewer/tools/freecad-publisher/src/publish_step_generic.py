"""Convert an arbitrary STEP assembly to a hierarchy-preserving browser GLB without AIDC model configuration."""

from __future__ import annotations

import hashlib
import json
import math
import os
from pathlib import Path

import FreeCAD as App
import Import

from publish_model import GlbBuilder, finite_geometry, mesh_topology, split_vertices_for_hard_edges


def triangulated_geometry(obj):
    points, faces = mesh_topology(obj)
    positions = [(point.x * 0.001, point.y * 0.001, point.z * 0.001) for point in points]
    triangles = []
    for face in faces:
        if len(face) < 3:
            continue
        first = int(face[0])
        for index in range(1, len(face) - 1):
            triangles.append((first, int(face[index]), int(face[index + 1])))
    if not positions or not triangles:
        return [], [], []
    return split_vertices_for_hard_edges(positions, triangles)


def main():
    source = Path(os.environ["AIDC_STEP_SOURCE"]).resolve()
    output = Path(os.environ["AIDC_STEP_OUTPUT"]).resolve()
    report_path = Path(os.environ.get("AIDC_STEP_REPORT", str(output.with_suffix(".report.json")))).resolve()
    if source.suffix.lower() not in {".step", ".stp"}:
        raise RuntimeError(f"unsupported source format: {source.suffix}")
    output.parent.mkdir(parents=True, exist_ok=True)
    document = App.newDocument("AIDC_Generic_STEP")
    Import.insert(str(source), document.Name)
    document.recompute()
    objects = [obj for obj in document.Objects if finite_geometry(obj)]
    if not objects:
        raise RuntimeError("STEP contains no finite solid or mesh geometry")

    builder = GlbBuilder()
    nodes = [{"name": source.stem, "children": [], "extras": {"kind": "assembly", "sourceFormat": "STEP"}}]
    exported = []
    triangle_count = 0
    palette = [
        [0.72, 0.75, 0.78, 1.0], [0.60, 0.67, 0.72, 1.0], [0.78, 0.70, 0.50, 1.0],
        [0.52, 0.66, 0.61, 1.0], [0.68, 0.60, 0.72, 1.0],
    ]
    for index, obj in enumerate(objects):
        positions, normals, faces = triangulated_geometry(obj)
        if not faces:
            continue
        mesh_index = builder.add_mesh(obj.Name, positions, normals, faces, palette[index % len(palette)], 0.12, 0.48)
        node_index = len(nodes)
        nodes.append({
            "name": str(obj.Label or obj.Name),
            "mesh": mesh_index,
            "extras": {"kind": "geometry", "sourceObject": obj.Name, "sourceFormat": "STEP"},
        })
        nodes[0]["children"].append(node_index)
        exported.append({"name": obj.Name, "label": obj.Label, "triangles": len(faces)})
        triangle_count += len(faces)
    if len(nodes) < 3:
        raise RuntimeError(f"STEP is not a multi-part assembly: exported {len(nodes) - 1} part")
    builder.write(output, nodes, [0], {
        "sourceFormat": "STEP", "sourceFile": source.name, "lengthUnit": "m",
        "configurationFree": True,
    })
    report = {
        "schemaVersion": "aidc.model3d.generic-step-report/0.1",
        "source": str(source),
        "sourceSha256": hashlib.sha256(source.read_bytes()).hexdigest(),
        "output": str(output),
        "configurationFree": True,
        "partCount": len(exported),
        "triangleCount": triangle_count,
        "parts": exported,
    }
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({key: report[key] for key in ("partCount", "triangleCount", "configurationFree")}, ensure_ascii=False))
    App.closeDocument(document.Name)


if __name__ == "__main__":
    main()

