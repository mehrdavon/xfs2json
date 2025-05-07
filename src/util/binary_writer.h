#ifndef BINARY_WRITER_H
#define BINARY_WRITER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef BINARY_WRITER_BUFFER_SIZE
#define BINARY_WRITER_BUFFER_SIZE 4096
#endif


typedef struct binary_writer {
    FILE* file;
    uint8_t* buffer;
    size_t buffer_size;
    size_t buffer_pos;
} binary_writer;

enum {
    BINARY_WRITER_OK,
    BINARY_WRITER_ERROR,
};

binary_writer* binary_writer_create(const char* path);
binary_writer* binary_writer_create_buffer(uint8_t* buffer, size_t size);
void binary_writer_destroy(binary_writer* writer);

size_t binary_writer_tell(binary_writer* writer);
size_t binary_writer_seek(binary_writer* writer, int offset, int origin);

void binary_writer_write(binary_writer* writer, const void* data, size_t size);
void binary_writer_write_str(binary_writer* writer, const char* str);
void binary_writer_write_u8(binary_writer* writer, uint8_t value);
void binary_writer_write_u16(binary_writer* writer, uint16_t value);
void binary_writer_write_u32(binary_writer* writer, uint32_t value);
void binary_writer_write_u64(binary_writer* writer, uint64_t value);
void binary_writer_write_s8(binary_writer* writer, int8_t value);
void binary_writer_write_s16(binary_writer* writer, int16_t value);
void binary_writer_write_s32(binary_writer* writer, int32_t value);
void binary_writer_write_s64(binary_writer* writer, int64_t value);
void binary_writer_write_f32(binary_writer* writer, float value);
void binary_writer_write_f64(binary_writer* writer, double value);
void binary_writer_write_bool(binary_writer* writer, bool value);

void binary_writer_set_u32(binary_writer* writer, size_t offset, uint32_t value);
void binary_writer_set_u64(binary_writer* writer, size_t offset, uint64_t value);
void binary_writer_write_at(binary_writer* writer, size_t offset, const void* data, size_t size);

#endif // BINARY_WRITER_H
