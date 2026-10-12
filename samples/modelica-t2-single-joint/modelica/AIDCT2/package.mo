within ;
package AIDCT2
  "HD-1910-C001 representative single-joint validation model"
  extends Modelica.Icons.Package;

  partial model PartialHD1910Joint
    "Reduced electromechanical model of one Microduck hip-pitch joint"
    parameter Integer scenario = 2 annotation(Evaluate = true, Dialog(group = "Scenario"));

    parameter Modelica.Units.SI.Voltage nominalVoltage = 6
      "Manufacturer calibration voltage" annotation(Dialog(group = "Actuator specification"));
    parameter Modelica.Units.SI.AngularVelocity noLoadSpeed = 92 * 2 * Modelica.Constants.pi / 60
      "Manufacturer no-load speed at 6 V" annotation(Dialog(group = "Actuator specification"));
    parameter Modelica.Units.SI.Torque stallTorque = 12 * 9.80665 / 100
      "Manufacturer stall torque at 6 V" annotation(Dialog(group = "Actuator specification"));
    parameter Modelica.Units.SI.Current stallCurrent = 1.6
      "Manufacturer stall current at 6 V" annotation(Dialog(group = "Actuator specification"));
    parameter Modelica.Units.SI.Torque ratedTorque = 3 * 9.80665 / 100
      "Manufacturer rated torque at 6 V" annotation(Dialog(group = "Actuator specification"));
    parameter Modelica.Units.SI.Current ratedCurrent = 0.69
      "Manufacturer rated current at 6 V" annotation(Dialog(group = "Actuator specification"));
    parameter Real gearRatio = 320
      "Manufacturer reduction ratio" annotation(Dialog(group = "Actuator specification"));

    parameter Modelica.Units.SI.Inertia loadInertia = 0.015
      "Engineering estimate pending CAD mass-property sign-off" annotation(Dialog(group = "Power and load"));
    parameter Modelica.Units.SI.RotationalDampingConstant jointDamping = 0.08
      "Engineering estimate pending coast-down test" annotation(Dialog(group = "Power and load"));
    parameter Modelica.Units.SI.Resistance batteryResistance = 0.12
      "2S pack and harness estimate pending pulse test" annotation(Dialog(group = "Power and load"));
    parameter Modelica.Units.SI.Current normalCurrentLimit = 1.6
      "Current limit used by nominal scenarios" annotation(Dialog(group = "Power and load"));

    parameter Real positionGain(unit = "V/rad") = 18
      annotation(Dialog(group = "Controller"));
    parameter Real speedGain(unit = "V.s/rad") = 0.9
      annotation(Dialog(group = "Controller"));
    parameter Modelica.Units.SI.Angle jointLimit = 60 * Modelica.Constants.pi / 180
      annotation(Dialog(group = "Controller"));

    final parameter Modelica.Units.SI.Resistance equivalentResistance = nominalVoltage / stallCurrent
      "Equivalent output-side resistance fitted to the stall point";
    final parameter Real torqueConstant(unit = "N.m/A") = stallTorque / stallCurrent
      "Output-side torque constant fitted to the stall point";
    final parameter Real backEmfConstant(unit = "V.s/rad") = nominalVoltage / noLoadSpeed
      "Output-side back-EMF constant fitted to the no-load point";

    output Modelica.Units.SI.Angle jointAngle(start = 0, fixed = true);
    output Modelica.Units.SI.AngularVelocity jointSpeed(start = 0, fixed = true);
    output Modelica.Units.SI.Temperature windingTemperature(start = 298.15, fixed = true);
    output Modelica.Units.SI.Energy electricalEnergy(start = 0, fixed = true);

    output Modelica.Units.SI.Angle referenceAngle;
    output Modelica.Units.SI.Angle trackingError;
    output Modelica.Units.SI.Torque loadTorque;
    output Modelica.Units.SI.Torque jointTorque;
    output Modelica.Units.SI.Current armatureCurrent;
    output Modelica.Units.SI.Voltage sourceVoltage;
    output Modelica.Units.SI.Voltage busVoltage;
    output Modelica.Units.SI.Voltage driveVoltage;
    output Modelica.Units.SI.Power electricalPower;
    output Boolean currentLimited;
    output Boolean voltageLimited;

  protected
    Modelica.Units.SI.Voltage controlVoltage;
    Modelica.Units.SI.Voltage availableVoltage;
    Modelica.Units.SI.Current unconstrainedCurrent;
    Modelica.Units.SI.Current activeCurrentLimit;
    Modelica.Units.SI.Power copperLoss;
    constant Modelica.Units.SI.Temperature ambientTemperature = 298.15;
    constant Real thermalResistance(unit = "K/W") = 5;
    constant Real thermalCapacity(unit = "J/K") = 18;

  equation
    referenceAngle = if scenario == 1 then 20 * Modelica.Constants.pi / 180
      else if scenario == 2 then (if time < 0.2 then 0 else 30 * Modelica.Constants.pi / 180)
      else if scenario == 3 then (if time < 0.2 then 0 else 20 * Modelica.Constants.pi / 180 * sin(2 * Modelica.Constants.pi * 0.5 * (time - 0.2)))
      else if scenario == 4 then (if time < 0.2 then 0 else 20 * Modelica.Constants.pi / 180)
      else (if time < 0.2 then 0 else 30 * Modelica.Constants.pi / 180);

    loadTorque = if scenario == 1 then 0.20
      else if scenario == 2 then 0.05
      else if scenario == 3 then 0.05
      else if scenario == 4 then (if time < 1.0 then 0.05 else 0.20)
      else 0.10;
    sourceVoltage = if scenario == 5 then 4.8 else nominalVoltage;
    activeCurrentLimit = if scenario == 5 then 0.80 else normalCurrentLimit;

    trackingError = referenceAngle - jointAngle;
    controlVoltage = positionGain * trackingError - speedGain * jointSpeed;
    availableVoltage = max(4.0, sourceVoltage - batteryResistance * activeCurrentLimit);
    driveVoltage = max(-availableVoltage, min(availableVoltage, controlVoltage));
    unconstrainedCurrent = (driveVoltage - backEmfConstant * jointSpeed) / equivalentResistance;
    armatureCurrent = max(-activeCurrentLimit, min(activeCurrentLimit, unconstrainedCurrent));
    jointTorque = torqueConstant * armatureCurrent;
    busVoltage = sourceVoltage - batteryResistance * abs(armatureCurrent);
    electricalPower = busVoltage * abs(armatureCurrent);
    copperLoss = equivalentResistance * armatureCurrent ^ 2;
    currentLimited = abs(unconstrainedCurrent) > activeCurrentLimit + 1e-8;
    voltageLimited = abs(controlVoltage) > availableVoltage + 1e-8;

    der(jointAngle) = jointSpeed;
    loadInertia * der(jointSpeed) = jointTorque - loadTorque - jointDamping * jointSpeed;
    der(electricalEnergy) = electricalPower;
    thermalCapacity * der(windingTemperature) = copperLoss - (windingTemperature - ambientTemperature) / thermalResistance;

    annotation(
      experiment(StartTime = 0, StopTime = 3, Interval = 0.002, Tolerance = 1e-7),
      Documentation(info = "<html><p>Representative Microduck hip-pitch joint using Feetech HD-1910-C001 manufacturer data at 6 V. Datasheet evidence: HD-1910-C001 serial specification A/0, 2026-09-07, archived by JoyandAI/OpenMicroDuck. CAD evidence: Microduck_12Axis_Final identifies HD-1910-C001 and a 6.4 V battery envelope.</p><p>The electrical constants are calibrated to the published no-load speed and stall point. Load inertia, damping, battery resistance and thermal constants remain engineering estimates until physical bench measurements are attached; results therefore support current-limit and test-planning decisions, not final hardware release.</p></html>"));
  end PartialHD1910Joint;

  model StaticHold "Worst-case static hold"
    extends PartialHD1910Joint(final scenario = 1);
  end StaticHold;

  model PositionStep "30 degree position step"
    extends PartialHD1910Joint(final scenario = 2);
  end PositionStep;

  model PeriodicMotion "Repeated 20 degree motion at 0.5 Hz"
    extends PartialHD1910Joint(final scenario = 3);
  end PeriodicMotion;

  model LoadChange "Load torque step from 0.05 to 0.20 N.m"
    extends PartialHD1910Joint(final scenario = 4);
  end LoadChange;

  model LowVoltageCurrentLimit "4.8 V degraded operation with 0.8 A current limit"
    extends PartialHD1910Joint(final scenario = 5);
  end LowVoltageCurrentLimit;

  model JointAxis "Reduced HD-1910 joint axis"
    parameter Real commandScale = 1 "Joint contribution to the leg command"
      annotation(Dialog(group = "Kinematics"));
    parameter Modelica.Units.SI.Inertia loadInertia = 0.0015
      annotation(Dialog(group = "Mechanics"));
    parameter Modelica.Units.SI.RotationalDampingConstant damping = 0.08
      annotation(Dialog(group = "Mechanics"));
    parameter Real positionGain(unit = "N.m/rad") = 2.4
      annotation(Dialog(group = "Controller"));
    parameter Real speedGain(unit = "N.m.s/rad") = 0.12
      annotation(Dialog(group = "Controller"));
    parameter Real torqueConstant(unit = "N.m/A") = 0.7355
      annotation(Dialog(group = "Actuator"));
    parameter Modelica.Units.SI.Resistance motorResistance = 3.75
      annotation(Dialog(group = "Actuator"));
    parameter Modelica.Units.SI.Current currentLimit = 1.6
      annotation(Dialog(group = "Actuator"));

    Modelica.Blocks.Interfaces.RealInput command(unit = "rad")
      annotation(Placement(transformation(extent = {{-120, 40}, {-80, 80}})));
    Modelica.Blocks.Interfaces.RealInput supplyVoltage(unit = "V")
      annotation(Placement(transformation(extent = {{-120, -80}, {-80, -40}})));
    Modelica.Blocks.Interfaces.RealOutput angle(unit = "rad")
      annotation(Placement(transformation(extent = {{80, 50}, {120, 90}})));
    Modelica.Blocks.Interfaces.RealOutput current(unit = "A")
      annotation(Placement(transformation(extent = {{80, -90}, {120, -50}})));

    Modelica.Units.SI.AngularVelocity speed(start = 0, fixed = true);
    Modelica.Units.SI.Torque torque;
  protected
    Modelica.Units.SI.Torque requestedTorque;
    Modelica.Units.SI.Current voltageCurrentLimit;
  initial equation
    angle = 0;
  equation
    der(angle) = speed;
    requestedTorque = positionGain * (commandScale * command - angle) - speedGain * speed;
    voltageCurrentLimit = max(0, min(currentLimit, supplyVoltage / motorResistance));
    current = max(-voltageCurrentLimit, min(voltageCurrentLimit, requestedTorque / torqueConstant));
    torque = torqueConstant * current;
    loadInertia * der(speed) = torque - damping * speed;
    annotation(
      Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}), graphics = {
        Rectangle(extent = {{-100, 88}, {100, -88}}, radius = 18, lineColor = {0, 95, 115}, fillColor = {225, 244, 241}, fillPattern = FillPattern.Solid),
        Ellipse(extent = {{-38, 38}, {38, -38}}, lineColor = {0, 95, 115}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
        Line(points = {{0, 38}, {0, 68}}, color = {0, 95, 115}, thickness = 2),
        Text(extent = {{-76, -40}, {76, -66}}, textString = "%name", textColor = {0, 70, 90}, fontSize = 12)}));
  end JointAxis;

  partial model PartialLegAssembly "Common dynamics for a six-axis duck leg"
    parameter Boolean leftSide = true annotation(Evaluate = true, Dialog(group = "Leg"));
    Modelica.Blocks.Interfaces.RealInput command(unit = "rad")
      annotation(Placement(transformation(extent = {{-120, 45}, {-80, 85}})));
    Modelica.Blocks.Interfaces.RealInput supplyVoltage(unit = "V")
      annotation(Placement(transformation(extent = {{-120, -85}, {-80, -45}})));
    Modelica.Blocks.Interfaces.RealOutput totalCurrent(unit = "A")
      annotation(Placement(transformation(extent = {{80, -85}, {120, -45}})));
    Modelica.Blocks.Interfaces.RealOutput supportAngle(unit = "rad")
      annotation(Placement(transformation(extent = {{80, 45}, {120, 85}})));

    JointAxis hipYaw(commandScale = if leftSide then 0.12 else -0.12)
      annotation(Placement(transformation(extent = {{-70, 54}, {-50, 74}})));
    JointAxis hipRoll(commandScale = if leftSide then 0.35 else -0.35)
      annotation(Placement(transformation(extent = {{-25, 54}, {-5, 74}})));
    JointAxis hipPitch(commandScale = 1.0)
      annotation(Placement(transformation(extent = {{20, 54}, {40, 74}})));
    JointAxis knee(commandScale = -1.35)
      annotation(Placement(transformation(extent = {{20, 4}, {40, 24}})));
    JointAxis anklePitch(commandScale = 0.45)
      annotation(Placement(transformation(extent = {{20, -46}, {40, -26}})));
    JointAxis ankleRoll(commandScale = if leftSide then -0.25 else 0.25)
      annotation(Placement(transformation(extent = {{20, -82}, {40, -62}})));
  equation
    connect(command, hipYaw.command)
      annotation(Line(points = {{-100, 65}, {-80, 65}, {-80, 70}, {-70, 70}}, color = {0, 0, 127}));
    connect(command, hipRoll.command)
      annotation(Line(points = {{-100, 65}, {-36, 65}, {-36, 70}, {-25, 70}}, color = {0, 0, 127}));
    connect(command, hipPitch.command)
      annotation(Line(points = {{-100, 65}, {8, 65}, {8, 70}, {20, 70}}, color = {0, 0, 127}));
    connect(command, knee.command)
      annotation(Line(points = {{-100, 65}, {-42, 65}, {-42, 20}, {20, 20}}, color = {0, 0, 127}));
    connect(command, anklePitch.command)
      annotation(Line(points = {{-100, 65}, {-52, 65}, {-52, -30}, {20, -30}}, color = {0, 0, 127}));
    connect(command, ankleRoll.command)
      annotation(Line(points = {{-100, 65}, {-62, 65}, {-62, -66}, {20, -66}}, color = {0, 0, 127}));
    connect(supplyVoltage, hipYaw.supplyVoltage)
      annotation(Line(points = {{-100, -65}, {-76, -65}, {-76, 58}, {-70, 58}}, color = {0, 85, 170}));
    connect(supplyVoltage, hipRoll.supplyVoltage)
      annotation(Line(points = {{-100, -65}, {-34, -65}, {-34, 58}, {-25, 58}}, color = {0, 85, 170}));
    connect(supplyVoltage, hipPitch.supplyVoltage)
      annotation(Line(points = {{-100, -65}, {10, -65}, {10, 58}, {20, 58}}, color = {0, 85, 170}));
    connect(supplyVoltage, knee.supplyVoltage)
      annotation(Line(points = {{-100, -65}, {0, -65}, {0, 8}, {20, 8}}, color = {0, 85, 170}));
    connect(supplyVoltage, anklePitch.supplyVoltage)
      annotation(Line(points = {{-100, -65}, {8, -65}, {8, -42}, {20, -42}}, color = {0, 85, 170}));
    connect(supplyVoltage, ankleRoll.supplyVoltage)
      annotation(Line(points = {{-100, -65}, {8, -65}, {8, -78}, {20, -78}}, color = {0, 85, 170}));
    totalCurrent = abs(hipYaw.current) + abs(hipRoll.current) + abs(hipPitch.current)
      + abs(knee.current) + abs(anklePitch.current) + abs(ankleRoll.current);
    supportAngle = hipPitch.angle + knee.angle + anklePitch.angle;
    annotation(Diagram(coordinateSystem(extent = {{-100, -100}, {100, 100}})));
  end PartialLegAssembly;

  model LeftLegAssembly "Left six-axis leg assembly"
    extends PartialLegAssembly(final leftSide = true);
    annotation(Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}), graphics = {
      Rectangle(extent = {{-100, 92}, {100, -92}}, radius = 10, lineColor = {116, 154, 150}, fillColor = {245, 250, 249}, fillPattern = FillPattern.Solid),
      Polygon(points = {{-34, 68}, {20, 68}, {34, 44}, {8, 14}, {26, -18}, {4, -48}, {16, -76}, {-22, -76}, {-10, -46}, {-30, -14}, {-8, 16}, {-36, 44}, {-34, 68}}, lineColor = {0, 94, 110}, fillColor = {199, 232, 226}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-30, 66}, {-14, 50}}, lineColor = {0, 94, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-7, 66}, {9, 50}}, lineColor = {0, 94, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{16, 66}, {32, 50}}, lineColor = {0, 94, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-2, 24}, {18, 4}}, lineColor = {0, 94, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{6, -26}, {26, -46}}, lineColor = {0, 94, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-2, -58}, {18, -78}}, lineColor = {0, 94, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Text(extent = {{-82, 90}, {82, 76}}, textString = "LEFT LEG", textColor = {0, 70, 90}, fontSize = 10),
      Text(extent = {{-82, -78}, {82, -90}}, textString = "6 AXES", textColor = {70, 105, 105}, fontSize = 9)}));
  end LeftLegAssembly;

  model RightLegAssembly "Right six-axis leg assembly"
    extends PartialLegAssembly(final leftSide = false);
    annotation(Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}), graphics = {
      Rectangle(extent = {{-100, 92}, {100, -92}}, radius = 10, lineColor = {116, 154, 150}, fillColor = {245, 250, 249}, fillPattern = FillPattern.Solid),
      Polygon(points = {{34, 68}, {-20, 68}, {-34, 44}, {-8, 14}, {-26, -18}, {-4, -48}, {-16, -76}, {22, -76}, {10, -46}, {30, -14}, {8, 16}, {36, 44}, {34, 68}}, lineColor = {0, 94, 110}, fillColor = {199, 232, 226}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-32, 66}, {-16, 50}}, lineColor = {0, 94, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-9, 66}, {7, 50}}, lineColor = {0, 94, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{14, 66}, {30, 50}}, lineColor = {0, 94, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-18, 24}, {2, 4}}, lineColor = {0, 94, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-26, -26}, {-6, -46}}, lineColor = {0, 94, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-18, -58}, {2, -78}}, lineColor = {0, 94, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Text(extent = {{-82, 90}, {82, 76}}, textString = "RIGHT LEG", textColor = {0, 70, 90}, fontSize = 10),
      Text(extent = {{-82, -78}, {82, -90}}, textString = "6 AXES", textColor = {70, 105, 105}, fontSize = 9)}));
  end RightLegAssembly;

  block GaitController "Walking gait command generator"
    parameter Modelica.Units.SI.Angle amplitude = 20 * Modelica.Constants.pi / 180
      annotation(Dialog(group = "Gait"));
    parameter Modelica.Units.SI.Frequency frequency = 0.8
      annotation(Dialog(group = "Gait"));
    parameter Modelica.Units.SI.Time startTime = 0.3
      annotation(Dialog(group = "Gait"));
    Modelica.Blocks.Interfaces.RealOutput leftCommand(unit = "rad")
      annotation(Placement(transformation(extent = {{80, 35}, {120, 75}})));
    Modelica.Blocks.Interfaces.RealOutput rightCommand(unit = "rad")
      annotation(Placement(transformation(extent = {{80, -75}, {120, -35}})));
  equation
    leftCommand = if time < startTime then 0 else amplitude * sin(2 * Modelica.Constants.pi * frequency * (time - startTime));
    rightCommand = -leftCommand;
    annotation(Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}), graphics = {
      Rectangle(extent = {{-100, 76}, {100, -76}}, radius = 12, lineColor = {0, 100, 90}, fillColor = {229, 246, 241}, fillPattern = FillPattern.Solid),
      Line(points = {{-64, -24}, {-34, 30}, {0, -30}, {34, 30}, {64, -24}}, color = {0, 100, 90}, thickness = 2),
      Text(extent = {{-80, -44}, {80, -66}}, textString = "GAIT", textColor = {0, 80, 70}, fontSize = 14)}));
  end GaitController;

  model BatteryPack "Shared 2S power bus"
    parameter Modelica.Units.SI.Voltage nominalVoltage = 6.0 annotation(Dialog(group = "Battery"));
    parameter Modelica.Units.SI.Resistance internalResistance = 0.06 annotation(Dialog(group = "Battery"));
    Modelica.Blocks.Interfaces.RealInput leftCurrent(unit = "A")
      annotation(Placement(transformation(extent = {{-120, 35}, {-80, 75}})));
    Modelica.Blocks.Interfaces.RealInput rightCurrent(unit = "A")
      annotation(Placement(transformation(extent = {{-120, -75}, {-80, -35}})));
    Modelica.Blocks.Interfaces.RealOutput leftBus(unit = "V")
      annotation(Placement(transformation(extent = {{80, 35}, {120, 75}})));
    Modelica.Blocks.Interfaces.RealOutput rightBus(unit = "V")
      annotation(Placement(transformation(extent = {{80, -75}, {120, -35}})));
    output Modelica.Units.SI.Voltage busVoltage;
    output Modelica.Units.SI.Current totalCurrent;
  equation
    totalCurrent = leftCurrent + rightCurrent;
    busVoltage = max(4.8, nominalVoltage - internalResistance * totalCurrent);
    leftBus = busVoltage;
    rightBus = busVoltage;
    annotation(Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}), graphics = {
      Rectangle(extent = {{-100, 76}, {100, -76}}, radius = 8, lineColor = {0, 92, 115}, fillColor = {220, 242, 237}, fillPattern = FillPattern.Solid),
      Rectangle(extent = {{82, 22}, {100, -22}}, lineColor = {0, 92, 115}, fillColor = {0, 92, 115}, fillPattern = FillPattern.Solid),
      Line(points = {{-42, 0}, {-10, 0}}, color = {0, 92, 115}, thickness = 3),
      Line(points = {{26, -16}, {26, 16}}, color = {0, 92, 115}, thickness = 3),
      Line(points = {{10, 0}, {42, 0}}, color = {0, 92, 115}, thickness = 3),
      Text(extent = {{-70, -38}, {70, -60}}, textString = "%name", textColor = {0, 70, 90}, fontSize = 12)}));
  end BatteryPack;

  model TorsoBody "Machine duck torso and balance response"
    parameter Modelica.Units.SI.Inertia pitchInertia = 0.035 annotation(Dialog(group = "Body"));
    parameter Modelica.Units.SI.RotationalDampingConstant pitchDamping = 0.35 annotation(Dialog(group = "Body"));
    Modelica.Blocks.Interfaces.RealInput leftSupport(unit = "rad")
      annotation(Placement(transformation(extent = {{-120, 35}, {-80, 75}})));
    Modelica.Blocks.Interfaces.RealInput rightSupport(unit = "rad")
      annotation(Placement(transformation(extent = {{-120, -75}, {-80, -35}})));
    Modelica.Blocks.Interfaces.RealOutput bodyPitch(unit = "rad")
      annotation(Placement(transformation(extent = {{80, -20}, {120, 20}})));
    Modelica.Units.SI.AngularVelocity pitchRate(start = 0, fixed = true);
  initial equation
    bodyPitch = 0;
  equation
    der(bodyPitch) = pitchRate;
    pitchInertia * der(pitchRate) = 0.18 * (leftSupport + rightSupport - 2 * bodyPitch) - pitchDamping * pitchRate;
    annotation(Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}), graphics = {
      Rectangle(extent = {{-100, 82}, {100, -82}}, radius = 14, lineColor = {116, 154, 150}, fillColor = {245, 250, 249}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-72, 46}, {54, -46}}, lineColor = {0, 92, 105}, fillColor = {226, 244, 239}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{28, 76}, {66, 40}}, lineColor = {0, 92, 105}, fillColor = {226, 244, 239}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{51, 66}, {59, 58}}, lineColor = {20, 50, 55}, fillColor = {20, 50, 55}, fillPattern = FillPattern.Solid),
      Polygon(points = {{66, 61}, {88, 53}, {66, 47}, {66, 61}}, lineColor = {220, 130, 0}, fillColor = {255, 190, 58}, fillPattern = FillPattern.Solid),
      Line(points = {{54, 0}, {100, 0}}, color = {0, 0, 127}),
      Text(extent = {{-64, 16}, {46, -14}}, textString = "TORSO", textColor = {0, 76, 88}, fontSize = 12)}));
  end TorsoBody;

  model HD1910Servo3D "HD-1910-C001 revolute servo with MuJoCo-compatible torque-speed envelope"
    parameter Boolean useTorqueCommand = false
      "Use an external torque command instead of the internal position controller"
      annotation(Evaluate = true, Dialog(group = "Interface"));
    parameter Modelica.Units.SI.Angle qMin = -Modelica.Constants.pi/2 annotation(Dialog(group = "Limits"));
    parameter Modelica.Units.SI.Angle qMax = Modelica.Constants.pi/2 annotation(Dialog(group = "Limits"));
    parameter Modelica.Units.SI.Angle qStart = 0 annotation(Dialog(tab = "Initialization"));
    parameter Real kp(unit = "N.m/rad") = 2.247518624647875 annotation(Dialog(group = "Controller"));
    parameter Real kd(unit = "N.m.s/rad") = 0.12486214581377084 annotation(Dialog(group = "Controller"));
    parameter Modelica.Units.SI.Torque stallTorque = 1.176798 annotation(Dialog(group = "Actuator"));
    parameter Modelica.Units.SI.Torque ratedTorque = 0.2941995 annotation(Dialog(group = "Actuator"));
    parameter Modelica.Units.SI.AngularVelocity noLoadSpeed = 9.42477796076938 annotation(Dialog(group = "Actuator"));
    parameter Modelica.Units.SI.Current stallCurrent = 1.5 annotation(Dialog(group = "Actuator"));
    parameter Modelica.Units.SI.Current ratedCurrent = 0.69 annotation(Dialog(group = "Actuator"));
    parameter Modelica.Units.SI.Current noLoadCurrent = 0.24 annotation(Dialog(group = "Actuator"));
    parameter Modelica.Units.SI.Current staticCurrent = 0.02 annotation(Dialog(group = "Actuator"));
    parameter Modelica.Units.SI.RotationalDampingConstant jointDamping = 0.001
      "MuJoCo output-joint passive damping" annotation(Dialog(group = "Actuator"));
    parameter Modelica.Units.SI.Inertia rotorInertia = 0.0005
      "MuJoCo output-side armature inertia" annotation(Dialog(group = "Actuator"));
    parameter Real stopStiffness(unit = "N.m/rad") = 80 annotation(Dialog(group = "Limits"));
    parameter Real stopDamping(unit = "N.m.s/rad") = 0.25 annotation(Dialog(group = "Limits"));
    Modelica.Mechanics.MultiBody.Interfaces.Frame_a frame_a annotation(Placement(transformation(extent = {{-116, -16}, {-84, 16}})));
    Modelica.Mechanics.MultiBody.Interfaces.Frame_b frame_b annotation(Placement(transformation(extent = {{84, -16}, {116, 16}})));
    Modelica.Blocks.Interfaces.RealInput target(unit = "rad") annotation(Placement(transformation(extent = {{-20, 80}, {20, 120}})));
    input Modelica.Units.SI.Torque torqueCommand
      "Raw MuJoCo-compatible actuator command, active when useTorqueCommand=true";
    Modelica.Blocks.Interfaces.RealInput supplyVoltage(unit = "V") annotation(Placement(transformation(extent = {{-80, 80}, {-40, 120}})));
    Modelica.Blocks.Interfaces.RealInput powerScale annotation(Placement(transformation(extent = {{40, 80}, {80, 120}})));
    output Modelica.Units.SI.Angle q = joint.phi;
    output Modelica.Units.SI.AngularVelocity qd = joint.w;
    output Modelica.Units.SI.Torque demandedTorque;
    output Modelica.Units.SI.Torque appliedTorque;
    output Modelica.Units.SI.Current current;
  protected
    Modelica.Mechanics.MultiBody.Joints.Revolute joint(
      useAxisFlange = true,
      n = {0, 0, 1},
      phi(start = qStart, fixed = true),
      w(start = 0, fixed = true),
      cylinderLength = 0.018,
      cylinderDiameter = 0.009);
    Modelica.Mechanics.Rotational.Sources.Torque drive;
    Modelica.Mechanics.Rotational.Components.Damper passiveDamping(d = jointDamping);
    Modelica.Mechanics.Rotational.Components.Inertia rotor(J = rotorInertia, phi(start = qStart, fixed = false), w(start = 0, fixed = false));
    Modelica.Units.SI.Torque speedLimitedTorque;
    Modelica.Units.SI.Torque torqueLimit;
    Modelica.Units.SI.Torque stopTorque;
    Real speedFraction;
    Modelica.Units.SI.Current zeroTorqueCurrent;
  equation
    connect(frame_a, joint.frame_a);
    connect(joint.frame_b, frame_b);
    connect(drive.flange, joint.axis);
    connect(passiveDamping.flange_a, joint.axis);
    connect(passiveDamping.flange_b, joint.support);
    connect(rotor.flange_a, joint.axis);
    demandedTorque = if useTorqueCommand then torqueCommand else kp * (target - joint.phi) - kd * joint.w;
    torqueLimit = if demandedTorque * joint.w > 0 then stallTorque * max(0, 1 - abs(joint.w) / noLoadSpeed) else stallTorque;
    speedLimitedTorque = max(-torqueLimit, min(torqueLimit, demandedTorque));
    stopTorque = if joint.phi < qMin then stopStiffness * (qMin - joint.phi) - stopDamping * min(joint.w, 0)
      else if joint.phi > qMax then stopStiffness * (qMax - joint.phi) - stopDamping * max(joint.w, 0) else 0;
    appliedTorque = (if supplyVoltage > 0 then speedLimitedTorque * max(0, min(1, powerScale)) else 0) + stopTorque;
    drive.tau = appliedTorque;
    speedFraction = min(1, abs(joint.w) / noLoadSpeed);
    zeroTorqueCurrent = staticCurrent + speedFraction * (noLoadCurrent - staticCurrent);
    current = if supplyVoltage <= 0 then 0 else if abs(speedLimitedTorque) <= ratedTorque
      then zeroTorqueCurrent + abs(speedLimitedTorque) / ratedTorque * (ratedCurrent - zeroTorqueCurrent)
      else ratedCurrent + (abs(speedLimitedTorque) - ratedTorque) / (stallTorque - ratedTorque) * (stallCurrent - ratedCurrent);
    annotation(Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}), graphics = {
      Rectangle(extent = {{-92, 66}, {92, -66}}, radius = 16, lineColor = {0, 94, 110}, fillColor = {220, 242, 237}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-30, 30}, {30, -30}}, lineColor = {0, 94, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Text(extent = {{-76, -36}, {76, -58}}, textString = "%name", textColor = {0, 70, 90}, fontSize = 10)}));
  end HD1910Servo3D;

  model Native12Leg3D "CAD-aligned five-axis Microduck leg"
    parameter Boolean leftSide = true annotation(Evaluate = true, Dialog(group = "Leg"));
    parameter Boolean useTorqueCommand = false annotation(Evaluate = true, Dialog(group = "Interface"));
    Modelica.Mechanics.MultiBody.Interfaces.Frame_a frame_a annotation(Placement(transformation(extent = {{-116, -16}, {-84, 16}})));
    Modelica.Mechanics.MultiBody.Interfaces.Frame_b footFrame annotation(Placement(transformation(extent = {{84, -16}, {116, 16}})));
    input Modelica.Units.SI.Angle target[5];
    input Modelica.Units.SI.Torque torqueCommand[5];
    Modelica.Blocks.Interfaces.RealInput supplyVoltage(unit = "V") annotation(Placement(transformation(extent = {{80, 40}, {120, 80}})));
    Modelica.Blocks.Interfaces.RealInput powerScale annotation(Placement(transformation(extent = {{80, -80}, {120, -40}})));
    output Modelica.Units.SI.Angle q[5] = {hipYaw.q, hipRoll.q, hipPitch.q, knee.q, ankle.q};
    output Modelica.Units.SI.AngularVelocity qd[5] = {hipYaw.qd, hipRoll.qd, hipPitch.qd, knee.qd, ankle.qd};
    output Modelica.Units.SI.Torque torque[5] = {hipYaw.appliedTorque, hipRoll.appliedTorque, hipPitch.appliedTorque, knee.appliedTorque, ankle.appliedTorque};
    output Modelica.Units.SI.Current current[5] = {hipYaw.current, hipRoll.current, hipPitch.current, knee.current, ankle.current};
  protected
    parameter Modelica.Mechanics.MultiBody.Types.RotationTypes twoAxes = Modelica.Mechanics.MultiBody.Types.RotationTypes.TwoAxesVectors;
    Modelica.Mechanics.MultiBody.Parts.FixedRotation hipYawMount(
      r = if leftSide then {0.006, 0.0175, -0.005} else {0.006, -0.0175, -0.005}, rotationType = twoAxes,
      n_x = {0, 1, 0}, n_y = {1, 0, 0}, animation = false);
    HD1910Servo3D hipYaw(useTorqueCommand = useTorqueCommand, qMin = if leftSide then -0.4363323129985824 else -0.5235987755982988,
      qMax = if leftSide then 0.5235987755982988 else 0.4363323129985824, qStart = 0);
    Modelica.Mechanics.MultiBody.Parts.Body hipYawBody(r_CM = {0, 0.000224984719393, 0.0170589013038}, m = 0.0260936428012,
      I_11 = 4.66657311939e-6, I_22 = 3.92613104921e-6, I_33 = 2.56080242838e-6,
      I_21 = -2.66720349501e-13, I_31 = 5.69997940388e-13, I_32 = -8.93468852439e-8, animation = false);
    Modelica.Mechanics.MultiBody.Parts.FixedRotation hipRollMount(r = {0, 0.0165, 0.0125}, rotationType = twoAxes,
      n_x = if leftSide then {1, 0, 0} else {-1, 0, 0}, n_y = if leftSide then {0, 0, -1} else {0, 0, 1}, animation = false);
    HD1910Servo3D hipRoll(useTorqueCommand = useTorqueCommand, qMin = -0.3839724354387525, qMax = 0.3839724354387525,
      qStart = if leftSide then -0.08726646259971647 else 0.08726646259971647);
    Modelica.Mechanics.MultiBody.Parts.Body hipRollBody(
      r_CM = if leftSide then {0.0135711386748, -1.3927024468e-7, -0.00674380986871} else {0.0135711386864, -1.39278945327e-7, -0.00674381000579},
      m = if leftSide then 0.00646946094685 else 0.00646946098075,
      I_11 = 8.2523777e-7, I_22 = 1.15131852e-6, I_33 = 6.6064291e-7,
      I_21 = 1.9456e-12, I_31 = 3.743445e-7, I_32 = 9.05e-13, animation = false);
    Modelica.Mechanics.MultiBody.Parts.FixedRotation hipPitchMount(r = {0.025, 0, -0.0185}, rotationType = twoAxes,
      n_x = {0, -1, 0}, n_y = {0, 0, -1}, animation = false);
    HD1910Servo3D hipPitch(useTorqueCommand = useTorqueCommand, qMin = -Modelica.Constants.pi/2, qMax = Modelica.Constants.pi/2,
      qStart = if leftSide then -0.457924 else 0.457924);
    Modelica.Mechanics.MultiBody.Parts.Body upperLegBody(
      r_CM = if leftSide then {0.00704947642725, 0.0222258924485, 0.0130033115958} else {-0.00704935102022, 0.0222261026586, 0.0130033029184},
      m = if leftSide then 0.0489429099486 else 0.0489433702001,
      I_11 = 1.65258e-5, I_22 = 9.55866e-6, I_33 = 1.95918e-5,
      I_21 = if leftSide then -4.7926615e-6 else 4.7925265e-6,
      I_31 = if leftSide then 9.8973079e-8 else -9.8964109e-8, I_32 = 2.10935e-7, animation = false);
    Modelica.Mechanics.MultiBody.Parts.FixedRotation kneeMount(
      r = if leftSide then {0.022, 0.0357771, -0.004} else {-0.022, 0.0357771, -0.004}, rotationType = twoAxes,
      n_x = if leftSide then {0, 1, 0} else {1, 0, 0}, n_y = if leftSide then {1, 0, 0} else {0, -1, 0}, animation = false);
    HD1910Servo3D knee(useTorqueCommand = useTorqueCommand, qMin = -Modelica.Constants.pi/2, qMax = Modelica.Constants.pi/2,
      qStart = if leftSide then -0.00494 else 0.00494);
    Modelica.Mechanics.MultiBody.Parts.Body lowerLegBody(
      r_CM = if leftSide then {0, 0.0317301531184, -0.00918622779438} else {-0.0317301531274, 0, -0.00918622779462},
      m = 0.0253255203124,
      I_11 = if leftSide then 6.12776828e-6 else 2.54829155e-6,
      I_22 = if leftSide then 2.54829155e-6 else 6.12776828e-6, I_33 = 5.12572485e-6,
      I_21 = 0, I_31 = if leftSide then 0 else -9.05420649e-7,
      I_32 = if leftSide then 9.05420652e-7 else 0, animation = false);
    Modelica.Mechanics.MultiBody.Parts.FixedRotation ankleMount(
      r = if leftSide then {0, 0.042, -0.026} else {-0.042, 0, -0.026}, rotationType = twoAxes,
      n_x = if leftSide then {1, 0, 0} else {0, 1, 0}, n_y = if leftSide then {0, -1, 0} else {1, 0, 0}, animation = false);
    HD1910Servo3D ankle(useTorqueCommand = useTorqueCommand, qMin = -Modelica.Constants.pi/2, qMax = Modelica.Constants.pi/2,
      qStart = if leftSide then 0.452984 else -0.452984);
    Modelica.Mechanics.MultiBody.Parts.Body footBody(
      r_CM = if leftSide then {-0.0070730653269, -0.01448479969, -0.0151804367344} else {0.00707304571935, -0.0144848031164, -0.0151806684071},
      m = if leftSide then 0.0310845592418 else 0.0310844913308,
      I_11 = 5.88043e-6, I_22 = 1.201245e-5, I_33 = 8.23442e-6,
      I_21 = if leftSide then -3.67618e-8 else 3.67438e-8,
      I_31 = if leftSide then -3.99796e-9 else 4.04245e-9, I_32 = -1.795e-7, animation = false);
    Modelica.Mechanics.MultiBody.Parts.FixedTranslation footSite(
      r = if leftSide then {-0.00708767903646, -0.0234889624596, -0.0167934813648}
        else {0.00491018426614, -0.0234268349194, -0.0160334650503}, animation = false);
  equation
    connect(frame_a, hipYawMount.frame_a); connect(hipYawMount.frame_b, hipYaw.frame_a);
    connect(hipYaw.frame_b, hipYawBody.frame_a); connect(hipYaw.frame_b, hipRollMount.frame_a);
    connect(hipRollMount.frame_b, hipRoll.frame_a); connect(hipRoll.frame_b, hipRollBody.frame_a); connect(hipRoll.frame_b, hipPitchMount.frame_a);
    connect(hipPitchMount.frame_b, hipPitch.frame_a); connect(hipPitch.frame_b, upperLegBody.frame_a); connect(hipPitch.frame_b, kneeMount.frame_a);
    connect(kneeMount.frame_b, knee.frame_a); connect(knee.frame_b, lowerLegBody.frame_a); connect(knee.frame_b, ankleMount.frame_a);
    connect(ankleMount.frame_b, ankle.frame_a); connect(ankle.frame_b, footBody.frame_a); connect(ankle.frame_b, footSite.frame_a); connect(footSite.frame_b, footFrame);
    hipYaw.target = target[1]; hipRoll.target = target[2]; hipPitch.target = target[3]; knee.target = target[4]; ankle.target = target[5];
    hipYaw.torqueCommand = torqueCommand[1]; hipRoll.torqueCommand = torqueCommand[2]; hipPitch.torqueCommand = torqueCommand[3];
    knee.torqueCommand = torqueCommand[4]; ankle.torqueCommand = torqueCommand[5];
    hipYaw.supplyVoltage = supplyVoltage; hipRoll.supplyVoltage = supplyVoltage; hipPitch.supplyVoltage = supplyVoltage; knee.supplyVoltage = supplyVoltage; ankle.supplyVoltage = supplyVoltage;
    hipYaw.powerScale = powerScale; hipRoll.powerScale = powerScale; hipPitch.powerScale = powerScale; knee.powerScale = powerScale; ankle.powerScale = powerScale;
    annotation(Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}), graphics = {
      Rectangle(extent = {{-96, 90}, {96, -90}}, radius = 10, lineColor = {0, 94, 110}, fillColor = {245, 250, 249}, fillPattern = FillPattern.Solid),
      Polygon(points = {{-22, 68}, {22, 68}, {8, 24}, {28, -8}, {6, -70}, {-24, -70}, {-6, -16}, {-26, 18}, {-22, 68}}, lineColor = {0, 94, 110}, fillColor = {199, 232, 226}, fillPattern = FillPattern.Solid),
      Text(extent = {{-80, 88}, {80, 70}}, textString = "%name", textColor = {0, 70, 90}, fontSize = 9)}));
  end Native12Leg3D;

  model Native12Head3D "CAD-aligned neck-pitch and head-yaw chain"
    parameter Boolean useTorqueCommand = false annotation(Evaluate = true, Dialog(group = "Interface"));
    Modelica.Mechanics.MultiBody.Interfaces.Frame_a frame_a annotation(Placement(transformation(extent = {{-116, -16}, {-84, 16}})));
    Modelica.Mechanics.MultiBody.Interfaces.Frame_b frame_b annotation(Placement(transformation(extent = {{84, -16}, {116, 16}})));
    input Modelica.Units.SI.Angle target[2];
    input Modelica.Units.SI.Torque torqueCommand[2];
    Modelica.Blocks.Interfaces.RealInput supplyVoltage(unit = "V") annotation(Placement(transformation(extent = {{80, 40}, {120, 80}})));
    Modelica.Blocks.Interfaces.RealInput powerScale annotation(Placement(transformation(extent = {{80, -80}, {120, -40}})));
    output Modelica.Units.SI.Angle q[2] = {neckPitch.q, headYaw.q};
    output Modelica.Units.SI.AngularVelocity qd[2] = {neckPitch.qd, headYaw.qd};
    output Modelica.Units.SI.Torque torque[2] = {neckPitch.appliedTorque, headYaw.appliedTorque};
    output Modelica.Units.SI.Current current[2] = {neckPitch.current, headYaw.current};
  protected
    parameter Modelica.Mechanics.MultiBody.Types.RotationTypes twoAxes = Modelica.Mechanics.MultiBody.Types.RotationTypes.TwoAxesVectors;
    Modelica.Mechanics.MultiBody.Parts.FixedRotation neckMount(r = {0.026, 0.0145, 0.0324215}, rotationType = twoAxes,
      n_x = {-1, 0, 0}, n_y = {0, 0, -1}, animation = false);
    HD1910Servo3D neckPitch(useTorqueCommand = useTorqueCommand, qMin = -Modelica.Constants.pi/2, qMax = 1.0471975511965976, qStart = 0.3490658503988659);
    Modelica.Mechanics.MultiBody.Parts.Body neckBody(r_CM = {0.000173762584464, -0.0138415637574, 0.0144999995348}, m = 0.0267910105167,
      I_11 = 8.79861589526e-6, I_22 = 2.5794181576e-6, I_33 = 7.96294176189e-6,
      I_21 = 1.505095979e-7, I_31 = 1.80134244396e-12, I_32 = -7.10221126853e-13, animation = false);
    Modelica.Mechanics.MultiBody.Parts.FixedRotation headMount(r = {0.000446986125312, -0.0487719157139, 0.0145}, rotationType = twoAxes,
      n_x = {-0.984807753014, 0.173648177667, 0}, n_y = {0, 0, -1}, animation = false);
    HD1910Servo3D headYaw(useTorqueCommand = useTorqueCommand, qMin = -2.967059728390373, qMax = 2.967059728390348, qStart = 0);
    Modelica.Mechanics.MultiBody.Parts.Body headBody(r_CM = {0.005521871273, 0.0000549439912213, 0.0272006910139}, m = 0.167431338437,
      I_11 = 0.000117725381103, I_22 = 0.000133646982551, I_33 = 0.000169526450326,
      I_21 = 2.76055054952e-7, I_31 = 3.69623420849e-6, I_32 = -2.95192421629e-8, animation = false);
  equation
    connect(frame_a, neckMount.frame_a); connect(neckMount.frame_b, neckPitch.frame_a); connect(neckPitch.frame_b, neckBody.frame_a); connect(neckPitch.frame_b, headMount.frame_a);
    connect(headMount.frame_b, headYaw.frame_a); connect(headYaw.frame_b, headBody.frame_a); connect(headYaw.frame_b, frame_b);
    neckPitch.target = target[1]; headYaw.target = target[2];
    neckPitch.torqueCommand = torqueCommand[1]; headYaw.torqueCommand = torqueCommand[2];
    neckPitch.supplyVoltage = supplyVoltage; headYaw.supplyVoltage = supplyVoltage;
    neckPitch.powerScale = powerScale; headYaw.powerScale = powerScale;
    annotation(Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}), graphics = {
      Ellipse(extent = {{-80, 56}, {60, -48}}, lineColor = {0, 92, 105}, fillColor = {226, 244, 239}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{34, 84}, {76, 44}}, lineColor = {0, 92, 105}, fillColor = {226, 244, 239}, fillPattern = FillPattern.Solid),
      Polygon(points = {{74, 67}, {98, 57}, {74, 51}, {74, 67}}, lineColor = {220, 130, 0}, fillColor = {255, 190, 58}, fillPattern = FillPattern.Solid),
      Text(extent = {{-70, 10}, {44, -18}}, textString = "HEAD 2-AXIS", textColor = {0, 76, 88}, fontSize = 10)}));
  end Native12Head3D;

  model Native12Battery "Shared 6.4 V 1.8 Ah battery and regulated 6.0 V servo bus"
    parameter Boolean enforceCurrentLimit = false annotation(Evaluate = true, Dialog(group = "Protection"));
    parameter Modelica.Units.SI.Voltage nominalVoltage = 6.4 annotation(Dialog(group = "Battery"));
    parameter Real capacityAh(unit = "A.h") = 1.8 annotation(Dialog(group = "Battery"));
    parameter Modelica.Units.SI.Resistance internalResistance = 0.12 annotation(Dialog(group = "Battery"));
    parameter Modelica.Units.SI.Current maxOutputCurrent = 3.0 annotation(Dialog(group = "Protection"));
    parameter Modelica.Units.SI.Voltage regulatedVoltage = 6.0 annotation(Dialog(group = "Converter"));
    parameter Real converterEfficiency = 0.9 annotation(Dialog(group = "Converter"));
    parameter Modelica.Units.SI.Current auxiliaryCurrent = 0.5 annotation(Dialog(group = "Loads"));
    input Modelica.Units.SI.Current servoCurrent[12];
    Modelica.Blocks.Interfaces.RealOutput servoBusVoltage(unit = "V") annotation(Placement(transformation(extent = {{-120, 35}, {-80, 75}})));
    Modelica.Blocks.Interfaces.RealOutput servoScale annotation(Placement(transformation(extent = {{-120, -75}, {-80, -35}})));
    output Modelica.Units.SI.Current requestedBatteryCurrent;
    output Modelica.Units.SI.Current batteryCurrent;
    output Modelica.Units.SI.Voltage terminalVoltage;
    output Real stateOfCharge(start = 1, fixed = true);
    output Modelica.Units.SI.Energy cumulativeEnergy(start = 0, fixed = true);
  protected
    Modelica.Units.SI.Current totalServoCurrent;
    Modelica.Units.SI.Current availableServoCurrent;
  equation
    totalServoCurrent = sum(abs(servoCurrent));
    requestedBatteryCurrent = auxiliaryCurrent + totalServoCurrent * regulatedVoltage / (nominalVoltage * converterEfficiency);
    batteryCurrent = if enforceCurrentLimit then min(maxOutputCurrent, requestedBatteryCurrent) else requestedBatteryCurrent;
    terminalVoltage = if enforceCurrentLimit then max(0, nominalVoltage - internalResistance * batteryCurrent) else nominalVoltage;
    availableServoCurrent = max(0, (batteryCurrent - auxiliaryCurrent) * nominalVoltage * converterEfficiency / regulatedVoltage);
    servoScale = if enforceCurrentLimit and totalServoCurrent > 1e-9 then min(1, availableServoCurrent / totalServoCurrent) else 1;
    servoBusVoltage = if stateOfCharge > 0 then regulatedVoltage else 0;
    der(stateOfCharge) = if stateOfCharge > 0 then -batteryCurrent / (capacityAh * 3600) else 0;
    der(cumulativeEnergy) = terminalVoltage * batteryCurrent;
    annotation(Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}), graphics = {
      Rectangle(extent = {{-90, 64}, {86, -64}}, radius = 10, lineColor = {0, 92, 115}, fillColor = {220, 242, 237}, fillPattern = FillPattern.Solid),
       Rectangle(extent = {{-100, 22}, {-86, -22}}, lineColor = {0, 92, 115}, fillColor = {0, 92, 115}, fillPattern = FillPattern.Solid),
      Text(extent = {{-72, 24}, {70, -8}}, textString = "6.4 V / 1.8 Ah", textColor = {0, 70, 90}, fontSize = 12),
      Text(extent = {{-70, -30}, {70, -54}}, textString = "%name", textColor = {0, 70, 90}, fontSize = 9)}));
  end Native12Battery;

  model FootGroundContact3D "Four-point regularized foot patch against z=0 plane"
    parameter Boolean leftSide = true annotation(Evaluate = true, Dialog(group = "Contact geometry"));
    parameter Real normalStiffness(unit = "N/m") = 8000 annotation(Dialog(group = "Contact"));
    parameter Real normalDamping(unit = "N.s/m") = 35 annotation(Dialog(group = "Contact"));
    parameter Real frictionCoefficient = 0.8 annotation(Dialog(group = "Contact"));
    parameter Modelica.Units.SI.Velocity frictionVelocity = 0.01 annotation(Dialog(group = "Contact"));
    parameter Modelica.Units.SI.Position contactPoints[4, 3] = if leftSide then
      {{0.012, -0.0015688, -0.0179315}, {0.012, 0.0015688, 0.0179315},
       {-0.012, -0.0015688, -0.0179315}, {-0.012, 0.0015688, 0.0179315}}
      else
      {{-0.012, 0.0015688, 0.0179315}, {-0.012, -0.0015688, -0.0179315},
       {0.012, 0.0015688, 0.0179315}, {0.012, -0.0015688, -0.0179315}}
      "Foot-patch corners in the foot-site frame" annotation(Dialog(group = "Contact geometry"));
    Modelica.Mechanics.MultiBody.Interfaces.Frame_a frame_a annotation(Placement(transformation(extent = {{-116, -16}, {-84, 16}})));
    output Modelica.Units.SI.Position position[3];
    output Modelica.Units.SI.Velocity velocity[3];
    output Modelica.Units.SI.Force contactForce[3];
    output Modelica.Units.SI.Force pointForce[4, 3];
    output Integer activeContactCount;
    output Boolean inContact;
  protected
    Modelica.Mechanics.MultiBody.Parts.FixedTranslation contactPoint[4](r = contactPoints, each animation = false);
    Modelica.Mechanics.MultiBody.Sensors.AbsolutePosition positionSensor[4](each resolveInFrame = Modelica.Mechanics.MultiBody.Types.ResolveInFrameA.world);
    Modelica.Mechanics.MultiBody.Sensors.AbsoluteVelocity velocitySensor[4](each resolveInFrame = Modelica.Mechanics.MultiBody.Types.ResolveInFrameA.world);
    Modelica.Mechanics.MultiBody.Forces.WorldForce worldForce[4](each animation = false);
    Modelica.Units.SI.Force normalForce[4];
  equation
    for i in 1:4 loop
      connect(frame_a, contactPoint[i].frame_a);
      connect(contactPoint[i].frame_b, positionSensor[i].frame_a);
      connect(contactPoint[i].frame_b, velocitySensor[i].frame_a);
      connect(contactPoint[i].frame_b, worldForce[i].frame_b);
      normalForce[i] = if positionSensor[i].r[3] < 0 then
        max(0, -normalStiffness / 4 * positionSensor[i].r[3] - normalDamping / 4 * min(velocitySensor[i].v[3], 0)) else 0;
      pointForce[i, 1] = -frictionCoefficient * normalForce[i] * tanh(velocitySensor[i].v[1] / frictionVelocity);
      pointForce[i, 2] = -frictionCoefficient * normalForce[i] * tanh(velocitySensor[i].v[2] / frictionVelocity);
      pointForce[i, 3] = normalForce[i];
      worldForce[i].force = pointForce[i, :];
    end for;
    position = {(positionSensor[1].r[j] + positionSensor[2].r[j] + positionSensor[3].r[j] + positionSensor[4].r[j]) / 4 for j in 1:3};
    velocity = {(velocitySensor[1].v[j] + velocitySensor[2].v[j] + velocitySensor[3].v[j] + velocitySensor[4].v[j]) / 4 for j in 1:3};
    contactForce = {sum(pointForce[:, j]) for j in 1:3};
    activeContactCount = sum({if positionSensor[i].r[3] < 0 then 1 else 0 for i in 1:4});
    inContact = activeContactCount > 0;
    annotation(Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}), graphics = {
      Line(points = {{-90, -42}, {90, -42}}, color = {70, 90, 86}, thickness = 2),
      Polygon(points = {{-24, 48}, {24, 48}, {38, -20}, {-38, -20}, {-24, 48}}, lineColor = {0, 94, 110}, fillColor = {199, 232, 226}, fillPattern = FillPattern.Solid),
      Line(points = {{0, -20}, {0, -40}}, color = {200, 80, 30}, thickness = 3)}));
  end FootGroundContact3D;

  model MachineDuck "MuJoCo-compatible Microduck Native12 multibody model"
    parameter Boolean useExternalJointTarget = false
      "Expose the Native12 joint command input for controller/FMI coupling"
      annotation(Evaluate = true, Dialog(group = "Interface"));
    parameter Boolean useExternalJointTorque = false
      "Use the MuJoCo-compatible 12-channel torque command interface"
      annotation(Evaluate = true, Dialog(group = "Interface"));
    final parameter String jointNames[12] = {"left_hip_yaw", "left_hip_roll", "left_hip_pitch", "left_knee", "left_ankle",
      "neck_pitch", "head_yaw", "right_hip_yaw", "right_hip_roll", "right_hip_pitch", "right_knee", "right_ankle"};
    parameter Modelica.Units.SI.Angle standPose[12] = {0, -0.08726646259971647, -0.457924, -0.00494, 0.452984,
      0.3490658503988659, 0, 0, 0.08726646259971647, 0.457924, 0.00494, -0.452984}
      annotation(Dialog(tab = "Initialization", group = "Native12 protocol"));
    Modelica.Blocks.Interfaces.RealInput jointTarget[12](each unit = "rad") if useExternalJointTarget
      annotation(Placement(visible = useExternalJointTarget, transformation(extent = {{-10, 80}, {10, 100}})));
    Modelica.Blocks.Interfaces.RealInput jointTorqueCommand[12](each unit = "N.m") if useExternalJointTorque
      annotation(Placement(visible = useExternalJointTorque, transformation(extent = {{-34, 80}, {-14, 100}})));
    inner Modelica.Mechanics.MultiBody.World world(
      gravityType = Modelica.Mechanics.MultiBody.Types.GravityTypes.UniformGravity,
      g = 9.81, n = {0, 0, -1}, enableAnimation = true)
      annotation(Placement(transformation(extent = {{-133, 50}, {-117, 66}})));
    Modelica.Mechanics.MultiBody.Joints.FreeMotion root(
      r_rel_a(start = {0, 0, 0.116719591352}, each fixed = true),
      v_rel_a(start = {0, 0, 0}, each fixed = true), angles_fixed = true,
      angles_start = {0, 0, 0}, w_rel_a_fixed = true, w_rel_a_start = {0, 0, 0}, animation = false)
      annotation(Placement(transformation(extent = {{-108, 50}, {-92, 66}})));
    Modelica.Mechanics.MultiBody.Parts.Body trunk(
      r_CM = {-0.0219402681457, -0.0000310782101326, 0.0050751936443}, m = 0.190757252019,
      I_11 = 0.000121877086842, I_22 = 0.000140424626415, I_33 = 0.000111752300357,
      I_21 = 8.45761930481e-8, I_31 = -2.31949813718e-5, I_32 = 1.32427125149e-7,
      sphereDiameter = 0.055, animation = true)
      annotation(Placement(transformation(extent = {{-82, 48}, {-62, 68}})));
    Native12Leg3D leftLeg(leftSide = true, useTorqueCommand = useExternalJointTorque) annotation(Placement(transformation(extent = {{-72, -5}, {-38, 25}})));
    Native12Leg3D rightLeg(leftSide = false, useTorqueCommand = useExternalJointTorque) annotation(Placement(transformation(extent = {{-72, -60}, {-38, -30}})));
    Native12Head3D head(useTorqueCommand = useExternalJointTorque) annotation(Placement(transformation(extent = {{-34, 48}, {-6, 68}})));
    Native12Battery battery annotation(Placement(transformation(extent = {{78, 42}, {112, 74}})));
    FootGroundContact3D leftContact(leftSide = true) annotation(Placement(transformation(extent = {{-20, 1}, {8, 25}})));
    FootGroundContact3D rightContact(leftSide = false) annotation(Placement(transformation(extent = {{-20, -59}, {8, -35}})));
    FootGroundContact3D trunkContact(contactPoints =
      {{-0.0461, -0.0317, -0.0440}, {-0.0461, 0.0312, -0.0440},
       {0.0345, -0.0317, -0.0440}, {0.0345, 0.0312, -0.0440}})
      "CAD-AABB proxy used when the torso hits the floor";
    FootGroundContact3D headContact(contactPoints =
      {{-0.0539, -0.0474, -0.0077}, {-0.0539, 0.0470, -0.0077},
       {0.0468, -0.0474, -0.0077}, {0.0468, 0.0470, -0.0077}})
      "CAD-AABB proxy used when the head hits the floor";
    Modelica.Mechanics.MultiBody.Sensors.AbsolutePosition basePositionSensor(resolveInFrame = Modelica.Mechanics.MultiBody.Types.ResolveInFrameA.world);
    Modelica.Mechanics.MultiBody.Sensors.AbsoluteVelocity baseVelocitySensor(resolveInFrame = Modelica.Mechanics.MultiBody.Types.ResolveInFrameA.world);
    Modelica.Mechanics.MultiBody.Sensors.AbsoluteAngularVelocity baseAngularVelocitySensor(resolveInFrame = Modelica.Mechanics.MultiBody.Types.ResolveInFrameA.world);
    Modelica.Mechanics.MultiBody.Parts.FixedTranslation imuMount(r = {-0.00825, 0, 0.0308}, animation = false);
    Modelica.Mechanics.MultiBody.Sensors.AbsolutePosition imuPositionWorld(resolveInFrame = Modelica.Mechanics.MultiBody.Types.ResolveInFrameA.world);
    Modelica.Mechanics.MultiBody.Sensors.AbsoluteVelocity imuVelocityWorld(resolveInFrame = Modelica.Mechanics.MultiBody.Types.ResolveInFrameA.world);
    Modelica.Mechanics.MultiBody.Sensors.AbsoluteVelocity imuVelocityLocal(resolveInFrame = Modelica.Mechanics.MultiBody.Types.ResolveInFrameA.frame_a);
    Modelica.Mechanics.MultiBody.Sensors.AbsoluteAngularVelocity imuGyro(resolveInFrame = Modelica.Mechanics.MultiBody.Types.ResolveInFrameA.frame_a);
    output Modelica.Units.SI.Angle jointPosition[12];
    output Modelica.Units.SI.AngularVelocity jointVelocity[12];
    output Modelica.Units.SI.Torque jointTorque[12];
    output Modelica.Units.SI.Current servoCurrent[12];
    output Modelica.Units.SI.Position basePosition[3] = basePositionSensor.r;
    output Modelica.Units.SI.Velocity baseVelocity[3] = baseVelocitySensor.v;
    output Modelica.Units.SI.AngularVelocity baseAngularVelocity[3] = baseAngularVelocitySensor.w;
    output Real imuOrientationQuaternion[4]
      "IMU frame orientation quaternion in MuJoCo w,x,y,z order";
    output Modelica.Units.SI.AngularVelocity imuAngularVelocity[3] = imuGyro.w;
    output Modelica.Units.SI.Velocity imuLinearVelocity[3] = imuVelocityLocal.v;
    output Modelica.Units.SI.Acceleration imuSpecificForce[3];
    output Modelica.Units.SI.Force leftFootContactForce[3] = leftContact.contactForce;
    output Modelica.Units.SI.Force rightFootContactForce[3] = rightContact.contactForce;
    output Modelica.Units.SI.Force trunkContactForce[3] = trunkContact.contactForce;
    output Modelica.Units.SI.Force headContactForce[3] = headContact.contactForce;
    output Modelica.Units.SI.Current totalCurrent = battery.batteryCurrent;
    output Modelica.Units.SI.Voltage batteryVoltage = battery.terminalVoltage;
    output Real batteryStateOfCharge = battery.stateOfCharge;
  protected
    Modelica.Units.SI.Angle commandedJointTarget[12];
    Modelica.Units.SI.Torque commandedJointTorque[12];
    Real modelicaImuQuaternion[4];
  equation
    connect(world.frame_b, root.frame_a)
      annotation(Line(points = {{-117, 58}, {-108, 58}}, color = {95, 115, 112}));
    connect(root.frame_b, trunk.frame_a)
      annotation(Line(points = {{-92, 58}, {-82, 58}}, color = {95, 115, 112}));
    connect(root.frame_b, leftLeg.frame_a)
      annotation(Line(points = {{-92, 58}, {-86, 58}, {-86, 10}, {-72, 10}}, color = {95, 115, 112}));
    connect(root.frame_b, rightLeg.frame_a)
      annotation(Line(points = {{-92, 58}, {-86, 58}, {-86, -45}, {-72, -45}}, color = {95, 115, 112}));
    connect(root.frame_b, head.frame_a)
      annotation(Line(points = {{-92, 58}, {-34, 58}}, color = {95, 115, 112}));
    connect(leftLeg.footFrame, leftContact.frame_a)
      annotation(Line(points = {{-38, 10}, {-20, 10}}, color = {95, 115, 112}));
    connect(rightLeg.footFrame, rightContact.frame_a)
      annotation(Line(points = {{-38, -45}, {-20, -45}}, color = {95, 115, 112}));
    connect(root.frame_b, trunkContact.frame_a);
    connect(head.frame_b, headContact.frame_a);
    connect(root.frame_b, basePositionSensor.frame_a);
    connect(root.frame_b, baseVelocitySensor.frame_a);
    connect(root.frame_b, baseAngularVelocitySensor.frame_a);
    connect(root.frame_b, imuMount.frame_a);
    connect(imuMount.frame_b, imuPositionWorld.frame_a);
    connect(imuMount.frame_b, imuVelocityWorld.frame_a);
    connect(imuMount.frame_b, imuVelocityLocal.frame_a);
    connect(imuMount.frame_b, imuGyro.frame_a);
    if useExternalJointTarget then
      commandedJointTarget = jointTarget;
    else
      commandedJointTarget = standPose;
    end if;
    if useExternalJointTorque then
      commandedJointTorque = jointTorqueCommand;
    else
      commandedJointTorque = zeros(12);
    end if;
    leftLeg.target = commandedJointTarget[1:5]; head.target = commandedJointTarget[6:7]; rightLeg.target = commandedJointTarget[8:12];
    leftLeg.torqueCommand = commandedJointTorque[1:5]; head.torqueCommand = commandedJointTorque[6:7]; rightLeg.torqueCommand = commandedJointTorque[8:12];
    modelicaImuQuaternion = Modelica.Mechanics.MultiBody.Frames.to_Q(imuMount.frame_b.R);
    imuOrientationQuaternion = {modelicaImuQuaternion[4], modelicaImuQuaternion[1], modelicaImuQuaternion[2], modelicaImuQuaternion[3]};
    imuSpecificForce = Modelica.Mechanics.MultiBody.Frames.resolve2(imuMount.frame_b.R,
      der(imuVelocityWorld.v) - world.gravityAcceleration(imuPositionWorld.r));
    connect(battery.servoBusVoltage, leftLeg.supplyVoltage)
      annotation(Line(points = {{78, 67}, {66, 67}, {66, 19}, {-38, 19}}, color = {0, 0, 127}));
    connect(battery.servoBusVoltage, rightLeg.supplyVoltage)
      annotation(Line(points = {{78, 67}, {66, 67}, {66, -36}, {-38, -36}}, color = {0, 0, 127}));
    connect(battery.servoBusVoltage, head.supplyVoltage)
      annotation(Line(points = {{78, 67}, {20, 67}, {20, 64}, {-6, 64}}, color = {0, 0, 127}));
    connect(battery.servoScale, leftLeg.powerScale)
      annotation(Line(points = {{78, 49}, {60, 49}, {60, 1}, {-38, 1}}, color = {0, 0, 127}));
    connect(battery.servoScale, rightLeg.powerScale)
      annotation(Line(points = {{78, 49}, {60, 49}, {60, -54}, {-38, -54}}, color = {0, 0, 127}));
    connect(battery.servoScale, head.powerScale)
      annotation(Line(points = {{78, 49}, {24, 49}, {24, 52}, {-6, 52}}, color = {0, 0, 127}));
    servoCurrent = {leftLeg.current[1], leftLeg.current[2], leftLeg.current[3], leftLeg.current[4], leftLeg.current[5],
      head.current[1], head.current[2], rightLeg.current[1], rightLeg.current[2], rightLeg.current[3], rightLeg.current[4], rightLeg.current[5]};
    battery.servoCurrent = servoCurrent;
    jointPosition = {leftLeg.q[1], leftLeg.q[2], leftLeg.q[3], leftLeg.q[4], leftLeg.q[5],
      head.q[1], head.q[2], rightLeg.q[1], rightLeg.q[2], rightLeg.q[3], rightLeg.q[4], rightLeg.q[5]};
    jointVelocity = {leftLeg.qd[1], leftLeg.qd[2], leftLeg.qd[3], leftLeg.qd[4], leftLeg.qd[5],
      head.qd[1], head.qd[2], rightLeg.qd[1], rightLeg.qd[2], rightLeg.qd[3], rightLeg.qd[4], rightLeg.qd[5]};
    jointTorque = {leftLeg.torque[1], leftLeg.torque[2], leftLeg.torque[3], leftLeg.torque[4], leftLeg.torque[5],
      head.torque[1], head.torque[2], rightLeg.torque[1], rightLeg.torque[2], rightLeg.torque[3], rightLeg.torque[4], rightLeg.torque[5]};
    annotation(
      Icon(coordinateSystem(extent = {{-100, -100}, {100, 100}}), graphics = {
        Ellipse(extent = {{-70, 62}, {56, -12}}, lineColor = {0, 91, 104}, fillColor = {216, 241, 236}, fillPattern = FillPattern.Solid),
        Ellipse(extent = {{28, 88}, {76, 42}}, lineColor = {0, 91, 104}, fillColor = {216, 241, 236}, fillPattern = FillPattern.Solid),
        Polygon(points = {{74, 66}, {100, 56}, {74, 50}, {74, 66}}, lineColor = {218, 128, 0}, fillColor = {255, 185, 45}, fillPattern = FillPattern.Solid),
        Polygon(points = {{-42, -6}, {-20, -6}, {-12, -72}, {-50, -72}, {-42, -6}}, lineColor = {0, 91, 104}, fillColor = {167, 220, 210}, fillPattern = FillPattern.Solid),
        Polygon(points = {{6, -6}, {28, -6}, {38, -72}, {0, -72}, {6, -6}}, lineColor = {0, 91, 104}, fillColor = {167, 220, 210}, fillPattern = FillPattern.Solid),
        Text(extent = {{-84, 40}, {48, 12}}, textString = "NATIVE12", textColor = {0, 76, 88}, fontSize = 13)}),
      Diagram(coordinateSystem(extent = {{-140, -90}, {140, 90}})),
      experiment(StartTime = 0, StopTime = 2, Interval = 0.005, Tolerance = 1e-8),
      Documentation(info = "<html><p>Native12 multibody replacement for the MuJoCo microduck_12axis_native baseline. It uses the same five leg joints per side, neck pitch, head yaw, CAD-derived rigid-body mass properties, floating base, gravity, joint limits, output-side damping and armature inertia, HD-1910 torque-speed/current envelope, shared 6.4 V battery, four-point foot contact patches, a raw torque command mode and IMU-compatible outputs.</p><p>The four-point patches reproduce foot support moments and edge loading while remaining portable across OMC installations. Arbitrary 40-convex-mesh self-collision remains outside the Modelica Standard Library and must be covered by proxy contacts or a dedicated contact library before contact-rich trajectory equivalence can be accepted.</p></html>"));
  end MachineDuck;

  annotation(uses(Modelica(version = "4.1.0")));
end AIDCT2;
