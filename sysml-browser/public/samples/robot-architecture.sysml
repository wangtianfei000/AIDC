package JingangNative10Architecture {
    private import Views::*;
    private import StandardViewDefinitions::*;

    port def ElectricalPowerPort;
    port def ServoBusPort;
    port def MotionCommandPort;
    port def ImuDataPort;

    calc def ComputeTotalMass {
        in actuatorGroupMass :> ISQ::mass;
        in structureAndFootMass :> ISQ::mass;
        in electronicsAndBatteryMass :> ISQ::mass;
        in bearingAndFastenerMass :> ISQ::mass;
        in harnessAndConnectorMass :> ISQ::mass;
        return totalMass :> ISQ::mass = actuatorGroupMass + structureAndFootMass +
            electronicsAndBatteryMass + bearingAndFastenerMass + harnessAndConnectorMass;
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

    part def ServoActuator {
        attribute componentType :> String = "HD-1910-C001";
        attribute publishedUpperMass :> ISQ::mass = 24.5[g];
        attribute massEvidenceStatus :> String = "公开质量范围上界，不参与CAD-2当前质量合计";
        attribute assumedStopResponseTime :> ISQ::duration = 120[ms];
        attribute stopResponseEvidenceStatus :> String = "C3方案假设，等待单舵机台架实测";
        attribute envelopeStatus :> String = "外廓宣传值；安装接口待验证";
        port powerIn;
        port servoBus;
    }

    part def JingangNative10 {
        attribute componentId :> String = "SYS-JG10";
        attribute requirementIds :> String = "REQ-FUN-001,REQ-PER-001,REQ-PHY-001,REQ-SAF-001";
        attribute cadRef :> String = "layout/jingang-10axis-layout.FCStd";

        attribute maximumMass :> ISQ::mass = 800[g];
        attribute minimumEndurance :> ISQ::duration = 2[h];
        attribute maximumStopTime :> ISQ::duration = 500[ms];

        doc /*以下五项来自 CAD-2 分类质量预算，均为规划估计而非实测值。*/
        attribute actuatorGroupPlanningMass :> ISQ::mass = 225[g];
        attribute structureAndFootPlanningMass :> ISQ::mass = 349.8[g];
        attribute electronicsAndBatteryPlanningMass :> ISQ::mass = 172[g];
        attribute bearingAndFastenerPlanningMass :> ISQ::mass = 45[g];
        attribute harnessAndConnectorPlanningMass :> ISQ::mass = 25[g];
        attribute controlPeriod :> ISQ::duration = 5[ms];
        attribute policyPeriod :> ISQ::duration = 20[ms];

        part bodyFrame {
            attribute componentId :> String = "ASM-BODY";
            attribute assemblyRelation :> String = "fixed";
            attribute responsibility :> String = "闭合左右髋载荷并安装电池、计算、控制和感知部件";
            attribute cadRef :> String = "layout://ASM-BODY";
        }

        part leftLeg {
            attribute componentId :> String = "ASM-LEG-L";
            attribute assemblyRelation :> String = "moving";
            attribute responsibility :> String = "左侧支撑、移重和迈步";
            attribute cadRef :> String = "layout://ASM-LEG-L";
            port powerIn;
            port controlIn;

            part leftHipRoll : ServoActuator {
                attribute componentId :> String = "ACT-L-HIP-ROLL";
                attribute jointAxis :> String = "X";
                attribute lowerLimit :> ISQ::angularMeasure = -18[deg];
                attribute upperLimit :> ISQ::angularMeasure = 18[deg];
            }
            part leftHipPitch : ServoActuator {
                attribute componentId :> String = "ACT-L-HIP-PITCH";
                attribute jointAxis :> String = "Y";
                attribute lowerLimit :> ISQ::angularMeasure = -35[deg];
                attribute upperLimit :> ISQ::angularMeasure = 35[deg];
            }
            part leftKneePitch : ServoActuator {
                attribute componentId :> String = "ACT-L-KNEE-PITCH";
                attribute jointAxis :> String = "Y";
                attribute lowerLimit :> ISQ::angularMeasure = 5[deg];
                attribute upperLimit :> ISQ::angularMeasure = 65[deg];
            }
            part leftAnklePitch : ServoActuator {
                attribute componentId :> String = "ACT-L-ANKLE-PITCH";
                attribute jointAxis :> String = "Y";
                attribute lowerLimit :> ISQ::angularMeasure = -30[deg];
                attribute upperLimit :> ISQ::angularMeasure = 30[deg];
            }

            connect powerIn to leftHipRoll.powerIn;
            connect powerIn to leftHipPitch.powerIn;
            connect powerIn to leftKneePitch.powerIn;
            connect powerIn to leftAnklePitch.powerIn;
            connect controlIn to leftHipRoll.servoBus;
            connect controlIn to leftHipPitch.servoBus;
            connect controlIn to leftKneePitch.servoBus;
            connect controlIn to leftAnklePitch.servoBus;
        }

        part rightLeg {
            attribute componentId :> String = "ASM-LEG-R";
            attribute assemblyRelation :> String = "moving";
            attribute responsibility :> String = "右侧支撑、移重和迈步";
            attribute cadRef :> String = "layout://ASM-LEG-R";
            port powerIn;
            port controlIn;

            part rightHipRoll : ServoActuator {
                attribute componentId :> String = "ACT-R-HIP-ROLL";
                attribute jointAxis :> String = "X";
                attribute lowerLimit :> ISQ::angularMeasure = -18[deg];
                attribute upperLimit :> ISQ::angularMeasure = 18[deg];
            }
            part rightHipPitch : ServoActuator {
                attribute componentId :> String = "ACT-R-HIP-PITCH";
                attribute jointAxis :> String = "Y";
                attribute lowerLimit :> ISQ::angularMeasure = -35[deg];
                attribute upperLimit :> ISQ::angularMeasure = 35[deg];
            }
            part rightKneePitch : ServoActuator {
                attribute componentId :> String = "ACT-R-KNEE-PITCH";
                attribute jointAxis :> String = "Y";
                attribute lowerLimit :> ISQ::angularMeasure = 5[deg];
                attribute upperLimit :> ISQ::angularMeasure = 65[deg];
            }
            part rightAnklePitch : ServoActuator {
                attribute componentId :> String = "ACT-R-ANKLE-PITCH";
                attribute jointAxis :> String = "Y";
                attribute lowerLimit :> ISQ::angularMeasure = -30[deg];
                attribute upperLimit :> ISQ::angularMeasure = 30[deg];
            }

            connect powerIn to rightHipRoll.powerIn;
            connect powerIn to rightHipPitch.powerIn;
            connect powerIn to rightKneePitch.powerIn;
            connect powerIn to rightAnklePitch.powerIn;
            connect controlIn to rightHipRoll.servoBus;
            connect controlIn to rightHipPitch.servoBus;
            connect controlIn to rightKneePitch.servoBus;
            connect controlIn to rightAnklePitch.servoBus;
        }

        part headNeck {
            attribute componentId :> String = "ASM-HEAD-NECK";
            attribute assemblyRelation :> String = "moving";
            attribute responsibility :> String = "承载头部并提供颈根俯仰和头部偏航";
            attribute cadRef :> String = "layout://ASM-HEAD-NECK";
            port powerIn;
            port controlIn;

            part neckPitch : ServoActuator {
                attribute componentId :> String = "ACT-NECK-PITCH";
                attribute jointAxis :> String = "local pitch axis";
                attribute lowerLimit :> ISQ::angularMeasure = 5[deg];
                attribute upperLimit :> ISQ::angularMeasure = 35[deg];
            }
            part headYaw : ServoActuator {
                attribute componentId :> String = "ACT-HEAD-YAW";
                attribute jointAxis :> String = "moving local yaw axis";
                attribute lowerLimit :> ISQ::angularMeasure = -60[deg];
                attribute upperLimit :> ISQ::angularMeasure = 60[deg];
            }

            connect powerIn to neckPitch.powerIn;
            connect powerIn to headYaw.powerIn;
            connect controlIn to neckPitch.servoBus;
            connect controlIn to headYaw.servoBus;
        }

        part powerSystem {
            attribute componentId :> String = "SYS-POWER";
            attribute requirementIds :> String = "REQ-PER-001";
            attribute responsibility :> String = "储能、配电、限流和欠压保护";
            attribute batteryVoltage :> ISQ::electricPotential = 6.4[V];
            attribute batteryCapacity :> ISQ::electricCharge = 1.8[Ah];
            attribute maximumSharedCurrent :> ISQ::electricCurrent = 3[A];
            attribute electronicsCurrent :> ISQ::electricCurrent = 0.5[A];
            attribute usableEnergyFraction : Real = 0.8;
            attribute servoBusAverageCurrent :> ISQ::electricCurrent = 0.25[A];
            attribute servoBusPeakCurrent :> ISQ::electricCurrent = 2.6[A];
            attribute powerAnalysisEvidenceStatus :> String = "训练模型基线与C3方案假设，等待实机电流和容量测试";
            attribute envelopeStatus :> String = "NP-F550 类预留；具体型号待选";
            attribute cadRef :> String = "layout://SYS-POWER";
            port servoPowerOut;
            port logicPowerOut;
        }

        part computeSystem {
            attribute componentId :> String = "SYS-COMPUTE";
            attribute responsibility :> String = "接收任务命令并运行策略推理";
            attribute envelopeStatus :> String = "主控参考外形已导入；实际版本和散热配置待确认";
            attribute cadRef :> String = "layout://SYS-COMPUTE";
            port powerIn;
            port commandOut;
        }

        part motionController {
            attribute componentId :> String = "SYS-MOTION-CONTROL";
            attribute requirementIds :> String = "REQ-FUN-001,REQ-SAF-001";
            attribute responsibility :> String = "执行 200 Hz 关节控制、10 轴总线通信和停止处理";
            attribute updatePeriod :> ISQ::duration = 5[ms];
            attribute commandDetectionTime :> ISQ::duration = 5[ms];
            attribute controlProcessingTime :> ISQ::duration = 3[ms];
            attribute timingEvidenceStatus :> String = "命令检测取控制周期，处理时间为C3方案假设";
            attribute envelopeStatus :> String = "具体控制板 SKU 待选";
            attribute cadRef :> String = "layout://SYS-MOTION-CONTROL";
            port powerIn;
            port commandIn;
            port imuIn;
            port servoBus;
        }

        part sensingSystem {
            attribute componentId :> String = "SYS-SENSING";
            attribute responsibility :> String = "测量躯干角速度和重力方向";
            attribute installationRequirement :> String = "固定于躯干并建立可标定坐标系";
            attribute envelopeStatus :> String = "IMU 桥接模块预留；具体 SKU 待选";
            attribute cadRef :> String = "layout://SYS-SENSING";
            port powerIn;
            port imuDataOut;
        }

        connect powerSystem.servoPowerOut to leftLeg.powerIn;
        connect powerSystem.servoPowerOut to rightLeg.powerIn;
        connect powerSystem.servoPowerOut to headNeck.powerIn;
        connect powerSystem.logicPowerOut to computeSystem.powerIn;
        connect powerSystem.logicPowerOut to motionController.powerIn;
        connect powerSystem.logicPowerOut to sensingSystem.powerIn;
        connect computeSystem.commandOut to motionController.commandIn;
        connect sensingSystem.imuDataOut to motionController.imuIn;
        connect motionController.servoBus to leftLeg.controlIn;
        connect motionController.servoBus to rightLeg.controlIn;
        connect motionController.servoBus to headNeck.controlIn;
    }

    part jingangConfiguration : JingangNative10;

    analysis coreParameterAnalysis {
        subject = jingangConfiguration;

        attribute totalMassResult :> ISQ::mass = ComputeTotalMass(
            jingangConfiguration.actuatorGroupPlanningMass,
            jingangConfiguration.structureAndFootPlanningMass,
            jingangConfiguration.electronicsAndBatteryPlanningMass,
            jingangConfiguration.bearingAndFastenerPlanningMass,
            jingangConfiguration.harnessAndConnectorPlanningMass);
        attribute massMargin :> ISQ::mass = jingangConfiguration.maximumMass - totalMassResult;
        constraint REQ_PHY_001_massWithinLimit {
            totalMassResult <= jingangConfiguration.maximumMass
        }

        attribute enduranceResult :> ISQ::duration = ComputeEndurance(
            jingangConfiguration.powerSystem.batteryVoltage,
            jingangConfiguration.powerSystem.batteryCapacity,
            jingangConfiguration.powerSystem.usableEnergyFraction,
            jingangConfiguration.powerSystem.electronicsCurrent,
            jingangConfiguration.powerSystem.servoBusAverageCurrent);
        attribute enduranceMargin :> ISQ::duration = enduranceResult - jingangConfiguration.minimumEndurance;
        constraint REQ_PER_001_enduranceMeetsMinimum {
            enduranceResult >= jingangConfiguration.minimumEndurance
        }

        attribute peakElectricalPowerResult :> ISQ::power = ComputePeakElectricalPower(
            jingangConfiguration.powerSystem.batteryVoltage,
            jingangConfiguration.powerSystem.electronicsCurrent,
            jingangConfiguration.powerSystem.servoBusPeakCurrent);
        attribute maximumPeakElectricalPower :> ISQ::power =
            jingangConfiguration.powerSystem.batteryVoltage *
            jingangConfiguration.powerSystem.maximumSharedCurrent;
        attribute peakElectricalPowerMargin :> ISQ::power =
            maximumPeakElectricalPower - peakElectricalPowerResult;
        constraint powerWithinSharedLimit {
            peakElectricalPowerResult <= maximumPeakElectricalPower
        }

        attribute stopResponseTimeResult :> ISQ::duration = ComputeStopResponseTime(
            jingangConfiguration.motionController.commandDetectionTime,
            jingangConfiguration.motionController.controlProcessingTime,
            ServoActuator.assumedStopResponseTime);
        attribute stopResponseMargin :> ISQ::duration =
            jingangConfiguration.maximumStopTime - stopResponseTimeResult;
        constraint REQ_SAF_001_stopResponseWithinLimit {
            stopResponseTimeResult <= jingangConfiguration.maximumStopTime
        }
    }

    rendering coreParameterTableRendering :> asElementTable {
        view parameterColumn :>> columnView { attribute label : String = "参数"; }
        view roleColumn :>> columnView { attribute label : String = "角色"; }
        view currentValueColumn :>> columnView { attribute label : String = "当前值"; }
        view unitColumn :>> columnView { attribute label : String = "单位"; }
        view targetColumn :>> columnView { attribute label : String = "目标"; }
        view marginColumn :>> columnView { attribute label : String = "余量"; }
        view statusColumn :>> columnView { attribute label : String = "状态"; }
        view sourceColumn :>> columnView { attribute label : String = "来源"; }
    }

    view def CoreParameterAnalysisTable :> GridView {
        render coreParameterTableRendering;
    }

    view coreParameterAnalysisTable : CoreParameterAnalysisTable {
        expose JingangNative10Architecture::coreParameterAnalysis::totalMassResult;
        expose JingangNative10Architecture::coreParameterAnalysis::enduranceResult;
        expose JingangNative10Architecture::coreParameterAnalysis::peakElectricalPowerResult;
        expose JingangNative10Architecture::coreParameterAnalysis::stopResponseTimeResult;
        render coreParameterTableRendering;
    }
}

