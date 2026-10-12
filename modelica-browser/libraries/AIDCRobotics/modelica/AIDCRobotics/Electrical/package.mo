within AIDCRobotics;
package Electrical "Robot electrical power components"
  extends Modelica.Icons.Package;

  model SimpleBatteryBus "Capacity, internal resistance and current-limited DC bus"
    parameter Modelica.Units.SI.Voltage nominalVoltage = 12;
    parameter Modelica.Units.SI.Resistance internalResistance(min = 0) = 0.1;
    parameter Real capacity_Ah(min = Modelica.Constants.small) = 1;
    parameter Modelica.Units.SI.Current maximumCurrent(min = Modelica.Constants.small) = 10;
    Modelica.Blocks.Interfaces.RealInput requestedCurrent(quantity = "ElectricCurrent", unit = "A")
      annotation(Placement(transformation(extent = {{-120, -20}, {-80, 20}})));
    Modelica.Blocks.Interfaces.RealOutput deliveredCurrent(quantity = "ElectricCurrent", unit = "A")
      annotation(Placement(transformation(extent = {{80, 50}, {100, 70}})));
    Modelica.Blocks.Interfaces.RealOutput terminalVoltage(quantity = "ElectricPotential", unit = "V")
      annotation(Placement(transformation(extent = {{80, 10}, {100, 30}})));
    Modelica.Blocks.Interfaces.RealOutput stateOfCharge(unit = "1")
      annotation(Placement(transformation(extent = {{80, -50}, {100, -30}})));
    output Boolean currentLimited;
  protected
    Real usedCharge_Ah(start = 0, fixed = true);
  equation
    deliveredCurrent = min(maximumCurrent, max(0, requestedCurrent));
    currentLimited = requestedCurrent > maximumCurrent;
    terminalVoltage = max(0, nominalVoltage - internalResistance * deliveredCurrent);
    der(usedCharge_Ah) = deliveredCurrent / 3600;
    stateOfCharge = max(0, 1 - usedCharge_Ah / capacity_Ah);
    annotation(Icon(coordinateSystem(preserveAspectRatio = false), graphics = {
      Rectangle(extent = {{-76, 62}, {68, -62}}, radius = 8, lineColor = {0, 120, 110}, fillColor = {232, 247, 244}, fillPattern = FillPattern.Solid),
      Rectangle(extent = {{68, 22}, {82, -22}}, lineColor = {0, 120, 110}, fillColor = {0, 120, 110}, fillPattern = FillPattern.Solid),
      Line(points = {{-45, 14}, {-17, 14}}, color = {0, 90, 84}, thickness = 1),
      Line(points = {{22, 14}, {50, 14}}, color = {0, 90, 84}, thickness = 1),
      Line(points = {{36, 28}, {36, 0}}, color = {0, 90, 84}, thickness = 1),
      Text(extent = {{-68, -34}, {60, -56}}, textString = "BATTERY", textColor = {0, 90, 84})}));
  end SimpleBatteryBus;
end Electrical;
