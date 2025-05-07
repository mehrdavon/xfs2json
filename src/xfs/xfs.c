#include "xfs.h"
#include "util/binary_reader.h"
#include "util/binary_writer.h"

#include "xfs/v16/arch_32.h"
#include "xfs/v15/arch_64.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define XFS_ERROR(...) \
    fprintf(stderr, __VA_ARGS__); \
    xfs_free(xfs); \
    binary_reader_destroy(reader); \
    return XFS_RESULT_ERROR

static xfs_object* xfs_load_object(xfs* xfs, binary_reader* r);
static bool xfs_load_data(xfs* xfs, xfs_type_t type, xfs_data* data, binary_reader* r);

static bool xfs_save_object(const xfs* xfs, const xfs_object* obj, binary_writer* w);
static bool xfs_save_data(const xfs * xfs, const xfs_data* data, xfs_type_t type, binary_writer* w);

static void xfs_free_def(xfs_def* def);
static void xfs_free_property_def(xfs_property_def* prop);
static void xfs_free_object(xfs_object* obj);
static void xfs_free_field(xfs_field* field);
static void xfs_free_data(xfs_type_t type, xfs_data* data);

int xfs_load(const char* path, xfs* xfs) {
    if (path == NULL || xfs == NULL) {
        return XFS_RESULT_ERROR;
    }

    binary_reader* reader = binary_reader_create(path);

    binary_reader_read(reader, &xfs->header, sizeof(xfs_header));
    if (xfs->header.magic != XFS_MAGIC) {
        fprintf(stderr, "Invalid XFS file: %s\n", path);
        binary_reader_destroy(reader);
        return XFS_RESULT_INVALID;
    }

    switch (xfs->header.major_version) {
    case XFS_VERSION_15:
        if (xfs_v15_64_load(reader, xfs) != XFS_RESULT_OK) {
            binary_reader_destroy(reader);
            return XFS_RESULT_ERROR;
        }
        break;
    case XFS_VERSION_16:
        if (xfs_v16_32_load(reader, xfs) != XFS_RESULT_OK) {
            binary_reader_destroy(reader);
            return XFS_RESULT_ERROR;
        }
        break;
    default:
        fprintf(stderr, "Unsupported XFS version: %04X-%04X\n", xfs->header.major_version, xfs->header.minor_version);
        binary_reader_destroy(reader);
        return XFS_RESULT_INVALID;
    }

    xfs->root = xfs_load_object(xfs, reader);
    if (xfs->root == NULL) {
        XFS_ERROR("Failed to load root object\n");
    }

    binary_reader_destroy(reader);

    return XFS_RESULT_OK;
}

int xfs_save(const char* path, const xfs* xfs) {
    if (path == NULL || xfs == NULL) {
        return XFS_RESULT_ERROR;
    }

    binary_writer* writer = binary_writer_create(path);
    if (writer == NULL) {
        fprintf(stderr, "Failed to create binary writer for XFS file: %s\n", path);
        return XFS_RESULT_ERROR;
    }

    binary_writer_write(writer, &xfs->header, sizeof(xfs_header));
    
    switch (xfs->header.major_version) {
    case XFS_VERSION_15:
        if (xfs_v15_64_save(writer, xfs) != XFS_RESULT_OK) {
            binary_writer_destroy(writer);
            return XFS_RESULT_ERROR;
        }
        break;
    case XFS_VERSION_16:
        if (xfs_v16_32_save(writer, xfs) != XFS_RESULT_OK) {
            binary_writer_destroy(writer);
            return XFS_RESULT_ERROR;
        }
        break;
    default:
        fprintf(stderr, "Unsupported XFS version: %04X-%04X\n", xfs->header.major_version, xfs->header.minor_version);
        binary_writer_destroy(writer);
        return XFS_RESULT_INVALID;
    }

    if (!xfs_save_object(xfs, xfs->root, writer)) {
        fprintf(stderr, "Failed to save XFS object\n");
        binary_writer_destroy(writer);
        return XFS_RESULT_ERROR;
    }

    binary_writer_destroy(writer);

    return XFS_RESULT_OK;
}

void xfs_free(xfs* xfs) {
    for (uint32_t i = 0; i < xfs->header.def_count; i++) {
        xfs_free_def(&xfs->defs[i]);
    }

    xfs_free_object(xfs->root);
    free(xfs->defs);
}

