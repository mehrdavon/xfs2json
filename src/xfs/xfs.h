#ifndef XFS_H
#define XFS_H

#include "prop_types.h"

#include <stdint.h>
#include <stdbool.h>
#include <cJSON.h>

#define XFS_MAGIC 0x534658 // "XFS\0"


typedef enum {
    XFS_TYPE_UNDEFINED = 0x0,
    XFS_TYPE_CLASS = 0x1,
    XFS_TYPE_CLASSREF = 0x2,
    XFS_TYPE_BOOL = 0x3,
    XFS_TYPE_U8 = 0x4,
    XFS_TYPE_U16 = 0x5,
    XFS_TYPE_U32 = 0x6,
    XFS_TYPE_U64 = 0x7,
    XFS_TYPE_S8 = 0x8,
    XFS_TYPE_S16 = 0x9,
    XFS_TYPE_S32 = 0xA,
    XFS_TYPE_S64 = 0xB,
    XFS_TYPE_F32 = 0xC,
    XFS_TYPE_F64 = 0xD,
    XFS_TYPE_STRING = 0xE,
    XFS_TYPE_COLOR = 0xF,
    XFS_TYPE_POINT = 0x10,
    XFS_TYPE_SIZE = 0x11,
    XFS_TYPE_RECT = 0x12,
    XFS_TYPE_MATRIX = 0x13,
    XFS_TYPE_VECTOR3 = 0x14,
    XFS_TYPE_VECTOR4 = 0x15,
    XFS_TYPE_QUATERNION = 0x16,
    XFS_TYPE_PROPERTY = 0x17,
    XFS_TYPE_EVENT = 0x18,
    XFS_TYPE_GROUP = 0x19,
    XFS_TYPE_PAGE_BEGIN = 0x1A,
    XFS_TYPE_PAGE_END = 0x1B,
    XFS_TYPE_EVENT32 = 0x1C,
    XFS_TYPE_ARRAY = 0x1D,
    XFS_TYPE_PROPERTYLIST = 0x1E,
    XFS_TYPE_GROUP_END = 0x1F,
    XFS_TYPE_CSTRING = 0x20,
    XFS_TYPE_TIME = 0x21,
    XFS_TYPE_FLOAT2 = 0x22,
    XFS_TYPE_FLOAT3 = 0x23,
    XFS_TYPE_FLOAT4 = 0x24,
    XFS_TYPE_FLOAT3x3 = 0x25,
    XFS_TYPE_FLOAT4x3 = 0x26,
    XFS_TYPE_FLOAT4x4 = 0x27,
    XFS_TYPE_EASECURVE = 0x28,
    XFS_TYPE_LINE = 0x29,
    XFS_TYPE_LINESEGMENT = 0x2A,
    XFS_TYPE_RAY = 0x2B,
    XFS_TYPE_PLANE = 0x2C,
    XFS_TYPE_SPHERE = 0x2D,
    XFS_TYPE_CAPSULE = 0x2E,
    XFS_TYPE_AABB = 0x2F,
    XFS_TYPE_OBB = 0x30,
    XFS_TYPE_CYLINDER = 0x31,
    XFS_TYPE_TRIANGLE = 0x32,
    XFS_TYPE_CONE = 0x33,
    XFS_TYPE_TORUS = 0x34,
    XFS_TYPE_ELLIPSOID = 0x35,
    XFS_TYPE_RANGE = 0x36,
    XFS_TYPE_RANGEF = 0x37,
    XFS_TYPE_RANGEU16 = 0x38,
    XFS_TYPE_HERMITECURVE = 0x39,
    XFS_TYPE_ENUMLIST = 0x3A,
    XFS_TYPE_FLOAT3x4 = 0x3B,
    XFS_TYPE_LINESEGMENT4 = 0x3C,
    XFS_TYPE_AABB4 = 0x3D,
    XFS_TYPE_OSCILLATOR = 0x3E,
    XFS_TYPE_VARIABLE = 0x3F,
    XFS_TYPE_VECTOR2 = 0x40,
    XFS_TYPE_MATRIX33 = 0x41,
    XFS_TYPE_RECT3D_XZ = 0x42,
    XFS_TYPE_RECT3D = 0x43,
    XFS_TYPE_RECT3D_COLLISION = 0x44,
    XFS_TYPE_PLANE_XZ = 0x45,
    XFS_TYPE_RAY_Y = 0x46,
    XFS_TYPE_POINTF = 0x47,
    XFS_TYPE_SIZEF = 0x48,
    XFS_TYPE_RECTF = 0x49,
    XFS_TYPE_EVENT64 = 0x4A,
    XFS_TYPE_END = 0x4B,
    XFS_TYPE_CUSTOM = 0x80
} xfs_type_t;

