#include "xfs.h"
#include "xfs/common.h"
#include "xfs/v16/arch_32.h"
#include "xfs/v15/arch_64.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static cJSON* xfs_object_to_json(const xfs_object* obj);
static cJSON* xfs_data_to_json(xfs_type_t type, const xfs_data* data);

static cJSON* xfs_json_create_float2(const float* values);
static cJSON* xfs_json_create_float3(const float* values);
static cJSON* xfs_json_create_float4(const float* values);
static cJSON* xfs_json_create_matrix(const float* values, int m, int n);
static cJSON* xfs_json_create_soa_vector3(const xfs_soa_vector3* value);

static double xfs_json_get_number(const cJSON* json, const char* key);
static double xfs_json_get_array_number(const cJSON* json, int index);
static void xfs_json_get_float2(const cJSON* json, const char* key, float* values);
static void xfs_json_get_float3(const cJSON* json, const char* key, float* values);
static void xfs_json_get_float4(const cJSON* json, const char* key, float* values);
static void xfs_json_get_matrix(const cJSON* json, const char* key, float* values, int m, int n);
static void xfs_json_get_soa_vector3(const cJSON* json, const char* key, xfs_soa_vector3* value);
#define xfs_json_get_t(type, json, key) (type)xfs_json_get_number(json, key)
#define xfs_json_get_array_t(type, json, index) (type)xfs_json_get_array_number(json, index)

static xfs_object* xfs_object_from_json(const cJSON* json, xfs* xfs);
static bool xfs_data_from_json(const cJSON* json, xfs_type_t type, xfs_data* data, xfs* xfs);

cJSON* xfs_to_json(const xfs* xfs) {
    cJSON* json = cJSON_CreateObject();

    // Add definitions
    cJSON* defs = cJSON_CreateArray();
    for (int i = 0; i < xfs->header.def_count; i++) {
        const xfs_def* def = &xfs->defs[i];
        cJSON* def_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(def_json, "dti", def->dti_hash);

        cJSON* props = cJSON_CreateArray();
        for (int j = 0; j < def->prop_count; j++) {
            const xfs_property_def* prop = &def->props[j];
            cJSON* prop_json = cJSON_CreateObject();

            cJSON_AddStringToObject(prop_json, "name", prop->name);
            cJSON_AddNumberToObject(prop_json, "type", prop->type);
            cJSON_AddNumberToObject(prop_json, "attr", prop->attr);
            cJSON_AddNumberToObject(prop_json, "bytes", prop->bytes);
            cJSON_AddBoolToObject(prop_json, "disable", prop->disable);

            cJSON_AddItemToArray(props, prop_json);
        }

        cJSON_AddItemToObject(def_json, "props", props);
        cJSON_AddItemToArray(defs, def_json);
    }

    cJSON_AddItemToObject(json, "root", xfs_object_to_json(xfs->root));
    cJSON_AddItemToObject(json, "$defs", defs);
    cJSON_AddNumberToObject(json, "$major_version", xfs->header.major_version);
    cJSON_AddNumberToObject(json, "$minor_version", xfs->header.minor_version);

    return json;
}

xfs* xfs_from_json(const cJSON* json) {
    xfs* xfs = calloc(1, sizeof(struct xfs));
    if (xfs == NULL) {
        return NULL;
    }

    const cJSON* defs = cJSON_GetObjectItem(json, "$defs");
    const cJSON* root = cJSON_GetObjectItem(json, "root");

    if (defs == NULL || root == NULL) {
        free(xfs);
        return NULL;
    }

    if (!cJSON_IsArray(defs)) {
        free(xfs);
        return NULL;
    }

    xfs->header.magic = XFS_MAGIC;
    xfs->header.major_version = (uint16_t)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "$major_version"));
    xfs->header.minor_version = (uint16_t)cJSON_GetNumberValue(cJSON_GetObjectItem(json, "$minor_version"));
    xfs->header.class_count = 0; // Will be filled later
    xfs->header.def_count = cJSON_GetArraySize(defs);

    xfs->defs = calloc(xfs->header.def_count, sizeof(xfs_def));
    if (xfs->defs == NULL) {
        free(xfs);
        return NULL;
    }

    for (uint32_t i = 0; i < xfs->header.def_count; i++) {
        const cJSON* def_json = cJSON_GetArrayItem(defs, i);
        if (!cJSON_IsObject(def_json)) {
            xfs_free(xfs);
            free(xfs);
            return NULL;
        }

        const cJSON* props_json = cJSON_GetObjectItem(def_json, "props");
        if (!cJSON_IsArray(props_json)) {
            xfs_free(xfs);
            free(xfs);
            return NULL;
        }

        xfs_def* def = &xfs->defs[i];

        def->dti_hash = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(def_json, "dti"));
        def->init = cJSON_HasObjectItem(def_json, "init")
            ? cJSON_IsTrue(cJSON_GetObjectItem(def_json, "init"))
            : false;

        def->prop_count = (uint32_t)cJSON_GetArraySize(props_json);
        def->props = calloc(def->prop_count, sizeof(xfs_property_def));
        if (def->props == NULL) {
            xfs_free(xfs);
            free(xfs);
            return NULL;
        }

        for (uint32_t j = 0; j < def->prop_count; j++) {
            const cJSON* prop_json = cJSON_GetArrayItem(props_json, j);
            if (!cJSON_IsObject(prop_json)) {
                xfs_free(xfs);
                free(xfs);
                return NULL;
            }

            xfs_property_def* prop = &def->props[j];

            const char* name = cJSON_GetStringValue(cJSON_GetObjectItem(prop_json, "name"));
            prop->name = strdup(name);
            if (prop->name == NULL) {
                xfs_free(xfs);
                free(xfs);
                return NULL;
            }

            prop->type = (xfs_type_t)cJSON_GetNumberValue(cJSON_GetObjectItem(prop_json, "type"));
            prop->attr = (uint8_t)cJSON_GetNumberValue(cJSON_GetObjectItem(prop_json, "attr"));
            prop->bytes = (uint16_t)cJSON_GetNumberValue(cJSON_GetObjectItem(prop_json, "bytes"));
            prop->disable = cJSON_IsTrue(cJSON_GetObjectItem(prop_json, "disable"));
        }
    }

    // Calculate the size of the definitions
    switch (xfs->header.major_version) {
    case XFS_VERSION_15:
        xfs->header.def_size = (int32_t)xfs_v15_64_get_def_size(xfs, true);
        break;
    case XFS_VERSION_16:
        xfs->header.def_size = (int32_t)xfs_v16_32_get_def_size(xfs, true);
        break;
    default:
        fprintf(stderr, "Unsupported XFS version: %04X-%04X\n", xfs->header.major_version, xfs->header.minor_version);
        return NULL;
    }

    xfs->root = xfs_object_from_json(root, xfs);

    return xfs;
}

