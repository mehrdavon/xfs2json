#include "arch_64.h"
#include "xfs/xfs.h"

#include <stdlib.h>
#include <string.h>


int xfs_v15_64_load(binary_reader* r, struct xfs* xfs) {
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
    const uint64_t* def_offsets = (uint64_t*)buffer;

    xfs->defs = calloc(xfs->header.def_count, sizeof(xfs_def));
    if (xfs->defs == NULL) {
        fprintf(stderr, "Failed to allocate memory for XFS defs\n");
        free(buffer);
        return XFS_RESULT_ERROR;
    }

    for (uint32_t i = 0; i < xfs->header.def_count; i++) {
        xfs_def* d = &xfs->defs[i];
        
        if (def_offsets[i] != 0) {
            const xfs_v15_64_def* def = (xfs_v15_64_def*)(buffer + def_offsets[i]);

            // Preserve raw header bytes FIRST for perfect round-trip
            memcpy(d->raw_header, buffer + def_offsets[i], 16);
            
            d->dti_hash = def->dti_hash;
            d->prop_count = def->prop_count;
            d->init = def->init;
            d->props = calloc(d->prop_count, sizeof(xfs_property_def));
            if (d->props == NULL) {
                fprintf(stderr, "Failed to allocate memory for XFS property defs\n");
                free(buffer);
                return XFS_RESULT_ERROR;
            }

            for (uint32_t j = 0; j < d->prop_count; j++) {
                const xfs_v15_64_property_def* prop = &def->props[j];
                xfs_property_def* p = &d->props[j];

                p->name = strdup((char*)buffer + prop->name_offset);
                p->type = (xfs_type_t)prop->type;
                p->attr = prop->attr;
                p->bytes = prop->bytes;
                p->disable = prop->disable;
            }
        } else {
            // Empty definition - clear raw header
            memset(d->raw_header, 0, 16);
        }
    }

    free(buffer);

    return XFS_RESULT_OK;
}

size_t xfs_v15_64_get_def_size(const struct xfs* xfs, bool include_strings) {
    size_t def_size = 0;

    def_size += sizeof(uint64_t) * xfs->header.def_count;
    def_size += sizeof(xfs_v15_64_def) * xfs->header.def_count;

    size_t prop_count = 0;
    size_t string_buffer_size = 0;

    for (uint32_t i = 0; i < xfs->header.def_count; i++) {
        def_size += sizeof(xfs_v15_64_property_def) * xfs->defs[i].prop_count;
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

int xfs_v15_64_save(binary_writer* w, const struct xfs* xfs) {
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
        binary_writer_write_u64(writer, 0); // Placeholder for offset
    }

    size_t string_offset = xfs_v15_64_get_def_size(xfs, false);

    for (uint32_t i = 0; i < xfs->header.def_count; i++) {
        const xfs_def* def = &xfs->defs[i];

        binary_writer_set_u64(writer, i * sizeof(uint64_t), writer->buffer_pos);
        // Write the preserved raw header for perfect round-trip
        binary_writer_write(writer, def->raw_header, 16);

        for (int j = 0; j < def->prop_count; j++) {
            const xfs_property_def* prop = &def->props[j];

            const xfs_v15_64_property_def p = {
                .name_offset = string_offset,
                .type = (uint8_t)prop->type,
                .attr = prop->attr,
                .bytes = prop->bytes,
                .disable = prop->disable,
                .pad0 = 0,
                .unknown = {0},
            };

            binary_writer_write(writer, &p, sizeof(p));

            const size_t length = strlen(prop->name) + 1; // +1 for null terminator
            binary_writer_write_at(writer, string_offset, prop->name, length);
            string_offset += length;
        }
    }

    binary_writer_write(w, data, xfs->header.def_size);

    free(data);
    binary_writer_destroy(writer);

    return XFS_RESULT_OK;
}