bool is_xfs_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }

    xfs_header header;
    if (fread(&header, sizeof(xfs_header), 1, file) != 1) {
        fclose(file);
        return false;
    }

    fclose(file);

    return header.magic == XFS_MAGIC;
}

static void xfs_free_def(xfs_def* def) {
    for (uint32_t i = 0; i < def->prop_count; i++) {
        xfs_free_property_def(&def->props[i]);
    }

    free(def->props);
}

static void xfs_free_property_def(xfs_property_def* prop) {
    free(prop->name);
}

static void xfs_free_object(xfs_object* obj) {
    if (obj == NULL) {
        return;
    }

    for (uint32_t i = 0; i < obj->def->prop_count; i++) {
        xfs_free_field(&obj->fields[i]);
    }

    free(obj->fields);
    free(obj);
}

void xfs_free_field(xfs_field* field) {
    if (field == NULL) {
        return;
    }

    if (field->is_array) {
        for (uint32_t j = 0; j < field->data.array.count; j++) {
            xfs_free_data(field->type, &field->data.array.entries[j]);
        }

        free(field->data.array.entries);
    } else {
        xfs_free_data(field->type, &field->data);
    }
}

void xfs_free_data(xfs_type_t type, xfs_data* data) {
    if (data == NULL) {
        return;
    }

    if (type == XFS_TYPE_CLASS || type == XFS_TYPE_CLASSREF) {
        xfs_free_object(data->obj);
    } else if (type == XFS_TYPE_STRING || type == XFS_TYPE_CSTRING) {
        free(data->str);
    } else if (type == XFS_TYPE_CUSTOM) {
        for (uint8_t j = 0; j < data->custom.count; j++) {
            free(data->custom.values[j]);
        }

        free((void*)data->custom.values);
    }
}

static xfs_object* xfs_load_object(xfs* xfs, binary_reader* r) {
    xfs_class_ref ref;
    if (binary_reader_read(r, &ref, sizeof(xfs_class_ref)) != BINARY_READER_OK) {
        fprintf(stderr, "Failed to read XFS class reference\n");
        return NULL;
    }

    if ((ref.class_id >> 1 & 0x7FFF) == 0x7FFF || (ref.class_id & 1) == 0) {
        return NULL; // Skip invalid class ID
    }

    xfs_object* obj = calloc(1, sizeof(xfs_object));
    if (obj == NULL) {
        fprintf(stderr, "Failed to allocate memory for XFS object\n");
        return NULL;
    }

    obj->def = &xfs->defs[ref.class_id >> 1];
    obj->def_id = ref.class_id >> 1;
    obj->id = ref.var;
    obj->fields = calloc(obj->def->prop_count, sizeof(xfs_field));

    if (obj->fields == NULL) {
        fprintf(stderr, "Failed to allocate memory for XFS object fields\n");
        free(obj);
        return NULL;
    }

    const uint32_t size = binary_reader_read_u32(r);
    const size_t start_pos = binary_reader_tell(r);

    if (xfs->header.major_version == XFS_VERSION_15) {
        (void)binary_reader_read_u32(r);
    }

    for (uint32_t i = 0; i < obj->def->prop_count; i++) {
        const xfs_property_def* prop = &obj->def->props[i];
        xfs_field* field = &obj->fields[i];

        field->name = prop->name;
        field->type = (xfs_type_t)prop->type;
        field->is_array = false;

        const uint32_t count = binary_reader_read_u32(r);
        if (count == 0 || count > 1) {
            field->is_array = true;
            field->data.array.count = count;
            field->data.array.entries = calloc(count, sizeof(xfs_data));
            if (field->data.array.entries == NULL) {
                fprintf(stderr, "Failed to allocate memory for XFS array entries\n");
                free(obj->fields);
                free(obj);
                binary_reader_seek(r, (int)(start_pos + size), SEEK_SET);
                return NULL;
            }

            for (uint32_t j = 0; j < count; j++) {
                if (!xfs_load_data(xfs, field->type, &field->data.array.entries[j], r)) {
                    fprintf(stderr, "Failed to load array entry\n");
                    free(obj->fields);
                    free(obj);
                    binary_reader_seek(r, (int)(start_pos + size), SEEK_SET);
                    return NULL;
                }
            }
        } else {
            if (!xfs_load_data(xfs, field->type, &field->data, r)) {
                fprintf(stderr, "Failed to load field value\n");
                free(obj->fields);
                free(obj);
                binary_reader_seek(r, (int)(start_pos + size), SEEK_SET);
                return NULL;
            }
        }
    }

    return obj;
}