cJSON* xfs_object_to_json(const xfs_object* obj) {
    if (obj == NULL) {
        return cJSON_CreateNull();
    }

    cJSON* json = cJSON_CreateObject();

    cJSON_AddNumberToObject(json, "$id", obj->def_id);

    for (int i = 0; i < obj->def->prop_count; i++) {
        const xfs_field* field = &obj->fields[i];
        
        if (field->is_array) {
            cJSON* items = cJSON_CreateArray();
            for (int j = 0; j < field->data.array.count; j++) {
                cJSON_AddItemToArray(items, xfs_data_to_json(field->type, &field->data.array.entries[j]));
            }
            
            cJSON_AddItemToObject(json, field->name, items);
        } else {
            cJSON_AddItemToObject(json, field->name, xfs_data_to_json(field->type, &field->data));
        }
    }

    return json;
}

cJSON* xfs_data_to_json(xfs_type_t type, const xfs_data* data) {
    char string_buffer[128];
    cJSON* json = NULL;
    cJSON* array = NULL;

    switch (type) {
    case XFS_TYPE_UNDEFINED:
    case XFS_TYPE_PROPERTY:
    case XFS_TYPE_EVENT:
    case XFS_TYPE_GROUP:
    case XFS_TYPE_PAGE_BEGIN:
    case XFS_TYPE_PAGE_END:
    case XFS_TYPE_EVENT32:
    case XFS_TYPE_ARRAY:
    case XFS_TYPE_PROPERTYLIST:
    case XFS_TYPE_GROUP_END:
    case XFS_TYPE_ENUMLIST:
    case XFS_TYPE_OSCILLATOR:
    case XFS_TYPE_VARIABLE:
    case XFS_TYPE_RECT3D_COLLISION:
    case XFS_TYPE_EVENT64:
    case XFS_TYPE_END:
        return NULL;

    case XFS_TYPE_CLASS:
    case XFS_TYPE_CLASSREF:
        return xfs_object_to_json(data->obj);
    case XFS_TYPE_BOOL:
        return cJSON_CreateBool(data->value.b);
    case XFS_TYPE_U8:
        return cJSON_CreateNumber(data->value.u8);
    case XFS_TYPE_U16:
        return cJSON_CreateNumber(data->value.u16);
    case XFS_TYPE_U32:
        return cJSON_CreateNumber(data->value.u32);
    case XFS_TYPE_U64:
        return cJSON_CreateNumber(data->value.u64);
    case XFS_TYPE_S8:
        return cJSON_CreateNumber(data->value.s8);
    case XFS_TYPE_S16:
        return cJSON_CreateNumber(data->value.s16);
    case XFS_TYPE_S32:
        return cJSON_CreateNumber(data->value.s32);
    case XFS_TYPE_S64:
        return cJSON_CreateNumber(data->value.s64);
    case XFS_TYPE_F32:
        return cJSON_CreateNumber(data->value.f32);
    case XFS_TYPE_F64:
        return cJSON_CreateNumber(data->value.f64);
    case XFS_TYPE_STRING:
    case XFS_TYPE_CSTRING:
        return cJSON_CreateString(data->str);
    case XFS_TYPE_COLOR:
        snprintf(string_buffer, sizeof(string_buffer), "#%08X", data->value.color);
        return cJSON_CreateString(string_buffer);
    case XFS_TYPE_POINT:
        json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "x", data->value.point.x);
        cJSON_AddNumberToObject(json, "y", data->value.point.y);
        return json;
    case XFS_TYPE_SIZE:
        json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "w", data->value.size.w);
        cJSON_AddNumberToObject(json, "h", data->value.size.h);
        return json;
    case XFS_TYPE_RECT:
        json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "t", data->value.rect.t);
        cJSON_AddNumberToObject(json, "l", data->value.rect.l);
        cJSON_AddNumberToObject(json, "r", data->value.rect.r);
        cJSON_AddNumberToObject(json, "b", data->value.rect.b);
        return json;
    case XFS_TYPE_MATRIX:
        json = cJSON_CreateObject();
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                snprintf(string_buffer, sizeof(string_buffer), "m%d%d", i, j);
                cJSON_AddNumberToObject(json, string_buffer, data->value.matrix.m[i][j]);
            }
        }
        return json;
    case XFS_TYPE_VECTOR3:
        return xfs_json_create_float3(&data->value.vector3.x);
    case XFS_TYPE_VECTOR4:
        return xfs_json_create_float4(&data->value.vector4.x);
    case XFS_TYPE_QUATERNION:
        return xfs_json_create_float4(&data->value.quaternion.x);
    case XFS_TYPE_TIME:
        return cJSON_CreateNumber(data->value.time.time);
    case XFS_TYPE_FLOAT2:
        return xfs_json_create_float2(&data->value.float2.x);
    case XFS_TYPE_FLOAT3:
        return xfs_json_create_float3(&data->value.float3.x);
    case XFS_TYPE_FLOAT4:
        return xfs_json_create_float4(&data->value.float4.x);
    case XFS_TYPE_FLOAT3x3:
        return xfs_json_create_matrix(&data->value.float3x3.m[0][0], 3, 3);
    case XFS_TYPE_FLOAT4x3:
        return xfs_json_create_matrix(&data->value.float4x3.m[0][0], 4, 3);
    case XFS_TYPE_FLOAT4x4:
        return xfs_json_create_matrix(&data->value.float4x4.m[0][0], 4, 4);
    case XFS_TYPE_EASECURVE:
        json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "p1", data->value.easecurve.p1);
        cJSON_AddNumberToObject(json, "p2", data->value.easecurve.p2);
        return json;
    case XFS_TYPE_LINE:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "from", xfs_json_create_float3(&data->value.line.from.x));
        cJSON_AddItemToObject(json, "dir", xfs_json_create_float3(&data->value.line.dir.x));
        return json;
    case XFS_TYPE_LINESEGMENT:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "p0", xfs_json_create_float3(&data->value.linesegment.p0.x));
        cJSON_AddItemToObject(json, "p1", xfs_json_create_float3(&data->value.linesegment.p1.x));
        return json;
    case XFS_TYPE_RAY:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "from", xfs_json_create_float3(&data->value.ray.from.x));
        cJSON_AddItemToObject(json, "dir", xfs_json_create_float3(&data->value.ray.dir.x));
        return json;
    case XFS_TYPE_PLANE:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "normal", xfs_json_create_float3(&data->value.plane.normal.x));
        cJSON_AddNumberToObject(json, "dist", data->value.plane.dist);
        return json;
    case XFS_TYPE_SPHERE:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "center", xfs_json_create_float3(&data->value.sphere.center.x));
        cJSON_AddNumberToObject(json, "radius", data->value.sphere.radius);
        return json;
    case XFS_TYPE_CAPSULE:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "p0", xfs_json_create_float3(&data->value.capsule.p0.x));
        cJSON_AddItemToObject(json, "p1", xfs_json_create_float3(&data->value.capsule.p1.x));
        cJSON_AddNumberToObject(json, "radius", data->value.capsule.radius);
        return json;
    case XFS_TYPE_AABB:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "min", xfs_json_create_float3(&data->value.aabb.min.x));
        cJSON_AddItemToObject(json, "max", xfs_json_create_float3(&data->value.aabb.max.x));
        return json;
    case XFS_TYPE_OBB:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "transform", xfs_json_create_matrix(&data->value.obb.transform.m[0][0], 4, 4));
        cJSON_AddItemToObject(json, "extent", xfs_json_create_float3(&data->value.obb.extent.x));
        return json;
    case XFS_TYPE_CYLINDER:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "p0", xfs_json_create_float3(&data->value.cylinder.p0.x));
        cJSON_AddItemToObject(json, "p1", xfs_json_create_float3(&data->value.cylinder.p1.x));
        cJSON_AddNumberToObject(json, "radius", data->value.cylinder.radius);
        return json;
    case XFS_TYPE_TRIANGLE:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "p0", xfs_json_create_float3(&data->value.triangle.p0.x));
        cJSON_AddItemToObject(json, "p1", xfs_json_create_float3(&data->value.triangle.p1.x));
        cJSON_AddItemToObject(json, "p2", xfs_json_create_float3(&data->value.triangle.p2.x));
        return json;
    case XFS_TYPE_CONE:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "p0", xfs_json_create_float3(&data->value.cone.p0.x));
        cJSON_AddItemToObject(json, "p1", xfs_json_create_float3(&data->value.cone.p1.x));
        cJSON_AddNumberToObject(json, "r0", data->value.cone.r0);
        cJSON_AddNumberToObject(json, "r1", data->value.cone.r1);
        return json;
    case XFS_TYPE_TORUS:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "pos", xfs_json_create_float3(&data->value.torus.pos.x));
        cJSON_AddItemToObject(json, "axis", xfs_json_create_float3(&data->value.torus.axis.x));
        cJSON_AddNumberToObject(json, "r", data->value.torus.r);
        cJSON_AddNumberToObject(json, "cr", data->value.torus.cr);
        return json;
    case XFS_TYPE_ELLIPSOID:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "pos", xfs_json_create_float3(&data->value.ellipsoid.pos.x));
        cJSON_AddItemToObject(json, "r", xfs_json_create_float3(&data->value.ellipsoid.r.x));
        return json;
    case XFS_TYPE_RANGE:
        json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "s", data->value.range.s);
        cJSON_AddNumberToObject(json, "r", data->value.range.r);
        return json;
    case XFS_TYPE_RANGEF:
        json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "s", data->value.rangef.s);
        cJSON_AddNumberToObject(json, "r", data->value.rangef.r);
        return json;
    case XFS_TYPE_RANGEU16:
        json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "s", data->value.rangeu16.s);
        cJSON_AddNumberToObject(json, "r", data->value.rangeu16.r);
        return json;
    case XFS_TYPE_HERMITECURVE:
        json = cJSON_CreateObject();

        array = cJSON_CreateArray();
        for (int i = 0; i < 8; i++) {
            cJSON_AddItemToArray(array, cJSON_CreateNumber(data->value.hermitecurve.x[i]));
        }
        cJSON_AddItemToObject(json, "x", array);

        array = cJSON_CreateArray();
        for (int i = 0; i < 8; i++) {
            cJSON_AddItemToArray(array, cJSON_CreateNumber(data->value.hermitecurve.y[i]));
        }
        cJSON_AddItemToObject(json, "y", array);
        return json;
    case XFS_TYPE_FLOAT3x4:
        return xfs_json_create_matrix(&data->value.float3x4.m[0][0], 3, 4);
    case XFS_TYPE_LINESEGMENT4:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "p0", xfs_json_create_soa_vector3(&data->value.linesegment4.p0_4));
        cJSON_AddItemToObject(json, "p1", xfs_json_create_soa_vector3(&data->value.linesegment4.p1_4));
        return json;
    case XFS_TYPE_AABB4:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "min", xfs_json_create_soa_vector3(&data->value.aabb4.min_4));
        cJSON_AddItemToObject(json, "max", xfs_json_create_soa_vector3(&data->value.aabb4.max_4));
        return json;
    case XFS_TYPE_VECTOR2:
        return xfs_json_create_float2(&data->value.vector2.x);
    case XFS_TYPE_MATRIX33:
        return xfs_json_create_matrix(&data->value.matrix33.m[0][0], 3, 3);
    case XFS_TYPE_RECT3D_XZ:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "lt", xfs_json_create_float2(&data->value.rect3d_xz.lt.x));
        cJSON_AddItemToObject(json, "lb", xfs_json_create_float2(&data->value.rect3d_xz.lb.x));
        cJSON_AddItemToObject(json, "rt", xfs_json_create_float2(&data->value.rect3d_xz.rt.x));
        cJSON_AddItemToObject(json, "rb", xfs_json_create_float2(&data->value.rect3d_xz.rb.x));
        cJSON_AddNumberToObject(json, "height", data->value.rect3d_xz.height);
        return json;
    case XFS_TYPE_RECT3D:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "normal", xfs_json_create_float3(&data->value.rect3d.normal.x));
        cJSON_AddItemToObject(json, "center", xfs_json_create_float3(&data->value.rect3d.center.x));
        cJSON_AddNumberToObject(json, "size_w", data->value.rect3d.size_w);
        cJSON_AddNumberToObject(json, "size_h", data->value.rect3d.size_h);
        return json;
    case XFS_TYPE_PLANE_XZ:
        json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "dist", data->value.plane_xz.dist);
        return json;
    case XFS_TYPE_RAY_Y:
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "from", xfs_json_create_float3(&data->value.ray_y.from.x));
        cJSON_AddNumberToObject(json, "dir", data->value.ray_y.dir);
        return json;
    case XFS_TYPE_POINTF:
        json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "x", data->value.pointf.x);
        cJSON_AddNumberToObject(json, "y", data->value.pointf.y);
        return json;
    case XFS_TYPE_SIZEF:
        json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "w", data->value.sizef.w);
        cJSON_AddNumberToObject(json, "h", data->value.sizef.h);
        return json;
    case XFS_TYPE_RECTF:
        json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "l", data->value.rectf.l);
        cJSON_AddNumberToObject(json, "t", data->value.rectf.t);
        cJSON_AddNumberToObject(json, "r", data->value.rectf.r);
        cJSON_AddNumberToObject(json, "b", data->value.rectf.b);
        return json;
    case XFS_TYPE_CUSTOM:
        json = cJSON_CreateObject();
        array = cJSON_CreateArray();
        for (uint8_t i = 0; i < data->custom.count; i++) {
            cJSON_AddItemToArray(array, cJSON_CreateString(data->custom.values[i]));
        }
        cJSON_AddItemToObject(json, "values", array);
        return json;
    }

    return NULL;
}

