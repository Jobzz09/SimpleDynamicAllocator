
#include <iostream>
#include <string>

#define minBlockSize 16

struct Header {
    //Start marker of the block
    unsigned char free; // Is block free to allocate?
    std::size_t actualSize; // Size to use without Header and Tail
};
struct Tail{
    //End marker of the block
    unsigned char free;
    std::size_t actualSize;
};

//Function for moving to the next Header
typedef struct Header* (*HeaderIterator)(struct Header*);

struct Header* _startHeader; // The first Header
struct Tail* _endTail; // The end Tail
std::size_t _size; // Size of the entire block
std::size_t minSize; // Minimum block size

struct Header* initBlock(void *buf, std::size_t size) {
    //Initializing new block

    struct Header* ph = (struct Header*)buf;//Getting Header for the block
    ph->free = 1;
    ph->actualSize = size - sizeof(struct Header) - sizeof(struct Tail);


    struct Tail* pt = (struct Tail*)((unsigned char*)buf + size - sizeof(struct Tail));//Getting Tail
    pt->free = 1;
    pt->actualSize = ph->actualSize;

    return ph;
}

struct Tail* getTail(struct Header* phead) {
    //Getting Tail of the block
    unsigned char* base = (unsigned char*)phead;
    base += sizeof(struct Header) + phead->actualSize;
    return (struct Tail*)base;
}

struct Header* getHeader(struct Tail* ptail) {
    //Get Header corresponds to the Tail
    unsigned char* base = (unsigned char*)ptail;
    base -= sizeof(struct Header) + ptail->actualSize;
    return (struct Header*)base;
}

struct Header* getNext(struct Header* phead) {

    struct Tail* ptail = getTail(phead);
    if(ptail == _endTail)
        return NULL;

    unsigned char* base = (unsigned char*)ptail;
    base += sizeof(struct Tail);
    return (struct Header*)base;
}

struct Header* getPrevious(struct Header* phead) {
    if(phead == _startHeader)
        return NULL;

    unsigned char* base = (unsigned char*)phead;
    base -= sizeof(struct Tail);
    return getHeader((struct Tail*)base);
}

std::size_t getAllSize(std::size_t size) {
    return size + sizeof(struct Header) + sizeof(struct Tail);
}

std::size_t getActualSize(struct Header* ph, struct Tail* pt) {
    //Return actual size of the block between Header and Tail
    if((unsigned char*)ph >= (unsigned char*)pt)
        return 0;
    return (unsigned char*)pt - (unsigned char*)ph - sizeof(struct Header);
}

void joinBlocks(struct Header* phStart, struct Header* phEnd) {
    if(phStart == phEnd)
        return;

    if(phStart > phEnd) {
        // phStart has to precede to phEnd
        struct Header* tmp = phStart;
        phStart = phEnd;
        phEnd = tmp;
    }
    struct Tail* ptEnd = getTail(phEnd);

    phStart->actualSize = getActualSize(phStart, ptEnd);
    ptEnd->actualSize = phStart->actualSize;
}

struct Header* utilizeBlock(struct Header* ph, std::size_t size) {
    if (!ph->free || ph->actualSize < size)
        return NULL;

    // size with Header and Tail
    std::size_t allSize = getAllSize(size);

    struct Tail *ptEnd = getTail(ph);

    if (ph->actualSize <= allSize) {
        ph->free = 0;
        ptEnd->free = 0;
        return ph;
    }
    std::size_t newSize = ph->actualSize - allSize;

    if(newSize < minBlockSize) {
        // it does not make sens to divide the block
        ph->free = 0;
        ptEnd->free = 0;
        return ph;
    }

    // split block on two: Start and End.
    // Because of adjusting, we may take anoter size of the blocks.
    // Compute it.
    unsigned char* base = (unsigned char*)ptEnd;
    base -= allSize;

    // Tail of the start block
    struct Tail* ptStart = (struct Tail*)base;
    ptStart->free = 1;
    // find new size of the Start block in memory with adjusting
    ptStart->actualSize = getActualSize(ph, ptStart);
    ph->actualSize = ptStart->actualSize;

    // Create Header of the second block next the Tail of the first block
    struct Header* phEnd = (struct Header*)(base + sizeof(struct Tail));
    phEnd->free = 0;
    phEnd->actualSize = getActualSize(phEnd, ptEnd);

    // The end of the start block became end of the end block
    ptEnd->free = 0;
    ptEnd->actualSize = phEnd->actualSize;

    return phEnd;
}

struct Header* joinNearestFreeBlocks(struct Header* ph, HeaderIterator iterator) {
    if(ph == NULL || !ph->free)
        return ph;

    struct Header* next = iterator(ph);

    while(next != NULL && next->free) {
        joinBlocks(ph, next); // it does not need to ph precede next
        ph = next;
        next = iterator(ph);
    }
    return ph;
}

void mysetup(void *buf, std::size_t size) {
    _size = size;
    _startHeader = initBlock(buf, size);
    _endTail = getTail(_startHeader);
    // minimum size of the block
    minSize = getAllSize(minBlockSize);
}

void *myalloc(std::size_t size) {
    // look up for free block with apropriate size
    std::size_t allSize = getAllSize(size);
    struct Header* ph = _startHeader;
    while(ph != NULL) {
        if(ph->free && (ph->actualSize >= allSize || ph->actualSize >= size)) {
            // we find free block with needed size
            unsigned char *base = (unsigned char*)utilizeBlock(ph, size);
            base += sizeof(struct Header);
            return (void*)base;
        }
        ph = getNext(ph);
    }
    // there is not any free block with needed size
    return NULL;
}

void myfree(void *p) {
    if(p == NULL)
        return;

    struct Header* ph = (struct Header*)((unsigned char*)p - sizeof(struct Header));
    ph->free = 1;

    struct Tail* pt = getTail(ph);
    pt->free = 1;

    // join blocks prevous to the current
    ph = joinNearestFreeBlocks(ph, getPrevious);
    // join blocks next to the current
    joinNearestFreeBlocks(ph, getNext);
}

int main(){
    void* test = malloc(100);
    
        mysetup(test,100);
    
    void* alloctest = myalloc(10);
    myfree(alloctest);
    free(test);
    return 0;
}




