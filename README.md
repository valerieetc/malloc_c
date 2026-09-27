# Welcome to My Malloc
***

## Task
The task was to implement custom memory allocation functions: my_malloc(), my_realloc(), my_calloc() and my_free() that mimic 
the behaviour of malloc(), realloc(), calloc() and free().

## Description
When first called, my_malloc() reserves a much bigger chunk of memory by calling mmap(). This chunk of memory 
gets used for further calls to my_malloc() until it runs out, then mmap() gets called repeatedly. The metadata for 
each allocation is stored in a linked list as a header struct together with the size of the allocation and its 
state (free or used). The pointer returned to the user is calculated from the header's address.
my_realloc() returns the same pointer that was passed to it if the requested size for reallocation is smaller or 
equal to the previously allocated size, and a new pointer if the requested size is bigger.
my_calloc() allocates memory of the requested size, sets all allocated bytes to 0 and returns a new pointer.
my_free() finds the pointer passed to it and marks the chunk of memory as unused. This chunk of memory can be further 
reused by any of the three allocation functions.

Several helper functions ensure that the memory is used efficiently. coalesce() joins adjacent free memory chunks into 
a single memory chunk. split_chunks() splits one chunk into two chunks if the big chunk is reused, marking the 
remaining memory as free to use.

## Installation
As a prerequisite you need to have a GCC compiler and the Make utility.
Run make to create the file my_malloc.a with the library.
Include the header in your main.c file with #include "my_malloc.h" 
To compile the library with your main.c file, run the following command:
```
gcc main.c -L. -lmy_malloc -o program_name
```

## Usage
Include the header and call the functions as you would with standard malloc:
```
char *str = my_malloc(100);
my_free(str);
```