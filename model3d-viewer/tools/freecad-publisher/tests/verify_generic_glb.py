"""Validate the configuration-free STEP conversion artifact using only Python's standard library."""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path


def main():
    path = Path(sys.argv[1]).resolve()
    payload = path.read_bytes()
    magic, version, _ = struct.unpack_from("<III", payload, 0)
    if magic != 0x46546C67 or version != 2:
        raise RuntimeError("not a GLB 2.0 file")
    json_length, json_type = struct.unpack_from("<II", payload, 12)
    if json_type != 0x4E4F534A:
        raise RuntimeError("GLB JSON chunk missing")
    document = json.loads(payload[20:20 + json_length].decode("utf-8"))
    roots = document.get("scenes", [{}])[0].get("nodes", [])
    nodes = document.get("nodes", [])
    mesh_nodes = [node for node in nodes if "mesh" in node]
    extras = document.get("scenes", [{}])[0]
    if len(roots) != 1 or len(mesh_nodes) < 2:
        raise RuntimeError(f"expected one assembly root and at least two parts: roots={len(roots)}, parts={len(mesh_nodes)}")
    root = nodes[roots[0]]
    if len(root.get("children", [])) != len(mesh_nodes):
        raise RuntimeError("assembly root does not contain every exported STEP part")
    print(json.dumps({"root": root.get("name"), "partCount": len(mesh_nodes), "nodeCount": len(nodes)}, ensure_ascii=False))


if __name__ == "__main__":
    main()