cJSON* xfs_json_create_float2(const float* values) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "x", values[0]);
    cJSON_AddNumberToObject(json, "y", values[1]);
    return json;
}

cJSON* xfs_json_create_float3(const float* values) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "x", values[0]);
    cJSON_AddNumberToObject(json, "y", values[1]);
    cJSON_AddNumberToObject(json, "z", values[2]);
    return json;
}

cJSON* xfs_json_create_float4(const float* values) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "x", values[0]);
    cJSON_AddNumberToObject(json, "y", values[1]);
    cJSON_AddNumberToObject(json, "z", values[2]);
    cJSON_AddNumberToObject(json, "w", values[3]);
    return json;
}

cJSON* xfs_json_create_matrix(const float* values, int m, int n) {
    cJSON* json = cJSON_CreateObject();
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            char string_buffer[16];
            snprintf(string_buffer, sizeof(string_buffer), "m%d%d", i, j);
            cJSON_AddNumberToObject(json, string_buffer, values[i * n + j]);
        }
    }

    return json;
}

cJSON* xfs_json_create_soa_vector3(const xfs_soa_vector3* value) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddItemToObject(json, "x", xfs_json_create_float4(&value->x.x));
    cJSON_AddItemToObject(json, "y", xfs_json_create_float4(&value->y.x));
    cJSON_AddItemToObject(json, "z", xfs_json_create_float4(&value->z.x));
    return json;
}

