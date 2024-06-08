#include "heap.h"
#include "heap_impl.h"
#include <stdio.h>


// Print data into a block
void blockPrintDataDebug(void* ptr)
{
    block_t* block = (block_t*)ptr - 1;
    char* blockData = (char*)ptr;

    printf("Data content:\n");
    for (int i = 0; i < block->blockSize; ++i)
    {
        printf("%x", blockData[i] & 0xff);
    }
    printf("\n");
}

// Print each block informations inside the heap
void heapPrintDebug(heap_t* heap)
{
    printf("Print Debug:\n/===========================================================\\\n");
    printf("Heap: (pageSize=%I64d, firstBlock=0x%p, lastBlock=0x%p)\n",
            heap->pageSize, heap->firstBlock, heap->lastBlock);

    block_t* currentBlock = heap->firstBlock;
    while (currentBlock != NULL)
    {
        printf(" - Block: 0x%p : data(0x%p, size=%I64d, freed=%d)\n",
        currentBlock, (void*)(currentBlock + 1),
        currentBlock->blockSize, currentBlock->freed);
        blockPrintDataDebug(currentBlock + 1);

        currentBlock = nextBlock(heap, currentBlock);
    }
    printf("\\===========================================================/\n\n");
}
