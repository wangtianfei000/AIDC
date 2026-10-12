# modelica-native-geometry

Stable C ABI used by `AIDCRobotics.NativeGeometry`. Meshes are loaded once, stored behind an opaque handle and indexed by a deterministic median-split BVH. Queries use metres, +X forward/+Y left/+Z up, and poses `[x,y,z,qx,qy,qz,qw]`.

The runtime loads OBJ and binary/ASCII STL triangle meshes. `aidc_mesh_distance` returns nearest points and an A-to-B normal. `aidc_mesh_contacts` emits fixed-width records `[x,y,z,nx,ny,nz,depth]`; a negative return value is an error and a non-negative value is the contact count.
