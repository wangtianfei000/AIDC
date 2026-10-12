#!/usr/bin/env python3
from __future__ import annotations

import json
import socket
import subprocess
import time
import urllib.request
import urllib.parse
from datetime import datetime
from pathlib import Path

HERE = Path(__file__).resolve()
MODEL_BROWSER = HERE.parents[1]
AIDC_ROOT = MODEL_BROWSER.parent
REPO_ROOT = AIDC_ROOT.parent
PYTHON = REPO_ROOT / "tools" / "openmodelica" / "fmpy-venv" / "Scripts" / "python.exe"
SERVER = MODEL_BROWSER / "services" / "project-server" / "server.py"
WEB_ROOT = MODEL_BROWSER / "apps" / "web" / "dist"
RESULTS = MODEL_BROWSER / "results"


def require(value: bool, message: str) -> None:
    if not value:
        raise AssertionError(message)


def free_port() -> int:
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def fetch(url: str) -> tuple[int, str, bytes]:
    with urllib.request.urlopen(url, timeout=60) as response:
        return response.status, response.headers.get("Content-Type", ""), response.read()


def verify_project(project: Path, scratch: Path) -> dict[str, object]:
    port = free_port()
    ready = scratch / f"{project.name}.json"
    log = scratch / f"{project.name}.log"
    with log.open("w", encoding="utf-8") as handle:
        process = subprocess.Popen(
            [str(PYTHON), str(SERVER), "--project", str(project), "--port", str(port), "--write-ready", str(ready), "--web-root", str(WEB_ROOT)],
            cwd=AIDC_ROOT, stdout=handle, stderr=subprocess.STDOUT, text=True,
        )
    try:
        deadline = time.monotonic() + 60
        while not ready.is_file() and time.monotonic() < deadline:
            require(process.poll() is None, "Project Server exited early")
            time.sleep(.15)
        require(ready.is_file(), "Project Server startup timeout")
        base = f"http://127.0.0.1:{port}"
        status, content_type, html_bytes = fetch(base + "/")
        html = html_bytes.decode("utf-8")
        require(status == 200 and "text/html" in content_type, "SPA entry is not served")
        require("Modelica 模型浏览器" in html, "SPA title is missing")
        asset_paths = []
        for token in html.replace('"', "'").split("'"):
            if token.startswith("/assets/"):
                asset_paths.append(token)
        require(len(asset_paths) >= 2, "Built JS/CSS assets are missing")
        asset_sizes = []
        bundle_text = ""
        for path in asset_paths:
            asset_status, asset_type, body = fetch(base + path)
            require(asset_status == 200 and len(body) > 100, f"Asset unavailable: {path}")
            if path.endswith(".js"):
                require("javascript" in asset_type, "JavaScript MIME type is invalid")
                bundle_text += body.decode("utf-8")
            asset_sizes.append(len(body))
        _, _, project_bytes = fetch(base + "/api/v1/project")
        project_info = json.loads(project_bytes)
        _, _, tree_bytes = fetch(base + "/api/v1/model/tree")
        tree = json.loads(tree_bytes)
        experiment = next(item for item in tree["classes"] if item["id"] == tree["defaultExperiment"])
        require(bool(experiment.get("description")), "Default experiment type description is missing")
        source_ref = experiment.get("source", {})
        require(source_ref.get("line") and source_ref.get("endLine", 0) >= source_ref["line"], "Selected type source range is missing")
        source_query = urllib.parse.urlencode({"path": source_ref["path"], "line": source_ref["line"], "endLine": source_ref["endLine"]})
        _, _, source_bytes = fetch(base + "/api/v1/source?" + source_query)
        selected_source = json.loads(source_bytes)
        require(selected_source["startLine"] == source_ref["line"] and selected_source["endLine"] == source_ref["endLine"], "Source API did not return the selected type range")
        require(experiment["name"] in selected_source["text"], "Selected type source text does not contain its declaration")
        declared_parameters = [item for item in experiment["components"] if item.get("variability") == "parameter"]
        if project.name == "modelica-t0-single-joint":
            require([item["name"] for item in declared_parameters] == ["voltageLimit", "gearRatio"], "T0 model parameters do not match the Modelica declaration")
            require([item.get("binding") for item in declared_parameters] == ["12", "30"], "T0 model parameter bindings were not indexed")
            motor_inertia = next(item for item in experiment["components"] if item["name"] == "motorInertia")
            require(motor_inertia.get("modifiers", {}).get("phi.start") == "0", "Nested component initial values were not indexed")
            require(motor_inertia.get("modifiers", {}).get("w.fixed") == "true", "Nested component initialization flags were not indexed")
        placed = sum(1 for item in experiment["components"] if item.get("placement"))
        routed = sum(1 for item in experiment["connections"] if item.get("points"))
        icon_types = {item["type"] for item in experiment["components"] if item.get("placement")}
        rendered_icons = sum(1 for item in icon_types if tree.get("typeIcons", {}).get(item, {}).get("graphics"))
        ellipse_count = sum(1 for item in tree.get("typeIcons", {}).values() for graphic in item.get("graphics", []) if graphic.get("type") == "ellipse" and len(graphic.get("extent", [])) == 2)
        require(experiment.get("diagram") and placed > 0, "Default experiment has no Modelica diagram placement data")
        require(routed == len(experiment["connections"]), "Some connections have no Modelica line route")
        require(rendered_icons >= max(1, int(len(icon_types) * .8)), "Standard-library icon coverage is below 80 percent")
        require(ellipse_count > 0, "Inherited circular standard-library graphics were not indexed")
        limpid = tree.get("typeIcons", {}).get("Modelica.Blocks.Continuous.LimPID")
        if limpid:
            require(len(limpid.get("parameters", [])) >= 20, "Standard-library component parameters were not indexed")
            strict_parameter = next((item for item in limpid.get("parameters", []) if item.get("name") == "strict"), None)
            require(strict_parameter is not None and strict_parameter.get("tab") == "Advanced", "Advanced parameter Dialog metadata was not indexed")
            initial_parameter = next((item for item in limpid.get("parameters", []) if item.get("name") == "y_start"), None)
            require(initial_parameter is not None and initial_parameter.get("group") == "Initialization", "Parameter Dialog group metadata was not indexed")
            feed_forward = next((item for item in limpid.get("connectors", []) if item.get("name") == "u_ff"), None)
            require(feed_forward is not None and feed_forward.get("condition") == "withFeedForward", "Conditional LimPID feed-forward connector was not indexed")
            require(limpid.get("defaults", {}).get("withFeedForward") == "false", "LimPID Boolean defaults were not indexed")
        require(project_info["name"] not in bundle_text, "Project-specific name was compiled into browser bundle")
        _, _, configuration_bytes = fetch(base + "/api/v1/model/configuration")
        configuration = json.loads(configuration_bytes)
        require(configuration["modelClass"] == tree["defaultExperiment"], "Model configuration does not target the default experiment")
        require(configuration["outputs"], "Model outputs were not derived from Modelica declarations")
        _, _, history_bytes = fetch(base + "/deep-link/results")
        require("Modelica 模型浏览器" in history_bytes.decode("utf-8"), "SPA history fallback failed")
        return {"projectId": project_info["projectId"], "projectName": project_info["name"], "assetCount": len(asset_paths), "assetBytes": sum(asset_sizes), "outputCount": len(configuration["outputs"]), "placedComponentCount": placed, "routedConnectionCount": routed, "standardIconTypeCount": rendered_icons}
    finally:
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()


def main() -> int:
    require((WEB_ROOT / "index.html").is_file(), "Web app is not built")
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    scratch = AIDC_ROOT / ".codex-runtime" / f"t1-3-{stamp}"
    scratch.mkdir(parents=True)
    projects = [AIDC_ROOT / "samples" / "modelica-t0-single-joint", AIDC_ROOT / "samples" / "modelica-t1-dc-motor"]
    items = [verify_project(project, scratch) for project in projects]
    result = {"passed": True, "builtAt": datetime.now().isoformat(), "projects": items}
    RESULTS.mkdir(parents=True, exist_ok=True)
    (RESULTS / "t1-3-summary.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
