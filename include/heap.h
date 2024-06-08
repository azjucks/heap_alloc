#pragma once

#include <stddef.h>

typedef struct heap_t heap_t;

// Create the heap allocator
heap_t* heapCreate(void);
// Destroy the heap
void heapDestroy(heap_t* heap);

//print data relative to the heap
void heapPrintDebug(heap_t* heap);

// Alloc memory but sets all elements to \0
void* heapCalloc(heap_t* heap, size_t bytes);

// Realloc memory with different size
void* heapRealloc(heap_t* heap, void* ptr, size_t size);

// Alloc memory
void* heapAlloc(heap_t* heap, size_t bytes);

// Free memory
void heapFree(heap_t* heap, void* ptr);