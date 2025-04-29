#include <stdint.h>
#include <stdbool.h>


typedef enum {
    DTI_TYPE_UNDEFINED = 0x0,
    DTI_TYPE_CLASS = 0x1,
    DTI_TYPE_CLASSREF = 0x2,
    DTI_TYPE_BOOL = 0x3,
    DTI_TYPE_U8 = 0x4,
    DTI_TYPE_U16 = 0x5,
    DTI_TYPE_U32 = 0x6,
    DTI_TYPE_U64 = 0x7,
    DTI_TYPE_S8 = 0x8,
    DTI_TYPE_S16 = 0x9,
    DTI_TYPE_S32 = 0xA,
    DTI_TYPE_S64 = 0xB,
    DTI_TYPE_F32 = 0xC,
    DTI_TYPE_F64 = 0xD,
    DTI_TYPE_STRING = 0xE,
    DTI_TYPE_COLOR = 0xF,
    DTI_TYPE_POINT = 0x10,
    DTI_TYPE_SIZE = 0x11,
    DTI_TYPE_RECT = 0x12,
    DTI_TYPE_MATRIX = 0x13,
    DTI_TYPE_VECTOR3 = 0x14,
    DTI_TYPE_VECTOR4 = 0x15,
    DTI_TYPE_QUATERNION = 0x16,
    DTI_TYPE_PROPERTY = 0x17,
    DTI_TYPE_EVENT = 0x18,
    DTI_TYPE_GROUP = 0x19,
    DTI_TYPE_PAGE_BEGIN = 0x1A,
    DTI_TYPE_PAGE_END = 0x1B,
    DTI_TYPE_EVENT32 = 0x1C,
    DTI_TYPE_ARRAY = 0x1D,
    DTI_TYPE_PROPERTYLIST = 0x1E,
    DTI_TYPE_GROUP_END = 0x1F,
    DTI_TYPE_CSTRING = 0x20,
    DTI_TYPE_TIME = 0x21,
    DTI_TYPE_FLOAT2 = 0x22,
    DTI_TYPE_FLOAT3 = 0x23,
    DTI_TYPE_FLOAT4 = 0x24,
    DTI_TYPE_FLOAT3x3 = 0x25,
    DTI_TYPE_FLOAT4x3 = 0x26,
    DTI_TYPE_FLOAT4x4 = 0x27,
    DTI_TYPE_EASECURVE = 0x28,
    DTI_TYPE_LINE = 0x29,
    DTI_TYPE_LINESEGMENT = 0x2A,
    DTI_TYPE_RAY = 0x2B,
    DTI_TYPE_PLANE = 0x2C,
    DTI_TYPE_SPHERE = 0x2D,
    DTI_TYPE_CAPSULE = 0x2E,
    DTI_TYPE_AABB = 0x2F,
    DTI_TYPE_OBB = 0x30,
    DTI_TYPE_CYLINDER = 0x31,
    DTI_TYPE_TRIANGLE = 0x32,
    DTI_TYPE_CONE = 0x33,
    DTI_TYPE_TORUS = 0x34,
    DTI_TYPE_ELLIPSOID = 0x35,
    DTI_TYPE_RANGE = 0x36,
    DTI_TYPE_RANGEF = 0x37,
    DTI_TYPE_RANGEU16 = 0x38,
    DTI_TYPE_HERMITECURVE = 0x39,
    DTI_TYPE_ENUMLIST = 0x3A,
    DTI_TYPE_FLOAT3x4 = 0x3B,
    DTI_TYPE_LINESEGMENT4 = 0x3C,
    DTI_TYPE_AABB4 = 0x3D,
    DTI_TYPE_OSCILLATOR = 0x3E,
    DTI_TYPE_VARIABLE = 0x3F,
    DTI_TYPE_VECTOR2 = 0x40,
    DTI_TYPE_MATRIX33 = 0x41,
    DTI_TYPE_RECT3D_XZ = 0x42,
    DTI_TYPE_RECT3D = 0x43,
    DTI_TYPE_RECT3D_COLLISION = 0x44,
    DTI_TYPE_PLANE_XZ = 0x45,
    DTI_TYPE_RAY_Y = 0x46,
    DTI_TYPE_POINTF = 0x47,
    DTI_TYPE_SIZEF = 0x48,
    DTI_TYPE_RECTF = 0x49,
    DTI_TYPE_EVENT64 = 0x4A,
    DTI_TYPE_END = 0x4B,
    DTI_TYPE_CUSTOM = 0x80
} dti_type_t;

typedef struct xfs_header {
    uint32_t magic;
    uint16_t version;
    uint16_t type;
    int64_t class_count;
    int32_t def_count;
    int32_t def_size;
    uint32_t def_offset;
} xfs_header;

typedef struct xfs_property_def {
    uint32_t name_offset;
    dti_type_t type;
    uint8_t attr;
    uint16_t bytes : 15;
    uint16_t disable : 1;
    uint64_t pad[4];
} xfs_property_def;

typedef struct xfs_def {
    uint32_t dti_hash;
    uint32_t prop_count : 15;
    uint32_t : 17;
    xfs_property_def props[1];
} xfs_def;

typedef struct xfs_class_ref {
    int16_t class_id;
    int16_t var;
} xfs_class_ref;

struct xfs_field;

typedef struct xfs_object {
    xfs_def* def;
    xfs_field* fields; //< Free this
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
} xfs_value;

typedef union xfs_data {
    xfs_value* value;
    const char* str;
    xfs_object* obj; //< Free this
    struct {
        uint32_t count;
        union xfs_data* entries; //< Free this
    } array;
} xfs_data;

typedef struct xfs_field {
    const char* name;
    dti_type_t type;
    xfs_data data;
} xfs_field;

typedef struct xfs {
    void* data;
    size_t size;

    xfs_header* header;
    xfs_def* defs;
    xfs_class_ref* root;

    xfs_object* objects; //< Free this
} xfs;

enum {
    XFS_RESULT_OK = 0,
    XFS_RESULT_ERROR = -1,
    XFS_RESULT_INVALID = -2,
};

int xfs_load(const char* path, xfs* xfs);
void xfs_free(xfs* xfs);
