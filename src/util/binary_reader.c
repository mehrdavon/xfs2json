#include "binary_reader.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define BINARY_READER_READ_IMPL(T) \
    if (reader->buffer_pos + sizeof(T) > reader->buffer_size) { \
        if (binary_reader_refill_buffer(reader) != BINARY_READER_OK) { \
            return 0; \
        } \
    } \
    const T value = *(T*)(reader->buffer + reader->buffer_pos); \
    reader->buffer_pos += sizeof(value); \
    return value


static int binary_reader_refill_buffer(binary_reader* reader);

binary_reader* binary_reader_create(const char* path) {
    if (path == NULL) {
        return NULL;
    }

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }

    binary_reader* reader = malloc(sizeof(binary_reader));
    if (reader == NULL) {
        fclose(file);
        return NULL;
    }

    memset(reader, 0, sizeof(binary_reader));

    reader->file = file;
    fseek(file, 0, SEEK_END);
    reader->size = ftell(file);
    fseek(file, 0, SEEK_SET);

    reader->buffer_size = BINARY_READER_BUFFER_SIZE;

    return reader;
}

void binary_reader_destroy(binary_reader* reader) {
    if (reader == NULL) {
        return;
    }

    if (reader->file != NULL) {
        fclose(reader->file);
    }

    free(reader);
}

size_t binary_reader_seek(binary_reader* reader, int offset, int origin) {
    if (reader == NULL || reader->file == NULL) {
        return (size_t)-1;
    }

    if (fseek(reader->file, offset, origin) == 0) {
        reader->buffer_pos = 0;
        reader->buffer_size = 0;
        return ftell(reader->file);
    }

    return (size_t)-1;
}

size_t binary_reader_tell(binary_reader* reader) {
    if (reader == NULL || reader->file == NULL) {
        return (size_t)-1;
    }

    return ftell(reader->file) - reader->buffer_size + reader->buffer_pos;
}

int binary_reader_read(binary_reader* reader, void* data, size_t size) {
    if (data == NULL || size == 0) {
        return BINARY_READER_ERROR;
    }

    if (reader->buffer_pos + size < reader->buffer_size) {
        memcpy(data, reader->buffer + reader->buffer_pos, size);
        reader->buffer_pos += size;
        return BINARY_READER_OK;
    }

    const size_t remaining_size = reader->buffer_size - reader->buffer_pos;
    if (remaining_size > 0) {
        memcpy(data, reader->buffer + reader->buffer_pos, remaining_size);
        size -= remaining_size;
        data = (char*)data + remaining_size;
    }

    if (size > 0) {
        fread(data, 1, size, reader->file);
        if (ferror(reader->file)) {
            return BINARY_READER_ERROR;
        }

        // Refill the buffer
        fread(reader->buffer, 1, reader->buffer_size, reader->file);
        if (ferror(reader->file)) {
            return BINARY_READER_ERROR;
        }

        reader->buffer_pos = 0;
    }

    return BINARY_READER_OK;
}

int binary_reader_read_str(binary_reader* reader, char* str, size_t max) {
    if (str == NULL || max == 0) {
        return BINARY_READER_ERROR;
    }

    size_t bytes_read = 0;
    while (bytes_read < max - 1) {
        if (reader->buffer_pos >= reader->buffer_size) {
            if (binary_reader_refill_buffer(reader) != BINARY_READER_OK) {
                break;
            }
        }
        str[bytes_read] = reader->buffer[reader->buffer_pos++];
        if (str[bytes_read] == '\0') {
            break;
        }
        bytes_read++;
    }

    str[bytes_read] = '\0';

    return (bytes_read < max - 1) ? BINARY_READER_OK : BINARY_READER_ERROR;
}

uint8_t binary_reader_read_u8(binary_reader* reader) {
    BINARY_READER_READ_IMPL(uint8_t);
}

uint16_t binary_reader_read_u16(binary_reader* reader) {
    BINARY_READER_READ_IMPL(uint16_t);
}

uint32_t binary_reader_read_u32(binary_reader* reader) {
    BINARY_READER_READ_IMPL(uint32_t);
}

uint64_t binary_reader_read_u64(binary_reader* reader) {
    BINARY_READER_READ_IMPL(uint64_t);
}

int8_t binary_reader_read_s8(binary_reader* reader) {
    BINARY_READER_READ_IMPL(int8_t);
}

int16_t binary_reader_read_s16(binary_reader* reader) {
    BINARY_READER_READ_IMPL(int16_t);
}

int32_t binary_reader_read_s32(binary_reader* reader) {
    BINARY_READER_READ_IMPL(int32_t);
}

int64_t binary_reader_read_s64(binary_reader* reader) {
    BINARY_READER_READ_IMPL(int64_t);
}

float binary_reader_read_f32(binary_reader* reader) {
    BINARY_READER_READ_IMPL(float);
}

double binary_reader_read_f64(binary_reader* reader) {
    BINARY_READER_READ_IMPL(double);
}

bool binary_reader_read_bool(binary_reader* reader) {
    BINARY_READER_READ_IMPL(bool);
}

static int binary_reader_refill_buffer(binary_reader* reader) {
    if (reader == NULL || reader->file == NULL) {
        return BINARY_READER_ERROR;
    }

    const size_t remaining_size = reader->buffer_size - reader->buffer_pos;
    if (remaining_size > 0) {
        memmove(reader->buffer, reader->buffer + reader->buffer_pos, remaining_size);
    }

    const size_t bytes_to_read = reader->buffer_size - remaining_size;
    const size_t bytes_read = fread(reader->buffer + remaining_size, 1, bytes_to_read, reader->file);
    if (bytes_read == 0) {
        return feof(reader->file) ? BINARY_READER_EOF : BINARY_READER_ERROR;
    }

    reader->buffer_pos = 0;

    return BINARY_READER_OK;
}
