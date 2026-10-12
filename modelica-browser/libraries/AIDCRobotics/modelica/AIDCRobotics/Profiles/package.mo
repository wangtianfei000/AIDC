within AIDCRobotics;
package Profiles "Composable robot capability contracts"
  extends Modelica.Icons.Package;

  partial model ArticulatedRobot "Ordered joint command and state contract"
    parameter Integer nJoints(min = 1) = 1;
    parameter String jointIds[nJoints] = {"joint-" + String(i) for i in 1:nJoints};
    Modelica.Blocks.Interfaces.RealInput jointTorqueCommand[nJoints](each unit = "N.m");
    Modelica.Blocks.Interfaces.RealOutput jointPosition[nJoints](each unit = "rad");
    Modelica.Blocks.Interfaces.RealOutput jointVelocity[nJoints](each unit = "rad/s");
    Modelica.Blocks.Interfaces.RealOutput jointTorque[nJoints](each unit = "N.m");
  end ArticulatedRobot;

  partial model FloatingBaseRobot "Six degree-of-freedom base state contract"
    Modelica.Blocks.Interfaces.RealOutput basePosition[3](each unit = "m");
    Modelica.Blocks.Interfaces.RealOutput baseOrientationWxyz[4];
    Modelica.Blocks.Interfaces.RealOutput baseLinearVelocity[3](each unit = "m/s");
    Modelica.Blocks.Interfaces.RealOutput baseAngularVelocity[3](each unit = "rad/s");
  end FloatingBaseRobot;

  partial model BatteryPoweredRobot "DC supply telemetry contract"
    Modelica.Blocks.Interfaces.RealOutput busVoltage(unit = "V");
    Modelica.Blocks.Interfaces.RealOutput totalCurrent(unit = "A");
    Modelica.Blocks.Interfaces.RealOutput stateOfCharge(unit = "1");
  end BatteryPoweredRobot;

  partial model ImuEquippedRobot "Public scalar-first IMU contract"
    Modelica.Blocks.Interfaces.RealOutput imuOrientationWxyz[4];
    Modelica.Blocks.Interfaces.RealOutput imuAngularVelocity[3](each unit = "rad/s");
    Modelica.Blocks.Interfaces.RealOutput imuLinearAcceleration[3](each unit = "m/s2");
  end ImuEquippedRobot;
end Profiles;
