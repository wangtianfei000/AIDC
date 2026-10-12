within AIDCRobotics;
package Contacts "Regularized analytic contact components"
  extends Modelica.Icons.Package;

  partial block PartialPlaneContact "Common plane contact law; geometry is causal and stateless"
    parameter Modelica.Units.SI.Position planePoint[3] = {0, 0, 0};
    parameter Real planeNormal[3] = {0, 0, 1};
    parameter Modelica.Units.SI.TranslationalSpringConstant stiffness(min = 0) = 20000;
    parameter Modelica.Units.SI.TranslationalDampingConstant damping(min = 0) = 100;
    parameter Real exponent(min = 1) = 1.5;
    parameter Real frictionCoefficient(min = 0) = 0.8;
    parameter Modelica.Units.SI.Velocity regularizationVelocity(min = Modelica.Constants.small) = 0.01;
    parameter Modelica.Units.SI.Length supportRadius(min = 0) = 0;
    input Modelica.Units.SI.Position position[3];
    input Modelica.Units.SI.Velocity velocity[3];
    output Modelica.Units.SI.Position contactPoint[3];
    output Real normal[3];
    output Modelica.Units.SI.Length signedDistance;
    output Modelica.Units.SI.Length penetration;
    output Modelica.Units.SI.Force normalForce;
    output Modelica.Units.SI.Force force[3];
    output Modelica.Units.SI.Energy elasticEnergy;
    output Modelica.Units.SI.Power dissipatedPower;
  protected
    Real tangentVelocity[3](each unit = "m/s");
    Modelica.Units.SI.Velocity normalVelocity;
    Modelica.Units.SI.Velocity tangentSpeed;
    Modelica.Units.SI.Force springForce;
    Modelica.Units.SI.Force dampingForce;
  equation
    normal = planeNormal / sqrt(planeNormal * planeNormal);
    signedDistance = (position - planePoint) * normal - supportRadius;
    penetration = max(0, -signedDistance);
    contactPoint = position - supportRadius * normal + 0.5 * penetration * normal;
    normalVelocity = velocity * normal;
    tangentVelocity = velocity - normalVelocity * normal;
    tangentSpeed = sqrt(tangentVelocity * tangentVelocity);
    springForce = if penetration > 0 then stiffness * penetration ^ exponent else 0;
    dampingForce = if penetration > 0 then damping * max(0, -normalVelocity) else 0;
    normalForce = springForce + dampingForce;
    force = normalForce * normal - frictionCoefficient * normalForce * tangentVelocity / sqrt(tangentSpeed ^ 2 + regularizationVelocity ^ 2);
    elasticEnergy = stiffness * penetration ^ (exponent + 1) / (exponent + 1);
    dissipatedPower = dampingForce * max(0, -normalVelocity) + frictionCoefficient * normalForce * tangentSpeed ^ 2 / sqrt(tangentSpeed ^ 2 + regularizationVelocity ^ 2);
  end PartialPlaneContact;

  block PointPlaneContact "Point against an infinite plane"
    extends PartialPlaneContact(final supportRadius = 0);
    annotation(Icon(graphics = {Ellipse(extent = {{-12, 12}, {12, -12}}, lineColor = {0, 100, 90}, fillColor = {0, 100, 90}, fillPattern = FillPattern.Solid), Line(points = {{-82, -40}, {82, -40}}, color = {0, 90, 100})}));
  end PointPlaneContact;

  block SpherePlaneContact "Sphere against an infinite plane"
    parameter Modelica.Units.SI.Radius radius(min = 0) = 0.05;
    extends PartialPlaneContact(final supportRadius = radius);
    annotation(Icon(graphics = {Ellipse(extent = {{-48, 48}, {48, -48}}, lineColor = {0, 100, 90}, fillColor = {230, 247, 244}, fillPattern = FillPattern.Solid), Line(points = {{-82, -58}, {82, -58}}, color = {0, 90, 100})}));
  end SpherePlaneContact;

  block BoxPlaneContact "Axis-aligned box against an infinite plane"
    parameter Modelica.Units.SI.Length halfExtents[3] = {0.05, 0.05, 0.05};
    extends PartialPlaneContact(final supportRadius = abs(planeNormal) * halfExtents);
    annotation(Icon(graphics = {Rectangle(extent = {{-52, 42}, {52, -42}}, lineColor = {0, 100, 90}, fillColor = {230, 247, 244}, fillPattern = FillPattern.Solid), Line(points = {{-82, -58}, {82, -58}}, color = {0, 90, 100})}));
  end BoxPlaneContact;

  block CapsulePlaneContact "Capsule against an infinite plane"
    parameter Modelica.Units.SI.Radius radius(min = 0) = 0.02;
    parameter Modelica.Units.SI.Length halfLength(min = 0) = 0.05;
    parameter Real capsuleAxis[3] = {1, 0, 0};
    extends PartialPlaneContact(final supportRadius = radius + halfLength * abs(capsuleAxis * planeNormal) / sqrt(capsuleAxis * capsuleAxis));
    annotation(Icon(graphics = {Rectangle(extent = {{-48, 24}, {48, -24}}, radius = 24, lineColor = {0, 100, 90}, fillColor = {230, 247, 244}, fillPattern = FillPattern.Solid), Line(points = {{-82, -58}, {82, -58}}, color = {0, 90, 100})}));
  end CapsulePlaneContact;

  block FootPatchPlaneContact "Four-point rectangular foot contact proxy"
    parameter Modelica.Units.SI.Position offsets[4, 3] = {{-0.03, -0.02, 0}, {-0.03, 0.02, 0}, {0.03, -0.02, 0}, {0.03, 0.02, 0}};
    parameter Modelica.Units.SI.Position planePoint[3] = {0, 0, 0};
    parameter Real planeNormal[3] = {0, 0, 1};
    parameter Modelica.Units.SI.TranslationalSpringConstant stiffness(min = 0) = 5000;
    parameter Modelica.Units.SI.TranslationalDampingConstant damping(min = 0) = 25;
    parameter Real frictionCoefficient(min = 0) = 0.8;
    input Modelica.Units.SI.Position position[3];
    input Modelica.Units.SI.Velocity velocity[3];
    output Modelica.Units.SI.Force force[3];
    output Modelica.Units.SI.Length penetration[4];
    output Modelica.Units.SI.Position contactPoint[4, 3];
    output Modelica.Units.SI.Energy elasticEnergy;
    output Modelica.Units.SI.Power dissipatedPower;
    PointPlaneContact points[4](each planePoint = planePoint, each planeNormal = planeNormal, each stiffness = stiffness, each damping = damping, each frictionCoefficient = frictionCoefficient);
  equation
    for i in 1:4 loop
      points[i].position = position + offsets[i, :];
      points[i].velocity = velocity;
      penetration[i] = points[i].penetration;
      contactPoint[i, :] = points[i].contactPoint;
    end for;
    for j in 1:3 loop
      force[j] = sum(points[i].force[j] for i in 1:4);
    end for;
    elasticEnergy = sum(points.elasticEnergy);
    dissipatedPower = sum(points.dissipatedPower);
    annotation(Icon(graphics = {Rectangle(extent = {{-64, 42}, {64, -42}}, lineColor = {0, 100, 90}, fillColor = {230, 247, 244}, fillPattern = FillPattern.Solid), Ellipse(extent = {{-52, 30}, {-40, 18}}, fillColor = {0, 100, 90}, fillPattern = FillPattern.Solid), Ellipse(extent = {{40, 30}, {52, 18}}, fillColor = {0, 100, 90}, fillPattern = FillPattern.Solid), Ellipse(extent = {{-52, -18}, {-40, -30}}, fillColor = {0, 100, 90}, fillPattern = FillPattern.Solid), Ellipse(extent = {{40, -18}, {52, -30}}, fillColor = {0, 100, 90}, fillPattern = FillPattern.Solid)}));
  end FootPatchPlaneContact;
end Contacts;