double xfs_json_get_number(const cJSON* json, const char* key) {
    if (key != NULL) {
        json = cJSON_GetObjectItem(json, key);
        if (json == NULL) {
            return 0.0;
        }
    }

    return cJSON_GetNumberValue(json);
}

double xfs_json_get_array_number(const cJSON* json, int index) {
    const cJSON* item = cJSON_GetArrayItem(json, index);
    if (item == NULL || !cJSON_IsNumber(item)) {
        return 0.0;
    }

    return cJSON_GetNumberValue(item);
}

void xfs_json_get_float2(const cJSON* json, const char* key, float* values) {
    if (key != NULL) {
        json = cJSON_GetObjectItem(json, key);
        if (json == NULL || !cJSON_IsObject(json)) {
            return;
        }
    }

    values[0] = xfs_json_get_t(float, json, "x");
    values[1] = xfs_json_get_t(float, json, "y");
}

void xfs_json_get_float3(const cJSON* json, const char* key, float* values) {
    if (key != NULL) {
        json = cJSON_GetObjectItem(json, key);
        if (json == NULL || !cJSON_IsObject(json)) {
            return;
        }
    }

    values[0] = xfs_json_get_t(float, json, "x");
    values[1] = xfs_json_get_t(float, json, "y");
    values[2] = xfs_json_get_t(float, json, "z");
}

