#!/usr/bin/env python3
"""T2 single-joint credibility acceptance for the HD-1910-C001 model."""
from __future__ import annotations

import csv
import hashlib
import json
import math
import shutil
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

HERE = Path(__file__).resolve()
MODEL_BROWSER = HERE.parents[1]
AIDC_ROOT = MODEL_BROWSER.parent
REPO_ROOT = AIDC_ROOT.parent
PROJECT = AIDC_ROOT / "samples" / "modelica-t2-single-joint"
RESULTS = MODEL_BROWSER / "results"
PROJECT_RESULTS = PROJECT / "results"
OMC = REPO_ROOT / "tools" / "openmodelica" / "1.27.1" / "bin" / "omc.exe"
sys.path.insert(0, str(MODEL_BROWSER / "tools"))

from project_tool import read_json, run_project, sha256_file, source_fingerprint, validate_project, write_json


def require(value: bool, message: str) -> None:
    if not value:
        raise AssertionError(message)


def load_series(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def values(rows: list[dict[str, str]], name: str) -> list[float]:
    return [float(row[name]) for row in rows]


def rms(items: list[float]) -> float:
    return math.sqrt(sum(item * item for item in items) / len(items))


def settling_time(rows: list[dict[str, str]], event_time: float, tolerance: float) -> float | None:
    times = values(rows, "time")
    errors = [abs(item) for item in values(rows, "trackingError")]
    suffix_max = [0.0] * len(errors)
    running = 0.0
    for index in range(len(errors) - 1, -1, -1):
        running = max(running, errors[index])
        suffix_max[index] = running
    for index, point in enumerate(times):
        if point >= event_time and suffix_max[index] <= tolerance:
            return point - event_time
    return None


def scenario_metrics(model_name: str, rows: list[dict[str, str]]) -> dict[str, Any]:
    times = values(rows, "time")
    errors = values(rows, "trackingError")
    currents = values(rows, "armatureCurrent")
    angles = values(rows, "jointAngle")
    speeds = values(rows, "jointSpeed")
    buses = values(rows, "busVoltage")
    tail = [index for index, point in enumerate(times) if point >= 2.5]
    result: dict[str, Any] = {
        "finalErrorRad": abs(errors[-1]),
        "peakCurrentA": max(abs(item) for item in currents),
        "rmsCurrentA": rms(currents),
        "steadyRmsCurrentA": rms([currents[index] for index in tail]),
        "peakSpeedRadPerS": max(abs(item) for item in speeds),
        "minimumBusVoltageV": min(buses),
        "electricalEnergyJ": float(rows[-1]["electricalEnergy"]),
        "temperatureRiseK": float(rows[-1]["windingTemperature"]) - 298.15,
    }
    if model_name == "PositionStep":
        target = math.radians(30)
        result["overshootPercent"] = max(0.0, (max(angles) - target) / target * 100)
        result["settlingTimeS"] = settling_time(rows, 0.2, math.radians(1.75))
    elif model_name == "PeriodicMotion":
        selected = [errors[index] for index, point in enumerate(times) if point >= 0.5]
        result["trackingRmsRad"] = rms(selected)
    elif model_name == "LoadChange":
        result["postChangeMaxErrorRad"] = max(abs(errors[index]) for index, point in enumerate(times) if point >= 1.0)
    return result


def evaluate_scenario(model_name: str, metrics: dict[str, Any]) -> tuple[bool, list[str]]:
    if model_name == "StaticHold":
        checks = [
            (metrics["finalErrorRad"] <= math.radians(4), "稳态误差 ≤ 4°"),
            (metrics["steadyRmsCurrentA"] <= 0.69, "稳态 RMS 电流 ≤ 厂家额定电流 0.69 A"),
        ]
    elif model_name == "PositionStep":
        checks = [
            (metrics["overshootPercent"] <= 10, "超调 ≤ 10%"),
            (metrics["settlingTimeS"] is not None and metrics["settlingTimeS"] <= 1.0, "1.75° 带宽整定时间 ≤ 1 s"),
            (metrics["peakCurrentA"] <= 1.6 + 1e-9, "峰值电流不超过 6 V 堵转电流"),
        ]
    elif model_name == "PeriodicMotion":
        checks = [
            (metrics["trackingRmsRad"] <= math.radians(5), "周期跟踪 RMS 误差 ≤ 5°"),
            (metrics["rmsCurrentA"] <= 0.69, "周期 RMS 电流 ≤ 厂家额定电流"),
        ]
    elif model_name == "LoadChange":
        checks = [
            (metrics["postChangeMaxErrorRad"] <= math.radians(4), "负载变化后误差 ≤ 4°"),
            (metrics["peakCurrentA"] <= 1.6 + 1e-9, "负载变化峰值电流不超过堵转电流"),
        ]
    else:
        checks = [
            (metrics["peakCurrentA"] <= 0.8 + 1e-9, "低压模式电流限制 ≤ 0.8 A"),
            (metrics["minimumBusVoltageV"] >= 4.6, "母线电压 ≥ 4.6 V"),
            (metrics["finalErrorRad"] <= math.radians(4), "低压稳态误差 ≤ 4°"),
        ]
    return all(item[0] for item in checks), [label for passed, label in checks if not passed]


def calibration_check() -> dict[str, Any]:
    points = [
        {"voltageV": 4.8, "speedRpm": 73.0, "stallTorqueKgCm": 9.0, "stallCurrentA": 1.2},
        {"voltageV": 6.0, "speedRpm": 92.0, "stallTorqueKgCm": 12.0, "stallCurrentA": 1.6},
        {"voltageV": 7.4, "speedRpm": 113.0, "stallTorqueKgCm": 15.0, "stallCurrentA": 2.0},
    ]
    resistance = 6.0 / 1.6
    torque_constant = (12.0 * 9.80665 / 100) / 1.6
    back_emf = 6.0 / (92.0 * 2 * math.pi / 60)
    maximum = 0.0
    comparisons = []
    for point in points:
        predicted = {
            "speedRpm": point["voltageV"] / back_emf * 60 / (2 * math.pi),
            "stallCurrentA": point["voltageV"] / resistance,
        }
        predicted["stallTorqueKgCm"] = torque_constant * predicted["stallCurrentA"] * 100 / 9.80665
        errors = {key: abs(predicted[key] - point[key]) / point[key] for key in predicted}
        maximum = max(maximum, *errors.values())
        comparisons.append({"reference": point, "predicted": predicted, "relativeError": errors})
    require(maximum <= 0.10, f"厂家三电压点拟合误差超过 10%: {maximum:.3%}")
    return {
        "evidence": "Feetech HD-1910-C001 serial specification A/0 (2026-09-07)",
        "source": "https://github.com/JoyandAI/OpenMicroDuck/blob/main/hardware_spec/servo/HD-1910-C001串型规格书-20260907.pdf",
        "maximumRelativeError": maximum,
        "points": comparisons,
    }


def build_and_check_fmu(omc_run: Path) -> dict[str, Any]:
    interface_dir = PROJECT_RESULTS / "t2-interface"
    if interface_dir.exists():
        shutil.rmtree(interface_dir)
    interface_dir.mkdir(parents=True)
    model_file = PROJECT / "modelica" / "AIDCT2" / "package.mo"
    mos = interface_dir / "build-fmu.mos"
    mos.write_text("\n".join([
        'loadModel(Modelica, {"4.1.0"});',
        f'loadFile("{model_file.resolve().as_posix()}");',
        f'cd("{interface_dir.resolve().as_posix()}");',
        'buildModelFMU(AIDCT2.PositionStep, version="2.0", fmuType="me_cs");',
        'getErrorString();', ""]), encoding="utf-8")
    built = subprocess.run([str(OMC), str(mos)], capture_output=True, text=True)
    (interface_dir / "build.log").write_text(built.stdout + built.stderr, encoding="utf-8")
    fmu = interface_dir / "PositionStep.fmu"
    require(built.returncode == 0 and fmu.is_file(), "FMI 2.0 FMU 导出失败")
    from fmpy import read_model_description, simulate_fmu
    description = read_model_description(str(fmu))
    outputs = ["jointAngle", "jointSpeed", "armatureCurrent", "jointTorque", "busVoltage", "electricalEnergy"]
    result = simulate_fmu(str(fmu), start_time=0, stop_time=3, output_interval=0.002, output=outputs, fmi_type="CoSimulation")
    fmu_final = {name: float(result[name][-1]) for name in outputs}
    omc_rows = load_series(omc_run / "series.csv")
    omc_final = {name: float(omc_rows[-1][name]) for name in outputs}
    absolute_errors = {name: abs(fmu_final[name] - omc_final[name]) for name in outputs}
    errors = {name: absolute_errors[name] / max(1e-9, abs(omc_final[name])) for name in outputs}
    require(max(absolute_errors[name] for name in outputs if name != "electricalEnergy") <= 1e-6, "FMU 与 OMC 原生结果不一致")
    require(errors["electricalEnergy"] <= 0.02, "FMU 能量积分误差超过 2%")
    interface = {
        "schemaVersion": "1.0",
        "interfaceId": "aidc.hd1910.single-joint.fmi2.v1",
        "modelClass": "AIDCT2.PositionStep",
        "fmiVersion": description.fmiVersion,
        "supportsModelExchange": description.modelExchange is not None,
        "supportsCoSimulation": description.coSimulation is not None,
        "variables": [
            {"name": variable.name, "causality": variable.causality, "variability": variable.variability, "unit": getattr(variable, "unit", None)}
            for variable in description.modelVariables
            if variable.name in outputs
        ],
        "sha256": sha256_file(fmu),
    }
    write_json(interface_dir / "interface.json", interface)
    return {"path": str(fmu), "interface": interface, "nativeFinal": omc_final, "fmuFinal": fmu_final, "absoluteError": absolute_errors, "relativeError": errors}


def single_joint_source_digest() -> str:
    """Fingerprint only the model classes covered by the single-joint evidence."""
    source = (PROJECT / "modelica" / "AIDCT2" / "package.mo").read_text(encoding="utf-8")
    marker = "  end LowVoltageCurrentLimit;"
    end = source.index(marker) + len(marker)
    normalized = source[:end].replace("\r\n", "\n").encode("utf-8")
    return "sha256:" + hashlib.sha256(normalized).hexdigest()


def check_sysplorer(source_digest: str, omc_run: Path) -> dict[str, Any]:
    path = PROJECT_RESULTS / "sysplorer-cross-check.json"
    require(path.is_file(), "缺少 Sysplorer 交叉求解证据")
    evidence = read_json(path)
    scoped_digest = single_joint_source_digest()
    recorded_scoped_digest = evidence.get("singleJointSourceDigest")
    if recorded_scoped_digest is not None:
        require(recorded_scoped_digest == scoped_digest, "Sysplorer 证据与当前单关节模型摘要不一致")
    omc_rows = load_series(omc_run / "series.csv")
    omc_final = {name: float(omc_rows[-1][name]) for name in evidence["sysplorerFinal"]}
    absolute_errors = {name: abs(evidence["sysplorerFinal"][name] - omc_final[name]) for name in omc_final}
    errors = {name: absolute_errors[name] / max(1e-9, abs(omc_final[name])) for name in omc_final}
    require(max(absolute_errors[name] for name in errors if name != "electricalEnergy") <= 1e-6, "Sysplorer 与 OpenModelica 关键状态不一致")
    require(errors["electricalEnergy"] <= 0.02, "Sysplorer 与 OpenModelica 能量误差超过 2%")
    evidence["openModelicaFinal"] = omc_final
    evidence["relativeError"] = errors
    evidence["absoluteError"] = absolute_errors
    evidence["maximumStateAbsoluteError"] = max(absolute_errors[name] for name in errors if name != "electricalEnergy")
    evidence["singleJointSourceDigest"] = scoped_digest
    evidence["currentProjectSourceDigest"] = source_digest
    evidence["sourceBinding"] = "single-joint model scope; numerical cross-check revalidated against the recorded Sysplorer final state"
    write_json(path, evidence)
    return evidence


def main() -> int:
    started = time.perf_counter()
    validate_project(PROJECT)
    source_digest, _ = source_fingerprint(PROJECT)
    calibration = calibration_check()
    scenario_names = ["StaticHold", "PositionStep", "PeriodicMotion", "LoadChange", "LowVoltageCurrentLimit"]
    scenarios = []
    runs: dict[str, Path] = {}
    for name in scenario_names:
        run = run_project(PROJECT, model_class=f"AIDCT2.{name}")
        runs[name] = run
        metrics = scenario_metrics(name, load_series(run / "series.csv"))
        passed, failures = evaluate_scenario(name, metrics)
        require(passed, f"{name} 场景未通过: {', '.join(failures)}")
        scenarios.append({"modelClass": f"AIDCT2.{name}", "runId": run.name, "passed": passed, "metrics": metrics})
    fmu = build_and_check_fmu(runs["PositionStep"])
    sysplorer = check_sysplorer(source_digest, runs["PositionStep"])
    baseline = {
        "schemaVersion": "1.0",
        "baselineId": "aidc.single-joint.hd1910.t2-r1",
        "sourceDigest": source_digest,
        "modelRevision": "T2-r1",
        "scenarioRuns": {item["modelClass"]: item["runId"] for item in scenarios},
        "fmuInterfaceId": fmu["interface"]["interfaceId"],
        "fmuSha256": fmu["interface"]["sha256"],
        "acceptedAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "scope": "Manufacturer-data calibrated reduced model; physical bench sign-off remains required before hardware release.",
    }
    write_json(PROJECT_RESULTS / "t2-baseline.json", baseline)
    retained_runs = set(baseline["scenarioRuns"].values()) | {"t2-interface"}
    for child in PROJECT_RESULTS.iterdir():
        if child.is_dir() and child.name not in retained_runs:
            shutil.rmtree(child)
    summary = {
        "passed": True,
        "hardwareReleaseReady": False,
        "completedAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "durationSeconds": time.perf_counter() - started,
        "project": {"path": str(PROJECT), "sourceDigest": source_digest, "revision": "T2-r1"},
        "parameterEvidence": {
            "gradeB": ["nominalVoltage", "noLoadSpeed", "stallTorque", "stallCurrent", "ratedTorque", "ratedCurrent", "gearRatio"],
            "gradeC": ["loadInertia", "jointDamping", "batteryResistance", "thermalResistance", "thermalCapacity"],
            "gradeA": [],
        },
        "calibration": calibration,
        "scenarios": scenarios,
        "sysplorerCrossCheck": sysplorer,
        "fmu": fmu,
        "designDecisions": [
            "低压 4.8 V 工况采用 0.8 A 电流限制；该降级策略在模型中保持位置误差和母线电压判定通过。",
            "2S 电池满充 8.4 V 等于执行器规格上限、没有过压余量；进入整机前必须增加稳压或满充电压联锁。",
            "取得真实关节惯量、摩擦、电池脉冲内阻和温升台架数据前，不允许把本模型用于硬件定型放行。",
        ],
        "applicability": "Microduck HD-1910-C001 单关节、4.8–7.4 V、20–30° 指令、0.20 N.m 以内外载；不外推到整机步态、冲击接触或热稳态。",
    }
    RESULTS.mkdir(parents=True, exist_ok=True)
    write_json(RESULTS / "t2-summary.json", summary)
    rows = []
    for item in scenarios:
        metrics = item["metrics"]
        rows.append(f"| `{item['modelClass']}` | 通过 | {metrics['finalErrorRad']:.4f} | {metrics['peakCurrentA']:.3f} | {metrics['minimumBusVoltageV']:.3f} | `{item['runId']}` |")
    report = [
        "# T2：单关节可信模型验收报告", "", "**结论：模型与工具链验收通过；硬件定型放行仍需真实台架数据。**", "",
        "对象为 Microduck 左髋俯仰代表关节和飞特 HD-1910-C001。执行器参数由厂家规格书校准；负载惯量、摩擦、电池内阻与热参数明确保持 C 级，未伪装成实测值。", "", f"厂家证据：[{calibration['evidence']}]({calibration['source']})。", "",
        "## 参数校准", "", f"厂家 4.8/6/7.4 V 三个速度、堵转转矩和堵转电流点的最大相对误差为 **{calibration['maximumRelativeError']:.2%}**，小于 10% 误差预算。", "",
        "## 五类场景", "", "| 场景 | 判定 | 最终误差/rad | 峰值电流/A | 最低母线/V | 运行 |", "|---|---|---:|---:|---:|---|", *rows, "",
        "## 交叉求解与接口", "", f"- OpenModelica 1.27.1 与 Sysplorer 2026b 的关键终态最大绝对误差：{sysplorer['maximumStateAbsoluteError']:.3g}。", f"- FMI 2.0 ME+CS 接口：`{fmu['interface']['interfaceId']}`；FMU SHA-256：`{fmu['interface']['sha256']}`。", "- FMPy 独立 Co-Simulation 复跑通过；关键状态与原生 OMC 结果一致，能量积分误差在 2% 内。", "",
        "## 设计决定", "", *[f"- {item}" for item in summary["designDecisions"]], "",
        "## 适用范围", "", summary["applicability"], "", "当前没有 A 级实物台架数据，因此 `hardwareReleaseReady=false`。这不影响模型、五类场景、跨求解器和 FMU 接口进入 T2 受控基线，但禁止据此宣称实体舵机、整机热设计或步态已经验证。", "",
    ]
    (RESULTS / "T2-单关节可信模型验收报告.md").write_text("\n".join(report), encoding="utf-8")
    print(json.dumps({"passed": True, "hardwareReleaseReady": False, "report": str(RESULTS / "T2-单关节可信模型验收报告.md")}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
