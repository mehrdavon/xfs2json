#ifndef XFS_DEF_32_H
#define XFS_DEF_32_H

#include <stdint.h>
#include <stdbool.h>

#include "xfs/common.h"
#include "util/binary_reader.h"
#include "util/binary_writer.h"


typedef struct XFS_NAME(16, 32, property_def) {
    uint32_t name_offset;
    uint8_t type;
    uint8_t attr;
    uint16_t bytes : 15;
    uint16_t disable : 1;
    uint64_t pad[4];
} XFS_NAME(16, 32, property_def);

typedef struct XFS_NAME(16, 32, def) {
    uint32_t dti_hash;
    uint32_t prop_count : 15;
    uint32_t : 17;
    XFS_NAME(16, 32, property_def) props[0];
} XFS_NAME(16, 32, def);

typedef struct XFS_NAME(16, 32, class_ref) {
    int16_t class_id;
    int16_t var;
} XFS_NAME(16, 32, class_ref);

int XFS_NAME(16, 32, load)(binary_reader* r, struct xfs* xfs);
size_t XFS_NAME(16, 32, get_def_size)(const struct xfs* xfs, bool include_strings);
int XFS_NAME(16, 32, save)(binary_writer* w, const struct xfs* xfs);

#endif // XFS_DEF_32_H
