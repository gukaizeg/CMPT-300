#include "myalloc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#define MULTI_THREAD

#ifdef MULTI_THREAD
#include <pthread.h>
#endif

////////////////////////////////////////////////////////////
//---------------    ADT of Linked List    ---------------//
////////////////////////////////////////////////////////////

struct Node
{
    void* data;
    struct Node* next;
};

// create a new node
static struct Node* createNode(void* data, struct Node* next)
{
    struct Node* node = (struct Node*)malloc(sizeof(struct Node));
    assert(node);
    node->data = data;
    node->next = next;
    return node;
}

// destroy the list
static void freeList(struct Node* head)
{
    if (head)
    {
        freeList(head->next);
        free(head);
    }
}

// does data contained in list?
static int containsNode(struct Node* head, void* data)
{
    while (head->next)
    {
        if (head->next->data == data)
        {
            return 1;
        }
        head = head->next;
    }
    return 0;
}

// insert node into linked list
static void addNode(struct Node* head, void* data)
{
    struct Node* node = createNode(data, NULL);
    assert(!containsNode(head, data));
    while (head->next && head->next->data < data)
    {
        head = head->next;
    }
    node->next = head->next;
    head->next = node;
}

// remove node from linked list
static void removeNode(struct Node* head, void* data)
{
    assert(containsNode(head, data));
    while (head->next->data != data)
    {
        head = head->next;
    }
    struct Node* tmp;
    tmp = head->next;
    head->next = head->next->next;
    free(tmp);
}

////////////////////////////////////////////////////////////
//-----------------    Utils Function    -----------------//
////////////////////////////////////////////////////////////
void setHeadSize(void* data, int size)
{
    *((int64_t*)data) = size;
}

int getHeadSize(void* data)
{
    int64_t i64 = *((int64_t*)data);
    return (int)i64;
}


struct Myalloc
{
    enum allocation_algorithm aalgorithm;
    int size;
    void* memory;
    // Some other data members you want,
    // such as lists to record allocated/free memory
    struct Node* free_head;
    struct Node* allocate_head;
#ifdef MULTI_THREAD
    pthread_mutex_t mutex;
#endif
};

struct Myalloc myalloc;

void initialize_allocator(int _size, enum allocation_algorithm _aalgorithm)
{
    assert(_size > 0);
    myalloc.aalgorithm = _aalgorithm;
    myalloc.size = _size;
    myalloc.memory = malloc((size_t)myalloc.size);

    // Add some other initialization
    assert(myalloc.memory);
    memset(myalloc.memory, 0xcc, myalloc.size);

    myalloc.free_head = createNode(NULL, NULL);
    myalloc.allocate_head = createNode(NULL, NULL);

    setHeadSize(myalloc.memory, myalloc.size);

    struct Node* node = createNode(myalloc.memory, NULL);
    myalloc.free_head->next = node;

#ifdef MULTI_THREAD
    pthread_mutex_init(&myalloc.mutex, NULL);
#endif
}

void destroy_allocator()
{
    free(myalloc.memory);

    // free other dynamic allocated memory to avoid memory leak
    freeList(myalloc.allocate_head);
    freeList(myalloc.free_head);
#ifdef MULTI_THREAD
    pthread_mutex_destroy(&myalloc.mutex);
#endif
}

void* allocate(int _size)
{
#ifdef MULTI_THREAD
    pthread_mutex_lock(&myalloc.mutex);
#endif
    void* ptr = NULL;

    // Allocate memory from myalloc.memory
    // ptr = address of allocated memory

    // (1) select a free chunk from free list
    int freeSize;
    struct Node* node = myalloc.free_head->next;
    if (myalloc.aalgorithm == FIRST_FIT)
    {
        // FIRST_FIT
        while (node)
        {
            freeSize = getHeadSize(node->data);
            if (freeSize - 8 >= _size)
            {
                break;
            }
            node = node->next;
        }
    }
    else if (myalloc.aalgorithm == WORST_FIT)
    {
        // WORST_FIT
        int largestSize = 0;
        struct Node* largestNode = NULL;
        while (node)
        {
            freeSize = getHeadSize(node->data);
            if (largestSize < freeSize)
            {
                largestSize = freeSize;
                largestNode = node;
            }
            node = node->next;
        }
        if (largestSize - 8 < _size)
        {
            node = NULL;
        }
        else
        {
            node = largestNode;
        }
    }
    else
    {
        // BEST_FIT
        assert(myalloc.aalgorithm == BEST_FIT);

        int smallestSize = 0;
        struct Node* smallestNode = NULL;
        while (node)
        {
            freeSize = getHeadSize(node->data);
            if (freeSize - 8 >= _size)
            {
                if (smallestNode == NULL ||
                    smallestSize > freeSize - 8 - _size)
                {
                    smallestSize = freeSize - 8 - _size;
                    smallestNode = node;
                }

            }
            node = node->next;
        }
        node = smallestNode;
    }

    // cannot find free chunk, return NULL
    if (node == NULL)
    {
        // no enough memory
#ifdef MULTI_THREAD  
        pthread_mutex_unlock(&myalloc.mutex);
#endif
        return NULL;
    }

    void* beginning = node->data;
    freeSize = getHeadSize(beginning);

    assert(freeSize - 8 >= _size);

    if (freeSize - 8 - _size >= 8)
    {
        // split the chunk
        setHeadSize(beginning, _size + 8);
        addNode(myalloc.allocate_head, beginning);
        removeNode(myalloc.free_head, beginning);

        // beginning of other node
        void* new_beginning = (char*)beginning + 8 + _size;
        setHeadSize(new_beginning, freeSize - 8 - _size);
        addNode(myalloc.free_head, new_beginning);

        ptr = (char*)beginning + 8;
    }
    else
    {
        // do not split the chunk
        addNode(myalloc.allocate_head, beginning);
        removeNode(myalloc.free_head, beginning);

        ptr = (char*)beginning + 8;
    }
#ifdef MULTI_THREAD
    pthread_mutex_unlock(&myalloc.mutex);
#endif
    return ptr;
}

