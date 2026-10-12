within AIDCRobotics;
package DeviceData "Physical device parameter records separated from equations"
  extends Modelica.Icons.MaterialPropertiesPackage;

  record ServoData "Common output-side servo specification"
    String manufacturer = "generic";
    String modelName = "generic-servo";
    Modelica.Units.SI.Voltage nominalVoltage;
    Modelica.Units.SI.AngularVelocity noLoadSpeed;
    Modelica.Units.SI.Torque stallTorque;
    Modelica.Units.SI.Current stallCurrent;
    Modelica.Units.SI.Torque ratedTorque;
    Modelica.Units.SI.Current ratedCurrent;
    Real gearRatio(min = Modelica.Constants.small);
  end ServoData;

  package Feetech "Feetech device records"
    extends Modelica.Icons.MaterialPropertiesPackage;
    constant AIDCRobotics.DeviceData.ServoData HD1910(
      manufacturer = "Feetech",
      modelName = "HD-1910-C001",
      nominalVoltage = 6,
      noLoadSpeed = 92 * 2 * Modelica.Constants.pi / 60,
      stallTorque = 12 * 9.80665 / 100,
      stallCurrent = 1.6,
      ratedTorque = 3 * 9.80665 / 100,
      ratedCurrent = 0.69,
      gearRatio = 320) "Manufacturer 6 V calibration point";
  end Feetech;
end DeviceData;
