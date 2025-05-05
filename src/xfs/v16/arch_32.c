#include "arch_32.h"
#include "xfs/xfs.h"

#include <stdlib.h>
#include <string.h>


int XFS_NAME(16, 32, load)(binary_reader* r, struct xfs* xfs) {
    uint8_t* buffer = malloc(xfs->header.def_size);
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for XFS data\n");
        return XFS_RESULT_ERROR;
    }

    if (binary_reader_read(r, buffer, xfs->header.def_size) != BINARY_READER_OK) {
        fprintf(stderr, "Failed to read XFS definitions\n");
        free(buffer);
        return XFS_RESULT_ERROR;
    }

    if (xfs->header.def_size < sizeof(uint32_t) * xfs->header.def_count) {
        fprintf(stderr, "Invalid XFS definition size\n");
        free(buffer);
        return XFS_RESULT_ERROR;
    }

    // Def offsets always start right after the header
    uint32_t* def_offsets = (uint32_t*)buffer;

    xfs->defs = calloc(xfs->header.def_count, sizeof(xfs_def));
    if (xfs->defs == NULL) {
        fprintf(stderr, "Failed to allocate memory for XFS defs\n");
        free(buffer);
        return XFS_RESULT_ERROR;
    }

    for (uint32_t i = 0; i < xfs->header.def_count; i++) {
        if (def_offsets[i] != 0) {
            XFS_NAME(16, 32, def)* def = (XFS_NAME(16, 32, def)*)(buffer + def_offsets[i]);
            xfs_def* d = &xfs->defs[i];

            d->dti_hash = def->dti_hash;
            d->prop_count = def->prop_count;
            d->init = false;
            d->props = calloc(d->prop_count, sizeof(xfs_def));
            if (d->props == NULL) {
                fprintf(stderr, "Failed to allocate memory for XFS property defs\n");
                free(buffer);
                return XFS_RESULT_ERROR;
            }

            for (uint32_t j = 0; j < d->prop_count; j++) {
                XFS_NAME(16, 32, property_def)* prop = &def->props[j];
                xfs_property_def* p = &d->props[j];

                p->name = strdup(buffer + prop->name_offset);
                p->type = (xfs_type_t)prop->type;
                p->attr = prop->attr;
                p->bytes = prop->bytes;
                p->disable = prop->disable;
            }
        }
    }
    
    free(buffer);

    return XFS_RESULT_OK;
}

size_t XFS_NAME(16, 32, get_def_size)(const struct xfs* xfs, bool include_strings) {
    size_t def_size = 0;

    def_size += sizeof(uint32_t) * xfs->header.def_count;
    def_size += sizeof(XFS_NAME(16, 32, def)) * xfs->header.def_count;

    size_t prop_count = 0;
    size_t string_buffer_size = 0;

    for (uint32_t i = 0; i < xfs->header.def_count; i++) {
        def_size += sizeof(XFS_NAME(16, 32, property_def)) * xfs->defs[i].prop_count;
        prop_count += xfs->defs[i].prop_count;

        for (int j = 0; j < xfs->defs[i].prop_count; j++) {
            string_buffer_size += strlen(xfs->defs[i].props[j].name) + 1; // +1 for null terminator
        }
    }

    if (!include_strings) {
        return def_size;
    }

    size_t data_size = def_size + string_buffer_size;
    data_size = (data_size + 3) & ~3; // Align to 4 bytes

    return data_size;
}

int XFS_NAME(16, 32, save)(binary_writer* w, const struct xfs* xfs) {
    uint8_t* data = malloc(xfs->header.def_size);
    if (data == NULL) {
        return XFS_RESULT_ERROR;
    }

    memset(data, 0, xfs->header.def_size);

    binary_writer* writer = binary_writer_create_buffer(data, xfs->header.def_size);
    if (writer == NULL) {
        free(data);
        return XFS_RESULT_ERROR;
    }

    for (uint32_t i = 0; i < xfs->header.def_count; i++) {
        binary_writer_write_u32(writer, 0); // Placeholder for offset
    }

    size_t string_offset = xfs_v16_32_get_def_size(xfs, false);
    
    for (uint32_t i = 0; i < xfs->header.def_count; i++) {
        const xfs_def* def = &xfs->defs[i];

        binary_writer_set_u32(writer, i * sizeof(uint32_t), (uint32_t)writer->buffer_pos);
        binary_writer_write_u32(writer, def->dti_hash);
        binary_writer_write_s32(writer, def->prop_count);

        for (int j = 0; j < def->prop_count; j++) {
            const xfs_property_def* prop = &def->props[j];

            const size_t length = strlen(prop->name) + 1; // +1 for null terminator
            binary_writer_write_u32(writer, string_offset);
            binary_writer_write_at(writer, string_offset, prop->name, length);
            string_offset += length;

            binary_writer_write_u8(writer, (uint8_t)prop->type);
            binary_writer_write_u8(writer, (uint8_t)prop->attr);

            uint16_t bytes = (uint16_t)prop->bytes;
            bytes |= (uint16_t)prop->disable << 15;
            binary_writer_write_u16(writer, bytes);

            binary_writer_write_u64(writer, 0); // Padding
            binary_writer_write_u64(writer, 0); // Padding
            binary_writer_write_u64(writer, 0); // Padding
            binary_writer_write_u64(writer, 0); // Padding
        }
    }

    binary_writer_write(w, data, xfs->header.def_size);

    free(data);
    binary_writer_destroy(writer);

    return XFS_RESULT_OK;
}