bool xfs_load_data(xfs* xfs, xfs_type_t type, xfs_data* data, binary_reader* r) {
    char string_buffer_1[512];
    char string_buffer_2[128];

    switch (type) {
    case XFS_TYPE_UNDEFINED: break;
    case XFS_TYPE_CLASS:
    case XFS_TYPE_CLASSREF:
        data->obj = xfs_load_object(xfs, r);
        break;
    case XFS_TYPE_BOOL:
        data->value.b = binary_reader_read_bool(r);
        break;
    case XFS_TYPE_U8:
        data->value.u8 = binary_reader_read_u8(r);
        break;
    case XFS_TYPE_U16:
        data->value.u16 = binary_reader_read_u16(r);
        break;
    case XFS_TYPE_U32:
        data->value.u32 = binary_reader_read_u32(r);
        break;
    case XFS_TYPE_U64:
        data->value.u64 = binary_reader_read_u64(r);
        break;
    case XFS_TYPE_S8:
        data->value.s8 = binary_reader_read_s8(r);
        break;
    case XFS_TYPE_S16:
        data->value.s16 = binary_reader_read_s16(r);
        break;
    case XFS_TYPE_S32:
        data->value.s32 = binary_reader_read_s32(r);
        break;
    case XFS_TYPE_S64:
        data->value.s64 = binary_reader_read_s64(r);
        break;
    case XFS_TYPE_F32:
        data->value.f32 = binary_reader_read_f32(r);
        break;
    case XFS_TYPE_F64:
        data->value.f64 = binary_reader_read_f64(r);
        break;
    case XFS_TYPE_STRING:
    case XFS_TYPE_CSTRING:
        if (binary_reader_read_str(r, string_buffer_1, sizeof(string_buffer_1)) != BINARY_READER_OK) {
            fprintf(stderr, "Failed to read XFS string\n");
            return false;
        }

        data->str = strdup(string_buffer_1);
        if (data->str == NULL) {
            fprintf(stderr, "Failed to allocate memory for XFS string\n");
            return false;
        }
        break;
    case XFS_TYPE_COLOR:
        data->value.color = binary_reader_read_u32(r);
        break;
    case XFS_TYPE_POINT:
        data->value.point.x = binary_reader_read_s32(r);
        data->value.point.y = binary_reader_read_s32(r);
        break;
    case XFS_TYPE_SIZE:
        data->value.size.w = binary_reader_read_s32(r);
        data->value.size.h = binary_reader_read_s32(r);
        break;
    case XFS_TYPE_RECT:
        data->value.rect.l = binary_reader_read_s32(r);
        data->value.rect.t = binary_reader_read_s32(r);
        data->value.rect.r = binary_reader_read_s32(r);
        data->value.rect.b = binary_reader_read_s32(r);
        break;
    case XFS_TYPE_MATRIX:
        binary_reader_read(r, &data->value.matrix, sizeof(xfs_matrix));
        break;
    case XFS_TYPE_VECTOR3:
        binary_reader_read(r, &data->value.vector3, sizeof(xfs_vector3));
        break;
    case XFS_TYPE_VECTOR4:
        binary_reader_read(r, &data->value.vector4, sizeof(xfs_vector4));
        break;
    case XFS_TYPE_QUATERNION:
        binary_reader_read(r, &data->value.quaternion, sizeof(xfs_quaternion));
        break;
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
        fprintf(stderr, "Unsupported type: %d\n", type);
        break;
    case XFS_TYPE_TIME:
        data->value.time.time = binary_reader_read_s64(r);
        break;
    case XFS_TYPE_FLOAT2:
        data->value.float2.x = binary_reader_read_f32(r);
        data->value.float2.y = binary_reader_read_f32(r);
        break;
    case XFS_TYPE_FLOAT3:
        data->value.float3.x = binary_reader_read_f32(r);
        data->value.float3.y = binary_reader_read_f32(r);
        data->value.float3.z = binary_reader_read_f32(r);
        break;
    case XFS_TYPE_FLOAT4:
        data->value.float4.x = binary_reader_read_f32(r);
        data->value.float4.y = binary_reader_read_f32(r);
        data->value.float4.z = binary_reader_read_f32(r);
        data->value.float4.w = binary_reader_read_f32(r);
        break;
    case XFS_TYPE_FLOAT3x3:
        binary_reader_read(r, &data->value.float3x3, sizeof(xfs_float3x3));
        break;
    case XFS_TYPE_FLOAT4x3:
        binary_reader_read(r, &data->value.float4x3, sizeof(xfs_float4x3));
        break;
    case XFS_TYPE_FLOAT4x4:
        binary_reader_read(r, &data->value.float4x4, sizeof(xfs_float4x4));
        break;
    case XFS_TYPE_EASECURVE:
        data->value.easecurve.p1 = binary_reader_read_f32(r);
        data->value.easecurve.p2 = binary_reader_read_f32(r);
        break;
    case XFS_TYPE_LINE:
        binary_reader_read(r, &data->value.line, sizeof(xfs_line));
        break;
    case XFS_TYPE_LINESEGMENT:
        binary_reader_read(r, &data->value.linesegment, sizeof(xfs_linesegment));
        break;
    case XFS_TYPE_RAY:
        binary_reader_read(r, &data->value.ray, sizeof(xfs_ray));
        break;
    case XFS_TYPE_PLANE:
        binary_reader_read(r, &data->value.plane, sizeof(xfs_plane));
        break;
    case XFS_TYPE_SPHERE:
        binary_reader_read(r, &data->value.sphere, sizeof(xfs_sphere));
        break;
    case XFS_TYPE_CAPSULE:
        binary_reader_read(r, &data->value.capsule, sizeof(xfs_capsule));
        break;
    case XFS_TYPE_AABB:
        binary_reader_read(r, &data->value.aabb, sizeof(xfs_aabb));
        break;
    case XFS_TYPE_OBB:
        binary_reader_read(r, &data->value.obb, sizeof(xfs_obb));
        break;
    case XFS_TYPE_CYLINDER:
        binary_reader_read(r, &data->value.cylinder, sizeof(xfs_cylinder));
        break;
    case XFS_TYPE_TRIANGLE:
        binary_reader_read(r, &data->value.triangle, sizeof(xfs_triangle));
        break;
    case XFS_TYPE_CONE:
        binary_reader_read(r, &data->value.cone, sizeof(xfs_cone));
        break;
    case XFS_TYPE_TORUS:
        binary_reader_read(r, &data->value.torus, sizeof(xfs_torus));
        break;
    case XFS_TYPE_ELLIPSOID:
        binary_reader_read(r, &data->value.ellipsoid, sizeof(xfs_ellipsoid));
        break;
    case XFS_TYPE_RANGE:
        data->value.range.s = binary_reader_read_s32(r);
        data->value.range.r = binary_reader_read_u32(r);
        break;
    case XFS_TYPE_RANGEF:
        data->value.rangef.s = binary_reader_read_f32(r);
        data->value.rangef.r = binary_reader_read_f32(r);
        break;
    case XFS_TYPE_RANGEU16:
        data->value.rangeu16.s = binary_reader_read_u16(r);
        data->value.rangeu16.r = binary_reader_read_u16(r);
        break;
    case XFS_TYPE_HERMITECURVE:
        binary_reader_read(r, &data->value.hermitecurve, sizeof(xfs_hermitecurve));
        break;
    case XFS_TYPE_FLOAT3x4:
        binary_reader_read(r, &data->value.float3x4, sizeof(xfs_float3x4));
        break;
    case XFS_TYPE_LINESEGMENT4:
        binary_reader_read(r, &data->value.linesegment4, sizeof(xfs_linesegment4));
        break;
    case XFS_TYPE_AABB4:
        binary_reader_read(r, &data->value.aabb4, sizeof(xfs_aabb4));
        break;
    case XFS_TYPE_VECTOR2:
        data->value.vector2.x = binary_reader_read_f32(r);
        data->value.vector2.y = binary_reader_read_f32(r);
        break;
    case XFS_TYPE_MATRIX33:
        binary_reader_read(r, &data->value.matrix33, sizeof(xfs_matrix33));
        break;
    case XFS_TYPE_RECT3D_XZ:
        binary_reader_read(r, &data->value.rect3d_xz, sizeof(xfs_rect3d_xz));
        break;
    case XFS_TYPE_RECT3D:
        binary_reader_read(r, &data->value.rect3d, sizeof(xfs_rect3d));
        break;
    case XFS_TYPE_PLANE_XZ:
        binary_reader_read(r, &data->value.plane_xz, sizeof(xfs_plane_xz));
        break;
    case XFS_TYPE_RAY_Y:
        binary_reader_read(r, &data->value.ray_y, sizeof(xfs_ray_y));
        break;
    case XFS_TYPE_POINTF:
        data->value.pointf.x = binary_reader_read_f32(r);
        data->value.pointf.y = binary_reader_read_f32(r);
        break;
    case XFS_TYPE_SIZEF:
        data->value.sizef.w = binary_reader_read_f32(r);
        data->value.sizef.h = binary_reader_read_f32(r);
        break;
    case XFS_TYPE_RECTF:
        data->value.rectf.t = binary_reader_read_f32(r);
        data->value.rectf.l = binary_reader_read_f32(r);
        data->value.rectf.b = binary_reader_read_f32(r);
        data->value.rectf.r = binary_reader_read_f32(r);
        break;
    case XFS_TYPE_CUSTOM:
        data->custom.count = binary_reader_read_u8(r);
        data->custom.values = (char**)malloc(data->custom.count * sizeof(char*));
        if (data->custom.values == NULL) {
            fprintf(stderr, "Failed to allocate memory for XFS custom values\n");
            return false;
        }

        for (uint8_t i = 0; i < data->custom.count; i++) {
            if (binary_reader_read_str(r, string_buffer_2, sizeof(string_buffer_2)) != BINARY_READER_OK) {
                fprintf(stderr, "Failed to read XFS custom value\n");
                return false;
            }
            data->custom.values[i] = strdup(string_buffer_2);
            if (data->custom.values[i] == NULL) {
                fprintf(stderr, "Failed to allocate memory for XFS custom value\n");
                return false;
            }
        }
        break;
    }

    return true;
}

