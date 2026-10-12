#!/usr/bin/env python3
from __future__ import annotations
import copy, json, sys, tempfile
from pathlib import Path
HERE=Path(__file__).resolve();MODEL_BROWSER=HERE.parents[3];AIDC_ROOT=MODEL_BROWSER.parent;sys.path.insert(0,str(MODEL_BROWSER/"tools"))
from project_tool import ContractError, load_schema, validate_document, validate_project
def require(value:bool,message:str)->None:
    if not value:raise AssertionError(message)
def declared_project_entries(project:Path)->set[str]:
    """Return contract-relevant entries, excluding known local tool residue."""
    return {
        item.name
        for item in project.iterdir()
        if item.name != "__pycache__" and not (item.is_file() and item.match("tmp*.mos"))
    }
def main()->int:
    projects=[AIDC_ROOT/"samples"/"modelica-t0-single-joint",AIDC_ROOT/"samples"/"modelica-t1-dc-motor",AIDC_ROOT/"samples"/"modelica-t2-single-joint",AIDC_ROOT/"samples"/"modelica-p2-manual-single-joint",MODEL_BROWSER/"generated"/"p2-single-joint",MODEL_BROWSER/"generated"/"microduck-native12"];loaded=[validate_project(p) for p in projects]
    required={"model-project.json","modelica"};allowed=required|{"results","robot-ir","overrides","generation-diagnostics.json","generation.request.json","compare-mujoco.py"}
    require(all(required<=declared_project_entries(p)<=allowed for p in projects),"模型工程包含未声明的顶层文件")
    invalid=copy.deepcopy(loaded[0]["project"]);invalid["modelica"]["rootPackageFile"]="../escape.mo"
    with tempfile.TemporaryDirectory() as directory:
        path=Path(directory)/"invalid-project.json";path.write_text(json.dumps(invalid),encoding="utf-8")
        try:validate_document(invalid,"model-project.schema.json",path)
        except ContractError:pass
        else:raise AssertionError("路径越界工程必须被拒绝")
    for schema_path in (MODEL_BROWSER/"packages"/"contracts"/"schemas").glob("*.schema.json"):
        require(load_schema(schema_path.name).get("$schema")=="https://json-schema.org/draft/2020-12/schema",f"Schema 草案错误: {schema_path}")
    print(f"contract tests passed: {len(projects)} minimal projects, path traversal rejected")
    return 0
if __name__=="__main__":raise SystemExit(main())
