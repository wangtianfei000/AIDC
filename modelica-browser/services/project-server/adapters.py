from __future__ import annotations

from pathlib import Path
from typing import Any

import fmpy

from project_tool import OMC


def adapter_capabilities() -> list[dict[str, Any]]:
    return [
        {
            "id": "openmodelica",
            "installed": OMC.is_file(),
            "versionCommand": str(OMC),
            "capabilities": ["model-check", "compile-native", "solve-native", "export-fmi2-me-cs", "model-index"],
        },
        {
            "id": "fmpy",
            "installed": True,
            "version": fmpy.__version__,
            "capabilities": ["inspect-fmu", "simulate-fmi2-me", "simulate-fmi2-cs"],
        },
    ]


def inspect_fmu(path: Path) -> dict[str, Any]:
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
