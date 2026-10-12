from __future__ import annotations

import json
import hashlib
import re
import subprocess
import tempfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from project_tool import AIDC_ROOT, OMC, ContractError, source_fingerprint, validate_project, write_json


def _run_mos(lines: list[str], cwd: Path) -> str:
    with tempfile.NamedTemporaryFile("w", suffix=".mos", encoding="utf-8", delete=False, dir=cwd) as handle:
        handle.write("\n".join(lines) + "\n")
        path = Path(handle.name)
    try:
        process = subprocess.run([str(OMC), str(path)], cwd=cwd, capture_output=True, text=True)
        output = process.stdout + ("\n" + process.stderr if process.stderr else "")
        if process.returncode != 0:
            raise ContractError(f"omc 索引查询失败，退出码 {process.returncode}: {output[-2000:]}")
        return output
    finally:
        path.unlink(missing_ok=True)


def _base_lines(loaded: dict[str, Any]) -> list[str]:
    project = loaded["project"]
    lines = [f'setCommandLineOptions("--std={project["modelica"]["languageVersion"]}");']
    for library in project["modelica"]["libraries"]:
        if "path" in library:
            resolved = next(item["path"] for item in loaded["libraryFiles"] if item["name"] == library["name"])
            lines.append(f'loadFile("{resolved.resolve().as_posix()}");')
        else:
            lines.append(f'loadModel({library["name"]}, {{"{library["version"]}"}});')
    lines.append(f'loadFile("{loaded["modelFile"].resolve().as_posix()}");')
    return lines


def _after_marker(output: str, marker: str) -> str:
    lines = output.splitlines()
    try:
        index = lines.index(marker)
    except ValueError as exc:
        raise ContractError(f"omc 索引输出缺少标记 {marker}") from exc
    for line in lines[index + 1:]:
        if line.strip():
            return line.strip()
    raise ContractError(f"omc 索引标记后没有数据 {marker}")


def _split_top_level(value: str) -> list[str]:
    items: list[str] = []
    start = 0
    depth = 0
    quoted = False
    escaped = False
    for index, char in enumerate(value):
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
            continue
        if char == '"':
            quoted = True
        elif char in "{(":
            depth += 1
        elif char in "})":
            depth -= 1
        elif char == "," and depth == 0:
            items.append(value[start:index].strip())
            start = index + 1
    items.append(value[start:].strip())
    return items


def _unquote(value: str) -> str:
    value = value.strip()
    if value.startswith('"') and value.endswith('"'):
        try:
            return json.loads(value)
        except json.JSONDecodeError:
            return value[1:-1]
    return value


def _parse_classes(value: str) -> list[str]:
    if not value.startswith("{") or not value.endswith("}"):
        raise ContractError(f"无法解析类列表: {value}")
    inner = value[1:-1].strip()
    return [] if not inner else [item.strip() for item in _split_top_level(inner)]


def _parse_string_list(value: str) -> list[str]:
    if not value.startswith("{") or not value.endswith("}"):
        return []
    inner = value[1:-1].strip()
    return [] if not inner else [_unquote(item) for item in _split_top_level(inner)]


def _parse_components(value: str) -> list[dict[str, Any]]:
    if value == "{}":
        return []
    if not value.startswith("{{") or not value.endswith("}}"):
        raise ContractError("无法解析 getComponents 输出")
    entries = _split_top_level(value[1:-1])
    components = []
    for entry in entries:
        fields = _split_top_level(entry[1:-1])
        if len(fields) < 11:
            continue
        components.append({
            "type": fields[0],
            "name": fields[1],
            "comment": _unquote(fields[2]),
            "visibility": _unquote(fields[3]),
            "variability": _unquote(fields[8]),
            "causality": _unquote(fields[10]),
            "dimensions": _parse_string_list(fields[11]) if len(fields) > 11 else [],
        })
    return components


