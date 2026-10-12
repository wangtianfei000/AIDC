"""Compare one MachineDuck OMC result with the frozen Native12 MuJoCo model.

The check intentionally runs both engines from the same STAND keyframe and
holds the twelve position targets fixed.  It is a parity gate, not a unit test:
the generated JSON records the source artifacts and every applied threshold.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import pathlib
import sys
from typing import Iterable

import mujoco
import numpy as np


HERE = pathlib.Path(__file__).resolve().parent
ROB_ROOT = HERE.parents[2]
DEFAULT_MUJOCO_ROOT = ROB_ROOT / "Jingang" / "apps" / "sim-cpu"
RESULTS = HERE / "results"


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def latest_series() -> pathlib.Path:
    candidates = sorted(
        RESULTS.glob("*-MachineDuck-*/series.csv"),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    if not candidates:
        raise FileNotFoundError("no MachineDuck series.csv result found")
    return candidates[0]


def read_final_row(path: pathlib.Path) -> tuple[dict[str, float], float]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.DictReader(stream))
    if not rows:
        raise ValueError(f"empty Modelica result: {path}")
    return {name: float(value) for name, value in rows[-1].items()}, float(rows[-1]["time"])


def values(row: dict[str, float], stem: str, count: int) -> np.ndarray:
    return np.asarray([row[f"{stem}[{index}]"] for index in range(1, count + 1)], dtype=float)


def sensor(model: mujoco.MjModel, data: mujoco.MjData, name: str) -> np.ndarray:
    sensor_id = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_SENSOR, name)
    address = int(model.sensor_adr[sensor_id])
    dimension = int(model.sensor_dim[sensor_id])
    return np.asarray(data.sensordata[address : address + dimension], dtype=float).copy()


def quaternion_error(a: Iterable[float], b: Iterable[float]) -> float:
    qa = np.asarray(tuple(a), dtype=float)
    qb = np.asarray(tuple(b), dtype=float)
    qa /= np.linalg.norm(qa)
    qb /= np.linalg.norm(qb)
    return 2.0 * math.acos(float(np.clip(abs(np.dot(qa, qb)), -1.0, 1.0)))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--series", type=pathlib.Path, default=None)
    parser.add_argument("--mujoco-root", type=pathlib.Path, default=DEFAULT_MUJOCO_ROOT)
    parser.add_argument("--output", type=pathlib.Path, default=RESULTS / "t2-mujoco-parity.json")
    args = parser.parse_args()

    series_path = (args.series or latest_series()).resolve()
    row, duration = read_final_row(series_path)
    run_manifest_path = series_path.parent / "run-manifest.json"
    run_manifest = json.loads(run_manifest_path.read_text(encoding="utf-8"))
    app_root = args.mujoco_root.resolve()
    model_root = app_root / "models" / "microduck_12axis_native"
    scene = model_root / "mujoco" / "scene.xml"
    robot = model_root / "mujoco" / "robot.xml"
    rigid_path = model_root / "rigid_bodies.json"
    sys.path.insert(0, str(app_root))
    from sim.actuators.mujoco_hd1910 import NativeHD1910Controller

    rigid = json.loads(rigid_path.read_text(encoding="utf-8"))
    names = rigid["active_joints"]
    model = mujoco.MjModel.from_xml_path(str(scene))
    data = mujoco.MjData(model)
    mujoco.mj_resetDataKeyframe(model, data, 0)
    mujoco.mj_forward(model, data)
    controller = NativeHD1910Controller(model, names)
    qpos_addresses = [
        int(model.jnt_qposadr[mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, name)])
        for name in names
    ]
    targets = np.asarray(data.qpos[qpos_addresses], dtype=float).copy()
    state = None
    while data.time < duration - 1e-12:
        state = controller.step(data, targets)
        mujoco.mj_step(model, data)
    assert state is not None

    modelica_base = values(row, "basePosition", 3)
    modelica_joint = values(row, "jointPosition", 12)
    modelica_quaternion = values(row, "imuOrientationQuaternion", 4)
    modelica_gyro = values(row, "imuAngularVelocity", 3)
    modelica_accel = values(row, "imuSpecificForce", 3)
    modelica_velocity = values(row, "imuLinearVelocity", 3)
    mujoco_joint = np.asarray(state.positions_rad, dtype=float)
    joint_error = modelica_joint - mujoco_joint

    metrics = {
        "basePositionErrorNorm_m": float(np.linalg.norm(modelica_base - data.qpos[:3])),
        "jointPositionRmsError_rad": float(np.sqrt(np.mean(joint_error**2))),
        "jointPositionMaxError_rad": float(np.max(np.abs(joint_error))),
        "imuOrientationError_rad": quaternion_error(modelica_quaternion, sensor(model, data, "imu_orientation")),
        "imuAngularVelocityErrorNorm_rad_s": float(np.linalg.norm(modelica_gyro - sensor(model, data, "imu_ang_vel"))),
        "imuAccelerationErrorNorm_m_s2": float(np.linalg.norm(modelica_accel - sensor(model, data, "imu_accel"))),
        "imuLinearVelocityErrorNorm_m_s": float(np.linalg.norm(modelica_velocity - sensor(model, data, "imu_lin_vel"))),
        "batteryCurrentError_A": abs(float(row["totalCurrent"]) - state.power.battery.delivered_current_a),
    }
    thresholds = {
        "basePositionErrorNorm_m": 0.010,
        "jointPositionRmsError_rad": 0.005,
        "jointPositionMaxError_rad": 0.010,
        "imuOrientationError_rad": 0.10,
        "imuAngularVelocityErrorNorm_rad_s": 0.50,
        "imuAccelerationErrorNorm_m_s2": 5.0,
        "imuLinearVelocityErrorNorm_m_s": 0.25,
        "batteryCurrentError_A": 0.25,
    }
    checks = {name: metrics[name] <= limit for name, limit in thresholds.items()}
    report = {
        "schemaVersion": "1.0",
        "scope": "STAND hold parity; contact-rich locomotion requires additional scenarios",
        "duration_s": duration,
        "passed": all(checks.values()),
        "checks": checks,
        "metrics": metrics,
        "thresholds": thresholds,
        "artifacts": {
            "modelicaSeries": str(series_path),
            "modelicaSeriesSha256": sha256(series_path),
            "modelicaRunManifest": str(run_manifest_path),
            "modelicaSourceDigest": run_manifest["sourceDigest"],
            "mujocoRobot": str(robot),
            "mujocoRobotSha256": sha256(robot),
            "mujocoScene": str(scene),
            "mujocoSceneSha256": sha256(scene),
        },
        "states": {
            "modelicaBasePosition_m": modelica_base.tolist(),
            "mujocoBasePosition_m": np.asarray(data.qpos[:3], dtype=float).tolist(),
            "modelicaJointPosition_rad": modelica_joint.tolist(),
            "mujocoJointPosition_rad": mujoco_joint.tolist(),
            "modelicaActiveFootContacts": [
                int(row["leftContact.activeContactCount"]),
                int(row["rightContact.activeContactCount"]),
            ],
            "mujocoContactCount": int(data.ncon),
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
