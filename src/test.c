
#include <stdio.h>
#include <stdint.h>
#include <windows.h>

#include "perf.h"
#include "vmem.h"

#include <heap.h>

void testVmem(void)
{
    uint32_t pageSize;
    void* baseAddress = vmemInit(&pageSize);
    printf("baseAddress: 0x%p (pageSize=%dKB)\n", baseAddress, pageSize / 1024);

    // Allocate an array of int of 32 elements
    int intArraySize = 32;
    int* intArray = baseAddress;
    //intArray[0] = 1; // Will segfault since mem has not been commit

    vmemCommit(baseAddress, intArraySize * sizeof(int));
    intArray[0] = 1;
    intArray[intArraySize+1] = 1; // Won't segfault since the committed size is at least equals to pageSize (4096 octets)

    vmemDecommit(baseAddress, intArraySize * sizeof(int)); // The entire page will be decommitted (even if size < pageSize)
    //intArray[0] = 1; // Will segfault since mem has been decommitted

    vmemShutdown(baseAddress);
}

void testVmemPerfs(void)
{
    uint32_t pageSize;
    void* baseAddress = vmemInit(&pageSize);
    printf("baseAddress: 0x%p (pageSize=%dKB)\n", baseAddress, pageSize / 1024);

    vmemCommit(baseAddress, 32);

    char* test = baseAddress;

    {
        uint64_t start = perfQuery();
        test[0] = 0;
        printf("[0] = %s (first write after commit)\n", perfGetDurationStr(perfQuery() - start));
    }

    {
        uint64_t start = perfQuery();
        test[1] = 1;
        printf("[1] = %s (second write)\n", perfGetDurationStr(perfQuery() - start));
    }
    vmemDecommit(baseAddress, 0);
    vmemCommit(baseAddress, 32);

    {
        uint64_t start = perfQuery();
        test[2] = 2;
        printf("[2] = %s (first write after commit)\n", perfGetDurationStr(perfQuery() - start));
    }

    {
        uint64_t start = perfQuery();
        test[3] = 3;
        printf("[3] = %s (second write)\n", perfGetDurationStr(perfQuery() - start));
    }

    {
        uint64_t start = perfQuery();
        for (int i = 0; i < pageSize; ++i)
        {
            test[i] = i;
        }
        printf("[4] = %s / iterations, for %d write\n", perfGetDurationItStr((perfQuery() - start), pageSize), pageSize);
    }

    vmemShutdown(baseAddress);
}

void testHeap()
{
    heap_t* heap = heapCreate();

    int size = 32;
    size_t byteSize = size * sizeof(int);
    int* array1 = heapAlloc(heap, byteSize);
    memset(array1, 0, byteSize);
    printf("array1 = 0x%p\n", array1);

    int* array2 = heapAlloc(heap, byteSize);
    memset(array2, 0, byteSize);
    printf("array2 = 0x%p\n", array2);

    int* array3 = heapAlloc(heap, byteSize);
    memset(array3, 0, byteSize);
    printf("array3 = 0x%p\n", array3);
    for (int i = 0; i < size; ++i)
    {
        array1[i] = i;
    }
    heapPrintDebug(heap);
    heapFree(heap, array3);

    heapPrintDebug(heap);

    byteSize *= 2;
    int* array4 = heapAlloc(heap, byteSize);
    memset(array4, 0, byteSize);
    printf("array4 = 0x%p\n", array4);

    heapPrintDebug(heap);

    array1 = heapRealloc(heap, array1, byteSize/4);

    heapPrintDebug(heap);

    heapDestroy(heap);
}

int main(int argc, char* argv[])
{
    perfInit();

    //testVmem();

    //testVmemPerfs();

    testHeap();

    return 0;
}