// merge the neighborhood chunk
static int compaction()
{
    struct Node* node = myalloc.free_head->next;
    while (node && node->next)
    {
        struct Node* next = node->next;
        assert((char*)(node->data) + getHeadSize(node->data) <= (char*)next->data);
        if ((char*)(node->data) + getHeadSize(node->data) == next->data)
        {
            setHeadSize(node->data, getHeadSize(node->data) + getHeadSize(next->data));
            memset((char*)(node->data) + 8, 0x11, getHeadSize(node->data) - 8);
            removeNode(myalloc.free_head, next->data);
            return 1;
        }
        node = node->next;
    }
    return 0;
}

int compact_allocation(void** _before, void** _after)
{
#ifdef MULTI_THREAD
    pthread_mutex_lock(&myalloc.mutex);
#endif
    int compacted_size = 0;

    // compact allocated memory
    // update _before, _after and compacted_size
    int idx = 0;

    struct Node* node;
    struct Node* oldHead = myalloc.allocate_head->next;
    myalloc.allocate_head->next = NULL;

    int startAddr = 0;
    for (node = oldHead; node; node = node->next)
    {
        _before[idx] = (char*)node->data + 8;
        int size = getHeadSize(node->data);
        memcpy((char*)myalloc.memory + startAddr, node->data, size);
        _after[idx] = (char*)myalloc.memory + startAddr + 8;

        addNode(myalloc.allocate_head, (char*)myalloc.memory + startAddr);

        startAddr += size;
        idx++;
    }
    freeList(oldHead);

    freeList(myalloc.free_head->next);
    myalloc.free_head->next = NULL;

    setHeadSize((char*)myalloc.memory + startAddr, myalloc.size - startAddr);
    addNode(myalloc.free_head, (char*)myalloc.memory + startAddr);

    compacted_size = startAddr;
#ifdef MULTI_THREAD
    pthread_mutex_unlock(&myalloc.mutex);
#endif
    return compacted_size;
}
void deallocate(void* _ptr)
{
#ifdef MULTI_THREAD
    pthread_mutex_lock(&myalloc.mutex);
#endif
    assert(_ptr != NULL);

    // Free allocated memory
    // Note: _ptr points to the user-visible memory. The size information is
    // stored at (char*)_ptr - 8.
    void* beginning = (char*)_ptr - 8;
    assert(containsNode(myalloc.allocate_head, beginning));

    removeNode(myalloc.allocate_head, beginning);
    addNode(myalloc.free_head, beginning);
    while (compaction());
#ifdef MULTI_THREAD
    pthread_mutex_unlock(&myalloc.mutex);
#endif
}



int available_memory()
{
    struct Stats stat;
    get_statistics(&stat);
    return stat.free_size;
}

void get_statistics(struct Stats* _stat)
{
#ifdef MULTI_THREAD
    pthread_mutex_lock(&myalloc.mutex);
#endif
    struct Node* node;
    int size;
    memset(_stat, 0, sizeof(struct Stats));
    _stat->smallest_free_chunk_size = INT_MAX;
    for (node = myalloc.free_head->next; node; node = node->next)
    {
        _stat->free_chunks++;
        size = getHeadSize(node->data);
        _stat->free_size += size - 8;
        _stat->smallest_free_chunk_size = _stat->smallest_free_chunk_size < size ? _stat->smallest_free_chunk_size : size - 8;
        _stat->largest_free_chunk_size = _stat->largest_free_chunk_size > size ? _stat->largest_free_chunk_size : size - 8;
    }

    for (node = myalloc.allocate_head->next; node; node = node->next)
    {
        _stat->allocated_chunks++;
        size = getHeadSize(node->data);
        _stat->allocated_size += size - 8;
    }
#ifdef MULTI_THREAD
    pthread_mutex_unlock(&myalloc.mutex);
#endif
}

void printStat(struct Stats* _stat)
{
    printf("=========================================\n");
    printf("allocated_size  = %d\n", _stat->allocated_size);
    printf("allocated_chunks= %d\n", _stat->allocated_chunks);
    printf("free_size       = %d\n", _stat->free_size);
    printf("free_chunks     = %d\n", _stat->free_chunks);
    printf("smallest_free   = %d\n", _stat->smallest_free_chunk_size);
    printf("largest_free    = %d\n", _stat->largest_free_chunk_size);
    printf("=========================================\n");
}