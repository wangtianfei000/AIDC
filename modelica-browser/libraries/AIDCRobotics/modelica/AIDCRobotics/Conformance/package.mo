within AIDCRobotics;
package Conformance "Executable behavioral checks for compatible components"
  extends Modelica.Icons.Package;

  model P0SingleJoint "P0 interface and sign conformance check"
    extends AIDCRobotics.Examples.SingleJoint;
  equation
    when terminal() then
      assert(valid, "Joint diagnostic must remain valid");
      assert(abs(jointPosition - 0.25) < 1e-4, "Static torque/stiffness response violates P0 contract");
      assert(abs(appliedTorque - 0.5) < 1e-9, "Applied torque sign or scale violates P0 contract");
    end when;
    annotation(experiment(StartTime = 0, StopTime = 2, Interval = 0.001, Tolerance = 1e-8));
  end P0SingleJoint;

  model P1DeviceInterfaces "P1 battery limiting and IMU ordering check"
    extends AIDCRobotics.Examples.BatteryAndImu;
  equation
    when terminal() then
      assert(abs(battery.deliveredCurrent - 1.6) < 1e-9, "Battery current limit violated");
      assert(abs(battery.terminalVoltage - 6.24) < 1e-9, "Battery voltage droop violated");
      assert(max(abs(imu.orientationWxyz - {0.9, 0.1, 0.2, 0.3})) < 1e-12, "IMU quaternion must use wxyz order");
    end when;
  end P1DeviceInterfaces;

  model P4AnalyticContacts "Primitive and foot-patch contact law checks"
    AIDCRobotics.Contacts.PointPlaneContact point(stiffness = 20000, damping = 100);
    AIDCRobotics.Contacts.SpherePlaneContact sphere(radius = 0.05, stiffness = 20000, damping = 100);
    AIDCRobotics.Contacts.BoxPlaneContact box(halfExtents = {0.02, 0.03, 0.04}, stiffness = 20000, damping = 100);
    AIDCRobotics.Contacts.CapsulePlaneContact capsule(radius = 0.02, halfLength = 0.05, capsuleAxis = {0, 0, 1}, stiffness = 20000, damping = 100);
    AIDCRobotics.Contacts.FootPatchPlaneContact foot(stiffness = 5000, damping = 25);
  equation
    point.position = {0, 0, -0.01};
    point.velocity = {0.2, 0, -0.3};
    sphere.position = {0, 0, 0.04};
    sphere.velocity = {0.2, 0, -0.3};
    box.position = {0, 0, 0.035};
    box.velocity = {0, 0, -0.1};
    capsule.position = {0, 0, 0.065};
    capsule.velocity = {0, 0, -0.1};
    foot.position = {0, 0, -0.002};
    foot.velocity = {0.1, 0, -0.05};
    when initial() then
      assert(abs(point.penetration - 0.01) < 1e-12, "point penetration mismatch");
      assert(abs(sphere.penetration - 0.01) < 1e-12, "sphere penetration mismatch");
      assert(abs(box.penetration - 0.005) < 1e-12, "box support mapping mismatch");
      assert(abs(capsule.penetration - 0.005) < 1e-12, "capsule support mapping mismatch");
      assert(min(foot.penetration) > 0, "all four foot points must be active");
      assert(point.normalForce > 0 and point.elasticEnergy > 0 and point.dissipatedPower > 0, "contact force and energy must be positive");
      assert(point.force[1] < 0, "friction must oppose positive slip velocity");
    end when;
    annotation(experiment(StartTime = 0, StopTime = 0.001, Interval = 0.001, Tolerance = 1e-10));
  end P4AnalyticContacts;

  model P4NativeGeometry "ExternalObject cache, BVH distance and fixed-size contact output"
    parameter String meshPath = Modelica.Utilities.Files.loadResource("modelica://AIDCRobotics/Resources/Data/unit_cube.obj");
    AIDCRobotics.NativeGeometry.MeshHandle meshA = AIDCRobotics.NativeGeometry.MeshHandle(meshPath, {0.1, 0.1, 0.1});
    AIDCRobotics.NativeGeometry.MeshHandle meshB = AIDCRobotics.NativeGeometry.MeshHandle(meshPath, {0.1, 0.1, 0.1});
    Integer status;
    Integer triangleCount;
    Integer bvhNodeCount;
    Integer contactCount;
    Modelica.Units.SI.Length distance;
    Modelica.Units.SI.Position pointA[3];
    Modelica.Units.SI.Position pointB[3];
    Real normal[3];
    Real contacts[4, 7];
  equation
    triangleCount = AIDCRobotics.NativeGeometry.triangleCount(meshA);
    bvhNodeCount = AIDCRobotics.NativeGeometry.bvhNodeCount(meshA);
    (status, distance, pointA, pointB, normal) = AIDCRobotics.NativeGeometry.meshDistance(meshA, {0, 0, 0, 0, 0, 0, 1}, meshB, {0.12, 0, 0, 0, 0, 0, 1});
    (contactCount, contacts) = AIDCRobotics.NativeGeometry.meshContacts(meshA, {0, 0, 0, 0, 0, 0, 1}, meshB, {0.08, 0, 0, 0, 0, 0, 1}, 4);
    when initial() then
      assert(status == 0, "mesh distance query failed");
      assert(triangleCount == 12 and bvhNodeCount == 7, "mesh cache or BVH structure mismatch");
      assert(abs(distance - 0.02) < 1e-10, "mesh distance mismatch");
      assert(contactCount == 1 and abs(contacts[1, 7] - 0.02) < 1e-10, "mesh contact output mismatch");
    end when;
    annotation(experiment(StartTime = 0, StopTime = 0.001, Interval = 0.001, Tolerance = 1e-10));
  end P4NativeGeometry;

  model P4FmuPackaging "External runtime link smoke model for FMU packaging"
    output Integer runtimeVersion = AIDCRobotics.NativeGeometry.runtimeVersionCode();
  equation
    when initial() then
      assert(runtimeVersion == 100, "native geometry runtime version mismatch");
    end when;
    annotation(experiment(StartTime = 0, StopTime = 0.001, Interval = 0.001, Tolerance = 1e-10));
  end P4FmuPackaging;
end Conformance;
