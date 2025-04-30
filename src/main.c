#include "args.h"
#include "xfs/convert.h"

#include <stdio.h>

int main(int argc, char** argv) {
    Args args;
    if (args_parse(argc, argv, &args) != ARGS_RESULT_OK) {
        return -1;
    }

    if (!xfs_converter_run(&args)) {
        fprintf(stderr, "Failed to convert files\n");
        args_free(&args);
        return -1;
    }

    fprintf(stdout, "Conversion completed successfully\n");
    args_free(&args);

    return 0;
}
