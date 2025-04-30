#include <stdio.h>

#include "xfs.h"

static cJSON* xfs_object_to_json(const xfs_object* obj);
static cJSON* xfs_data_to_json(xfs_type_t type, const xfs_data* data);

static cJSON* xfs_json_create_float2(const float* values);
static cJSON* xfs_json_create_float3(const float* values);
static cJSON* xfs_json_create_float4(const float* values);
static cJSON* xfs_json_create_matrix(const float* values, int m, int n);
static cJSON* xfs_json_create_soa_vector3(const xfs_soa_vector3* value);

cJSON* xfs_to_json(const xfs* xfs) {
    cJSON* json = cJSON_CreateObject();

    // Add definitions
    cJSON* defs = cJSON_CreateArray();
    for (int i = 0; i < xfs->header.def_count; i++) {
        const xfs_def* def = xfs->defs[i];
        cJSON* def_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(def_json, "dti", def->dti_hash);

        cJSON* props = cJSON_CreateArray();
        for (int j = 0; j < def->prop_count; j++) {
            const xfs_property_def* prop = &def->props[j];
            cJSON* prop_json = cJSON_CreateObject();

            cJSON_AddStringToObject(prop_json, "name", xfs_get_property_name(xfs, prop));
            cJSON_AddNumberToObject(prop_json, "type", prop->type);
            cJSON_AddNumberToObject(prop_json, "attr", prop->attr);
            cJSON_AddNumberToObject(prop_json, "bytes", prop->bytes);
            cJSON_AddBoolToObject(prop_json, "disable", prop->disable);

            cJSON_AddItemToArray(props, prop_json);
        }

        cJSON_AddItemToObject(def_json, "props", props);
        cJSON_AddItemToArray(defs, def_json);
    }

    // Add root object
    cJSON_AddItemToObject(json, "root", xfs_object_to_json(xfs->root));

    return json;
}

xfs* xfs_from_json(const cJSON* json) {
    return NULL;
}

cJSON* xfs_object_to_json(const xfs_object* obj) {
    if (obj == NULL) {
        return cJSON_CreateNull();
    }

    cJSON* json = cJSON_CreateObject();

    cJSON_AddNumberToObject(json, "__def_id", obj->def_id);

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
