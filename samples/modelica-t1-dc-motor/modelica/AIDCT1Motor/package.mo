within ;
package AIDCT1Motor
  extends Modelica.Icons.Package;

  model OpenLoopMotor
    "Independent T1 sample: open-loop DC motor"
    Modelica.Blocks.Sources.Constant command(k = 6)
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {-130, 60})));
    Modelica.Electrical.Analog.Sources.SignalVoltage voltageSource
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {-80, 20}, rotation = -90)));
    Modelica.Electrical.Analog.Sensors.CurrentSensor currentSensor
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {-35, 20})));
    Modelica.Electrical.Analog.Basic.Resistor armatureResistance(R = 1.5)
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {5, 20})));
    Modelica.Electrical.Analog.Basic.Inductor armatureInductance(
      L = 0.003,
      i(start = 0, fixed = true))
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {45, 20})));
    Modelica.Electrical.Analog.Basic.RotationalEMF motor(k = 0.05)
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {90, 20})));
    Modelica.Electrical.Analog.Basic.Ground ground
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {-80, -20})));
    Modelica.Mechanics.Rotational.Components.Inertia rotor(
      J = 0.001,
      phi(start = 0, fixed = true),
      w(start = 0, fixed = true))
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {90, -25})));
    Modelica.Mechanics.Rotational.Components.Damper bearing(d = 0.002)
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {40, -25})));
    Modelica.Mechanics.Rotational.Components.Fixed fixed
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {-5, -25})));
    Modelica.Mechanics.Rotational.Sensors.SpeedSensor speedSensor
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {90, -65})));

    output Modelica.Units.SI.AngularVelocity motorSpeed = speedSensor.w;
    output Modelica.Units.SI.Current armatureCurrent = currentSensor.i;
    output Modelica.Units.SI.Torque motorTorque = motor.tauElectrical;
    output Modelica.Units.SI.Voltage driveVoltage = voltageSource.v;
    output Modelica.Units.SI.Power electricalPower = voltageSource.v * currentSensor.i;

  equation
    connect(command.y, voltageSource.v)
      annotation(Line(points = {{-119, 60}, {-80, 60}, {-80, 32}}, color = {0, 0, 127}));
    connect(voltageSource.p, currentSensor.p)
      annotation(Line(points = {{-70, 20}, {-45, 20}}, color = {0, 0, 255}));
    connect(currentSensor.n, armatureResistance.p)
      annotation(Line(points = {{-25, 20}, {-5, 20}}, color = {0, 0, 255}));
    connect(armatureResistance.n, armatureInductance.p)
      annotation(Line(points = {{15, 20}, {35, 20}}, color = {0, 0, 255}));
    connect(armatureInductance.n, motor.p)
      annotation(Line(points = {{55, 20}, {80, 20}}, color = {0, 0, 255}));
    connect(motor.n, voltageSource.n)
      annotation(Line(points = {{100, 20}, {115, 20}, {115, 0}, {-80, 0}, {-80, 10}}, color = {0, 0, 255}));
    connect(voltageSource.n, ground.p)
      annotation(Line(points = {{-80, 10}, {-80, -10}}, color = {0, 0, 255}));
    connect(motor.flange, rotor.flange_a)
      annotation(Line(points = {{90, 10}, {90, -15}}, color = {0, 0, 0}));
    connect(rotor.flange_b, bearing.flange_a)
      annotation(Line(points = {{80, -25}, {50, -25}}, color = {0, 0, 0}));
    connect(bearing.flange_b, fixed.flange)
      annotation(Line(points = {{30, -25}, {5, -25}}, color = {0, 0, 0}));
    connect(rotor.flange_b, speedSensor.flange)
      annotation(Line(points = {{80, -25}, {70, -25}, {70, -50}, {90, -50}, {90, -55}}, color = {0, 0, 0}));

    annotation(
      Diagram(coordinateSystem(extent = {{-150, -90}, {125, 90}})),
      experiment(StartTime = 0, StopTime = 1, Interval = 0.001, Tolerance = 1e-7));
  end OpenLoopMotor;
end AIDCT1Motor;
