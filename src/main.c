#include "args.h"
#include "xfs/xfs.h"

#include <stdio.h>

int main(int argc, char** argv) {
    Args args;
    if (args_parse(argc, argv, &args) != ARGS_RESULT_OK) {
        return -1;
    }

    printf("Input: %s\n", args.input);
    printf("Output: %s\n", args.output);

    xfs xfs;
    if (xfs_load(args.input, &xfs) != XFS_RESULT_OK) {
        printf("Failed to load XFS file: %s\n", args.input);
        args_free(&args);
        return -1;
    }

    for (int i = 0; i < xfs.header.def_count; i++) {
        const xfs_def* def = &xfs.defs[i];
        printf("Class ID: %u\n", def->dti_hash);
        for (int j = 0; j < def->prop_count; j++) {
            const xfs_property_def* prop = &def->props[j];
            printf("  Property: %s, Type: %u, Size: %u\n", xfs_get_property_name(&xfs, prop), prop->type, prop->bytes);
        }
    }

    xfs_free(&xfs);

    return 0;
}
