#include "xfs.h"

#include <stdlib.h>
#include <stdio.h>

static int xfs_load_class(xfs* xfs, int class_id, size_t offset) {
    if (xfs == NULL || class_id < 0 || class_id >= xfs->header->class_count) {
        return XFS_RESULT_ERROR;
    }

}

int xfs_load(const char* path, xfs* xfs) {
    if (path == NULL || xfs == NULL) {
        return XFS_RESULT_ERROR;
    }

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return XFS_RESULT_ERROR;
    }

    fseek(file, 0, SEEK_END);
    xfs->size = ftell(file);
    fseek(file, 0, SEEK_SET);

    xfs->data = malloc(xfs->size);
    if (xfs->data == NULL) {
        fclose(file);
        return XFS_RESULT_ERROR;
    }

    fread(xfs->data, 1, xfs->size, file);
    fclose(file);

    xfs->header = (xfs_header*)xfs->data;
    xfs->defs = (xfs_def*)((char*)xfs->data + xfs->header->def_offset);
    xfs->root = (xfs_class_ref*)((char*)xfs->data + sizeof(xfs_header) + xfs->header->def_size);
}

void xfs_free(xfs* xfs) {
    free(xfs->data);
}
