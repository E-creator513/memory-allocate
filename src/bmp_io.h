#ifndef BMP_IO_H
#define BMP_IO_H
#include "bmp_struct.h"
#include "stdio.h"

enum read_status {
    READ_OK = 0,
    READ_INVALID_SIGNATURE,
    READ_INVALID_BITS,
    READ_INVALID_HEADER,
    READ_INVALID_PATH
} ;

enum write_status {
    WRITE_OK = 0,
    WRITE_ERROR
} ;

void free_bmp(struct image* img);
enum read_status from_bmp( FILE* in, struct image* img);
enum write_status to_bmp( FILE* out, struct image const* img );

enum read_status print_read_status(enum read_status status);
enum read_status print_write_status(enum write_status status);
#endif
