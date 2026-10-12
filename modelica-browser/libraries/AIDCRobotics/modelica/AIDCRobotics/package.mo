within ;
package AIDCRobotics "AIDC reusable robotics modeling standard"
  extends Modelica.Icons.Package;

  annotation(
    version = "0.3.0",
    versionDate = "2026-10-10",
    preferredView = "info",
    uses(Modelica(version = "4.1.0")),
    Documentation(info = "<html><p>AIDCRobotics defines reusable Modelica interfaces and components for robot models. Public interfaces follow SI units, +X forward, +Y left, +Z up and scalar-first wxyz quaternions at FMI boundaries.</p><p>Version 0.3.0 adds P4 analytic contacts and the optional cached native geometry runtime. Robot-specific packages shall reference this library instead of copying common equations.</p></html>"));
end AIDCRobotics;
