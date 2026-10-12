within AIDCRobotics;
package Sensors "Robot sensor templates and frame conventions"
  extends Modelica.Icons.SensorsPackage;

  model IdealJointSensor "Ideal rotational joint state sensor"
    Modelica.Mechanics.Rotational.Interfaces.Flange_a flange
      annotation(Placement(transformation(extent = {{-110, -10}, {-90, 10}})));
    AIDCRobotics.Interfaces.JointStateOutput state
      annotation(Placement(transformation(extent = {{80, -10}, {100, 10}})));
  protected
    Modelica.Units.SI.AngularVelocity w = der(flange.phi);
  equation
    flange.tau = 0;
    state.position = flange.phi;
    state.velocity = w;
    state.acceleration = der(w);
    state.appliedTorque = 0;
    annotation(Icon(coordinateSystem(preserveAspectRatio = false), graphics = {
      Ellipse(extent = {{-70, 70}, {70, -70}}, lineColor = {0, 120, 110}, fillColor = {232, 247, 244}, fillPattern = FillPattern.Solid),
      Line(points = {{0, 0}, {38, 36}}, color = {0, 90, 84}, thickness = 1),
      Ellipse(extent = {{-8, 8}, {8, -8}}, lineColor = {0, 90, 84}, fillColor = {0, 90, 84}, fillPattern = FillPattern.Solid),
      Text(extent = {{-64, -36}, {64, -62}}, textString = "SENSOR", textColor = {0, 90, 84})}));
  end IdealJointSensor;

  block ImuSignalAdapter "Convert geometry xyzw orientation to public wxyz IMU state"
    Modelica.Blocks.Interfaces.RealInput orientationXyzw[4]
      annotation(Placement(transformation(extent = {{-120, 50}, {-80, 90}})));
    Modelica.Blocks.Interfaces.RealInput angularVelocity[3](each unit = "rad/s")
      annotation(Placement(transformation(extent = {{-120, -20}, {-80, 20}})));
    Modelica.Blocks.Interfaces.RealInput linearAcceleration[3](each unit = "m/s2")
      annotation(Placement(transformation(extent = {{-120, -90}, {-80, -50}})));
    Modelica.Blocks.Interfaces.RealOutput orientationWxyz[4]
      annotation(Placement(transformation(extent = {{80, 50}, {100, 70}})));
    Modelica.Blocks.Interfaces.RealOutput gyro[3](each unit = "rad/s")
      annotation(Placement(transformation(extent = {{80, -10}, {100, 10}})));
    Modelica.Blocks.Interfaces.RealOutput accelerometer[3](each unit = "m/s2")
      annotation(Placement(transformation(extent = {{80, -70}, {100, -50}})));
  equation
    orientationWxyz = {orientationXyzw[4], orientationXyzw[1], orientationXyzw[2], orientationXyzw[3]};
    gyro = angularVelocity;
    accelerometer = linearAcceleration;
    annotation(Icon(coordinateSystem(preserveAspectRatio = false), graphics = {
      Rectangle(extent = {{-76, 66}, {76, -66}}, radius = 10, lineColor = {0, 120, 110}, fillColor = {232, 247, 244}, fillPattern = FillPattern.Solid),
      Ellipse(extent = {{-32, 32}, {32, -32}}, lineColor = {0, 120, 110}, fillColor = {255, 255, 255}, fillPattern = FillPattern.Solid),
      Line(points = {{0, 0}, {0, 48}}, color = {0, 90, 84}, thickness = 1),
      Line(points = {{0, 0}, {44, -24}}, color = {180, 100, 0}, thickness = 1),
      Line(points = {{0, 0}, {-38, -28}}, color = {0, 90, 84}, thickness = 1),
      Text(extent = {{-68, -38}, {68, -60}}, textString = "IMU", textColor = {0, 90, 84})}));
  end ImuSignalAdapter;
end Sensors;
