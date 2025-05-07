#ifndef XFS_V16_ARCH_32_H
#define XFS_V16_ARCH_32_H

#include <stdint.h>
#include <stdbool.h>

#include "xfs/common.h"
#include "util/binary_reader.h"
#include "util/binary_writer.h"


typedef struct xfs_v16_32_property_def {
    uint32_t name_offset;
    uint8_t type;
    uint8_t attr;
    uint16_t bytes : 15;
    uint16_t disable : 1;
    uint64_t pad[4];
} xfs_v16_32_property_def;

typedef struct xfs_v16_32_def {
    uint32_t dti_hash;
    uint32_t prop_count : 15;
    uint32_t : 17;
    xfs_v16_32_property_def props[0];
} xfs_v16_32_def;

typedef struct xfs_v16_32_class_ref {
    int16_t class_id;
    int16_t var;
} xfs_v16_32_class_ref;

int xfs_v16_32_load(binary_reader* r, struct xfs* xfs);
size_t xfs_v16_32_get_def_size(const struct xfs* xfs, bool include_strings);
int xfs_v16_32_save(binary_writer* w, const struct xfs* xfs);

#endif // XFS_V16_ARCH_32_H