def _parse_connection(value: str) -> dict[str, str]:
    fields = re.findall(r'"((?:\\.|[^"\\])*)"', value)
    if len(fields) < 2:
        raise ContractError(f"无法解析连接: {value}")
    return {"from": fields[0], "to": fields[1], "comment": fields[2] if len(fields) > 2 else ""}


def _number(value: str) -> float | None:
    try:
        return float(value.strip())
    except ValueError:
        return None


def _parse_component_annotations(value: str, count: int) -> list[dict[str, Any] | None]:
    """Parse the stable Placement tuple returned by omc getComponentAnnotations."""
    if not value.startswith("{") or not value.endswith("}"):
        return [None] * count
    entries = _split_top_level(value[1:-1]) if value[1:-1].strip() else []
    result: list[dict[str, Any] | None] = []
    pattern = re.compile(
        r"Placement\((true|false),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),",
        re.IGNORECASE,
    )
    for entry in entries:
        match = pattern.search(entry)
        if not match:
            result.append(None)
            continue
        ox, oy = _number(match.group(2)), _number(match.group(3))
        values = [_number(item) for item in match.groups()[3:7]]
        rotation = _number(match.group(8))
        if any(item is None for item in values):
            result.append(None)
            continue
        x1, y1, x2, y2 = values
        if ox is None or oy is None:
            ox, oy = (x1 + x2) / 2, (y1 + y2) / 2
            x1, y1, x2, y2 = x1 - ox, y1 - oy, x2 - ox, y2 - oy
        result.append({"visible": match.group(1).lower() == "true", "origin": [ox, oy], "extent": [[x1, y1], [x2, y2]], "rotation": rotation})
    return (result + [None] * count)[:count]


def _parse_component_dialogs(value: str, components: list[dict[str, Any]]) -> dict[str, dict[str, str]]:
    if not value.startswith("{") or not value.endswith("}"):
        return {}
    entries = _split_top_level(value[1:-1]) if value[1:-1].strip() else []
    dialogs: dict[str, dict[str, str]] = {}
    for component, entry in zip(components, entries):
        match = re.search(r'Dialog\("([^"]*)"\s*,\s*"([^"]*)"', entry)
        if match:
            dialogs[component["name"]] = {"tab": match.group(1), "group": match.group(2)}
    return dialogs


def _parse_diagram_annotation(value: str) -> dict[str, Any] | None:
    fields = _split_top_level(value[1:-1]) if value.startswith("{") and value.endswith("}") else []
    values = [_number(item) for item in fields[:4]]
    if len(values) < 4 or any(item is None for item in values):
        return None
    return {"extent": [[values[0], values[1]], [values[2], values[3]]]}


def _connection_routes(listing: str) -> dict[tuple[str, str], dict[str, Any]]:
    pair = r"\{\s*([-+0-9.eE]+)\s*,\s*([-+0-9.eE]+)\s*\}"
    pattern = re.compile(
        rf"connect\s*\(\s*([^,]+?)\s*,\s*([^)]+?)\s*\)\s*annotation\s*\(\s*Line\s*\(\s*points\s*=\s*\{{((?:\s*{pair}\s*,?)+)\}}",
        re.DOTALL,
    )
    routes: dict[tuple[str, str], dict[str, Any]] = {}
    for match in pattern.finditer(listing):
        points = [[float(x), float(y)] for x, y in re.findall(pair, match.group(3))]
        tail = listing[match.end():listing.find("));", match.end()) + 3]
        color_match = re.search(r"color\s*=\s*\{([^}]+)\}", tail)
        color = [int(float(item)) for item in color_match.group(1).split(",")] if color_match else [0, 0, 0]
        routes[(match.group(1).strip(), match.group(2).strip())] = {"points": points, "lineColor": color[:3]}
    return routes


