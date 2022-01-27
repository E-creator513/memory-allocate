#include <stdarg.h>
#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include "mem_internals.h"
#include "mem.h"
#include "util.h"

void debug_block(struct block_header* b, const char* fmt, ... );
void debug(const char* fmt, ... );

extern inline block_size size_from_capacity( block_capacity cap );
extern inline block_capacity capacity_from_size( block_size sz );

static bool            block_is_big_enough( size_t query, struct block_header* block ) { return block->capacity.bytes >= query; }
static size_t          pages_count   ( size_t mem )                      { return mem / getpagesize() + ((mem % getpagesize()) > 0); }
static size_t          round_pages   ( size_t mem )                      { return getpagesize() * pages_count( mem ) ; }

static void block_init( void* restrict addr, block_size block_sz, void* restrict next ) {
    *((struct block_header*)addr) = (struct block_header) {
            .next = next,
            .capacity = capacity_from_size(block_sz),
            .is_free = true
    };
}

static size_t region_actual_size( size_t query ) { return size_max( round_pages( query ), REGION_MIN_SIZE ); }

extern inline bool region_is_invalid( const struct region* r );



static void* map_pages(void const* addr, size_t length, int additional_flags) {
    return mmap( (void*) addr, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | additional_flags , 0, 0 );
}

/*  аллоцировать регион памяти и инициализировать его блоком */
static struct region alloc_region  ( void const * addr, size_t query ) {

    query = region_actual_size(query);
    void *next_addr = map_pages(addr, query, MAP_FIXED);

    struct region new_region;
    if(next_addr == MAP_FAILED){
        next_addr = map_pages(addr, query, 0);
        new_region    = (struct region){next_addr,query,false};
    }
    else new_region = (struct region){next_addr,query,true};

    block_init(next_addr, (block_size){query}, NULL);
    return new_region;

}

static void* block_after( struct block_header const* block )         ;

void* heap_init( size_t initial ) {
    const struct region region = alloc_region( HEAP_START, initial );
    if( region_is_invalid(&region) ) return NULL;

    return region.addr;
}

#define BLOCK_MIN_CAPACITY 24

/*  --- Разделение блоков (если найденный свободный блок слишком большой )--- */

static bool block_splittable( struct block_header* restrict block, size_t query) {
    return block-> is_free && query + offsetof( struct block_header, contents ) + BLOCK_MIN_CAPACITY <= block->capacity.bytes;
}

static bool split_if_too_big( struct block_header* block, size_t query ) {
    if (block_splittable(block, query)){
        struct block_header *ending = (struct block_header* )(block->contents + query);
        block_size ending_size = (block_size){block->capacity.bytes - query};

        block_init(ending, ending_size, block->next);
        block->capacity.bytes = query;
        block->next           = ending;
        return true;
    }
    return false;

}


/*  --- Слияние соседних свободных блоков --- */

static void* block_after( struct block_header const* block )              {
    return  (void*) (block->contents + block->capacity.bytes);
}
static bool blocks_continuous (
        struct block_header const* fst,
        struct block_header const* snd ) {
    return (void*)snd == block_after(fst);
}

/*Проверка, что оба блока пусты*/
static bool mergeable(struct block_header const* restrict fst, struct block_header const* restrict snd) {
    return fst->is_free && snd->is_free && blocks_continuous( fst, snd ) ;
}

/*Попытка объединения блока со следующим*/
static bool try_merge_with_next( struct block_header* block ) {
    struct block_header * next_block = block->next;
    if(next_block && mergeable(block, next_block)){
        block->next = next_block->next;
        block->capacity.bytes = block->capacity.bytes +
                                next_block->capacity.bytes +
                                offsetof(struct block_header, contents);
        return true;
    }
    return false;
}

static bool try_merge_with_all_next( struct block_header* block ) {
    bool flag = false;
    while(block->next){
        if(!try_merge_with_next(block)) break;
        flag = true;
    }
    return flag;
}

/*  --- ... ecли размера кучи хватает --- */

struct block_search_result {
    enum {BSR_FOUND_GOOD_BLOCK, BSR_REACHED_END_NOT_FOUND, BSR_CORRUPTED} type;
    struct block_header* block;
};


static struct block_search_result find_good_or_last  ( struct block_header* restrict block, size_t sz )    {
    struct block_header* current_block = block;

    do{
        try_merge_with_all_next(current_block);
        if (current_block->is_free && block_is_big_enough(sz, current_block))
            return (struct block_search_result){.type = BSR_FOUND_GOOD_BLOCK, .block = current_block};
        current_block = current_block->next;
    }while (current_block->next);

    return (struct block_search_result){.type = BSR_REACHED_END_NOT_FOUND, .block = current_block};
}

/*  Попробовать выделить память в куче начиная с блока `block` не пытаясь расширить кучу
 Можно переиспользовать как только кучу расширили. */
static struct block_search_result try_memalloc_existing ( size_t query, struct block_header* block ) {
    struct block_search_result suitable = find_good_or_last(block, query);
    if (suitable.type == BSR_FOUND_GOOD_BLOCK)split_if_too_big(suitable.block, query);
    return suitable;
}


/* Увеличивает кучу, выделяя память как минимум для query и заголовка */
static struct block_header* grow_heap( struct block_header* restrict last, size_t query ) {
    query += offsetof(struct block_header, contents);
    void *addr = last->contents + last->capacity.bytes;

    const struct region region = alloc_region(addr, query);
    if (region_is_invalid(&region)) return NULL;

    last->next = (struct block_header *)region.addr;
    return region.addr;
}

/*  Реализует основную логику malloc и возвращает заголовок выделенного блока */
static struct block_header* memalloc( size_t query, struct block_header* heap_start) {
    if(!heap_start) return NULL;
    if(query < BLOCK_MIN_CAPACITY) query = BLOCK_MIN_CAPACITY;

    struct block_search_result new_block_result = try_memalloc_existing(query, heap_start);
    if(new_block_result.type == BSR_REACHED_END_NOT_FOUND){
        if(!grow_heap(new_block_result.block, query)) return NULL;
        new_block_result = try_memalloc_existing(query, heap_start);
    }
    if(new_block_result.type == BSR_CORRUPTED) return NULL;
    new_block_result.block->is_free = false;
    return new_block_result.block;
}

void* _malloc( size_t query ) {
    struct block_header* const addr = memalloc( query, (struct block_header*) HEAP_START );
    if(addr) return addr->contents;
    else return NULL;
}

static struct block_header* block_get_header(void* contents) {
    return (struct block_header*) (((uint8_t*)contents)-offsetof(struct block_header, contents));
}

/* Очистка блока и попытка объединения блока со следующими*/
void _free( void* mem ) {
    if(!mem) return ;
    struct block_header* header = block_get_header( mem );
    header->is_free = true;
    try_merge_with_all_next(header);
}

/* Очистка всей кучи и объединение в один блок*/
void _free_heap() {
    for(struct block_header* header = (struct block_header*) HEAP_START; header; header = header->next )
        _free((struct block_header*) ((uint64_t)header + offsetof(struct block_header, contents)));
    try_merge_with_all_next((struct block_header*) HEAP_START);
}