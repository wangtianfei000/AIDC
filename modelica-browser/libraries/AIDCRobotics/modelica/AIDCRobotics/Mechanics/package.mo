within AIDCRobotics;
package Mechanics "Reusable mechanical components"
  extends Modelica.Icons.Package;

  model StandardTorqueAxis "P0 torque-driven compliant rotational axis"
    extends AIDCRobotics.Interfaces.PartialTorqueControlledJoint;

    parameter Modelica.Units.SI.Inertia inertia(min = Modelica.Constants.small) = 0.01;
    parameter Modelica.Units.SI.RotationalSpringConstant stiffness(min = Modelica.Constants.small) = 2;
    parameter Modelica.Units.SI.RotationalDampingConstant damping(min = 0) = 0.1;
    parameter Modelica.Units.SI.Angle phi_start = 0;
    parameter Modelica.Units.SI.AngularVelocity w_start = 0;

    Modelica.Mechanics.Rotational.Sources.Torque actuator(useSupport = false)
      annotation(Placement(transformation(extent = {{-60, -10}, {-40, 10}})));
    Modelica.Mechanics.Rotational.Components.Inertia load(
      J = inertia,
      phi(start = phi_start, fixed = true),
      w(start = w_start, fixed = true))
      annotation(Placement(transformation(extent = {{-20, -10}, {0, 10}})));
    Modelica.Mechanics.Rotational.Components.SpringDamper compliance(c = stiffness, d = damping)
      annotation(Placement(transformation(extent = {{20, -10}, {40, 10}})));
    Modelica.Mechanics.Rotational.Components.Fixed fixed
      annotation(Placement(transformation(extent = {{60, -10}, {80, 10}})));

  protected
    Modelica.Units.SI.AngularVelocity velocity = der(load.phi);
    Modelica.Units.SI.AngularAcceleration acceleration = der(velocity);

  equation
    assert(direction == 1 or direction == -1, "AIDCRobotics direction must be +1 or -1");
    actuator.tau = direction * command;
    connect(actuator.flange, load.flange_a)
      annotation(Line(points = {{-40, 0}, {-20, 0}}, color = {0, 0, 0}));
    connect(load.flange_b, compliance.flange_a)
      annotation(Line(points = {{0, 0}, {20, 0}}, color = {0, 0, 0}));
    connect(compliance.flange_b, fixed.flange)
      annotation(Line(points = {{40, 0}, {70, 0}}, color = {0, 0, 0}));
    state.position = direction * load.phi;
    state.velocity = direction * velocity;
    state.acceleration = direction * acceleration;
    state.appliedTorque = direction * actuator.tau;
    diagnostic.valid = true;
    diagnostic.limited = false;
    diagnostic.statusCode = 0;

    annotation(
      Icon(graphics = {
        Rectangle(extent = {{-80, 50}, {80, -50}}, radius = 12, lineColor = {0, 120, 105}, fillColor = {230, 247, 244}, fillPattern = FillPattern.Solid),
        Ellipse(extent = {{-26, 26}, {26, -26}}, lineColor = {0, 100, 90}),
        Line(points = {{-70, 0}, {-26, 0}}, color = {0, 100, 90}),
        Line(points = {{26, 0}, {70, 0}}, color = {0, 100, 90}),
        Text(extent = {{-78, 48}, {78, 24}}, textString = "%jointId", textColor = {0, 80, 72})}),
      Documentation(info = "<html><p>Minimal standard axis used to freeze P0 command/state semantics. Higher-fidelity actuators may redeclare the implementation while retaining the public interface.</p></html>"));
  end StandardTorqueAxis;

  model RigidBody3D "Rigid body with explicit CAD mass properties"
    parameter Modelica.Units.SI.Mass mass(min = Modelica.Constants.small) = 1;
    parameter Modelica.Units.SI.Position centerOfMass[3] = {0, 0, 0};
    parameter Modelica.Units.SI.Inertia I_11(min = Modelica.Constants.small) = 1e-3;
    parameter Modelica.Units.SI.Inertia I_22(min = Modelica.Constants.small) = 1e-3;
    parameter Modelica.Units.SI.Inertia I_33(min = Modelica.Constants.small) = 1e-3;
    parameter Modelica.Units.SI.Inertia I_21 = 0;
    parameter Modelica.Units.SI.Inertia I_31 = 0;
    parameter Modelica.Units.SI.Inertia I_32 = 0;
    Modelica.Mechanics.MultiBody.Interfaces.Frame_a frame_a
      annotation(Placement(transformation(extent = {{-116, -16}, {-84, 16}})));
    Modelica.Mechanics.MultiBody.Interfaces.Frame_b frame_b
      annotation(Placement(transformation(extent = {{84, -16}, {116, 16}})));
    Modelica.Mechanics.MultiBody.Parts.Body body(
      m = mass, r_CM = centerOfMass,
      I_11 = I_11, I_22 = I_22, I_33 = I_33,
      I_21 = I_21, I_31 = I_31, I_32 = I_32)
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}})));
  equation
    connect(frame_a, body.frame_a)
      annotation(Line(points = {{-100, 0}, {-10, 0}}, color = {95, 95, 95}, thickness = 0.5));
    connect(body.frame_a, frame_b)
      annotation(Line(points = {{10, 0}, {100, 0}}, color = {95, 95, 95}, thickness = 0.5));
    annotation(Icon(graphics = {Rectangle(extent = {{-80, 50}, {80, -50}}, lineColor = {0, 100, 90}, fillColor = {225, 244, 241}, fillPattern = FillPattern.Solid), Text(extent = {{-74, 18}, {74, -18}}, textString = "m=%mass kg", fontSize = 11)}));
  end RigidBody3D;

  model FixedBase3D "Fixed anchor in the enclosing MultiBody world"
    outer Modelica.Mechanics.MultiBody.World world;
    parameter Modelica.Units.SI.Position origin[3] = {0, 0, 0};
    Modelica.Mechanics.MultiBody.Interfaces.Frame_b frame_b
      annotation(Placement(transformation(extent = {{84, -16}, {116, 16}})));
    Modelica.Mechanics.MultiBody.Parts.Fixed fixed(r = origin)
      annotation(Placement(transformation(extent = {{-20, -10}, {0, 10}})));
  equation
    connect(fixed.frame_b, frame_b)
      annotation(Line(points = {{0, 0}, {100, 0}}, color = {95, 95, 95}, thickness = 0.5));
    annotation(Icon(graphics = {
      Line(points = {{-72, -42}, {72, -42}}, color = {0, 90, 100}, thickness = 1.5),
      Line(points = {{-54, -42}, {-38, -62}}, color = {0, 90, 100}),
      Line(points = {{-18, -42}, {-2, -62}}, color = {0, 90, 100}),
      Line(points = {{18, -42}, {34, -62}}, color = {0, 90, 100}),
      Line(points = {{54, -42}, {70, -62}}, color = {0, 90, 100}),
      Line(points = {{0, -42}, {0, 42}, {80, 42}}, color = {0, 90, 100}, thickness = 2)}));
  end FixedBase3D;

  model FloatingBase3D "Six degree-of-freedom connection between reference and base frame"
    parameter Modelica.Units.SI.Position r_start[3] = {0, 0, 0};
    parameter Modelica.Units.SI.Angle angles_start[3] = {0, 0, 0};
    parameter Modelica.Units.SI.Velocity v_start[3] = {0, 0, 0};
    parameter Modelica.Units.SI.AngularVelocity w_start[3] = {0, 0, 0};
    Modelica.Mechanics.MultiBody.Interfaces.Frame_a frame_a
      annotation(Placement(transformation(extent = {{-116, -16}, {-84, 16}})));
    Modelica.Mechanics.MultiBody.Interfaces.Frame_b frame_b
      annotation(Placement(transformation(extent = {{84, -16}, {116, 16}})));
    Modelica.Mechanics.MultiBody.Joints.FreeMotion freeMotion(
      r_rel_a(start = r_start, each fixed = true),
      v_rel_a(start = v_start, each fixed = true),
      angles_fixed = true,
      angles_start = angles_start,
      w_rel_a_fixed = true,
      w_rel_a_start = w_start)
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}})));
  equation
    connect(frame_a, freeMotion.frame_a)
      annotation(Line(points = {{-100, 0}, {-10, 0}}, color = {95, 95, 95}, thickness = 0.5));
    connect(freeMotion.frame_b, frame_b)
      annotation(Line(points = {{10, 0}, {100, 0}}, color = {95, 95, 95}, thickness = 0.5));
    annotation(Icon(graphics = {
      Ellipse(extent = {{-48, 48}, {48, -48}}, lineColor = {0, 100, 90}, fillColor = {230, 247, 244}, fillPattern = FillPattern.Solid),
      Line(points = {{-82, 0}, {-48, 0}}, color = {0, 90, 100}, thickness = 2),
      Line(points = {{48, 0}, {82, 0}}, color = {0, 90, 100}, thickness = 2),
      Text(extent = {{-42, 18}, {42, -18}}, textString = "6 DOF", textColor = {0, 80, 72})}));
  end FloatingBase3D;

  model FixedJoint3D "Fixed parent-to-child frame transform"
    parameter Modelica.Units.SI.Position translation[3] = {0, 0, 0};
    parameter Real rotationAxis[3] = {0, 0, 1};
    parameter Modelica.Units.SI.Angle rotationAngle = 0;
    Modelica.Mechanics.MultiBody.Interfaces.Frame_a frame_a
      annotation(Placement(transformation(extent = {{-116, -16}, {-84, 16}})));
    Modelica.Mechanics.MultiBody.Interfaces.Frame_b frame_b
      annotation(Placement(transformation(extent = {{84, -16}, {116, 16}})));
    Modelica.Mechanics.MultiBody.Parts.FixedRotation transform(
      r = translation,
      rotationType = Modelica.Mechanics.MultiBody.Types.RotationTypes.RotationAxis,
      n = rotationAxis,
      angle = rotationAngle)
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}})));
  equation
    connect(frame_a, transform.frame_a)
      annotation(Line(points = {{-100, 0}, {-10, 0}}, color = {95, 95, 95}, thickness = 0.5));
    connect(transform.frame_b, frame_b)
      annotation(Line(points = {{10, 0}, {100, 0}}, color = {95, 95, 95}, thickness = 0.5));
    annotation(Icon(graphics = {
      Line(points = {{-82, 0}, {82, 0}}, color = {0, 90, 100}, thickness = 2),
      Rectangle(extent = {{-28, 28}, {28, -28}}, lineColor = {0, 100, 90}, fillColor = {230, 247, 244}, fillPattern = FillPattern.Solid),
      Text(extent = {{-24, 16}, {24, -16}}, textString = "F", textColor = {0, 80, 72})}));
  end FixedJoint3D;

  model RevoluteJoint3D "Standard 3D revolute joint with exposed drive flanges"
    parameter Real axis[3] = {0, 0, 1};
    parameter Modelica.Units.SI.Angle initialPosition = 0;
    Modelica.Mechanics.MultiBody.Interfaces.Frame_a frame_a
      annotation(Placement(transformation(extent = {{-116, -16}, {-84, 16}})));
    Modelica.Mechanics.MultiBody.Interfaces.Frame_b frame_b
      annotation(Placement(transformation(extent = {{84, -16}, {116, 16}})));
    Modelica.Mechanics.Rotational.Interfaces.Flange_a drive
      annotation(Placement(transformation(extent = {{-10, 90}, {10, 110}})));
    Modelica.Mechanics.Rotational.Interfaces.Flange_b support
      annotation(Placement(transformation(extent = {{-10, -110}, {10, -90}})));
    Modelica.Mechanics.MultiBody.Joints.Revolute joint(
      n = axis, useAxisFlange = true,
      phi(start = initialPosition, fixed = true))
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}})));
  equation
    connect(frame_a, joint.frame_a)
      annotation(Line(points = {{-100, 0}, {-10, 0}}, color = {95, 95, 95}, thickness = 0.5));
    connect(joint.frame_b, frame_b)
      annotation(Line(points = {{10, 0}, {100, 0}}, color = {95, 95, 95}, thickness = 0.5));
    connect(joint.axis, drive)
      annotation(Line(points = {{0, 10}, {0, 100}}, color = {0, 0, 0}));
    connect(joint.support, support)
      annotation(Line(points = {{0, -10}, {0, -100}}, color = {0, 0, 0}));
    annotation(Icon(graphics = {
      Line(points = {{-82, 0}, {-34, 0}}, color = {0, 90, 100}, thickness = 2),
      Line(points = {{34, 0}, {82, 0}}, color = {0, 90, 100}, thickness = 2),
      Ellipse(extent = {{-34, 34}, {34, -34}}, lineColor = {0, 100, 90}, fillColor = {230, 247, 244}, fillPattern = FillPattern.Solid),
      Line(points = {{-16, 0}, {16, 0}}, color = {0, 90, 100}, thickness = 2),
      Line(points = {{0, -16}, {0, 16}}, color = {0, 90, 100}, thickness = 2)}));
  end RevoluteJoint3D;

  model PrismaticJoint3D "Standard 3D prismatic joint with exposed drive flanges"
    parameter Real axis[3] = {1, 0, 0};
    parameter Modelica.Units.SI.Position initialPosition = 0;
    Modelica.Mechanics.MultiBody.Interfaces.Frame_a frame_a
      annotation(Placement(transformation(extent = {{-116, -16}, {-84, 16}})));
    Modelica.Mechanics.MultiBody.Interfaces.Frame_b frame_b
      annotation(Placement(transformation(extent = {{84, -16}, {116, 16}})));
    Modelica.Mechanics.Translational.Interfaces.Flange_a drive
      annotation(Placement(transformation(extent = {{-10, 90}, {10, 110}})));
    Modelica.Mechanics.Translational.Interfaces.Flange_b support
      annotation(Placement(transformation(extent = {{-10, -110}, {10, -90}})));
    Modelica.Mechanics.MultiBody.Joints.Prismatic joint(
      n = axis, useAxisFlange = true,
      s(start = initialPosition, fixed = true))
      annotation(Placement(transformation(extent = {{-10, -10}, {10, 10}})));
  equation
    connect(frame_a, joint.frame_a)
      annotation(Line(points = {{-100, 0}, {-10, 0}}, color = {95, 95, 95}, thickness = 0.5));
    connect(joint.frame_b, frame_b)
      annotation(Line(points = {{10, 0}, {100, 0}}, color = {95, 95, 95}, thickness = 0.5));
    connect(joint.axis, drive)
      annotation(Line(points = {{0, 10}, {0, 100}}, color = {0, 127, 0}));
    connect(joint.support, support)
      annotation(Line(points = {{0, -10}, {0, -100}}, color = {0, 127, 0}));
    annotation(Icon(graphics = {
      Line(points = {{-82, 0}, {-34, 0}}, color = {0, 90, 100}, thickness = 2),
      Line(points = {{34, 0}, {82, 0}}, color = {0, 90, 100}, thickness = 2),
      Rectangle(extent = {{-34, 24}, {34, -24}}, lineColor = {0, 100, 90}, fillColor = {230, 247, 244}, fillPattern = FillPattern.Solid),
      Polygon(points = {{12, 12}, {28, 0}, {12, -12}, {12, 12}}, lineColor = {0, 90, 100}, fillColor = {0, 90, 100}, fillPattern = FillPattern.Solid)}));
  end PrismaticJoint3D;

  model RotationalJointLoss "Viscous/Coulomb loss and compliant end stops"
    parameter Modelica.Units.SI.RotationalDampingConstant damping(min = 0) = 0;
    parameter Modelica.Units.SI.Torque coulombFriction(min = 0) = 0;
    parameter Modelica.Units.SI.AngularVelocity frictionVelocity(min = Modelica.Constants.small) = 1e-3;
    parameter Modelica.Units.SI.Angle lowerLimit = -Modelica.Constants.inf;
    parameter Modelica.Units.SI.Angle upperLimit = Modelica.Constants.inf;
    parameter Modelica.Units.SI.RotationalSpringConstant stopStiffness(min = 0) = 1e5;
    parameter Modelica.Units.SI.RotationalDampingConstant stopDamping(min = 0) = 10;
    Modelica.Mechanics.Rotational.Interfaces.Flange_a flange_a
      annotation(Placement(transformation(extent = {{-110, -10}, {-90, 10}})));
    Modelica.Mechanics.Rotational.Interfaces.Flange_b flange_b
      annotation(Placement(transformation(extent = {{90, -10}, {110, 10}})));
    output Boolean lowerStopActive;
    output Boolean upperStopActive;
  protected
    Modelica.Units.SI.Angle phiRel;
    Modelica.Units.SI.AngularVelocity wRel;
    Modelica.Units.SI.Torque lossTorque;
  equation
    phiRel = flange_a.phi - flange_b.phi;
    wRel = der(phiRel);
    lowerStopActive = phiRel < lowerLimit;
    upperStopActive = phiRel > upperLimit;
    lossTorque = damping * wRel + coulombFriction * tanh(wRel / frictionVelocity)
      + (if lowerStopActive then stopStiffness * (phiRel - lowerLimit) + stopDamping * min(0, wRel) else 0)
      + (if upperStopActive then stopStiffness * (phiRel - upperLimit) + stopDamping * max(0, wRel) else 0);
    flange_a.tau = lossTorque;
    flange_b.tau = -lossTorque;
    annotation(Icon(graphics = {
      Rectangle(extent = {{-74, 46}, {74, -46}}, lineColor = {0, 100, 90}, fillColor = {230, 247, 244}, fillPattern = FillPattern.Solid),
      Line(points = {{-62, 0}, {-38, 0}, {-26, 24}, {-6, -24}, {14, 24}, {34, -24}, {46, 0}, {62, 0}}, color = {0, 90, 100}, thickness = 1),
      Text(extent = {{-68, 42}, {68, 20}}, textString = "LOSS", textColor = {0, 80, 72})}));
  end RotationalJointLoss;

  model RotationalBacklash "Replaceable elastic backlash transmission"
    parameter Modelica.Units.SI.Angle backlash(min = 0) = 0;
    parameter Modelica.Units.SI.RotationalSpringConstant stiffness(min = Modelica.Constants.small) = 1e5;
    parameter Modelica.Units.SI.RotationalDampingConstant damping(min = 0) = 1;
    Modelica.Mechanics.Rotational.Interfaces.Flange_a flange_a
      annotation(Placement(transformation(extent = {{-110, -10}, {-90, 10}})));
    Modelica.Mechanics.Rotational.Interfaces.Flange_b flange_b
      annotation(Placement(transformation(extent = {{90, -10}, {110, 10}})));
    Modelica.Mechanics.Rotational.Components.ElastoBacklash element(b = backlash, c = stiffness, d = damping)
      annotation(Placement(transformation(extent = {{-20, -10}, {20, 10}})));
  equation
    connect(flange_a, element.flange_a)
      annotation(Line(points = {{-100, 0}, {-20, 0}}, color = {0, 0, 0}));
    connect(element.flange_b, flange_b)
      annotation(Line(points = {{20, 0}, {100, 0}}, color = {0, 0, 0}));
    annotation(Icon(graphics = {
      Rectangle(extent = {{-74, 46}, {74, -46}}, lineColor = {0, 100, 90}, fillColor = {230, 247, 244}, fillPattern = FillPattern.Solid),
      Line(points = {{-62, 0}, {-28, 0}, {-18, 18}, {0, -18}, {18, 18}, {28, 0}, {62, 0}}, color = {0, 90, 100}, thickness = 1),
      Text(extent = {{-68, 42}, {68, 20}}, textString = "BACKLASH", textColor = {0, 80, 72})}));
  end RotationalBacklash;
end Mechanics;
