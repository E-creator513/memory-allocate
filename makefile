APP_NAME = rotator

CC = gcc
CFLAGS = --std=c18 -Isrc/ -pedantic -Wall -Werror
SRC_DIR = src/
BUILD_DIR = build/
APP = $(BUILD_DIR)$(APP_NAME) 

C_SRC_FILES = $(wildcard $(SRC_DIR)*.c)
OBJ_FILES = $(addprefix $(BUILD_DIR),$(notdir $(C_SRC_FILES:.c=.o)))

.PHONY: all clean

define build-obj
	$(CC) -c $(CFLAGS) $< -o $@ 
endef

all: $(APP)

$(APP): $(OBJ_FILES)
	$(CC) -o $(APP) $^

$(OBJ_FILES): $(BUILD_DIR)%.o: $(SRC_DIR)%.c
	mkdir -p $(BUILD_DIR)
	$(call build-obj) 

clean:
	rm  -rf $(BUILD_DIR)*
