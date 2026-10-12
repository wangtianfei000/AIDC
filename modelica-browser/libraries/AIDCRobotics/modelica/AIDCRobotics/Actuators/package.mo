within AIDCRobotics;
package Actuators "Reusable actuator equations"
  extends Modelica.Icons.Package;

  partial model PartialPositionServoAxis "Standard position-servo plant interface"
    AIDCRobotics.Interfaces.PositionCommand target
      annotation(Placement(transformation(extent = {{-120, 50}, {-80, 90}})));
    Modelica.Blocks.Interfaces.RealInput loadTorque(quantity = "Torque", unit = "N.m")
      annotation(Placement(transformation(extent = {{-120, 10}, {-80, 50}})));
    Modelica.Blocks.Interfaces.RealInput sourceVoltage(quantity = "ElectricPotential", unit = "V")
      annotation(Placement(transformation(extent = {{-120, -40}, {-80, 0}})));
    Modelica.Blocks.Interfaces.RealInput currentLimit(quantity = "ElectricCurrent", unit = "A")
      annotation(Placement(transformation(extent = {{-120, -90}, {-80, -50}})));
    AIDCRobotics.Interfaces.JointStateOutput state
      annotation(Placement(transformation(extent = {{80, 50}, {100, 70}})));
    AIDCRobotics.Interfaces.DiagnosticOutput diagnostic
      annotation(Placement(transformation(extent = {{80, 10}, {100, 30}})));
    Modelica.Blocks.Interfaces.RealOutput armatureCurrent(quantity = "ElectricCurrent", unit = "A")
      annotation(Placement(transformation(extent = {{80, -30}, {100, -10}})));
    Modelica.Blocks.Interfaces.RealOutput busVoltage(quantity = "ElectricPotential", unit = "V")
      annotation(Placement(transformation(extent = {{80, -70}, {100, -50}})));
    annotation(Icon(coordinateSystem(preserveAspectRatio = false), graphics = {
      Rectangle(extent = {{-80, 80}, {80, -80}}, radius = 12, lineColor = {0, 120, 110}, fillColor = {232, 247, 244}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-42, 42}, {42, -42}}, lineColor = {0, 120, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Line(points = {{-30, -28}, {0, 28}, {30, -28}}, color = {0, 120, 110}, thickness = 1),
      Text(extent = {{-76, -54}, {76, -76}}, textString = "%name", textColor = {0, 90, 84})}));
  end PartialPositionServoAxis;

  model LimitedPositionServoAxis "Generic servo, supply and single-axis load model"
    extends PartialPositionServoAxis;
    parameter AIDCRobotics.DeviceData.ServoData data = AIDCRobotics.DeviceData.Feetech.HD1910;
    parameter Modelica.Units.SI.Inertia loadInertia(min = Modelica.Constants.small) = 0.015;
    parameter Modelica.Units.SI.RotationalDampingConstant jointDamping(min = 0) = 0.08;
    parameter Modelica.Units.SI.Resistance sourceResistance(min = 0) = 0.12;
    parameter Real positionGain(unit = "V/rad") = 18;
    parameter Real speedGain(unit = "V.s/rad") = 0.9;
    parameter Real thermalResistance(unit = "K/W") = 5;
    parameter Real thermalCapacity(unit = "J/K") = 18;
    parameter Modelica.Units.SI.Temperature ambientTemperature = 298.15;

    output Modelica.Units.SI.Angle trackingError;
    output Modelica.Units.SI.Voltage driveVoltage;
    output Modelica.Units.SI.Power electricalPower;
    output Modelica.Units.SI.Energy electricalEnergy(start = 0, fixed = true);
    output Modelica.Units.SI.Temperature windingTemperature(start = 298.15, fixed = true);
    output Boolean currentLimited;
    output Boolean voltageLimited;

  protected
    final parameter Modelica.Units.SI.Resistance equivalentResistance = data.nominalVoltage / data.stallCurrent;
    final parameter Real torqueConstant(unit = "N.m/A") = data.stallTorque / data.stallCurrent;
    final parameter Real backEmfConstant(unit = "V.s/rad") = data.nominalVoltage / data.noLoadSpeed;
    Modelica.Units.SI.Angle phi(start = 0, fixed = true);
    Modelica.Units.SI.AngularVelocity w(start = 0, fixed = true);
    Modelica.Units.SI.Voltage controlVoltage;
    Modelica.Units.SI.Voltage availableVoltage;
    Modelica.Units.SI.Current unconstrainedCurrent;
    Modelica.Units.SI.Power copperLoss;

  equation
    trackingError = target - phi;
    controlVoltage = positionGain * trackingError - speedGain * w;
    availableVoltage = max(4.0, sourceVoltage - sourceResistance * currentLimit);
    driveVoltage = max(-availableVoltage, min(availableVoltage, controlVoltage));
    unconstrainedCurrent = (driveVoltage - backEmfConstant * w) / equivalentResistance;
    armatureCurrent = max(-currentLimit, min(currentLimit, unconstrainedCurrent));
    state.appliedTorque = torqueConstant * armatureCurrent;
    busVoltage = sourceVoltage - sourceResistance * abs(armatureCurrent);
    electricalPower = busVoltage * abs(armatureCurrent);
    copperLoss = equivalentResistance * armatureCurrent ^ 2;
    currentLimited = abs(unconstrainedCurrent) > currentLimit + 1e-8;
    voltageLimited = abs(controlVoltage) > availableVoltage + 1e-8;
    der(phi) = w;
    loadInertia * der(w) = state.appliedTorque - loadTorque - jointDamping * w;
    state.position = phi;
    state.velocity = w;
    state.acceleration = der(w);
    der(electricalEnergy) = electricalPower;
    thermalCapacity * der(windingTemperature) = copperLoss - (windingTemperature - ambientTemperature) / thermalResistance;
    diagnostic.valid = true;
    diagnostic.limited = currentLimited or voltageLimited;
    diagnostic.statusCode = if currentLimited then 1 else if voltageLimited then 2 else 0;

    annotation(
      Icon(coordinateSystem(preserveAspectRatio = false), graphics = {
        Rectangle(extent = {{-80, 80}, {80, -80}}, radius = 12, lineColor = {0, 120, 110}, fillColor = {232, 247, 244}, fillPattern = FillPattern.Solid),
        Ellipse(extent = {{-42, 42}, {42, -42}}, lineColor = {0, 120, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
        Line(points = {{-30, -28}, {0, 28}, {30, -28}}, color = {0, 120, 110}, thickness = 1),
        Text(extent = {{-76, -54}, {76, -76}}, textString = "SERVO", textColor = {0, 90, 84})}),
      Documentation(info = "<html><p>Parameter-driven position servo model with torque-speed back EMF, current limit, source droop, load dynamics and lumped winding thermal state.</p></html>"));
  end LimitedPositionServoAxis;

  model LimitedDcMotor "Reduced DC motor with back EMF and current limiting"
    parameter AIDCRobotics.DeviceData.ServoData data = AIDCRobotics.DeviceData.Feetech.HD1910;
    parameter Modelica.Units.SI.Inertia rotorInertia(min = Modelica.Constants.small) = 1e-4;
    parameter Modelica.Units.SI.RotationalDampingConstant damping(min = 0) = 1e-3;
    Modelica.Blocks.Interfaces.RealInput appliedVoltage(unit = "V")
      annotation(Placement(transformation(extent = {{-120, 50}, {-80, 90}})));
    Modelica.Blocks.Interfaces.RealInput currentLimit(unit = "A")
      annotation(Placement(transformation(extent = {{-120, -20}, {-80, 20}})));
    Modelica.Blocks.Interfaces.RealInput loadTorque(unit = "N.m")
      annotation(Placement(transformation(extent = {{-120, -90}, {-80, -50}})));
    Modelica.Blocks.Interfaces.RealOutput shaftSpeed(unit = "rad/s", start = 0, fixed = true)
      annotation(Placement(transformation(extent = {{80, 50}, {100, 70}})));
    Modelica.Blocks.Interfaces.RealOutput armatureCurrent(unit = "A")
      annotation(Placement(transformation(extent = {{80, -10}, {100, 10}})));
    Modelica.Blocks.Interfaces.RealOutput shaftTorque(unit = "N.m")
      annotation(Placement(transformation(extent = {{80, -70}, {100, -50}})));
  protected
    final parameter Modelica.Units.SI.Resistance resistance = data.nominalVoltage / data.stallCurrent;
    final parameter Real torqueConstant(unit = "N.m/A") = data.stallTorque / data.stallCurrent;
    final parameter Real backEmfConstant(unit = "V.s/rad") = data.nominalVoltage / data.noLoadSpeed;
    Modelica.Units.SI.Current unconstrainedCurrent;
  equation
    unconstrainedCurrent = (appliedVoltage - backEmfConstant * shaftSpeed) / resistance;
    armatureCurrent = max(-currentLimit, min(currentLimit, unconstrainedCurrent));
    shaftTorque = torqueConstant * armatureCurrent;
    rotorInertia * der(shaftSpeed) = shaftTorque - loadTorque - damping * shaftSpeed;
    annotation(Icon(coordinateSystem(preserveAspectRatio = false), graphics = {
      Rectangle(extent = {{-80, 70}, {80, -70}}, radius = 12, lineColor = {0, 120, 110}, fillColor = {232, 247, 244}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-42, 42}, {42, -42}}, lineColor = {0, 120, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Text(extent = {{-28, 22}, {28, -22}}, textString = "M", textColor = {0, 90, 84}),
      Text(extent = {{-76, -48}, {76, -68}}, textString = "%name", textColor = {0, 90, 84})}));
  end LimitedDcMotor;

  model IdealTorque "Ideal torque source with standard command interface"
    AIDCRobotics.Interfaces.TorqueCommand command
      annotation(Placement(transformation(extent = {{-120, -20}, {-80, 20}})));
    Modelica.Mechanics.Rotational.Interfaces.Flange_b flange
      annotation(Placement(transformation(extent = {{90, -10}, {110, 10}})));
  equation
    flange.tau = -command;
    annotation(Icon(coordinateSystem(preserveAspectRatio = false), graphics = {
      Rectangle(extent = {{-80, 60}, {80, -60}}, radius = 10, lineColor = {0, 120, 110}, fillColor = {232, 247, 244}, fillPattern = FillPattern.Solid),
      Line(points = {{-48, 0}, {38, 0}}, color = {0, 90, 84}, thickness = 1),
      Polygon(points = {{38, 20}, {72, 0}, {38, -20}, {38, 20}}, lineColor = {0, 90, 84}, fillColor = {0, 90, 84}, fillPattern = FillPattern.Solid),
      Text(extent = {{-72, 48}, {72, 18}}, textString = "TORQUE", textColor = {0, 90, 84}),
      Text(extent = {{-76, -30}, {76, -54}}, textString = "%name", textColor = {0, 90, 84})}));
  end IdealTorque;
end Actuators;
