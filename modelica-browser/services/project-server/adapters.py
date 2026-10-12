from __future__ import annotations

from pathlib import Path
from typing import Any

from project_tool import OMC


def _fmpy():
    try:
        import fmpy  # type: ignore
        return fmpy
    except ImportError:
        return None


def adapter_capabilities() -> list[dict[str, Any]]:
    fmpy = _fmpy()
    return [
        {
            "id": "openmodelica",
            "installed": OMC.is_file(),
            "versionCommand": str(OMC),
            "capabilities": ["model-check", "compile-native", "solve-native", "export-fmi2-me-cs", "model-index"],
        },
        {
            "id": "fmpy",
            "installed": fmpy is not None,
            "version": getattr(fmpy, "__version__", None),
            "capabilities": ["inspect-fmu", "simulate-fmi2-me", "simulate-fmi2-cs"],
        },
    ]


def inspect_fmu(path: Path) -> dict[str, Any]:
    fmpy = _fmpy()
    if fmpy is None:
        raise RuntimeError("FMPy 未安装；当前发行包未启用 FMU 检查能力")
    description = fmpy.read_model_description(path)
    return {
        "fmiVersion": description.fmiVersion,
        "modelName": description.modelName,
        "modelExchange": description.modelExchange is not None,
        "coSimulation": description.coSimulation is not None,
        "variables": [
            {"name": item.name, "causality": item.causality, "variability": item.variability}
            for item in description.modelVariables
        ],
    }
