#include "args.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <argparse.h>

#if defined(_WIN32) || defined(MSC_VER)
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

    char* input = NULL;
    char* output = NULL;
    const char* input_extension = NULL;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('o', "output", &output, "Output file/directory", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse parser;
    argparse_init(&parser, options, s_usages, 0);
    argparse_describe(&parser, s_description, NULL);

    const int remaining_args = argparse_parse(&parser, argc, (const char**)argv);
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

        if (!args->input_is_dir) {
            input_extension = strrchr(input, '.');
        }
    }

    if (output != NULL) {
        if (!util_fs_exists(output)) {
            printf("Error: %s does not exist!\n", output);
            return ARGS_RESULT_EXIT;
        }

        args->output = strdup(output);
        args->output_is_dir = util_fs_is_dir(output);
    } else {
        if (args->input_is_dir) {
            printf("Output directory not specified, using input directory.\n");
            args->output = strdup(input);
            args->output_is_dir = true;
        } else {
            // Just using .xfs for now because we can't guess the actual extension it should be
            const char* output_extension = strcmp(input_extension, ".json") == 0 ? "xfs" : "json";
            const int length = snprintf(NULL, 0, "%s.%s", input, output_extension);
            output = malloc(length + 1);
            if (output == NULL) {
                printf("Error: Failed to allocate memory for output path.\n");
                return ARGS_RESULT_EXIT;
            }

            snprintf(output, length + 1, "%s.%s", input, output_extension);

            args->output = output;
            args->output_is_dir = false;
        }
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
