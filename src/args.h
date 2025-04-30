#ifndef ARGS_H
#define ARGS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Args {
    const char* input;
    const char* output;

    bool input_is_dir;
    bool output_is_dir;
} Args;

enum {
    ARGS_RESULT_OK,
    ARGS_RESULT_EXIT,
};

int args_parse(int argc, char** argv, Args* args);
void args_free(Args* args);
void args_print_help();

#endif // ARGS_H
