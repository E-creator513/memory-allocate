#include <stdio.h>
#include <stdlib.h>
#include "file_io.h"
#include "bmp_io.h"
#include "bmp_struct.h"
#include "bmp_rotation.h"

#define STANDART_OUT_FILE "output.bmp"

// i use arch btw

int main(int argc, char *argv[]) {
    if (argc <2||argc >3) {
        printf("Usage: %s <input_file> [output file]\n", argv[0]);
        return 1;
    }
 const char *input_path = argv[1];
    const char *output_path;
    if (argc < 3) {
        output_path = STANDART_OUT_FILE;
    } else {
        output_path = argv[2];
    }
    printf("Input File: '%s' \nOutput File: '%s'\n", input_path, output_path);
    struct image img = {0};

    FILE * in = NULL; 
    enum open_status status_or = fopen_read(&in,input_path);
    if(print_open_status(status_or)){
        return status_or;
    }
    enum read_status status_r = from_bmp(in,&img);
    file_close(in);
    if(print_read_status(status_r)){
        return status_r;
    }
    struct image rotated_img = rotate(img);
    free_bmp(&img);

    FILE *out = NULL;
    enum open_status status_ow = fopen_write(&out,output_path);
    if(print_open_status(status_ow)){
        return status_ow;
    }
    uint8_t status_w = to_bmp(out,&rotated_img);
    file_close(out);
    if(print_write_status(status_w)){
        return status_w;
    }
    free_bmp(&rotated_img);
    printf("Memory freed\n");
    return 0;
}
