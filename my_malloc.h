#ifndef MY_MALLOC_H
#define MY_MALLOC_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

typedef struct s_elem {
    void*           ptr_for_the_user;   
    size_t          size;
    int             used; //0 - free, 1 - used
    void*           start_of_block;
    struct s_elem*  next;
} s_elem;

s_elem* heap = NULL;

//number of bytes for mmap
size_t ALLOC_BYTES = 4096*10000;

void* my_malloc(size_t size);
void* my_realloc(void *ptr1, size_t size); 
void* my_calloc(size_t nmemb, size_t size);
void my_free(void* ptr1);

//helper functions
void init_new_chunk(s_elem* chunk, size_t size, void* start_of_block);
int validate_ptr(void* ptr1);
void merge_chunks(s_elem* chunk1, s_elem* chunk2);
void split_chunks(s_elem* chunk1, size_t new_size);
void coalesce(s_elem* current);

#endif