#include "args.h"
#include <stdio.h>

int main(int argc, char** argv) {
    Args args;
    if (args_parse(argc, argv, &args) != ARGS_RESULT_OK) {
        return -1;
    }

    printf("Input: %s\n", args.input);
    printf("Output: %s\n", args.output);

    return 0;
}
