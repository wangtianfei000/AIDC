# AIDCRobotics

`AIDCRobotics` is the reusable Modelica standard library for AIDC robot models. Robot-specific packages depend on its public interfaces and components; they must not copy common equations.

## Public contract

- Physical values use SI units.
- Robot coordinates are +X forward, +Y left and +Z up.
- Public robot/FMI quaternions use `wxyz`; Model3D geometry interchange uses `xyzw` and must convert explicitly.
- Stable Robot IR IDs identify bodies, joints and devices. Display names are not identifiers.
- Public arrays publish explicit one-based channel indices. Declaration or connection order is not an interface contract.
- Joint directions are exactly `+1` or `-1`; scale and zero offset are separate values.
- A diagnostic `valid=false` means published physical state must not be consumed by a controller.

## Naming

- Packages, models, records and connector types use `UpperCamelCase`.
- Parameters, variables and connector instances use `lowerCamelCase`.
- Physical names include Modelica `quantity` and `unit`; Robot IR JSON fields include the SI unit suffix where practical.
- Robot-specific identifiers remain in Robot IR and instance packages, never in standard component class names.

## Versioning

The library follows semantic versioning.

- Major: may change public connector fields, semantics or equations in a behaviorally incompatible way.
- Minor: adds compatible components, optional fields or profiles.
- Patch: fixes implementation or documentation without changing public semantics.

Deprecated public elements remain available for at least one minor release and identify their replacement in documentation. Each release includes the package, examples, conformance results and a compatibility matrix.

## Verification

From the AIDC workspace root:

```powershell
npm run verify:modelica:p0-robotics
npm run verify:modelica:p1-robotics
npm run verify:modelica:p3-generator
npm run build:modelica:native-geometry
npm run verify:modelica:p4-contact-runtime
```

P0 validates Robot IR, native simulation, conformance behavior and FMI 2.0 ME/CS parity. P1 checks the mechanics, actuator, battery, sensor and profile classes, runs device conformance, and compares the reusable HD-1910 standard component against the original T2 model over the complete trajectory.
P3 additionally compiles a mixed Fixed/Revolute/Prismatic tree and the floating-base Microduck Native12 generated model. Mechanics components include explicit connector placement and type-specific icons so generated diagrams retain correct selectable bounds and port locations.
P4 checks point, sphere, box, capsule and four-point foot contacts; compares public landing, sliding and collision geometry against MuJoCo; validates OBJ/STL BVH queries through a cached `ExternalObject`; and executes an FMI 2.0 Co-Simulation package containing the native runtime and geometry resources.

## P1 library surface

- `Mechanics`: rigid body, fixed/floating base, fixed/revolute/prismatic joints, loss and backlash.
- `Actuators`: ideal torque, reduced DC motor and limited position servo.
- `Electrical`: current-limited battery bus with voltage droop and state of charge.
- `Sensors`: joint-state sensor and explicit `xyzw` to public `wxyz` IMU adapter.
- `DeviceData.Feetech.HD1910`: equations and manufacturer data are kept separate.
- `Profiles`: articulated, floating-base, battery-powered and IMU-equipped robot contracts.

## P4 contact and native geometry surface

- `Contacts.PartialPlaneContact`: shared stateless normal force, regularized Coulomb friction, elastic energy and dissipated power law.
- `Contacts.PointPlaneContact`, `SpherePlaneContact`, `BoxPlaneContact`, `CapsulePlaneContact`: analytic support mappings against a plane.
- `Contacts.FootPatchPlaneContact`: four independently active foot points with aggregate force and energy.
- `NativeGeometry.MeshHandle`: cached OBJ or STL triangle mesh managed as a Modelica `ExternalObject`.
- `NativeGeometry.meshDistance` and `meshContacts`: deterministic BVH distance and fixed-capacity contact records through a stable C ABI.

Continuous dynamics and contact constitutive state stay in Modelica. The C runtime owns immutable geometry and performs geometry queries only. Its pose input is `[x,y,z,qx,qy,qz,qw]` in metres using the standard robot axes.
