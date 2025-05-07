#ifndef XFS_V15_ARCH_64_H
#define XFS_V15_ARCH_64_H

#include <stdint.h>
#include <stdbool.h>

#include "xfs/common.h"
#include "util/binary_reader.h"
#include "util/binary_writer.h"


typedef struct xfs_v15_64_property_def {
    uint64_t name_offset;
    uint8_t type;
    uint8_t attr;
    uint16_t bytes : 15;
    uint16_t disable : 1;
    uint32_t pad0;
    uint64_t unknown[8];
} xfs_v15_64_property_def;

typedef struct xfs_v15_64_def {
    uint32_t dti_hash;
    uint32_t pad0;
    uint32_t prop_count : 15;
    uint32_t init : 1;
    uint32_t : 16;
    uint32_t pad1;
    xfs_v15_64_property_def props[0];
} xfs_v15_64_def;

typedef struct xfs_v15_64_class_ref {
    int16_t class_id;
    int16_t var;
} xfs_v15_64_class_ref;

int xfs_v15_64_load(binary_reader* r, struct xfs* xfs);
size_t xfs_v15_64_get_def_size(const struct xfs* xfs, bool include_strings);
int xfs_v15_64_save(binary_writer* w, const struct xfs* xfs);

#endif // XFS_V15_ARCH_64_H