bool xfs_save_object(const xfs* xfs, const xfs_object* obj, binary_writer* w) {
    if (obj == NULL || w == NULL) {
        return false;
    }

    const xfs_class_ref ref = {
        .class_id = (obj->def_id << 1) | 1,
        .var = obj->id
    };

    binary_writer_write(w, &ref, sizeof(xfs_class_ref));

    const size_t start_pos = binary_writer_tell(w);

    binary_writer_write_u32(w, 0); // Placeholder for size, filled out below
    if (xfs->header.major_version == XFS_VERSION_15) {
        binary_writer_write_u32(w, 0); // v15 size is 8 bytes
    }

    for (uint32_t i = 0; i < obj->def->prop_count; i++) {
        const xfs_property_def* prop = &obj->def->props[i];
        const xfs_field* field = &obj->fields[i];

        binary_writer_write_s32(w, field->is_array ? field->data.array.count : 1);

        if (field->is_array) {
            for (uint32_t j = 0; j < field->data.array.count; j++) {
                if (!xfs_save_data(xfs, &field->data.array.entries[j], field->type, w)) {
                    return false;
                }
            }
        } else {
            if (!xfs_save_data(xfs, &field->data, field->type, w)) {
                return false;
            }
        }
    }

    const size_t end_pos = binary_writer_tell(w);
    const size_t size = end_pos - start_pos;
    binary_writer_seek(w, (int)start_pos, SEEK_SET);

    switch (xfs->header.major_version) {
    case XFS_VERSION_15:
        binary_writer_write_u64(w, size);
        break;
    case XFS_VERSION_16:
        binary_writer_write_u32(w, (uint32_t)size);
        break;
    default:
        break; // Should not happen
    }

    binary_writer_seek(w, (int)end_pos, SEEK_SET);

    return true;
}

