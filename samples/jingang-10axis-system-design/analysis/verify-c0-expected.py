from __future__ import annotations

import json
from decimal import Decimal
from pathlib import Path


FIXTURE = Path(__file__).with_name("c0-expected-results.json")


def d(value: object) -> Decimal:
    return Decimal(str(value))


data = json.loads(FIXTURE.read_text(encoding="utf-8"))
inputs = {name: d(value) for name, value in data["inputs"].items()}

mass = inputs["servoCount"] * inputs["servoUnitMassG"] + inputs["otherPlannedMassG"]
nominal_energy = inputs["batteryVoltageV"] * inputs["batteryCapacityAh"]
usable_energy = nominal_energy * inputs["usableEnergyFraction"]
average_power = inputs["batteryVoltageV"] * (
    inputs["electronicsAverageCurrentA"] + inputs["servoBusAverageCurrentA"]
)
endurance = usable_energy / average_power
peak_power = inputs["batteryVoltageV"] * (
    inputs["electronicsPeakCurrentA"] + inputs["servoBusPeakCurrentA"]
)
peak_limit = inputs["batteryVoltageV"] * inputs["maximumSharedCurrentA"]
stop_response = (
    inputs["commandDetectionTimeMs"]
    + inputs["controlProcessingTimeMs"]
    + inputs["actuatorStopResponseTimeMs"]
)

actual = {
    "totalMassResult": (mass, inputs["maximumMassG"] - mass, "fail"),
    "enduranceResult": (
        endurance,
        endurance - inputs["minimumEnduranceH"],
        "fail",
    ),
    "peakElectricalPowerResult": (
        peak_power,
        peak_limit - peak_power,
        "fail",
    ),
    "stopResponseTimeResult": (
        stop_response,
        inputs["maximumStopTimeMs"] - stop_response,
        "pass",
    ),
}

tolerance = Decimal("0.000000001")
for row in data["rows"]:
    value, margin, status = actual[row["id"]]
    assert abs(value - d(row["value"])) <= tolerance, row["id"]
    assert abs(margin - d(row["margin"])) <= tolerance, row["id"]
    assert status == row["status"], row["id"]

print("C0 hand-calculation oracle verified: 4/4 rows")
