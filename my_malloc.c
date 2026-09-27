#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include "my_malloc.h"

void* my_malloc(size_t size) { 
    //adds one byte to avoid memory overlap
    if (size == 0) {
        size = 1;
    }

    //checks for overflow
    if (size > SIZE_MAX - sizeof(s_elem)) {
        return NULL;
    }

    //checks that size isn't bigger than space allocated by mmap
    if (size > ALLOC_BYTES - sizeof(s_elem)) {
        return NULL;
    }

    //if heap hasn't been allocated yet, requests a big chunk of memory and sets up first element
    if (heap == NULL) {
        heap = mmap(0, ALLOC_BYTES, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
        if (heap == MAP_FAILED) {
            return NULL;
        }
        //pointer returned to the user is located after the bytes taken up by s_elem
        init_new_chunk(heap, size, heap);
        return heap->ptr_for_the_user;

    } else {
        //if heap has already been allocated, loops through the linked list, looking for a chunk that can be reused first
        s_elem* ptr = heap;
        s_elem* prev = NULL;
        while (ptr != NULL) {
            //if a suitable empty chunk exists, the pointer to it is returned to the user
            if (ptr->used == 0 && ptr->size >= size) {
                ptr->used = 1;

                //check if remaining bytes allow to split unused memory
                if ((ptr->size - size) > sizeof(s_elem)) {
                    split_chunks(ptr, size);
                } else {
                    ptr->size = size;
                }
                return ptr->ptr_for_the_user;
            }
            prev = ptr; //stores the last pointer that was not NULL, so that we can append a new element
            ptr = ptr->next;
        }

        //if suitable empty chunk hasn't been found, a new pointer is created to be returned for the user
        
        //first - checks is there is enough memory left from the previously allocated chunk with mmap
        if (((char*)prev->ptr_for_the_user + prev->size + sizeof(s_elem) + size) <= ((char*)prev->start_of_block + ALLOC_BYTES - 1)) {
            //if yes - calculates next address in the old chunk of memory  
            prev->next = (s_elem*)((char*)prev->ptr_for_the_user + prev->size);
            s_elem* new = prev->next;
            init_new_chunk(new, size, prev->start_of_block);
            return new->ptr_for_the_user;
        
        } else {
            //if not - checks the size of unused "tail" of remaining memory
            if ((size_t)(((char*)prev->start_of_block + ALLOC_BYTES - 1) - ((char*)prev->ptr_for_the_user + prev->size)) > sizeof(s_elem)) {
                //if it is larger than sizeof(s_elem), saves it as a separate chunk
                prev->next = (s_elem*)((char*)prev->ptr_for_the_user + prev->size);
                s_elem* tail = prev->next;
                size_t tail_size = ((char*)prev->start_of_block + ALLOC_BYTES - 1) - ((char*)prev->ptr_for_the_user + prev->size) - sizeof(s_elem);
                init_new_chunk(tail, tail_size, prev->start_of_block);
                tail->used = 0;
                //calls mmap again and appends the next entry to the linked list with address of the new chunk
                tail->next = mmap(0, ALLOC_BYTES, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
                if (tail->next == MAP_FAILED) {
                    return NULL;
                }
                s_elem* new = tail->next;
                init_new_chunk(new, size, new);
                return new->ptr_for_the_user;
            } else {
                //if there is not enough space for separate chunk, ignores the remaining memory and calls mmap again
                prev->next = mmap(0, ALLOC_BYTES, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
                if (prev->next == MAP_FAILED) {
                    return NULL;
                }
                s_elem* new = prev->next;
                init_new_chunk(new, size, new);
                return new->ptr_for_the_user;
            }
        }
    }
}


void* my_realloc(void *ptr1, size_t size) {
    //if provided size is 0 and pointer is not NULL, this memory gets freed
    if (size == 0) {
        my_free(ptr1);
        return NULL;
    }

    //if pointer is NULL, allocates a new chunk of memory and returns a pointer to it
    if (ptr1 == NULL) {
        void* new_ptr = my_malloc(size);
        return new_ptr;
    }

    //looks for ptr1 on the heap to figure out its size
    s_elem* ptr2 = heap;
    while (ptr2 != NULL) {
        if (ptr2->ptr_for_the_user == ptr1 && ptr2->used == 1) {
            break;
        }
        ptr2 = ptr2->next;
    }

    //if ptr1 passed to realloc doesn't exist, creates a new pointer with requested size
    if (ptr2 == NULL) {
        void* new_ptr = my_malloc(size);
        return new_ptr;
    }

    //if the requested realloc size is smaller or equal to ptr1 size, returns same pointer with adjusted size
    if (size <= ptr2->size) {
        //if remaining unused size allows, splits the unused size into separate chunk
        if (ptr2->size - size > sizeof(s_elem)) {
            split_chunks(ptr2, size);
        } else {
            ptr2->size = size;
        }
        return ptr1;
    }

    //if requested size is larger than size of ptr1, allocates a new chunk, copies bytes to new chunk and frees ptr1
    void* new_ptr = my_malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    //memcpy uses the smaller size of the two chunks
    memcpy(new_ptr, ptr1, ptr2->size);
    my_free(ptr1);

    return new_ptr;
}


void* my_calloc(size_t nmemb, size_t size) {
    //returns NULL if any of the arguments are 0
    if (nmemb == 0 || size == 0) {
        return NULL;
    }

    //makes sure that size * nmemb won't cause overflow
    if (size > SIZE_MAX / nmemb) {
        return NULL;
    }

    //allocates requested memory and sets all bytes to 0 with memset
    size_t requested_bytes = nmemb * size;
    void* result = my_malloc(requested_bytes);
    if (result == NULL) {
        return NULL;
    }
    memset(result, 0, requested_bytes);
    return result;
}


void my_free(void* ptr1) {

    //edge case for calling my_free on a NULL pointer or before my_malloc gets called for the first time
    if (ptr1 == NULL || heap == NULL) {
        return;
    }

    //check if pointer belongs to heap
    if (validate_ptr(ptr1) == 0) {
        return;
    }

    //if heap has been initialized, looks for the corresponding pointer on heap and marks it as unused
    s_elem* ptr2 = heap;
    while (ptr2 != NULL) {
        if (ptr2->ptr_for_the_user == ptr1) {
            ptr2->used = 0;
            coalesce(ptr2);
            return;
        }
        ptr2 = ptr2->next;
    }

    return;
}

//initializes new chunk of memory on the heap
void init_new_chunk(s_elem* chunk, size_t size, void* start_of_block) {
    chunk->ptr_for_the_user = (char*)chunk + sizeof(s_elem);
    chunk->size = size;
    chunk->used = 1;
    chunk->start_of_block = start_of_block;
    chunk->next = NULL;
}

//checks if pointer matches a previously allocated address. match = 1, no match = 0
int validate_ptr(void* ptr1) {
    //looks for pointer on the heap
    s_elem* ptr2 = heap;
    while (ptr2 != NULL) {
        if (ptr2->ptr_for_the_user == ptr1) {
            return 1;
        }
        ptr2 = ptr2->next;
    }

    return 0;
}

//joins current free chunk of memory with previous and next chunk if they are also free
void coalesce(s_elem* current) { 
    //loops through list, gets prev and next chunks
    s_elem* prev = NULL;
    s_elem* next = NULL;

    s_elem* ptr2 = heap;
    while (ptr2 != NULL) {   
        if (ptr2 == current) {
            next = ptr2->next;
            break;
        }      
        prev = ptr2;
        ptr2 = ptr2->next;
    }

    //merges current chunk with next chunk, then prev chunk with current chunk
    merge_chunks(current, next);
    merge_chunks(prev, current);

}

//splits one memory chunk into two chunks (helpful for reusing freed memory for a smaller size)
void split_chunks(s_elem* chunk1, size_t new_size) {

    //saves ptr to next chunk and previous size of chunk that needs to be split
    s_elem* next = chunk1->next;
    size_t prev_size = chunk1->size;

    //sets chunk1 size to the new (smaller) size
    chunk1->size = new_size;

    //sets up new chunk with the remaining size
    chunk1->next = (s_elem*)((char*)chunk1->ptr_for_the_user + chunk1->size);
    s_elem* chunk2 = chunk1->next;
    init_new_chunk(chunk2, prev_size - new_size - sizeof(s_elem), chunk1->start_of_block);
    //marks it as unused
    chunk2->used = 0;
    //chains it to the next chunk that we saved
    chunk2->next = next;
    //runs coalesce in case the next chunk is unused
    coalesce(chunk2);
    
}

//merges two adjacent free chunks of memory
void merge_chunks(s_elem* chunk1, s_elem* chunk2) {
    
    //only valid for non-NULL pointers that point to empty chunks
    if (chunk1 == NULL || chunk2 == NULL || chunk1->used != 0 || chunk2->used != 0 || chunk1->start_of_block != chunk2->start_of_block) {
        return;
    }

    //sizes get merged, taking into account sizeof(s_elem)
    chunk1->size = chunk1->size + sizeof(s_elem) + chunk2->size;

    //if chunk2 is the last chunk in the linked list, chunk2 gets unlinked
    if (chunk2->next == NULL) {
        chunk1->next = NULL;
    //if it is not the last chunk, chunk1 gets linked to the next chunk after chunk2
    } else {
        chunk1->next = chunk2->next;
    }

}