within AIDCRobotics;
package NativeGeometry "ExternalObject wrapper for cached deterministic mesh queries"
  extends Modelica.Icons.Package;

  function runtimeVersionCode
    output Integer code;
    external "C" code = aidc_geometry_version_code()
      annotation(Include = "#include \"aidc_geometry.h\"", IncludeDirectory = "modelica://AIDCRobotics/Resources/Include", Library = "aidc_geometry", LibraryDirectory = "modelica://AIDCRobotics/Resources/Library/win64");
  end runtimeVersionCode;

  class MeshHandle
    extends ExternalObject;
    function constructor
      input String path;
      input Real scale[3] = {1, 1, 1};
      output MeshHandle mesh;
      external "C" mesh = aidc_mesh_load(path, scale)
        annotation(Include = "#include \"aidc_geometry.h\"", IncludeDirectory = "modelica://AIDCRobotics/Resources/Include", Library = "aidc_geometry", LibraryDirectory = "modelica://AIDCRobotics/Resources/Library/win64");
    end constructor;
    function destructor
      input MeshHandle mesh;
      external "C" aidc_mesh_destroy(mesh)
        annotation(Include = "#include \"aidc_geometry.h\"", IncludeDirectory = "modelica://AIDCRobotics/Resources/Include", Library = "aidc_geometry", LibraryDirectory = "modelica://AIDCRobotics/Resources/Library/win64");
    end destructor;
  end MeshHandle;

  function triangleCount
    input MeshHandle mesh;
    output Integer count;
    external "C" count = aidc_mesh_triangle_count(mesh)
      annotation(Include = "#include \"aidc_geometry.h\"", IncludeDirectory = "modelica://AIDCRobotics/Resources/Include", Library = "aidc_geometry", LibraryDirectory = "modelica://AIDCRobotics/Resources/Library/win64");
  end triangleCount;

  function bvhNodeCount
    input MeshHandle mesh;
    output Integer count;
    external "C" count = aidc_mesh_bvh_node_count(mesh)
      annotation(Include = "#include \"aidc_geometry.h\"", IncludeDirectory = "modelica://AIDCRobotics/Resources/Include", Library = "aidc_geometry", LibraryDirectory = "modelica://AIDCRobotics/Resources/Library/win64");
  end bvhNodeCount;

  function meshDistance
    input MeshHandle meshA;
    input Real poseA[7];
    input MeshHandle meshB;
    input Real poseB[7];
    output Integer status;
    output Modelica.Units.SI.Length distance;
    output Modelica.Units.SI.Position pointA[3];
    output Modelica.Units.SI.Position pointB[3];
    output Real normal[3];
    external "C" status = aidc_mesh_distance(meshA, poseA, meshB, poseB, distance, pointA, pointB, normal)
      annotation(Include = "#include \"aidc_geometry.h\"", IncludeDirectory = "modelica://AIDCRobotics/Resources/Include", Library = "aidc_geometry", LibraryDirectory = "modelica://AIDCRobotics/Resources/Library/win64");
  end meshDistance;

  function meshContacts
    input MeshHandle meshA;
    input Real poseA[7];
    input MeshHandle meshB;
    input Real poseB[7];
    input Integer capacity = 4;
    output Integer count;
    output Real contacts[capacity, 7] "Rows are x,y,z,nx,ny,nz,penetration";
    external "C" count = aidc_mesh_contacts(meshA, poseA, meshB, poseB, capacity, contacts)
      annotation(Include = "#include \"aidc_geometry.h\"", IncludeDirectory = "modelica://AIDCRobotics/Resources/Include", Library = "aidc_geometry", LibraryDirectory = "modelica://AIDCRobotics/Resources/Library/win64");
  end meshContacts;
end NativeGeometry;
