#ifndef AIDC_GEOMETRY_H
#define AIDC_GEOMETRY_H

#ifdef _WIN32
#  ifdef AIDC_GEOMETRY_BUILD
#    define AIDC_GEOMETRY_API __declspec(dllexport)
#  else
#    define AIDC_GEOMETRY_API
#  endif
#else
#  define AIDC_GEOMETRY_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* AidcMeshHandle;

AIDC_GEOMETRY_API const char* aidc_geometry_version(void);
AIDC_GEOMETRY_API int aidc_geometry_version_code(void);
AIDC_GEOMETRY_API AidcMeshHandle aidc_mesh_load(const char* path, const double scale[3]);
AIDC_GEOMETRY_API void aidc_mesh_destroy(AidcMeshHandle mesh);
AIDC_GEOMETRY_API int aidc_mesh_triangle_count(AidcMeshHandle mesh);
AIDC_GEOMETRY_API int aidc_mesh_bvh_node_count(AidcMeshHandle mesh);
AIDC_GEOMETRY_API int aidc_mesh_distance(
    AidcMeshHandle a, const double pose_a[7],
    AidcMeshHandle b, const double pose_b[7],
    double* distance, double point_a[3], double point_b[3], double normal[3]);
AIDC_GEOMETRY_API int aidc_mesh_contacts(
    AidcMeshHandle a, const double pose_a[7],
    AidcMeshHandle b, const double pose_b[7],
    int capacity, double contacts_xyz_normal_depth[]);

#ifdef __cplusplus
}
#endif
#endif