void xfs_json_get_float4(const cJSON* json, const char* key, float* values) {
    if (key != NULL) {
        json = cJSON_GetObjectItem(json, key);
        if (json == NULL || !cJSON_IsObject(json)) {
            return;
        }
    }

    values[0] = xfs_json_get_t(float, json, "x");
    values[1] = xfs_json_get_t(float, json, "y");
    values[2] = xfs_json_get_t(float, json, "z");
    values[3] = xfs_json_get_t(float, json, "w");
}

void xfs_json_get_matrix(const cJSON* json, const char* key, float* values, int m, int n) {
    if (key != NULL) {
        json = cJSON_GetObjectItem(json, key);
        if (json == NULL || !cJSON_IsObject(json)) {
            return;
        }
    }

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            char string_buffer[16];
            snprintf(string_buffer, sizeof(string_buffer), "m%d%d", i, j);
            values[i * n + j] = xfs_json_get_t(float, json, string_buffer);
        }
    }
}

void xfs_json_get_soa_vector3(const cJSON* json, const char* key, xfs_soa_vector3* value) {
    if (key != NULL) {
        json = cJSON_GetObjectItem(json, key);
        if (json == NULL || !cJSON_IsObject(json)) {
            return;
        }
    }

    xfs_json_get_float4(json, "x", &value->x.x);
    xfs_json_get_float4(json, "y", &value->y.x);
    xfs_json_get_float4(json, "z", &value->z.x);
}

