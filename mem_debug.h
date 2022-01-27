#ifndef _MEM_DEBUG_H_
#define _MEM_DEBUG_H_
#include "mem_internals.h"

void debug_struct_info(FILE* f, void const* addr);
void debug_heap(FILE* f, void const* ptr);
void debug_block(struct block_header* b, const char* fmt, ...);
void debug(const char* fmt, ...);

#endif