#include "binary_writer.h"

#include <stdlib.h>
#include <string.h>

#define BINARY_WRITER_WRITE_IMPL(T) \
    if (writer->buffer_pos + sizeof(T) > writer->buffer_size) { \
        binary_writer_flush(writer); \
    } \
    *(T*)(writer->buffer + writer->buffer_pos) = value; \
    writer->buffer_pos += sizeof(T)


static void binary_writer_flush(binary_writer* writer);

binary_writer* binary_writer_create(const char* path) {
    binary_writer* writer = malloc(sizeof(binary_writer));
    if (writer == NULL) {
        return NULL;
    }

    writer->file = fopen(path, "wb");
    if (writer->file == NULL) {
        free(writer);
        return NULL;
    }

    writer->buffer = malloc(BINARY_WRITER_BUFFER_SIZE);
    if (writer->buffer == NULL) {
        fclose(writer->file);
        free(writer);
        return NULL;
    }

    writer->buffer_size = BINARY_WRITER_BUFFER_SIZE;
    writer->buffer_pos = 0;

    return writer;
}

binary_writer* binary_writer_create_buffer(uint8_t* buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return NULL;
    }

    binary_writer* writer = malloc(sizeof(binary_writer));
    if (writer == NULL) {
        return NULL;
    }

    writer->file = NULL;
    writer->buffer = buffer;
    writer->buffer_size = size;
    writer->buffer_pos = 0;

    return writer;
}

void binary_writer_destroy(binary_writer* writer) {
    if (writer == NULL) {
        return;
    }

    binary_writer_flush(writer);

    if (writer->file != NULL) {
        fclose(writer->file);
        free(writer->buffer); // Free the buffer only if it was allocated by the writer
    }

    free(writer);
}

size_t binary_writer_tell(binary_writer* writer) {
    if (writer == NULL) {
        return (size_t)-1;
    }

    if (writer->file == NULL) {
        return writer->buffer_pos;
    }

    return ftell(writer->file) + writer->buffer_pos;
}

size_t binary_writer_seek(binary_writer* writer, int offset, int origin) {
    if (writer == NULL) {
        return (size_t)-1;
    }

    if (writer->file == NULL) {
        switch (origin) {
        case SEEK_SET:
            writer->buffer_pos = offset;
            break;
        case SEEK_CUR:
            writer->buffer_pos += offset;
            break;
        case SEEK_END:
            writer->buffer_pos = writer->buffer_size - offset;
            break;
        default:
            return (size_t)-1;
        }

        return writer->buffer_pos;
    }

    binary_writer_flush(writer);
    if (fseek(writer->file, offset, origin) == 0) {
        return ftell(writer->file);
    }

    return (size_t)-1;
}

void binary_writer_write(binary_writer* writer, const void* data, size_t size) {
    if (data == NULL || size == 0) {
        return;
    }

    if (size > writer->buffer_size) {
        // If the data is larger than the buffer, write it directly to the file
        // but flush the buffer first. If the file is NULL, we can't write.
        if (writer->file != NULL) {
            binary_writer_flush(writer);
            fwrite(data, 1, size, writer->file);
        }

        return;
    }

    if (writer->buffer_pos + size > writer->buffer_size) {
        // If the buffer is full, flush it before writing
        binary_writer_flush(writer);
    }

    memcpy((char*)writer->buffer + writer->buffer_pos, data, size);
    writer->buffer_pos += size;
}

void binary_writer_write_str(binary_writer* writer, const char* str) {
    if (str == NULL) {
        return;
    }

    const size_t length = strlen(str);
    binary_writer_write(writer, str, length);
    binary_writer_write_u8(writer, 0); // Null-terminate the string
}

void binary_writer_write_u8(binary_writer* writer, uint8_t value) {
    BINARY_WRITER_WRITE_IMPL(uint8_t);
}

void binary_writer_write_u16(binary_writer* writer, uint16_t value) {
    BINARY_WRITER_WRITE_IMPL(uint16_t);
}

void binary_writer_write_u32(binary_writer* writer, uint32_t value) {
    BINARY_WRITER_WRITE_IMPL(uint32_t);
}

void binary_writer_write_u64(binary_writer* writer, uint64_t value) {
    BINARY_WRITER_WRITE_IMPL(uint64_t);
}

void binary_writer_write_s8(binary_writer* writer, int8_t value) {
    BINARY_WRITER_WRITE_IMPL(int8_t);
}

void binary_writer_write_s16(binary_writer* writer, int16_t value) {
    BINARY_WRITER_WRITE_IMPL(int16_t);
}

void binary_writer_write_s32(binary_writer* writer, int32_t value) {
    BINARY_WRITER_WRITE_IMPL(int32_t);
}

void binary_writer_write_s64(binary_writer* writer, int64_t value) {
    BINARY_WRITER_WRITE_IMPL(int64_t);
}

void binary_writer_write_f32(binary_writer* writer, float value) {
    BINARY_WRITER_WRITE_IMPL(float);
}

void binary_writer_write_f64(binary_writer* writer, double value) {
    BINARY_WRITER_WRITE_IMPL(double);
}

void binary_writer_write_bool(binary_writer* writer, bool value) {
    BINARY_WRITER_WRITE_IMPL(bool);
}

void binary_writer_set_u32(binary_writer* writer, size_t offset, uint32_t value) {
    *(uint32_t*)(writer->buffer + offset) = value;
}

void binary_writer_set_u64(binary_writer* writer, size_t offset, uint64_t value) {
    *(uint64_t*)(writer->buffer + offset) = value;
}

void binary_writer_write_at(binary_writer* writer, size_t offset, const void* data, size_t size) {
    if (data == NULL || size == 0) {
        return;
    }

    if (offset + size > writer->buffer_size) {
        return;
    }

    memcpy(writer->buffer + offset, data, size);
}

void binary_writer_flush(binary_writer* writer) {
    if (writer->file != NULL && writer->buffer_pos > 0) {
        fwrite(writer->buffer, 1, writer->buffer_pos, writer->file);
        writer->buffer_pos = 0;
    }
}
