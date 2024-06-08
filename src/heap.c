
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vmem.h"
#include "heap.h"
#include "heap_impl.h"

// Create the heap to start working with memory
heap_t* heapCreate(void)
{
    uint32_t size;

    heap_t* heap = vmemInit(&size);

    vmemCommit(heap, size + sizeof(heap_t));

    heap->pageSize = size;
    heap->firstBlock = NULL;
    heap->lastBlock = NULL;

    return heap;
}

// Destroy the heap and its data
void heapDestroy(heap_t* heap)
{
    vmemShutdown(heap);
}

// Get the next block adress inside the memory
void* nextBlockAdress(block_t* block)
{
    if (block == NULL)
        return NULL;
    return ((unsigned char*)block) + block->blockSize + sizeof(block_t);
}

// Get the next block inside the heap
block_t* nextBlock(heap_t* heap, block_t* block)
{
    if (heap == NULL || block == NULL)
        return NULL;
    if (block >= heap->lastBlock)
        return NULL;

    return (block_t*)nextBlockAdress(block);
}

// Merge two freed block if they are next to each other
void mergeBlock(heap_t* heap)
{
    block_t* block = heap->firstBlock;
    block_t* next = nextBlock(heap, block);
    while (next != NULL)
    {
        if (block->freed && next->freed)
        {
            block->blockSize += sizeof(block_t) + next->blockSize;
            if (heap->lastBlock == next)
                heap->lastBlock = block;
        }

        block = nextBlock(heap, block);
        next = nextBlock(heap, next);
    }
}

// Split the given block into one of another size
block_t* splitBlock(block_t* block, size_t bytes)
{
    block_t* split = (block_t*)(((unsigned char*)block) + bytes + sizeof(block_t));

    split->blockSize = (block->blockSize - bytes - sizeof(block_t));
    split->freed = 1;

    block->blockSize = bytes;
    block->freed = 0;

    return block;
}

// Find a block freed with at least the given size
block_t* findFreeBlock(heap_t* heap, size_t bytes)
{
    if (heap == NULL)
        return NULL;
    
    block_t* block = heap->firstBlock;

    while (block != NULL)
    {
        if (block->freed && block->blockSize == bytes)
        {
            return block;
        }
        else if (block->freed && block->blockSize + sizeof(block_t) > bytes)
        {
            block = splitBlock(block, bytes);
            mergeBlock(heap);
            return block;
        }
        else if (block->freed && block == heap->lastBlock)
        {
            return block;
        }

        block = nextBlock(heap, block);
    }
    return NULL;
}

// Set the alignement of data inside of the memory
size_t getAlignedSize(size_t size, size_t alignment)
{
    size_t modulo = size % alignment;
    if (modulo == 0)
        return size;

    return size - ((size) % alignment) + alignment;
}

// Allocate memory to the last block (call of this function)
// Depends on heapAlloc()
void* heapAllocLastBlock(heap_t* heap, size_t bytes)
{
    block_t* block = heap->lastBlock;
    if (block == NULL)
    {
        block = (block_t*)(heap + 1);
        heap->firstBlock = block;
    }
    else
    {
        block = (block_t*)(((unsigned char*)heap->lastBlock) + 
                heap->lastBlock->blockSize + sizeof(block_t));
    }

    vmemCommit(block, sizeof(block_t) + bytes);

    heap->lastBlock = block;

    return block;
}

// Alloc a block inside of the heap
void* heapAlloc(heap_t* heap, size_t bytes)
{
    bytes = getAlignedSize(bytes, 8);

    printf("size aligned: %I64d\n", bytes);

    block_t* block = findFreeBlock(heap, bytes);

    if (block == NULL)
    {
        block = heapAllocLastBlock(heap, bytes);
    }

    block->blockSize = bytes;
    block->freed = 0;

    return block + 1;
}

// Old way to heap alloc
/*
void* heapAlloc(heap_t* heap, size_t bytes)
{
    block_t* block;

    if (heap->firstBlock == NULL)
    {
        block = (block_t*)(heap + 1);
        
        vmemCommit(block, sizeof(block_t) + bytes);

        heap->firstBlock = block;
        heap->lastBlock = block;
    }
    else
    {
        block = findFreeBlock(heap, bytes);
        if (block == NULL)
        {
            block = (block_t*)(((unsigned char*)heap->lastBlock) + 
                    heap->lastBlock->blockSize + sizeof(block_t));

            vmemCommit(block, sizeof(block_t) + bytes);

            heap->lastBlock = block;
        }
        else
        {
            if (block >= heap->lastBlock)
            {
                heap->lastBlock = block;
            }
            vmemCommit(block, sizeof(block_t) + bytes);
        }
    }
    
    block->freed = 0;
    block->blockSize = bytes;

    return block + 1;
}*/

// Alloc a block and set its data to 0
void* heapCalloc(heap_t* heap, size_t bytes)
{
    bytes = getAlignedSize(bytes, 8);
    block_t* block = heapAlloc(heap, bytes);
    memset(block + 1, 0, bytes);
    return block + 1;
}

// Realloc a block to a new size
void* heapRealloc(heap_t* heap, void* ptr, size_t size)
{
    size = getAlignedSize(size, 8);
    if (ptr == NULL)
        return heapAlloc(heap, size);

    heapFree(heap, ptr);

    if (size == 0)
    {
        return NULL;
    }

    block_t* block = (block_t*)ptr - 1;

    // If the block is reallocated with his current size
    // return the block (useless)
    if (block->blockSize == size)
        return ptr;

    // If the block reallocation is smaller
    // just split it
    if (block->blockSize > size)
        return splitBlock(block, size) + 1;

    // Otherwise, try to find a free block
    // with the needed size, if there is not
    // allocate a new block at the end
    block_t* freeBlock = findFreeBlock(heap, size);

    if (freeBlock == NULL)
        freeBlock = heapAlloc(heap, size);

    memmove(freeBlock, ptr, block->blockSize);

    freeBlock->freed = false;

    return freeBlock + 1;
}

// Free a given block
void heapFree(heap_t* heap, void* ptr)
{
    if (heap == NULL || ptr == NULL)
        return;

    block_t* block = (block_t*)ptr - 1;

    if (block == NULL)
        return;

    block->freed = true;
    
    mergeBlock(heap);
}


//WIP, supposed to decommit freed pages with only data and no block inside
block_t* decommitPages(heap_t* heap, block_t* block)
{
    void* startPtr = heap;
    void* currentPtr = block;

    size_t pageSize = heap->pageSize;
    size_t currentPos = (currentPtr - startPtr) % pageSize;
    size_t distanceUntilNextPage = pageSize - currentPos;
    size_t distanceToBeFreed = (currentPos + distanceUntilNextPage) + block->blockSize;
}

void heapRemoveFreePages(heap_t* heap)
{
    size_t pageSize = heap->pageSize;

    block_t* currentBlock = heap->firstBlock;

    while (currentBlock != NULL)
    {
        if (currentBlock->freed && currentBlock->blockSize >= pageSize)
            decommitPages(heap, currentBlock);
        currentBlock = nextBlock(heap, currentBlock);
    }
}