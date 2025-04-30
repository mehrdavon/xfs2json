#include "convert.h"
#include "xfs.h"

#include <stdlib.h>
#include <stdio.h>
#include <cJSON.h>
#include <string.h>


static bool xfs2json(const char* input, const char* output);
static bool json2xfs(const char* input, const char* output);
static bool convert_files(const char* input, const char* output);
static bool str_endswith(const char* str, const char* suffix);

bool xfs_converter_run(const Args* args) {
    if (args == NULL) {
        return false;
    }

    if (!args->input_is_dir && !args->output_is_dir) {
        return convert_files(args->input, args->output);
    }

    if (args->input_is_dir) {
        // Walk directory, convert each file
    }

    return true;
}

bool xfs2json(const char* input, const char* output) {
    xfs xfs;
    if (xfs_load(input, &xfs) != XFS_RESULT_OK) {
        fprintf(stderr, "Failed to load XFS file: %s\n", input);
        return false;
    }

    cJSON* const json = xfs_to_json(&xfs);
    char* const json_str = cJSON_Print(json);

    FILE* const file = fopen(output, "w");
    if (file == NULL) {
        fprintf(stderr, "Failed to open output file: %s\n", output);
        cJSON_Delete(json);
        free(json_str);
        xfs_free(&xfs);
        return false;
    }

    if (fwrite(json_str, sizeof(char), strlen(json_str), file) != strlen(json_str)) {
        fprintf(stderr, "Failed to write to output file: %s\n", output);
        fclose(file);
        cJSON_Delete(json);
        free(json_str);
        xfs_free(&xfs);
        return false;
    }

    fclose(file);
    cJSON_Delete(json);
    free(json_str);
    xfs_free(&xfs);

    fprintf(stdout, "Converted %s to %s\n", input, output);

    return true;
}

bool json2xfs(const char* input, const char* output) {
    return false;
}

bool convert_files(const char* input, const char* output) {
    if (str_endswith(input, ".json")) {
        return json2xfs(input, output);
    }

    if (is_xfs_file(input)) {
        return xfs2json(input, output);
    }

    fprintf(stderr, "Input file %s is neither JSON nor XFS.", input);
    return false;
}

static bool str_endswith(const char* str, const char* suffix) {
    if (str == NULL || suffix == NULL) {
        return false;
    }

    const size_t str_len = strlen(str);
    const size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) {
        return false;
    }

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}
