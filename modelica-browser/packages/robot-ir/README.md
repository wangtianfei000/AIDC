# AIDC Robot IR

Robot IR is the shared, simulator-neutral input for generated robot models.
P0 defines three independently versioned documents:

- `physics.json`: rigid-body tree, mass properties, joints and contact defaults.
- `devices.json`: actuators, sensors and power devices bound to stable body/joint IDs.
- `interfaces.json`: ordered commands, states and diagnostics exposed to controllers and FMI.

All physical values use SI units. The robot frame is +X forward, +Y left and +Z up. Geometry exchange uses `xyzw` quaternions; public robot/FMI state uses `wxyz`. Cross-document semantics are checked by `tests/validate_robot_ir.py` in addition to JSON Schema validation.

P3 adds `rootType` (`fixed` or `floating`) and an explicit initial root pose. The reproducible `examples/microduck-native12/build_bundle.py` materializes the first complete tree bundle: 13 rigid bodies, 12 revolute joints, 12 actuator channels and a floating root.

P4 adds optional `contactGeometries` bound to stable body IDs. Point, sphere, box, capsule, multi-point foot patch and mesh references carry explicit local poses and shape parameters. The Microduck bundle supplies trunk/head proxies, two shin capsules and two four-point foot patches; geometry files remain resources while contact semantics remain in Robot IR.
