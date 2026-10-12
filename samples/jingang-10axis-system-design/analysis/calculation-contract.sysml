package JingangNative10ParameterAnalysisContract {

    calc def ComputeTotalMass {
        in servoCount : Integer;
        in servoUnitMass :> ISQ::mass;
        in otherPlannedMass :> ISQ::mass;
        return totalMass :> ISQ::mass = servoCount * servoUnitMass + otherPlannedMass;
    }

    calc def ComputeEndurance {
        in batteryVoltage :> ISQ::electricPotential;
        in batteryCapacity :> ISQ::electricCharge;
        in usableEnergyFraction : Real;
        in electronicsAverageCurrent :> ISQ::electricCurrent;
        in servoBusAverageCurrent :> ISQ::electricCurrent;

        attribute nominalEnergy :> ISQ::energy = batteryVoltage * batteryCapacity;
        attribute usableEnergy :> ISQ::energy = nominalEnergy * usableEnergyFraction;
        attribute averagePower :> ISQ::power = batteryVoltage *
            (electronicsAverageCurrent + servoBusAverageCurrent);
        return endurance :> ISQ::duration = usableEnergy / averagePower;
    }

    calc def ComputePeakElectricalPower {
        in batteryVoltage :> ISQ::electricPotential;
        in electronicsPeakCurrent :> ISQ::electricCurrent;
        in servoBusPeakCurrent :> ISQ::electricCurrent;
        return peakElectricalPower :> ISQ::power = batteryVoltage *
            (electronicsPeakCurrent + servoBusPeakCurrent);
    }

    calc def ComputeStopResponseTime {
        in commandDetectionTime :> ISQ::duration;
        in controlProcessingTime :> ISQ::duration;
        in actuatorStopResponseTime :> ISQ::duration;
        return stopResponseTime :> ISQ::duration = commandDetectionTime +
            controlProcessingTime + actuatorStopResponseTime;
    }

    part def C0DesignInputs {

        attribute servoCount : Integer = 10;
        attribute servoUnitMass :> ISQ::mass = 24.5[g];
        attribute otherPlannedMass :> ISQ::mass = 572[g];
        attribute maximumMass :> ISQ::mass = 800[g];

        attribute batteryVoltage :> ISQ::electricPotential = 6.4[V];
        attribute batteryCapacity :> ISQ::electricCharge = 1.8[Ah];
        attribute usableEnergyFraction : Real = 0.8;
        attribute electronicsAverageCurrent :> ISQ::electricCurrent = 0.5[A];
        attribute servoBusAverageCurrent :> ISQ::electricCurrent = 0.25[A];
        attribute minimumEndurance :> ISQ::duration = 2[h];

        attribute electronicsPeakCurrent :> ISQ::electricCurrent = 0.5[A];
        attribute servoBusPeakCurrent :> ISQ::electricCurrent = 2.6[A];
        attribute maximumSharedCurrent :> ISQ::electricCurrent = 3[A];

        attribute commandDetectionTime :> ISQ::duration = 5[ms];
        attribute controlProcessingTime :> ISQ::duration = 3[ms];
        attribute actuatorStopResponseTime :> ISQ::duration = 120[ms];
        attribute maximumStopTime :> ISQ::duration = 500[ms];
    }

    part c0Configuration : C0DesignInputs;

    analysis coreParameterAnalysis {
        subject = c0Configuration;

        attribute totalMassResult :> ISQ::mass = ComputeTotalMass(
            c0Configuration.servoCount,
            c0Configuration.servoUnitMass,
            c0Configuration.otherPlannedMass);
        attribute massMargin :> ISQ::mass = c0Configuration.maximumMass - totalMassResult;
        constraint massWithinLimit { totalMassResult <= c0Configuration.maximumMass }

        attribute enduranceResult :> ISQ::duration = ComputeEndurance(
            c0Configuration.batteryVoltage,
            c0Configuration.batteryCapacity,
            c0Configuration.usableEnergyFraction,
            c0Configuration.electronicsAverageCurrent,
            c0Configuration.servoBusAverageCurrent);
        attribute enduranceMargin :> ISQ::duration = enduranceResult - c0Configuration.minimumEndurance;
        constraint enduranceMeetsMinimum { enduranceResult >= c0Configuration.minimumEndurance }

        attribute peakElectricalPowerResult :> ISQ::power = ComputePeakElectricalPower(
            c0Configuration.batteryVoltage,
            c0Configuration.electronicsPeakCurrent,
            c0Configuration.servoBusPeakCurrent);
        attribute maximumPeakElectricalPower :> ISQ::power =
            c0Configuration.batteryVoltage * c0Configuration.maximumSharedCurrent;
        attribute peakElectricalPowerMargin :> ISQ::power =
            maximumPeakElectricalPower - peakElectricalPowerResult;
        constraint peakElectricalPowerWithinLimit {
            peakElectricalPowerResult <= maximumPeakElectricalPower
        }

        attribute stopResponseTimeResult :> ISQ::duration = ComputeStopResponseTime(
            c0Configuration.commandDetectionTime,
            c0Configuration.controlProcessingTime,
            c0Configuration.actuatorStopResponseTime);
        attribute stopResponseMargin :> ISQ::duration =
            c0Configuration.maximumStopTime - stopResponseTimeResult;
        constraint stopResponseWithinLimit {
            stopResponseTimeResult <= c0Configuration.maximumStopTime
        }
    }
}
