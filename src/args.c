#include "args.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <argparse.h>

#if defined(_WIN32) || defined(MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif


static const char* const s_description = "Converts MT Framework XFS files to and from JSON.";
static const char* const s_usages[] = {
    "xfs2json [-h] [-o <output>] <input>",
    NULL,
};

static bool util_fs_exists(const char* path) {
#ifdef _WIN32
    DWORD file_attr = GetFileAttributesA(path);
    return file_attr != INVALID_FILE_ATTRIBUTES;
#else
    return access(path, F_OK) != -1;
#endif
}

static bool util_fs_is_dir(const char* path) {
#ifdef _WIN32
    DWORD file_attr = GetFileAttributesA(path);
    return (file_attr != INVALID_FILE_ATTRIBUTES && (file_attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat buffer;
    return (stat(path, &buffer) == 0 && S_ISDIR(buffer.st_mode));
#endif
}


int args_parse(int argc, char** argv, Args* args) {
    if (argv == NULL || args == NULL) {
        return ARGS_RESULT_EXIT;
    }

    const char* input = NULL;
    const char* output = NULL;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('o', "output", &output, "Output file/directory", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse parser;
    argparse_init(&parser, options, s_usages, 0);
    argparse_describe(&parser, s_description, NULL);

    int remaining_args = argparse_parse(&parser, argc, argv);
    if (output != NULL) {
        if (!util_fs_exists(output)) {
            printf("Error: %s does not exist!\n", output);
            return ARGS_RESULT_EXIT;
        }

        args->output = _strdup(output);
        args->output_is_dir = util_fs_is_dir(output);
    }

    if (remaining_args == 0) {
        printf("Error: missing required positional argument 'input'.\n");
        return ARGS_RESULT_EXIT;
    }
    
    input = argv[0]; {
        if (!util_fs_exists(input)) {
            printf("Error: %s does not exist!\n", input);
            return ARGS_RESULT_EXIT;
        }

        args->input = strdup(input);
        args->input_is_dir = util_fs_is_dir(input);
    }

    return ARGS_RESULT_OK;
}

void args_free(Args* args) {
    if (args == NULL) {
        return;
    }

    free((void*)args->input);
    free((void*)args->output);

    args->input = NULL;
    args->output = NULL;
}

void args_print_help() {
    printf("Usage: xfs2json [-h] [-o <output>] <input>\n");
    printf("\n");
    printf("Options:\n");
    printf("    -h, --help              Displays this help and exits.\n");
    printf("    -o, --output <output>   Sets the output file/directory.\n");
    printf("    <input>                 Sets the input file/directory (required)\n");
}
