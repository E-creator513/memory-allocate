#include <stdlib.h>
#include <stdio.h>
#include "bmp_io.h"
#include "bmp_struct.h"

void free_bmp(struct image* img){
    free(img->data);
}

uint64_t count_padding(struct image const *img){
    return (img->width % 4);
}

struct bmp_header * _malloc_bmp_header(){
    return (struct bmp_header *) malloc(sizeof(struct bmp_header));
}

void _free_bmp_header(struct bmp_header * header){
    free(header);
}

struct bmp_header _generate_header (struct image const *img) {

    struct bmp_header header = {
    .bfType = 0x4D42,
    .bfileSize = img->width * img->height * sizeof(struct pixel) + img->height * count_padding(img) + sizeof(struct bmp_header),
    .bfReserved = 0,
    .bOffBits = sizeof(struct bmp_header),

    .biSize = 40,
    .biWidth = img->width,
    .biHeight = img->height,
    .biPlanes = 1,
    .biBitCount = 24,
    .biCompression = 0,
    .biXPelsPerMeter = 0,
    .biYPelsPerMeter = 0,
    .biClrUsed = 0,
    .biClrImportant = 0};
    header.biSizeImage = header.bfileSize - header.bOffBits;
    return header;
}

static enum read_status _check_header(const struct bmp_header header){
if (header.bfType != 0x4D42){ return READ_INVALID_SIGNATURE; }
    if (header.biBitCount != 24){ return READ_INVALID_BITS; }
    if (header.biSize !=40 
    || header.biCompression!=0
    || header.bfileSize != header.bOffBits + header.biSizeImage){
       return READ_INVALID_HEADER;
    }
return READ_OK;
}

enum read_status from_bmp( FILE* in, struct image* img){
    if (in == NULL)  return READ_INVALID_PATH; 
    struct bmp_header header ;
    fread(&header, 1, sizeof(struct bmp_header), in);

    const enum read_status header_status = _check_header(header);
    if (header_status) return header_status;

    uint64_t data_size = header.biHeight * header.biWidth * sizeof(struct pixel);
    img->data = (struct pixel *) malloc(data_size);
    img->height = header.biHeight;
    img->width = header.biWidth;

    uint64_t padding = count_padding(img);

    for (size_t i = 0; i < header.biHeight; i++) {
            fread(&(img->data[i * img->width]), sizeof(struct pixel), img->width, in);
            fseek(in, padding, SEEK_CUR);
    }

    return READ_OK;
}

enum write_status to_bmp( FILE* out, struct image const* img ){   
    struct bmp_header header = _generate_header(img);
    uint64_t padding = count_padding(img);

    if(!fwrite(&header, 1, sizeof(struct bmp_header), out)){
        return WRITE_ERROR;
    }
    const struct pixel nulls[] = {{0},{0},{0}};
    for (size_t i = 0; i < img->height; i++) {
        if(!fwrite(&img->data[i * img->width], sizeof(struct pixel), img->width, out) ||
        ( padding && !fwrite(nulls, 1, padding, out))) 
        {return WRITE_ERROR;}
    }
    return WRITE_OK;
}

static char* const read_status_string[] = {
        [READ_OK]                     = "Image form file is loaded\n",
        [READ_INVALID_SIGNATURE]      = "Invalid Signature. Check file format (Only 24-bit bpm file supported).\n",
        [READ_INVALID_BITS]           = "Only 24-bit bpm file supported\n",
        [READ_INVALID_HEADER]         = "Invalid file header\n",
        [READ_INVALID_PATH]           = "Input file path not found\n"
};

enum read_status print_read_status(enum read_status status){
    printf(read_status_string[status]);
    return status;
}
static char* const write_status_string[] = {
        [WRITE_OK]      = "Image is saved in file\n",
        [WRITE_ERROR]   = "File write error\n"
};

enum read_status print_write_status(enum write_status status){
    printf(write_status_string[status]);
    return status;
}