typedef struct xfs_header {
    uint32_t magic;
    uint16_t major_version;
    uint16_t minor_version;
    int64_t class_count;
    int32_t def_count;
    int32_t def_size;
} xfs_header;

typedef struct xfs_property_def {
    char* name; //< Free this
    xfs_type_t type;
    uint8_t attr;
    uint16_t bytes;
    bool disable;
} xfs_property_def;

typedef struct xfs_def {
    uint32_t dti_hash;
    uint32_t prop_count : 31;
    uint32_t init : 1;
    xfs_property_def* props; //< Free this
} xfs_def;

typedef struct xfs_class_ref {
    int16_t class_id;
    int16_t var;
} xfs_class_ref;

struct xfs_field;

typedef struct xfs_object {
    xfs_def* def;
    size_t def_id;
    int16_t id;
    struct xfs_field* fields; //< Free this
} xfs_object;

typedef union xfs_value {
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int8_t s8;
    int16_t s16;
    int32_t s32;
    int64_t s64;
    float f32;
    double f64;
    bool b;
    xfs_point point;
    xfs_size size;
    xfs_rect rect;
    xfs_matrix matrix;
    xfs_vector3 vector3;
    xfs_vector4 vector4;
    xfs_quaternion quaternion;
    xfs_time time;
    xfs_float2 float2;
    xfs_float3 float3;
    xfs_float4 float4;
    xfs_color color;
    xfs_float3x3 float3x3;
    xfs_float4x3 float4x3;
    xfs_float4x4 float4x4;
    xfs_easecurve easecurve;
    xfs_line line;
    xfs_linesegment linesegment;
    xfs_ray ray;
    xfs_plane plane;
    xfs_sphere sphere;
    xfs_capsule capsule;
    xfs_aabb aabb;
    xfs_obb obb;
    xfs_cylinder cylinder;
    xfs_triangle triangle;
    xfs_cone cone;
    xfs_torus torus;
    xfs_ellipsoid ellipsoid;
    xfs_range range;
    xfs_rangef rangef;
    xfs_rangeu16 rangeu16;
    xfs_hermitecurve hermitecurve;
    xfs_float3x4 float3x4;
    xfs_linesegment4 linesegment4;
    xfs_aabb4 aabb4;
    xfs_vector2 vector2;
    xfs_matrix33 matrix33;
    xfs_rect3d_xz rect3d_xz;
    xfs_rect3d rect3d;
    xfs_plane_xz plane_xz;
    xfs_ray_y ray_y;
    xfs_pointf pointf;
    xfs_sizef sizef;
    xfs_rectf rectf;
} xfs_value;

typedef union xfs_data {
    xfs_value value;
    char* str; //< Free this
    xfs_object* obj; //< Free this
    struct {
        uint32_t count;
        union xfs_data* entries; //< Free this
    } array;
    struct {
        uint8_t count;
        char** values; //< Free this
    } custom;
} xfs_data;

typedef struct xfs_field {
    const char* name;
    xfs_type_t type;
    bool is_array;
    xfs_data data;
} xfs_field;

typedef struct xfs {
    xfs_header header;
    xfs_def* defs; //< Free this
    xfs_object* root; //< Free this
} xfs;

enum {
    XFS_RESULT_OK = 0,
    XFS_RESULT_ERROR = -1,
    XFS_RESULT_INVALID = -2,
};

int xfs_load(const char* path, xfs* xfs);
int xfs_save(const char* path, const xfs* xfs);
void xfs_free(xfs* xfs);

cJSON* xfs_to_json(const xfs* xfs);
xfs* xfs_from_json(const cJSON* json);

bool is_xfs_file(const char* path);

#endif // XFS_H