bool xfs_save_data(const xfs* xfs, const xfs_data* data, xfs_type_t type, binary_writer* w) {
    switch (type) {
    case XFS_TYPE_UNDEFINED: break;
    case XFS_TYPE_CLASS:
    case XFS_TYPE_CLASSREF:
        if (data->obj != NULL) {
            if (!xfs_save_object(xfs, data->obj, w)) {
                return false;
            }
        }
        break;
    case XFS_TYPE_BOOL:
        binary_writer_write_bool(w, data->value.b);
        break;
    case XFS_TYPE_U8:
        binary_writer_write_u8(w, data->value.u8);
        break;
    case XFS_TYPE_U16:
        binary_writer_write_u16(w, data->value.u16);
        break;
    case XFS_TYPE_U32:
        binary_writer_write_u32(w, data->value.u32);
        break;
    case XFS_TYPE_U64:
        binary_writer_write_u64(w, data->value.u64);
        break;
    case XFS_TYPE_S8:
        binary_writer_write_s8(w, data->value.s8);
        break;
    case XFS_TYPE_S16:
        binary_writer_write_s16(w, data->value.s16);
        break;
    case XFS_TYPE_S32:
        binary_writer_write_s32(w, data->value.s32);
        break;
    case XFS_TYPE_S64:
        binary_writer_write_s64(w, data->value.s64);
        break;
    case XFS_TYPE_F32:
        binary_writer_write_f32(w, data->value.f32);
        break;
    case XFS_TYPE_F64:
        binary_writer_write_f64(w, data->value.f64);
        break;
    case XFS_TYPE_STRING:
    case XFS_TYPE_CSTRING:
        if (data->str != NULL) {
            binary_writer_write_str(w, data->str);
        } else {
            binary_writer_write_str(w, "");
        }
        break;
    case XFS_TYPE_COLOR:
        binary_writer_write_u32(w, data->value.color);
        break;
    case XFS_TYPE_POINT:
        binary_writer_write_s32(w, data->value.point.x);
        binary_writer_write_s32(w, data->value.point.y);
        break;
    case XFS_TYPE_SIZE:
        binary_writer_write_s32(w, data->value.size.w);
        binary_writer_write_s32(w, data->value.size.h);
        break;
    case XFS_TYPE_RECT:
        binary_writer_write(w, &data->value.rect, sizeof(xfs_rect));
        break;
    case XFS_TYPE_MATRIX:
        binary_writer_write(w, &data->value.matrix, sizeof(xfs_matrix));
        break;
    case XFS_TYPE_VECTOR3:
        binary_writer_write(w, &data->value.vector3, sizeof(xfs_vector3));
        break;
    case XFS_TYPE_VECTOR4:
        binary_writer_write(w, &data->value.vector4, sizeof(xfs_vector4));
        break;
    case XFS_TYPE_QUATERNION:
        binary_writer_write(w, &data->value.quaternion, sizeof(xfs_quaternion));
        break;
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
        fprintf(stderr, "Unsupported type: %d\n", type);
        break;
    case XFS_TYPE_TIME:
        binary_writer_write_s64(w, data->value.time.time);
        break;
    case XFS_TYPE_FLOAT2:
        binary_writer_write_f32(w, data->value.float2.x);
        break;
    case XFS_TYPE_FLOAT3:
        binary_writer_write(w, &data->value.float3, sizeof(xfs_float3));
        break;
    case XFS_TYPE_FLOAT4:
        binary_writer_write(w, &data->value.float4, sizeof(xfs_float4));
        break;
    case XFS_TYPE_FLOAT3x3:
        binary_writer_write(w, &data->value.float3x3, sizeof(xfs_float3x3));
        break;
    case XFS_TYPE_FLOAT4x3:
        binary_writer_write(w, &data->value.float4x3, sizeof(xfs_float4x3));
        break;
    case XFS_TYPE_FLOAT4x4:
        binary_writer_write(w, &data->value.float4x4, sizeof(xfs_float4x4));
        break;
    case XFS_TYPE_EASECURVE:
        binary_writer_write_f32(w, data->value.easecurve.p1);
        binary_writer_write_f32(w, data->value.easecurve.p2);
        break;
    case XFS_TYPE_LINE:
        binary_writer_write(w, &data->value.line, sizeof(xfs_line));
        break;
    case XFS_TYPE_LINESEGMENT:
        binary_writer_write(w, &data->value.linesegment, sizeof(xfs_linesegment));
        break;
    case XFS_TYPE_RAY:
        binary_writer_write(w, &data->value.ray, sizeof(xfs_ray));
        break;
    case XFS_TYPE_PLANE:
        binary_writer_write(w, &data->value.plane, sizeof(xfs_plane));
        break;
    case XFS_TYPE_SPHERE:
        binary_writer_write(w, &data->value.sphere, sizeof(xfs_sphere));
        break;
    case XFS_TYPE_CAPSULE:
        binary_writer_write(w, &data->value.capsule, sizeof(xfs_capsule));
        break;
    case XFS_TYPE_AABB:
        binary_writer_write(w, &data->value.aabb, sizeof(xfs_aabb));
        break;
    case XFS_TYPE_OBB:
        binary_writer_write(w, &data->value.obb, sizeof(xfs_obb));
        break;
    case XFS_TYPE_CYLINDER:
        binary_writer_write(w, &data->value.cylinder, sizeof(xfs_cylinder));
        break;
    case XFS_TYPE_TRIANGLE:
        binary_writer_write(w, &data->value.triangle, sizeof(xfs_triangle));
        break;
    case XFS_TYPE_CONE:
        binary_writer_write(w, &data->value.cone, sizeof(xfs_cone));
        break;
    case XFS_TYPE_TORUS:
        binary_writer_write(w, &data->value.torus, sizeof(xfs_torus));
        break;
    case XFS_TYPE_ELLIPSOID:
        binary_writer_write(w, &data->value.ellipsoid, sizeof(xfs_ellipsoid));
        break;
    case XFS_TYPE_RANGE:
        binary_writer_write_s32(w, data->value.range.s);
        break;
    case XFS_TYPE_RANGEF:
        binary_writer_write_f32(w, data->value.rangef.s);
        break;
    case XFS_TYPE_RANGEU16:
        binary_writer_write_u16(w, data->value.rangeu16.s);
        break;
    case XFS_TYPE_HERMITECURVE:
        binary_writer_write(w, &data->value.hermitecurve, sizeof(xfs_hermitecurve));
        break;
    case XFS_TYPE_FLOAT3x4:
        binary_writer_write(w, &data->value.float3x4, sizeof(xfs_float3x4));
        break;
    case XFS_TYPE_LINESEGMENT4:
        binary_writer_write(w, &data->value.linesegment4, sizeof(xfs_linesegment4));
        break;
    case XFS_TYPE_AABB4:
        binary_writer_write(w, &data->value.aabb4, sizeof(xfs_aabb4));
        break;
    case XFS_TYPE_VECTOR2:
        binary_writer_write_f32(w, data->value.vector2.x);
        binary_writer_write_f32(w, data->value.vector2.y);
        break;
    case XFS_TYPE_MATRIX33:
        binary_writer_write(w, &data->value.matrix33, sizeof(xfs_matrix33));
        break;
    case XFS_TYPE_RECT3D_XZ:
        binary_writer_write(w, &data->value.rect3d_xz, sizeof(xfs_rect3d_xz));
        break;
    case XFS_TYPE_RECT3D:
        binary_writer_write(w, &data->value.rect3d, sizeof(xfs_rect3d));
        break;
    case XFS_TYPE_PLANE_XZ:
        binary_writer_write(w, &data->value.plane_xz, sizeof(xfs_plane_xz));
        break;
    case XFS_TYPE_RAY_Y:
        binary_writer_write(w, &data->value.ray_y, sizeof(xfs_ray_y));
        break;
    case XFS_TYPE_POINTF:
        binary_writer_write_f32(w, data->value.pointf.x);
        binary_writer_write_f32(w, data->value.pointf.y);
        break;
    case XFS_TYPE_SIZEF:
        binary_writer_write_f32(w, data->value.sizef.w);
        binary_writer_write_f32(w, data->value.sizef.h);
        break;
    case XFS_TYPE_RECTF:
        binary_writer_write(w, &data->value.rectf, sizeof(xfs_rectf));
        break;
    case XFS_TYPE_CUSTOM:
        binary_writer_write_u8(w, data->custom.count);
        for (uint8_t i = 0; i < data->custom.count; i++) {
            if (data->custom.values[i] != NULL) {
                binary_writer_write_str(w, data->custom.values[i]);
            } else {
                binary_writer_write_str(w, "");
            }
        }
        break;
    }

    return true;
}
