#include <stdio.h>
#include <string.h>
#include "mem_internals.h"
#include "mem.h"
#include "mem_debug.h"

void print_test(char* const header) {
  static size_t test_n = 1;
  
  
  
  
  printf("____________Test ##%zu ____________\n%s\n ", test_n,header);
  test_n++;
}

int main() {
  print_test("Init heap");
  void* heap = heap_init(10000);
  debug_heap(stdout, heap);

  print_test("Normal successful memory allocation");
  void* block1 = _malloc(100);
  
  
  
  
  void* block2 = _malloc(400);
  _malloc(100);
  void* block3 = _malloc(1);
  _malloc(1000);
  debug_heap(stdout, heap);
  
  
  

  print_test("Freeing one block from several allocated");
  _free(block3);
  debug_heap(stdout, heap);

  print_test("Freeing two blocks from several allocated");
  _free(block2);
  _free(block1);
  debug_heap(stdout, heap);

   print_test("The memory is over, the new region of memory expands the old");
  _malloc(200);
  
  
  
  _malloc(10000);
  _malloc(10300);
  
  
  
  
  _malloc(666);
  debug_heap(stdout, heap);


  print_test("Free heap");
  _free_heap();
  debug_heap(stdout, heap);

  return 0;
}