xfs_object* xfs_object_from_json(const cJSON* json, xfs* xfs) {
    if (cJSON_IsNull(json) || !cJSON_IsObject(json)) {
        return NULL;
    }

    const cJSON* id_item = cJSON_GetObjectItem(json, "$id");
    if (id_item == NULL || !cJSON_IsNumber(id_item)) {
        return NULL;
    }

    xfs_def* def = &xfs->defs[(int)cJSON_GetNumberValue(id_item)];
    if (def == NULL) {
        return NULL;
    }

    xfs_object* obj = malloc(sizeof(xfs_object));
    if (obj == NULL) {
        return NULL;
    }

    obj->def = def;
    obj->def_id = (size_t)cJSON_GetNumberValue(id_item);
    obj->fields = calloc(def->prop_count, sizeof(xfs_field));
    obj->id = (int16_t)xfs->header.class_count;
    xfs->header.class_count++;

    for (uint32_t i = 0; i < def->prop_count; i++) {
        xfs_field* field = &obj->fields[i];
        const xfs_property_def* prop = &def->props[i];

        field->name = prop->name;
        field->type = (xfs_type_t)prop->type;

        const cJSON* item = cJSON_GetObjectItem(json, field->name);
        if (item == NULL) {
            continue;
        }

        if (cJSON_IsArray(item)) {
            field->is_array = true;
            field->data.array.count = cJSON_GetArraySize(item);
            field->data.array.entries = calloc(field->data.array.count, sizeof(xfs_data));
            for (uint32_t j = 0; j < field->data.array.count; j++) {
                const cJSON* array_item = cJSON_GetArrayItem(item, j);
                if (array_item == NULL) {
                    continue;
                }

                xfs_data* data = &field->data.array.entries[j];
                if (!xfs_data_from_json(array_item, field->type, data, xfs)) {
                    free(obj->fields);
                    free(obj);
                    return NULL;
                }
            }
        } else {
            field->is_array = false;
            if (!xfs_data_from_json(item, field->type, &field->data, xfs)) {
                free(obj->fields);
                free(obj);
                return NULL;
            }
        }
    }

    return obj;
}

