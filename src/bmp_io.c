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
 /*printing information
 
    class Test {
    int screenWidth;
    int screenHeight;
    HWND targetWindow;
    HDC targetDC;
    HDC captureDC;
    RGBQUAD *pixels;
    HBITMAP captureBitmap;


    bool TakeScreenshot() {
        ZeroMemory(pixels, screenHeight*screenWidth);
        screenWidth = GetSystemMetrics(SM_CXSCREEN);
        screenHeight = GetSystemMetrics(SM_CYSCREEN);

        targetWindow = GetDesktopWindow();
        targetDC = GetDC(NULL);


        captureDC = CreateCompatibleDC(targetDC);

        captureBitmap = CreateCompatibleBitmap(targetDC, screenWidth, screenHeight);
        HGDIOBJ old = SelectObject(captureDC, captureBitmap);
        if (!old)
            printf("Error selecting object\n");

        OpenClipboard(NULL);
        EmptyClipboard();
        SetClipboardData(CF_BITMAP, captureBitmap);
        CloseClipboard();

        if (BitBlt(captureDC, 0, 0, screenWidth, screenHeight, targetDC, 0, 0, SRCCOPY)) {
            BITMAPINFO bmi = { 0 };
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = screenWidth;
            bmi.bmiHeader.biHeight = -screenHeight;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.bmiHeader.biSizeImage = 0;

            if (!SelectObject(captureDC, old))
                printf("Error unselecting object\n");
            if (!GetDIBits(captureDC,
                captureBitmap,
                0,
                screenHeight,
                pixels,
                &bmi,
                DIB_RGB_COLORS
            )) {
                printf("%s: GetDIBits failed\n", __FUNCTION__);
                return false;
            }

        }
        else {
            printf("%s: BitBlt failed\n", __FUNCTION__);
            return false;
        }
        return true;
    }
    // This is from somewhere on stackoverflow - can't find where.
    void MakePicture() {
        typedef struct                       /**** BMP file header structure ****/
        {
            unsigned int   bfSize;           /* Size of file */
            unsigned short bfReserved1;      /* Reserved */
            unsigned short bfReserved2;      /* ... */
            unsigned int   bfOffBits;        /* Offset to bitmap data */
        } BITMAPFILEHEADER;

        BITMAPFILEHEADER bfh;
        BITMAPINFOHEADER bih;

        unsigned short bfType = 0x4d42;
        bfh.bfReserved1 = 0;
        bfh.bfReserved2 = 0;
        bfh.bfSize = 2 + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 2560 * 1440 * 3;
        bfh.bfOffBits = 0x36;

        bih.biSize = sizeof(BITMAPINFOHEADER);
        bih.biWidth = screenWidth;
        bih.biHeight = screenHeight;
        bih.biPlanes = 1;
        bih.biBitCount = 24;
        bih.biCompression = 0;
        bih.biSizeImage = 0;
        bih.biXPelsPerMeter = 5000;
        bih.biYPelsPerMeter = 5000;
        bih.biClrUsed = 0;
        bih.biClrImportant = 0;

        FILE *file;
        fopen_s(&file, "test.bmp", "wb");
        if (!file)
        {
            printf("Could not write file\n");
            return;
        }

      */  
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
