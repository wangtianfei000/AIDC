within AIDCRobotics;
package Core "Version, coordinate and metadata contracts"
  extends Modelica.Icons.Package;

  constant String standardId = "org.aidc.robotics" "Stable standard identifier";
  constant String standardVersion = "0.2.0" "Semantic version";
  constant String lengthUnit = "m";
  constant String angleUnit = "rad";
  constant String forwardAxis = "+X";
  constant String leftAxis = "+Y";
  constant String upAxis = "+Z";

  type QuaternionOrder = enumeration(
      wxyz "Scalar first; required for public RobotState and FMI interfaces",
      xyzw "Geometry interchange order used by Model3D assets");

  type CommandKind = enumeration(
      torque "Joint effort in N.m",
      position "Joint position in rad",
      velocity "Joint velocity in rad/s");

  record JointDescriptor "Stable joint channel metadata"
    String jointId "Stable Robot IR identifier";
    String displayName = jointId;
    Integer channelIndex(min = 1) "One-based public channel index";
    Integer direction(min = -1, max = 1) = 1 "Must be +1 or -1";
    Modelica.Units.SI.Angle zeroOffset = 0;
  end JointDescriptor;

  record InterfaceDescriptor "Public command/state interface identity"
    String interfaceId;
    String version = "1.0.0";
    Integer jointCount(min = 1);
    Modelica.Units.SI.Time samplePeriod(min = 0);
    QuaternionOrder quaternionOrder = QuaternionOrder.wxyz;
  end InterfaceDescriptor;

  annotation(Documentation(info = "<html><p>Core contracts are tool-independent. Names and array order are explicit metadata and must not be inferred from Modelica declaration order.</p></html>"));
end Core;