def _numbers(value: str) -> list[float]:
    return [float(item) for item in re.findall(r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?", value)]


def _point(value: str) -> list[float]:
    values = _numbers(value)
    return values[:2] if len(values) >= 2 else [0.0, 0.0]


def _points(value: str) -> list[list[float]]:
    return [[float(x), float(y)] for x, y in re.findall(r"\{\s*([-+0-9.eE]+)\s*,\s*([-+0-9.eE]+)\s*\}", value)]


def _color(value: str) -> list[int]:
    values = _numbers(value)
    return [int(item) for item in values[:3]] if len(values) >= 3 else [0, 0, 0]


def _graphic_primitive(value: str) -> dict[str, Any] | None:
    match = re.match(r"(Line|Polygon|Rectangle|Ellipse|Text)\((.*)\)$", value.strip(), re.DOTALL)
    if not match:
        return None
    kind = match.group(1).lower()
    args = _split_top_level(match.group(2))
    if len(args) < 4 or args[0].strip().lower() == "false":
        return None
    base: dict[str, Any] = {"type": kind, "origin": _point(args[1]), "rotation": _number(args[2]) or 0.0}
    if args[0].strip().lower() != "true":
        base["visible"] = args[0].strip()
    if kind == "line" and len(args) >= 7:
        base.update({"points": _points(args[3]), "lineColor": _color(args[4]), "linePattern": args[5].rsplit(".", 1)[-1], "thickness": _number(args[6]) or 0.25})
    elif kind == "polygon" and len(args) >= 9:
        base.update({"lineColor": _color(args[3]), "fillColor": _color(args[4]), "linePattern": args[5].rsplit(".", 1)[-1], "fillPattern": args[6].rsplit(".", 1)[-1], "thickness": _number(args[7]) or 0.25, "points": _points(args[8])})
    elif kind in {"rectangle", "ellipse"} and len(args) >= 9:
        extent_index = 9 if kind == "rectangle" else 8
        base.update({"lineColor": _color(args[3]), "fillColor": _color(args[4]), "linePattern": args[5].rsplit(".", 1)[-1], "fillPattern": args[6].rsplit(".", 1)[-1], "thickness": _number(args[7]) or 0.25, "extent": _points(args[extent_index])})
        if kind == "rectangle" and len(args) > 10:
            base["radius"] = _number(args[10]) or 0.0
    elif kind == "text" and len(args) >= 12:
        base.update({"extent": _points(args[8]), "text": _unquote(args[9]), "fontSize": _number(args[10]) or 0.0, "textColor": _color(args[11]), "alignment": args[14].rsplit(".", 1)[-1] if len(args) > 14 else "Center"})
    else:
        return None
    return base


def _parse_icon_annotation(value: str) -> dict[str, Any] | None:
    if not value.startswith("{") or not value.endswith("}"):
        return None
    fields = _split_top_level(value[1:-1])
    if len(fields) < 9:
        return None
    # OMC reports omitted Icon coordinate-system values as "-" even though
    # Modelica defines the effective default extent as {-100,-100}..{100,100}.
    # Rejecting those placeholders discarded otherwise valid graphics for most
    # library classes and left only their connector glyphs in the browser.
    coordinate = [
        parsed if (parsed := _number(item)) is not None else fallback
        for item, fallback in zip(fields[:4], (-100.0, -100.0, 100.0, 100.0))
    ]
    graphics_value = fields[8]
    graphics = []
    if graphics_value.startswith("{") and graphics_value.endswith("}"):
        for item in _split_top_level(graphics_value[1:-1]):
            primitive = _graphic_primitive(item)
            if primitive:
                graphics.append(primitive)
    return {"extent": [[coordinate[0], coordinate[1]], [coordinate[2], coordinate[3]]], "graphics": graphics, "connectors": []}


def _parse_class_information(value: str, project_root: Path, fallback: Path) -> dict[str, Any]:
    if not value.startswith("(") or not value.endswith(")"):
        return {"kind": "class", "path": fallback.relative_to(project_root).as_posix(), "line": None}
    fields = _split_top_level(value[1:-1])
    kind = _unquote(fields[0]) if fields else "class"
    description = _unquote(fields[1]) if len(fields) > 1 else ""
    source = Path(_unquote(fields[5])) if len(fields) > 5 and _unquote(fields[5]) else fallback
    try:
        relative = source.resolve().relative_to(project_root).as_posix()
    except ValueError:
        relative = fallback.relative_to(project_root).as_posix()
    try:
        line = int(fields[7]) if len(fields) > 7 else None
    except ValueError:
        line = None
    try:
        end_line = int(fields[9]) if len(fields) > 9 else None
    except ValueError:
        end_line = None
    return {"kind": kind, "description": description, "path": relative, "line": line, "endLine": end_line, "sourcePath": source}


def _class_source_segment(info: dict[str, Any]) -> str:
    source = info.get("sourcePath")
    if not isinstance(source, Path) or not source.is_file():
        return ""
    text = source.read_text(encoding="utf-8-sig")
    lines = text.splitlines()
    start = max(0, int(info.get("line") or 1) - 1)
    end = min(len(lines), int(info.get("endLine") or len(lines)))
    return "\n".join(lines[start:end])


def _boolean_parameter_defaults(source: str) -> dict[str, str]:
    pattern = re.compile(r"\bparameter\s+(?:\.?[A-Za-z_]\w*\.)*Boolean\s+([A-Za-z_]\w*)(?:\s*\([^;=]*\))?\s*=\s*(true|false)\b", re.IGNORECASE | re.DOTALL)
    return {match.group(1): match.group(2).lower() for match in pattern.finditer(source)}


def _connector_condition(source: str, name: str) -> str | None:
    match = re.search(rf"\b{re.escape(name)}\b\s+if\s+(.+?)(?=\s*(?:\"|annotation\s*\(|;))", source, re.DOTALL)
    return " ".join(match.group(1).split()) if match else None


def _source_line(source_text: str, name: str) -> int | None:
    pattern = re.compile(rf"\b{re.escape(name)}\b")
    for number, line in enumerate(source_text.splitlines(), start=1):
        if pattern.search(line):
            return number
    return None


def _component_modifiers(source_text: str, name: str) -> dict[str, str]:
    match = re.search(rf"\b{re.escape(name)}\s*\(", source_text)
    if not match:
        return {}
    start = match.end() - 1
    depth = 0
    quoted = False
    escaped = False
    end = None
    for index in range(start, len(source_text)):
        char = source_text[index]
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
            continue
        if char == '"':
            quoted = True
        elif char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                end = index
                break
    if end is None:
        return {}
    modifiers: dict[str, str] = {}
    for item in _split_top_level(source_text[start + 1:end]):
        assignment = re.match(r"\s*([A-Za-z_]\w*)\s*=\s*(.+?)\s*$", item, re.DOTALL)
        if assignment:
            modifiers[assignment.group(1)] = " ".join(assignment.group(2).split())
            continue
        nested = re.match(r"\s*([A-Za-z_]\w*)\s*\((.*)\)\s*$", item, re.DOTALL)
        if nested:
            prefix = nested.group(1)
            for nested_item in _split_top_level(nested.group(2)):
                nested_assignment = re.match(r"\s*([A-Za-z_]\w*)\s*=\s*(.+?)\s*$", nested_item, re.DOTALL)
                if nested_assignment:
                    modifiers[f"{prefix}.{nested_assignment.group(1)}"] = " ".join(nested_assignment.group(2).split())
    return modifiers


def _parameter_binding(source_text: str, name: str) -> str | None:
    pattern = re.compile(
        rf"\bparameter\b(?:(?!;).)*?\b{re.escape(name)}\b(?:\s*\([^;]*?\))?\s*=\s*(.+?)(?=\s+\"|\s+annotation\s*\(|;)",
        re.DOTALL,
    )
    match = pattern.search(source_text)
    return " ".join(match.group(1).split()) if match else None


def build_model_index(project_root: Path, force: bool = False) -> dict[str, Any]:
    project_root = project_root.resolve()
    loaded = validate_project(project_root)
    project = loaded["project"]
    digest, _ = source_fingerprint(project_root.resolve())
    dependency_files = sorted(
        (
            {
                "name": item["name"],
                "path": str(item["path"].resolve()),
                "sha256": hashlib.sha256(item["path"].read_bytes()).hexdigest(),
            }
            for item in loaded["libraryFiles"]
        ),
        key=lambda item: (item["name"], item["path"]),
    )
    dependency_digest = "sha256:" + hashlib.sha256(
        json.dumps(dependency_files, ensure_ascii=False, sort_keys=True).encode("utf-8")
    ).hexdigest()
    if not OMC.is_file():
        source_text = loaded["modelFile"].read_text(encoding="utf-8-sig")
        root_package = project["modelica"]["rootPackage"]
        declarations = [(match.group(1), match.group(2)) for match in re.finditer(r"(?m)^\s*(?:partial\s+)?(package|model|block|record|function|connector)\s+([A-Za-z_]\w*)", source_text)]
        classes = []
        for kind, name in declarations:
            class_id = root_package if name == root_package else f"{root_package}.{name}"
            if any(item["id"] == class_id for item in classes): continue
            classes.append({
                "id": class_id, "name": name, "kind": kind, "description": "",
                "source": {"path": loaded["modelFile"].relative_to(project_root).as_posix(), "line": _source_line(source_text, name), "endLine": None},
                "components": [], "connections": [], "diagram": None,
            })
        return {
            "schemaVersion": "1.0", "projectId": project["projectId"], "sourceDigest": digest,
            "dependencyDigest": dependency_digest, "generatedAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
            "generator": "source-fallback", "indexerVersion": "3.3", "rootPackage": root_package,
            "defaultExperiment": project["modelica"]["defaultExperiment"], "typeIcons": {}, "classes": classes,
            "diagnostics": [{"severity": "warning", "code": "OMC_UNAVAILABLE", "message": "OpenModelica 未安装，使用只读源码索引"}],
        }
    index_path = AIDC_ROOT / ".codex-runtime" / "modelica-index" / f"{project['projectId']}.json"
    if index_path.is_file() and not force:
        current = json.loads(index_path.read_text(encoding="utf-8-sig"))
        if (
            current.get("sourceDigest") == digest
            and current.get("dependencyDigest") == dependency_digest
            and current.get("indexerVersion") == "3.3"
        ):
            return current

    base = _base_lines(loaded)
    root_package = project["modelica"]["rootPackage"]
    class_output = _run_mos(base + [f'print("__AIDC_CLASSES__\\n");', f"getClassNames({root_package}, recursive=true, qualified=true, sort=true);"], project_root)
    classes = _parse_classes(_after_marker(class_output, "__AIDC_CLASSES__"))
    if root_package not in classes:
        classes.insert(0, root_package)

    detail_lines = list(base)
    for class_name in classes:
        marker = class_name.replace(".", "_")
        detail_lines += [
            f'print("__AIDC_COMPONENTS__{marker}\\n");', f"getComponents({class_name});",
            f'print("__AIDC_COMPONENT_ANNOTATIONS__{marker}\\n");', f"getComponentAnnotations({class_name});",
            f'print("__AIDC_DIAGRAM__{marker}\\n");', f"getDiagramAnnotation({class_name});",
            f'print("__AIDC_COUNT__{marker}\\n");', f"getConnectionCount({class_name});",
            f'print("__AIDC_INFO__{marker}\\n");', f"getClassInformation({class_name});",
        ]
    detail_output = _run_mos(detail_lines, project_root)
    class_records = []
    counts: dict[str, int] = {}
    for class_name in classes:
        marker = class_name.replace(".", "_")
        components = _parse_components(_after_marker(detail_output, f"__AIDC_COMPONENTS__{marker}"))
        placements = _parse_component_annotations(_after_marker(detail_output, f"__AIDC_COMPONENT_ANNOTATIONS__{marker}"), len(components))
        diagram = _parse_diagram_annotation(_after_marker(detail_output, f"__AIDC_DIAGRAM__{marker}"))
        count = int(_after_marker(detail_output, f"__AIDC_COUNT__{marker}"))
        class_info = _parse_class_information(_after_marker(detail_output, f"__AIDC_INFO__{marker}"), project_root, loaded["modelFile"])
        source_path = class_info.pop("sourcePath")
        source_text = source_path.read_text(encoding="utf-8-sig")
        routes = _connection_routes(source_text)
        counts[class_name] = count
        for component, placement in zip(components, placements):
            component["source"] = {"path": class_info["path"], "line": _source_line(source_text, component["name"])}
            component["ports"] = []
            component["modifiers"] = _component_modifiers(source_text, component["name"])
            if component.get("variability") == "parameter":
                component["binding"] = _parameter_binding(source_text, component["name"])
            if placement:
                component["placement"] = placement
        class_records.append({
            "id": class_name,
            "name": class_name.rsplit(".", 1)[-1],
            "kind": class_info["kind"],
            "description": class_info["description"],
            "source": {
                "path": class_info["path"],
                "line": class_info["line"] or _source_line(source_text, class_name.rsplit(".", 1)[-1]),
                "endLine": class_info.get("endLine"),
            },
            "components": components,
            "connections": [],
            "diagram": diagram,
            "connectionRoutes": routes,
        })

    # Index the icon of every declared class as well as component types.  A
    # class icon is part of the Modelica public interface even when that class
    # is not instantiated by another model (for example a top-level system).
    icon_types = sorted(
        {record["id"] for record in class_records}
        | {component["type"] for record in class_records for component in record["components"] if component.get("placement")}
    )
    type_icons: dict[str, Any] = {}
    if icon_types:
        icon_metadata: dict[str, dict[str, Any]] = {}
        pending = set(icon_types)
        while pending:
            batch = sorted(pending)
            icon_lines = list(base)
            for index, type_name in enumerate(batch):
                marker = f"T{index}"
                icon_lines += [
                    f'print("__AIDC_ICON__{marker}\\n");', f"getIconAnnotation({type_name});",
                    f'print("__AIDC_ICON_COMPONENTS__{marker}\\n");', f"getComponents({type_name});",
                    f'print("__AIDC_ICON_COMPONENT_ANNOTATIONS__{marker}\\n");', f"getComponentAnnotations({type_name});",
                    f'print("__AIDC_ICON_INHERITED__{marker}\\n");', f"getInheritedClasses({type_name});",
                    f'print("__AIDC_ICON_INFO__{marker}\\n");', f"getClassInformation({type_name});",
                ]
            icon_output = _run_mos(icon_lines, project_root)
            pending = set()
            for index, type_name in enumerate(batch):
                marker = f"T{index}"
                icon = _parse_icon_annotation(_after_marker(icon_output, f"__AIDC_ICON__{marker}"))
                nested = _parse_components(_after_marker(icon_output, f"__AIDC_ICON_COMPONENTS__{marker}"))
                nested_placements = _parse_component_annotations(
                    _after_marker(icon_output, f"__AIDC_ICON_COMPONENT_ANNOTATIONS__{marker}"), len(nested)
                )
                dialogs = _parse_component_dialogs(
                    _after_marker(icon_output, f"__AIDC_ICON_COMPONENT_ANNOTATIONS__{marker}"), nested
                )
                bases = _parse_classes(_after_marker(icon_output, f"__AIDC_ICON_INHERITED__{marker}"))
                source_info = _parse_class_information(
                    _after_marker(icon_output, f"__AIDC_ICON_INFO__{marker}"), project_root, loaded["modelFile"]
                )
                source_segment = _class_source_segment(source_info)
                connectors = [
                    {
                        "name": item["name"],
                        "type": item["type"],
                        "placement": placement,
                        **({"condition": condition} if (condition := _connector_condition(source_segment, item["name"])) else {}),
                    }
                    for item, placement in zip(nested, nested_placements)
                    if placement and ("Interfaces" in item["type"] or "Pin" in item["type"] or "Flange" in item["type"])
                ]
                parameters = [
                    {
                        "name": item["name"],
                        "type": item["type"],
                        "comment": item.get("comment", ""),
                        "binding": _parameter_binding(source_segment, item["name"]),
                        **dialogs.get(item["name"], {}),
                    }
                    for item in nested
                    if item.get("variability") == "parameter"
                ]
                icon_metadata[type_name] = {
                    "icon": icon,
                    "connectors": connectors,
                    "bases": bases,
                    "defaults": _boolean_parameter_defaults(source_segment),
                    "parameters": parameters,
                }
                pending.update(base_type for base_type in bases if base_type not in icon_metadata)

        composed: dict[str, dict[str, Any]] = {}

        def compose_icon(type_name: str, visiting: set[str] | None = None) -> dict[str, Any]:
            if type_name in composed:
                return composed[type_name]
            visiting = set() if visiting is None else visiting
            if type_name in visiting:
                return {"extent": [[-100.0, -100.0], [100.0, 100.0]], "graphics": [], "connectors": [], "defaults": {}, "parameters": []}
            visiting.add(type_name)
            metadata = icon_metadata.get(type_name, {"icon": None, "connectors": [], "bases": []})
            own_icon = metadata["icon"] or {"extent": [[-100.0, -100.0], [100.0, 100.0]], "graphics": []}
            graphics: list[dict[str, Any]] = []
            connector_map: dict[str, dict[str, Any]] = {}
            parameter_map: dict[str, dict[str, Any]] = {}
            defaults: dict[str, str] = {}
            for base_type in metadata["bases"]:
                inherited = compose_icon(base_type, visiting.copy())
                graphics.extend(inherited["graphics"])
                defaults.update(inherited.get("defaults", {}))
                for connector in inherited["connectors"]:
                    connector_map[connector["name"]] = connector
                for parameter in inherited.get("parameters", []):
                    parameter_map[parameter["name"]] = parameter
            defaults.update(metadata.get("defaults", {}))
            graphics.extend(own_icon.get("graphics", []))
            for connector in metadata["connectors"]:
                connector_map[connector["name"]] = connector
            for parameter in metadata.get("parameters", []):
                parameter_map[parameter["name"]] = parameter
            result = {"extent": own_icon["extent"], "graphics": graphics, "connectors": list(connector_map.values()), "defaults": defaults, "parameters": list(parameter_map.values())}
            composed[type_name] = result
            return result

        for type_name in icon_types:
            type_icons[type_name] = compose_icon(type_name)

    connection_lines = list(base)
    for class_name, count in counts.items():
        marker = class_name.replace(".", "_")
        for index in range(1, count + 1):
            connection_lines += [f'print("__AIDC_CONNECTION__{marker}__{index}\\n");', f"getNthConnection({class_name}, {index});"]
    if any(counts.values()):
        connection_output = _run_mos(connection_lines, project_root)
        by_id = {item["id"]: item for item in class_records}
        for class_name, count in counts.items():
            marker = class_name.replace(".", "_")
            record = by_id[class_name]
            components = {item["name"]: item for item in record["components"]}
            for index in range(1, count + 1):
                connection = _parse_connection(_after_marker(connection_output, f"__AIDC_CONNECTION__{marker}__{index}"))
                route = record["connectionRoutes"].get((connection["from"], connection["to"]))
                if route:
                    connection.update(route)
                record["connections"].append(connection)
                for endpoint in (connection["from"], connection["to"]):
                    parts = endpoint.split(".", 1)
                    if len(parts) == 2 and parts[0] in components and parts[1] not in components[parts[0]]["ports"]:
                        components[parts[0]]["ports"].append(parts[1])
            record.pop("connectionRoutes", None)

    index = {
        "schemaVersion": "1.0",
        "projectId": project["projectId"],
        "sourceDigest": digest,
        "dependencyDigest": dependency_digest,
        "generatedAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "generator": "omc-scripting-api",
        "indexerVersion": "3.3",
        "rootPackage": root_package,
        "defaultExperiment": project["modelica"]["defaultExperiment"],
        "typeIcons": type_icons,
        "classes": class_records,
    }
    write_json(index_path, index)
    return index
