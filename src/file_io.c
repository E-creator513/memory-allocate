#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "file_io.h"

enum open_status fopen_read(FILE ** file, const char *filepath){
     *file = fopen(filepath, "rb");
    if (!*file) {
        return OPEN_READ_ERROR;
    }
    return OPEN_OK;
    }
    
enum open_status fopen_write(FILE ** file, const char *filepath){
    *file = fopen(filepath, "wb");
    if (!*file) {
        return OPEN_WRITE_ERROR;
    }
    return OPEN_OK;
    }

enum close_status file_close(FILE * file){
    fclose(file);
    return CLOSE_OK;
    }

static char* const open_status_string[] = {
        [OPEN_OK]             = "Файл для чтения/записи открыт\n",
        [OPEN_WRITE_ERROR]    = "Ошибка при открытии файла для записи\n",
        [OPEN_READ_ERROR]     = "Error opening file for reading\n",
};

enum open_status print_open_status(enum open_status status){
    printf(open_status_string[status]);
    return status;
}

enum open_status print_close_status(enum open_status status){
    printf("File is close");
    return status;
}
