#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct block_t
{
    size_t blockSize;
    bool freed;
} block_t;

typedef struct heap_t
{
    size_t pageSize;
    block_t* firstBlock;
    block_t* lastBlock;
} heap_t;

//Align blocks of data by a certain number multiplier
size_t getAlignedSize(size_t size, size_t alignment);

//Debug
void blockPrintDataDebug(void* ptr);

void heapPrintDebug(heap_t* heap);