bool xfs_data_from_json(const cJSON* json, xfs_type_t type, xfs_data* data, xfs* xfs) {
    if (cJSON_IsNull(json)) {
        memset(data, 0, sizeof(xfs_data));
        return true;
    }

    cJSON* temp[2] = { NULL, NULL };

    switch (type) {
    case XFS_TYPE_UNDEFINED:
    case XFS_TYPE_PROPERTY:
    case XFS_TYPE_EVENT:
    case XFS_TYPE_GROUP:
    case XFS_TYPE_PAGE_BEGIN:
    case XFS_TYPE_PAGE_END:
    case XFS_TYPE_EVENT32:
    case XFS_TYPE_ARRAY:
    case XFS_TYPE_PROPERTYLIST:
    case XFS_TYPE_GROUP_END:
    case XFS_TYPE_ENUMLIST:
    case XFS_TYPE_OSCILLATOR:
    case XFS_TYPE_VARIABLE:
    case XFS_TYPE_RECT3D_COLLISION:
    case XFS_TYPE_EVENT64:
    case XFS_TYPE_END:
        return false;
    case XFS_TYPE_CLASS:
    case XFS_TYPE_CLASSREF:
        data->obj = xfs_object_from_json(json, xfs);
        break;
    case XFS_TYPE_BOOL:
        data->value.b = cJSON_IsTrue(json);
        break;
    case XFS_TYPE_U8:
        data->value.u8 = xfs_json_get_t(uint8_t, json, NULL);
        break;
    case XFS_TYPE_U16:
        data->value.u16 = xfs_json_get_t(uint16_t, json, NULL);
        break;
    case XFS_TYPE_U32:
        data->value.u32 = xfs_json_get_t(uint32_t, json, NULL);
        break;
    case XFS_TYPE_U64:
        data->value.u64 = xfs_json_get_t(uint64_t, json, NULL);
        break;
    case XFS_TYPE_S8:
        data->value.s8 = xfs_json_get_t(int8_t, json, NULL);
        break;
    case XFS_TYPE_S16:
        data->value.s16 = xfs_json_get_t(int16_t, json, NULL);
        break;
    case XFS_TYPE_S32:
        data->value.s32 = xfs_json_get_t(int32_t, json, NULL);
        break;
    case XFS_TYPE_S64:
        data->value.s64 = xfs_json_get_t(int64_t, json, NULL);
        break;
    case XFS_TYPE_F32:
        data->value.f32 = xfs_json_get_t(float, json, NULL);
        break;
    case XFS_TYPE_F64:
        data->value.f64 = xfs_json_get_t(double, json, NULL);
        break;
    case XFS_TYPE_STRING:
    case XFS_TYPE_CSTRING:
        data->str = strdup(cJSON_GetStringValue(json));
        break;
    case XFS_TYPE_COLOR:
        if (cJSON_IsString(json)) {
            const char* color_str = cJSON_GetStringValue(json);
            if (color_str[0] == '#') {
                data->value.color = strtoul(color_str + 1, NULL, 16);
            } else {
                data->value.color = (uint32_t)strtoul(color_str, NULL, 16);
            }
        } else {
            data->value.color = 0;
        }
        break;
    case XFS_TYPE_POINT:
        data->value.point.x = xfs_json_get_t(int32_t, json, "x");
        data->value.point.y = xfs_json_get_t(int32_t, json, "y");
        break;
    case XFS_TYPE_SIZE:
        data->value.size.w = xfs_json_get_t(int32_t, json, "w");
        data->value.size.h = xfs_json_get_t(int32_t, json, "h");
        break;
    case XFS_TYPE_RECT:
        data->value.rect.l = xfs_json_get_t(int32_t, json, "l");
        data->value.rect.t = xfs_json_get_t(int32_t, json, "t");
        data->value.rect.r = xfs_json_get_t(int32_t, json, "r");
        data->value.rect.b = xfs_json_get_t(int32_t, json, "b");
        break;
    case XFS_TYPE_MATRIX:
        xfs_json_get_matrix(json, NULL, &data->value.matrix.m[0][0], 4, 4);
        break;
    case XFS_TYPE_VECTOR3:
        xfs_json_get_float3(json, NULL, &data->value.vector3.x);
        break;
    case XFS_TYPE_VECTOR4:
        xfs_json_get_float4(json, NULL, &data->value.vector4.x);
        break;
    case XFS_TYPE_QUATERNION:
        xfs_json_get_float4(json, NULL, &data->value.quaternion.x);
        break;
    case XFS_TYPE_TIME:
        data->value.time.time = xfs_json_get_t(int64_t, json, NULL);
        break;
    case XFS_TYPE_FLOAT2:
        xfs_json_get_float2(json, NULL, &data->value.float2.x);
        break;
    case XFS_TYPE_FLOAT3:
        xfs_json_get_float3(json, NULL, &data->value.float3.x);
        break;
    case XFS_TYPE_FLOAT4:
        xfs_json_get_float4(json, NULL, &data->value.float4.x);
        break;
    case XFS_TYPE_FLOAT3x3:
        xfs_json_get_matrix(json, NULL, &data->value.float3x3.m[0][0], 3, 3);
        break;
    case XFS_TYPE_FLOAT4x3:
        xfs_json_get_matrix(json, NULL, &data->value.float4x3.m[0][0], 4, 3);
        break;
    case XFS_TYPE_FLOAT4x4:
        xfs_json_get_matrix(json, NULL, &data->value.float4x4.m[0][0], 4, 4);
        break;
    case XFS_TYPE_EASECURVE:
        data->value.easecurve.p1 = xfs_json_get_t(float, json, "p1");
        data->value.easecurve.p2 = xfs_json_get_t(float, json, "p2");
        break;
    case XFS_TYPE_LINE:
        xfs_json_get_float3(json, "from", &data->value.line.from.x);
        xfs_json_get_float3(json, "dir", &data->value.line.dir.x);
        break;
    case XFS_TYPE_LINESEGMENT:
        xfs_json_get_float3(json, "p0", &data->value.linesegment.p0.x);
        xfs_json_get_float3(json, "p1", &data->value.linesegment.p1.x);
        break;
    case XFS_TYPE_RAY:
        xfs_json_get_float3(json, "from", &data->value.ray.from.x);
        xfs_json_get_float3(json, "dir", &data->value.ray.dir.x);
        break;
    case XFS_TYPE_PLANE:
        xfs_json_get_float3(json, "normal", &data->value.plane.normal.x);
        data->value.plane.dist = xfs_json_get_t(float, json, "dist");
        break;
    case XFS_TYPE_SPHERE:
        xfs_json_get_float3(json, "center", &data->value.sphere.center.x);
        data->value.sphere.radius = xfs_json_get_t(float, json, "radius");
        break;
    case XFS_TYPE_CAPSULE:
        xfs_json_get_float3(json, "p0", &data->value.capsule.p0.x);
        xfs_json_get_float3(json, "p1", &data->value.capsule.p1.x);
        data->value.capsule.radius = xfs_json_get_t(float, json, "radius");
        break;
    case XFS_TYPE_AABB:
        xfs_json_get_float3(json, "min", &data->value.aabb.min.x);
        xfs_json_get_float3(json, "max", &data->value.aabb.max.x);
        break;
    case XFS_TYPE_OBB:
        xfs_json_get_matrix(json, "transform", &data->value.obb.transform.m[0][0], 4, 4);
        xfs_json_get_float3(json, "extent", &data->value.obb.extent.x);
        break;
    case XFS_TYPE_CYLINDER:
        xfs_json_get_float3(json, "p0", &data->value.cylinder.p0.x);
        xfs_json_get_float3(json, "p1", &data->value.cylinder.p1.x);
        data->value.cylinder.radius = xfs_json_get_t(float, json, "radius");
        break;
    case XFS_TYPE_TRIANGLE:
        xfs_json_get_float3(json, "p0", &data->value.triangle.p0.x);
        xfs_json_get_float3(json, "p1", &data->value.triangle.p1.x);
        xfs_json_get_float3(json, "p2", &data->value.triangle.p2.x);
        break;
    case XFS_TYPE_CONE:
        xfs_json_get_float3(json, "p0", &data->value.cone.p0.x);
        xfs_json_get_float3(json, "p1", &data->value.cone.p1.x);
        data->value.cone.r0 = xfs_json_get_t(float, json, "r0");
        data->value.cone.r1 = xfs_json_get_t(float, json, "r1");
        break;
    case XFS_TYPE_TORUS:
        xfs_json_get_float3(json, "pos", &data->value.torus.pos.x);
        xfs_json_get_float3(json, "axis", &data->value.torus.axis.x);
        data->value.torus.r = xfs_json_get_t(float, json, "r");
        data->value.torus.cr = xfs_json_get_t(float, json, "cr");
        break;
    case XFS_TYPE_ELLIPSOID:
        xfs_json_get_float3(json, "pos", &data->value.ellipsoid.pos.x);
        xfs_json_get_float3(json, "r", &data->value.ellipsoid.r.x);
        break;
    case XFS_TYPE_RANGE:
        data->value.range.s = xfs_json_get_t(int32_t, json, "s");
        data->value.range.r = xfs_json_get_t(uint32_t, json, "r");
        break;
    case XFS_TYPE_RANGEF:
        data->value.rangef.s = xfs_json_get_t(float, json, "s");
        data->value.rangef.r = xfs_json_get_t(float, json, "r");
        break;
    case XFS_TYPE_RANGEU16:
        data->value.rangeu16.s = xfs_json_get_t(uint16_t, json, "s");
        data->value.rangeu16.r = xfs_json_get_t(uint16_t, json, "r");
        break;
    case XFS_TYPE_HERMITECURVE:
        temp[0] = cJSON_GetObjectItem(json, "x");
        temp[1] = cJSON_GetObjectItem(json, "y");
        if (temp[0] == NULL || temp[1] == NULL) {
            return false;
        }

        if (!cJSON_IsArray(temp[0]) || !cJSON_IsArray(temp[1])) {
            return false;
        }

        for (int i = 0; i < 8; i++) {
            data->value.hermitecurve.x[i] = xfs_json_get_array_t(float, temp[0], i);
            data->value.hermitecurve.y[i] = xfs_json_get_array_t(float, temp[1], i);
        }
        break;
    case XFS_TYPE_FLOAT3x4:
        xfs_json_get_matrix(json, NULL, &data->value.float3x4.m[0][0], 3, 4);
        break;
    case XFS_TYPE_LINESEGMENT4:
        xfs_json_get_soa_vector3(json, "p0", &data->value.linesegment4.p0_4);
        xfs_json_get_soa_vector3(json, "p1", &data->value.linesegment4.p1_4);
        break;
    case XFS_TYPE_AABB4:
        xfs_json_get_soa_vector3(json, "min", &data->value.aabb4.min_4);
        xfs_json_get_soa_vector3(json, "max", &data->value.aabb4.max_4);
        break;
    case XFS_TYPE_VECTOR2:
        xfs_json_get_float2(json, NULL, &data->value.vector2.x);
        break;
    case XFS_TYPE_MATRIX33:
        xfs_json_get_matrix(json, NULL, &data->value.matrix33.m[0][0], 3, 3);
        break;
    case XFS_TYPE_RECT3D_XZ:
        xfs_json_get_float2(json, "lt", &data->value.rect3d_xz.lt.x);
        xfs_json_get_float2(json, "lb", &data->value.rect3d_xz.lb.x);
        xfs_json_get_float2(json, "rt", &data->value.rect3d_xz.rt.x);
        xfs_json_get_float2(json, "rb", &data->value.rect3d_xz.rb.x);
        break;
    case XFS_TYPE_RECT3D:
        xfs_json_get_float3(json, "normal", &data->value.rect3d.normal.x);
        xfs_json_get_float3(json, "center", &data->value.rect3d.center.x);
        data->value.rect3d.size_w = xfs_json_get_t(float, json, "size_w");
        data->value.rect3d.size_h = xfs_json_get_t(float, json, "size_h");
        break;
    case XFS_TYPE_PLANE_XZ:
        data->value.plane_xz.dist = xfs_json_get_t(float, json, "dist");
        break;
    case XFS_TYPE_RAY_Y:
        xfs_json_get_float3(json, "from", &data->value.ray_y.from.x);
        data->value.ray_y.dir = xfs_json_get_t(float, json, "dir");
        break;
    case XFS_TYPE_POINTF:
        data->value.pointf.x = xfs_json_get_t(float, json, "x");
        data->value.pointf.y = xfs_json_get_t(float, json, "y");
        break;
    case XFS_TYPE_SIZEF:
        data->value.sizef.w = xfs_json_get_t(float, json, "w");
        data->value.sizef.h = xfs_json_get_t(float, json, "h");
        break;
    case XFS_TYPE_RECTF:
        data->value.rectf.l = xfs_json_get_t(float, json, "l");
        data->value.rectf.t = xfs_json_get_t(float, json, "t");
        data->value.rectf.r = xfs_json_get_t(float, json, "r");
        data->value.rectf.b = xfs_json_get_t(float, json, "b");
        break;
    case XFS_TYPE_CUSTOM:
        temp[0] = cJSON_GetObjectItem(json, "values");
        if (cJSON_IsArray(temp[0])) {
            data->custom.count = cJSON_GetArraySize(temp[0]);
            data->custom.values = (char**)calloc(data->custom.count, sizeof(char*));
            for (uint8_t i = 0; i < data->custom.count; i++) {
                const cJSON* item = cJSON_GetArrayItem(temp[0], i);
                if (item == NULL || !cJSON_IsString(item)) {
                    return false;
                }

                data->custom.values[i] = strdup(cJSON_GetStringValue(item));
            }
        } else {
            return false;
        }
        break;
    }

    return true;
}
