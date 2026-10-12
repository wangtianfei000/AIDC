within AIDCRobotics;
package Interfaces "Stable robot command, state and diagnostics connectors"
  extends Modelica.Icons.InterfacesPackage;

  connector TorqueCommand = Modelica.Blocks.Interfaces.RealInput(
    final quantity = "Torque",
    final unit = "N.m") "Single joint torque command";

  connector PositionCommand = Modelica.Blocks.Interfaces.RealInput(
    final quantity = "Angle",
    final unit = "rad",
    displayUnit = "deg") "Single joint position command";

  connector JointStateOutput "Single joint state in the standard positive direction"
    output Modelica.Units.SI.Angle position;
    output Modelica.Units.SI.AngularVelocity velocity;
    output Modelica.Units.SI.AngularAcceleration acceleration;
    output Modelica.Units.SI.Torque appliedTorque;
  end JointStateOutput;

  connector DiagnosticOutput "Portable component status"
    output Boolean valid "All published values are finite and usable";
    output Boolean limited "A configured physical or command limit is active";
    output Integer statusCode "0 means nominal; nonzero values are component-specific";
  end DiagnosticOutput;

  partial model PartialTorqueControlledJoint "Required interface for a torque-controlled axis"
    parameter String jointId = "joint" "Stable Robot IR joint identifier";
    parameter Integer channelIndex(min = 1) = 1 "One-based interface order";
    parameter Integer direction(min = -1, max = 1) = 1 "Command sign, constrained to +1 or -1";
    TorqueCommand command annotation(Placement(transformation(extent = {{-120, -20}, {-80, 20}})));
    JointStateOutput state annotation(Placement(transformation(extent = {{80, 10}, {100, 30}})));
    DiagnosticOutput diagnostic annotation(Placement(transformation(extent = {{80, -30}, {100, -10}})));
  end PartialTorqueControlledJoint;

  annotation(Documentation(info = "<html><p>All physical signals use SI units. Public multi-joint adapters must publish stable channel metadata in Robot IR; connection or declaration order is not a protocol.</p></html>"));
end Interfaces;
