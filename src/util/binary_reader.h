#ifndef BINARY_READER_H
#define BINARY_READER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


#ifndef BINARY_READER_BUFFER_SIZE
#define BINARY_READER_BUFFER_SIZE 4096
#endif


typedef struct binary_reader {
    FILE* file;
    uint8_t* buffer;
    size_t buffer_pos;
    size_t buffer_size;
    size_t size;
    uint8_t local_buffer[BINARY_READER_BUFFER_SIZE];
} binary_reader;

enum {
    BINARY_READER_OK,
    BINARY_READER_EOF,
    BINARY_READER_ERROR
};

binary_reader* binary_reader_create(const char* path);
void binary_reader_destroy(binary_reader* reader);

size_t binary_reader_seek(binary_reader* reader, int offset, int origin);
size_t binary_reader_tell(binary_reader* reader);

int binary_reader_read(binary_reader* reader, void* data, size_t size);
int binary_reader_read_str(binary_reader* reader, char* str, size_t max);
uint8_t binary_reader_read_u8(binary_reader* reader);
uint16_t binary_reader_read_u16(binary_reader* reader);
uint32_t binary_reader_read_u32(binary_reader* reader);
uint64_t binary_reader_read_u64(binary_reader* reader);

int8_t binary_reader_read_s8(binary_reader* reader);
int16_t binary_reader_read_s16(binary_reader* reader);
int32_t binary_reader_read_s32(binary_reader* reader);
int64_t binary_reader_read_s64(binary_reader* reader);

float binary_reader_read_f32(binary_reader* reader);
double binary_reader_read_f64(binary_reader* reader);

bool binary_reader_read_bool(binary_reader* reader);

#endif // BINARY_READER_H
