# Robot IR to Modelica generator

The generator has two deterministic profiles. P2 is the one-joint vertical slice. P3 accepts a fixed or floating rooted rigid-body tree and emits Fixed, Revolute and Prismatic joint instances. Microduck Native12 is the first complete P3 robot.

```powershell
npm run generate:modelica:p2
npm run verify:modelica:p2-generator
npm run generate:modelica:p3
npm run verify:modelica:p3-generator
```

The generator validates the Robot IR bundle, preserves `overrides/user-overrides.json`, and writes `robot-ir/generation.lock.json` plus `robot-ir/generated-structure.json`. The structure manifest retains the exact body mass, center of mass, inertia, joint transform, limit, component name and diagram position used during generation. Generated Modelica sources reference `AIDCRobotics`; they do not copy standard-library equations. Generated files should be changed through Robot IR or the override file and regenerated.

P4 contact geometry bindings are preserved in `generated-structure.json`. Their reusable force laws and native mesh queries live in `AIDCRobotics 0.3.0`; generated robot packages continue to contain parameters and topology rather than copied contact equations.
