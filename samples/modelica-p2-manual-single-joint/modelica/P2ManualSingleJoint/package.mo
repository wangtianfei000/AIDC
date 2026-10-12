within ;
package P2ManualSingleJoint "Manual P2 parity reference"
  model Robot
    inner Modelica.Mechanics.MultiBody.World world(enableAnimation = false,
      gravityType = Modelica.Mechanics.MultiBody.Types.GravityTypes.NoGravity);
    AIDCRobotics.Mechanics.FixedBase3D base;
    AIDCRobotics.Mechanics.FixedJoint3D jointOrigin(
      translation = {0, 0, 0}, rotationAxis = {0, 0, 1}, rotationAngle = 0);
    AIDCRobotics.Mechanics.RevoluteJoint3D revolute(
      axis = {0, 0, 1}, initialPosition = 0);
    AIDCRobotics.Mechanics.RigidBody3D link(
      mass = 0.25, centerOfMass = {0.05, 0, 0},
      I_11 = 0.0001, I_22 = 0.01, I_33 = 0.01,
      I_21 = 0, I_31 = 0, I_32 = 0);
    AIDCRobotics.Actuators.IdealTorque actuator;
    AIDCRobotics.Mechanics.RotationalJointLoss jointLoss(
      damping = 0.2, coulombFriction = 0,
      lowerLimit = -1.5707963267948966, upperLimit = 1.5707963267948966);
    Modelica.Blocks.Interfaces.RealInput torqueCommand(unit = "N.m");
    output Modelica.Units.SI.Angle jointPosition = revolute.joint.phi;
    output Modelica.Units.SI.AngularVelocity jointVelocity = der(revolute.joint.phi);
    output Modelica.Units.SI.Torque appliedTorque = torqueCommand;
  equation
    connect(base.frame_b, jointOrigin.frame_a);
    connect(jointOrigin.frame_b, revolute.frame_a);
    connect(revolute.frame_b, link.frame_a);
    connect(torqueCommand, actuator.command);
    connect(actuator.flange, revolute.drive);
    connect(revolute.drive, jointLoss.flange_a);
    connect(revolute.support, jointLoss.flange_b);
  end Robot;

  model Experiment
    Modelica.Blocks.Sources.Step command(height = 0.1, startTime = 0.1);
    Robot robot;
    output Modelica.Units.SI.Angle jointPosition = robot.jointPosition;
    output Modelica.Units.SI.AngularVelocity jointVelocity = robot.jointVelocity;
    output Modelica.Units.SI.Torque appliedTorque = robot.appliedTorque;
  equation
    connect(command.y, robot.torqueCommand);
    annotation(experiment(StartTime = 0, StopTime = 2, Interval = 0.001, Tolerance = 1e-8));
  end Experiment;
  annotation(uses(Modelica(version = "4.1.0"), AIDCRobotics(version = "0.2.0")));
end P2ManualSingleJoint;
