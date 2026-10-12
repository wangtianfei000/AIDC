within ;
package AIDCT0
  extends Modelica.Icons.Package;

  model SingleJointProbe
    "T0 minimal position-controlled electromechanical single joint"
    parameter Modelica.Units.SI.Voltage voltageLimit = 12;
    parameter Real gearRatio = 30;

    Modelica.Blocks.Sources.Step reference(
      height = 0.5,
      offset = 0,
      startTime = 0.2) "Joint-angle command"
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {-140, 60})));
    Modelica.Blocks.Continuous.LimPID controller(
      controllerType = Modelica.Blocks.Types.SimpleController.PI,
      k = 4,
      Ti = 0.25,
      yMax = voltageLimit,
      yMin = -voltageLimit,
      initType = Modelica.Blocks.Types.Init.InitialOutput,
      y_start = 0) "Position controller"
      annotation(Placement(transformation(extent = {{-12, -12}, {12, 12}}, origin = {-95, 60})));

    Modelica.Electrical.Analog.Sources.SignalVoltage voltageSource
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {-55, 20}, rotation = -90)));
    Modelica.Electrical.Analog.Basic.Resistor armatureResistance(R = 2.0)
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {15, 20})));
    Modelica.Electrical.Analog.Basic.Inductor armatureInductance(
      L = 0.002,
      i(start = 0, fixed = true))
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {55, 20})));
    Modelica.Electrical.Analog.Basic.RotationalEMF motor(k = 0.08)
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {100, 20})));
    Modelica.Electrical.Analog.Basic.Ground ground
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {-55, -20})));
    Modelica.Electrical.Analog.Sensors.CurrentSensor currentSensor
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {-15, 20})));

    Modelica.Mechanics.Rotational.Components.Inertia motorInertia(
      J = 2e-4,
      phi(start = 0, fixed = true),
      w(start = 0, fixed = true))
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {100, -25})));
    Modelica.Mechanics.Rotational.Components.IdealGear gear(ratio = gearRatio)
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {60, -25})));
    Modelica.Mechanics.Rotational.Sensors.TorqueSensor torqueSensor
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {20, -25})));
    Modelica.Mechanics.Rotational.Components.Inertia loadInertia(J = 0.015)
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {-20, -25})));
    Modelica.Mechanics.Rotational.Components.Damper jointDamping(d = 0.20)
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {-60, -65})));
    Modelica.Mechanics.Rotational.Components.Fixed fixed
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {-105, -65})));
    Modelica.Mechanics.Rotational.Sensors.AngleSensor angleSensor
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {-20, -65})));
    Modelica.Mechanics.Rotational.Sensors.SpeedSensor speedSensor
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}}, origin = {20, -65})));

    output Modelica.Units.SI.Angle jointAngle = angleSensor.phi;
    output Modelica.Units.SI.AngularVelocity jointSpeed = speedSensor.w;
    output Modelica.Units.SI.Current armatureCurrent = currentSensor.i;
    output Modelica.Units.SI.Torque jointTorque = torqueSensor.tau;
    output Modelica.Units.SI.Voltage driveVoltage = voltageSource.v;
    output Modelica.Units.SI.Power electricalPower = voltageSource.v * currentSensor.i;
    output Modelica.Units.SI.Angle referenceAngle = reference.y;
    output Modelica.Units.SI.Angle trackingError = reference.y - angleSensor.phi;

  equation
    connect(reference.y, controller.u_s)
      annotation(Line(points = {{-129, 60}, {-107, 60}}, color = {0, 0, 127}));
    connect(angleSensor.phi, controller.u_m)
      annotation(Line(points = {{-9, -65}, {-82, -65}, {-82, 53}}, color = {0, 0, 127}));
    connect(controller.y, voltageSource.v)
      annotation(Line(points = {{-82, 60}, {-55, 60}, {-55, 32}}, color = {0, 0, 127}));

    connect(voltageSource.p, currentSensor.p)
      annotation(Line(points = {{-45, 20}, {-25, 20}}, color = {0, 0, 255}));
    connect(currentSensor.n, armatureResistance.p)
      annotation(Line(points = {{-5, 20}, {5, 20}}, color = {0, 0, 255}));
    connect(armatureResistance.n, armatureInductance.p)
      annotation(Line(points = {{25, 20}, {45, 20}}, color = {0, 0, 255}));
    connect(armatureInductance.n, motor.p)
      annotation(Line(points = {{65, 20}, {90, 20}}, color = {0, 0, 255}));
    connect(motor.n, voltageSource.n)
      annotation(Line(points = {{110, 20}, {120, 20}, {120, 0}, {-55, 0}, {-55, 10}}, color = {0, 0, 255}));
    connect(voltageSource.n, ground.p)
      annotation(Line(points = {{-55, 10}, {-55, -10}}, color = {0, 0, 255}));

    connect(motor.flange, motorInertia.flange_a)
      annotation(Line(points = {{100, 10}, {100, -15}}, color = {0, 0, 0}));
    connect(motorInertia.flange_b, gear.flange_a)
      annotation(Line(points = {{90, -25}, {70, -25}}, color = {0, 0, 0}));
    connect(gear.flange_b, torqueSensor.flange_a)
      annotation(Line(points = {{50, -25}, {30, -25}}, color = {0, 0, 0}));
    connect(torqueSensor.flange_b, loadInertia.flange_a)
      annotation(Line(points = {{10, -25}, {-10, -25}}, color = {0, 0, 0}));
    connect(loadInertia.flange_b, jointDamping.flange_a)
      annotation(Line(points = {{-30, -25}, {-45, -25}, {-45, -65}, {-50, -65}}, color = {0, 0, 0}));
    connect(jointDamping.flange_b, fixed.flange)
      annotation(Line(points = {{-70, -65}, {-95, -65}}, color = {0, 0, 0}));
    connect(loadInertia.flange_b, angleSensor.flange)
      annotation(Line(points = {{-30, -25}, {-35, -25}, {-35, -50}, {-20, -50}, {-20, -55}}, color = {0, 0, 0}));
    connect(loadInertia.flange_b, speedSensor.flange)
      annotation(Line(points = {{-30, -25}, {-35, -25}, {-35, -45}, {20, -45}, {20, -55}}, color = {0, 0, 0}));

    annotation(
      Diagram(coordinateSystem(extent = {{-160, -100}, {130, 90}})),
      experiment(StartTime = 0, StopTime = 2, Interval = 0.001, Tolerance = 1e-7),
      Documentation(info = "<html><p>Minimal T0 probe using only Modelica Standard Library components. A limited PI controller drives a DC motor, ideal gearbox, inertial load and viscous joint.</p></html>"));
  end SingleJointProbe;
end AIDCT0;
