#include <stdlib.h>
#include <stdio.h>
#include "bmp_rotation.h"
#include "bmp_io.h"


struct image rotate(struct image const img) {
    struct image img_rotate = {.width = img.height,.height = img.width};
    img_rotate.data = malloc(img.height * img.width * sizeof(struct pixel));
    for (uint64_t row = 0; row < img.height; row++) {
        for (uint64_t col = 0; col < img.width; col++) {
            img_rotate.data[row+((img_rotate.height-col-1) * img_rotate.width)] = img.data[col+row * img.width];
        }
    }
    return img_rotate;
}
© 2022 GitHub, Inc
