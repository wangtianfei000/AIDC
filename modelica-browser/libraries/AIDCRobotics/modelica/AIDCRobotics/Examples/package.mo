within AIDCRobotics;
package Examples "Executable examples of the public standard"
  extends Modelica.Icons.ExamplesPackage;

  model SingleJoint "Minimal standard-library single joint"
    Modelica.Blocks.Sources.Step torqueCommand(height = 0.5, startTime = 0.1)
      annotation(Placement(transformation(extent = {{-80, -10}, {-60, 10}})));
    AIDCRobotics.Mechanics.StandardTorqueAxis axis(
      jointId = "joint-1",
      channelIndex = 1,
      direction = 1,
      inertia = 0.01,
      stiffness = 2,
      damping = 0.2)
      annotation(Placement(transformation(extent = {{-20, -20}, {20, 20}})));

    output Modelica.Units.SI.Angle jointPosition = axis.state.position;
    output Modelica.Units.SI.AngularVelocity jointVelocity = axis.state.velocity;
    output Modelica.Units.SI.Torque appliedTorque = axis.state.appliedTorque;
    output Boolean valid = axis.diagnostic.valid;

  equation
    connect(torqueCommand.y, axis.command)
      annotation(Line(points = {{-59, 0}, {-20, 0}}, color = {0, 0, 127}));

    annotation(
      experiment(StartTime = 0, StopTime = 2, Interval = 0.001, Tolerance = 1e-8),
      Documentation(info = "<html><p>P0 executable example. A 0.5 N.m step drives a compliant standard axis to the expected static angle of 0.25 rad.</p></html>"));
  end SingleJoint;

  model HD1910PositionStep "P1 reconstruction of the T2 HD-1910 position step"
    Modelica.Blocks.Sources.Step reference(height = 30 * Modelica.Constants.pi / 180, startTime = 0.2)
      annotation(Placement(transformation(extent = {{-90, 50}, {-70, 70}})));
    Modelica.Blocks.Sources.Constant load(k = 0.05)
      annotation(Placement(transformation(extent = {{-90, 10}, {-70, 30}})));
    Modelica.Blocks.Sources.Constant supply(k = 6)
      annotation(Placement(transformation(extent = {{-90, -30}, {-70, -10}})));
    Modelica.Blocks.Sources.Constant limit(k = 1.6)
      annotation(Placement(transformation(extent = {{-90, -70}, {-70, -50}})));
    AIDCRobotics.Actuators.LimitedPositionServoAxis servo
      annotation(Placement(transformation(extent = {{0, -50}, {80, 50}})));

    output Modelica.Units.SI.Angle jointAngle = servo.state.position;
    output Modelica.Units.SI.AngularVelocity jointSpeed = servo.state.velocity;
    output Modelica.Units.SI.Temperature windingTemperature = servo.windingTemperature;
    output Modelica.Units.SI.Energy electricalEnergy = servo.electricalEnergy;
    output Modelica.Units.SI.Angle referenceAngle = reference.y;
    output Modelica.Units.SI.Angle trackingError = servo.trackingError;
    output Modelica.Units.SI.Torque loadTorque = load.y;
    output Modelica.Units.SI.Torque jointTorque = servo.state.appliedTorque;
    output Modelica.Units.SI.Current armatureCurrent = servo.armatureCurrent;
    output Modelica.Units.SI.Voltage sourceVoltage = supply.y;
    output Modelica.Units.SI.Voltage busVoltage = servo.busVoltage;
    output Modelica.Units.SI.Voltage driveVoltage = servo.driveVoltage;
    output Modelica.Units.SI.Power electricalPower = servo.electricalPower;
    output Boolean currentLimited = servo.currentLimited;
    output Boolean voltageLimited = servo.voltageLimited;
  equation
    connect(reference.y, servo.target)
      annotation(Line(points = {{-69, 60}, {-20, 60}, {-20, 35}, {0, 35}}, color = {0, 0, 127}));
    connect(load.y, servo.loadTorque)
      annotation(Line(points = {{-69, 20}, {-20, 20}, {-20, 15}, {0, 15}}, color = {0, 0, 127}));
    connect(supply.y, servo.sourceVoltage)
      annotation(Line(points = {{-69, -20}, {-20, -20}, {-20, -10}, {0, -10}}, color = {0, 0, 127}));
    connect(limit.y, servo.currentLimit)
      annotation(Line(points = {{-69, -60}, {-20, -60}, {-20, -35}, {0, -35}}, color = {0, 0, 127}));
    annotation(
      Diagram(coordinateSystem(extent = {{-100, -80}, {100, 80}})),
      experiment(StartTime = 0, StopTime = 3, Interval = 0.002, Tolerance = 1e-7));
  end HD1910PositionStep;

  model BatteryAndImu "P1 battery and IMU interface smoke model"
    Modelica.Blocks.Sources.Constant requestedCurrent(k = 2)
      annotation(Placement(transformation(extent = {{-90, 30}, {-70, 50}})));
    AIDCRobotics.Electrical.SimpleBatteryBus battery(nominalVoltage = 6.4, internalResistance = 0.1, capacity_Ah = 1.8, maximumCurrent = 1.6)
      annotation(Placement(transformation(extent = {{-30, 10}, {30, 70}})));
    AIDCRobotics.Sensors.ImuSignalAdapter imu
      annotation(Placement(transformation(extent = {{-30, -70}, {30, -10}})));
  equation
    connect(requestedCurrent.y, battery.requestedCurrent)
      annotation(Line(points = {{-69, 40}, {-30, 40}}, color = {0, 0, 127}));
    imu.orientationXyzw = {0.1, 0.2, 0.3, 0.9};
    imu.angularVelocity = {1, 2, 3};
    imu.linearAcceleration = {4, 5, 6};
    annotation(
      Diagram(coordinateSystem(extent = {{-100, -80}, {100, 80}})),
      experiment(StartTime = 0, StopTime = 0.01, Interval = 0.001, Tolerance = 1e-8));
  end BatteryAndImu;

  model FixedBodyAssembly "P1 assembled MultiBody smoke model"
    inner Modelica.Mechanics.MultiBody.World world(enableAnimation = false, n = {0, 0, -1})
      annotation(Placement(transformation(extent = {{-90, -10}, {-70, 10}})));
    AIDCRobotics.Mechanics.FixedBase3D base
      annotation(Placement(transformation(extent = {{-55, -15}, {-25, 15}})));
    AIDCRobotics.Mechanics.FixedJoint3D mount(translation = {0, 0, 1})
      annotation(Placement(transformation(extent = {{-5, -15}, {25, 15}})));
    AIDCRobotics.Mechanics.RigidBody3D body(mass = 1, I_11 = 0.01, I_22 = 0.01, I_33 = 0.01)
      annotation(Placement(transformation(extent = {{45, -15}, {75, 15}})));
  equation
    connect(base.frame_b, mount.frame_a)
      annotation(Line(points = {{-25, 0}, {-5, 0}}, color = {95, 95, 95}, thickness = 0.5));
    connect(mount.frame_b, body.frame_a)
      annotation(Line(points = {{25, 0}, {45, 0}}, color = {95, 95, 95}, thickness = 0.5));
    annotation(
      Diagram(coordinateSystem(extent = {{-100, -60}, {100, 60}})),
      experiment(StartTime = 0, StopTime = 0.01, Interval = 0.001, Tolerance = 1e-8));
  end FixedBodyAssembly;
end Examples;
