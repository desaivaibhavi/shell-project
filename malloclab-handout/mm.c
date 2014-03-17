/* In this program, the explicit free list has been implemented. 
During the memory allocation, first free approach has been used.
Along with that, I have also implemented coalesceing to improve the
efficiency of the program.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"


/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* Student Name */
    "HIMANI KAPOOR",
    /* Email Id */
    "201101197@daiict.ac.in",
   " "," " 
};

/* Extra bytes used by header and footer */
#define OVERHEAD 8


#define WSIZE 4       // word size
#define DSIZE 8      // header size and footer size
#define XSIZE 16    // double word size


/* Initial size of heap (for expanding) */
#define CHUNKSIZE (1<<12) 


/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))


/* Read and write a word at address p */ 
#define GET(p) (*(size_t *)(p)) 
#define PUT(p, val)(*(size_t *)(p) = (val))


#define GET_SIZE(p) (GET(p) & ~0x7)                       // get the size of the block from header
#define HDRP(bp) ((char*)(bp)-DSIZE)                     //  get header of current block
#define FTRP(bp) ((char*)(bp)+GET_SIZE(HDRP(bp))-XSIZE) //   get footer of current block


/* Given block ptr, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - DSIZE)))      // get pointer to payload of next block
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - XSIZE)))     //  get pointer to payload of previous block


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8


/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))  

#define BLK_HDR_SIZE ALIGN(sizeof(blockHdr))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)       // get size of the current block
#define GET_ALLOC(p) (GET(p) & 0x1)      //  get status of current block
 


typedef struct block blockHdr;

// struct for a block of the heap
struct block{
	size_t size;
	blockHdr *next;     // pointer to next free block in the free list
	blockHdr *prev;    //  pointer to previous free block in the free list
};


/*declaring functions with their prototypes*/
void *first_fit(size_t size); 
void print_the_heap();
static void *coalescing(void *);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) 
{
    	blockHdr *ptr = mem_sbrk(BLK_HDR_SIZE);          // requesting space of block-header size
	
	// initialising the block : defining the prologue
        ptr->size = BLK_HDR_SIZE;                       
	ptr->next = ptr;
	ptr->prev = ptr;
	return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(BLK_HDR_SIZE + size); 	//making the size aligned to 8.
    blockHdr *bp = first_fit(newsize); 		// traversing the heap to get the first freeblock using first fit
    if(bp==NULL)  				// required free space not available
    {
	bp = mem_sbrk(newsize); 
    
    if ((long)bp == -1) 			//  requested space is not available in the memory
	return NULL;
    else                          		// requested space present in the memory
	bp->size = newsize | 1;  		// setting the status : allocated 
    }
    else{                       		// implementing the functionality of 'extend_heap'
		bp->size |= 1;
		bp->prev->next = bp->next;
		bp->next->prev = bp->prev;
	}
	return (char *)bp + BLK_HDR_SIZE; 	// returning the payload
	
}
void print_the_heap()          			// printing the information related to blocks in the heap 
{                 
	blockHdr *bp = mem_heap_lo();
	while(bp<(blockHdr *)mem_heap_hi()){
		printf("%s block at:  %p,size: %d\n",(bp->size&1)?"allocated":"free",bp ,(int)(bp->size & ~1));
		bp = (blockHdr *)((char *)bp +(bp->size & ~1));

	}
}
void *first_fit(size_t size) 			// finding the first free block available
{
	blockHdr *p;
	for(p = ((blockHdr *)mem_heap_lo())->next;
		p != mem_heap_lo() && p->size < size;
		p = p->next); 			// traversing in the list till we get the first free block 
	if(p!=mem_heap_lo())  			// block available
		return p;
	else 
		return NULL; 			// block not available
}
	
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)   // to free an allocated block 
{
	blockHdr *bp = ptr-BLK_HDR_SIZE, 
                 *head = mem_heap_lo();     

	/*  status changed  from allocated to free  */
	
	bp->size &= ~1;
	bp->next = head->next;
	bp->prev = head;
	head->next = bp;
	bp->next->prev = bp;

}



/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)  	//reallocating an allocated block 
{
    void *oldptr = ptr;
    void *newptr;
    size_t copy;
    
    newptr = mm_malloc(size);  			// allocate the old block with modified size
    if (newptr == NULL) 			// enough space not available in the memory
      return NULL;
    copy = *(size_t *)((char *)oldptr - SIZE_T_SIZE);  	// aligning the modified size
    if (size < copy) 					//requested size is lesser than the size of previously allocated block
      copy = size;   
    memcpy(newptr, oldptr, copy); 		// creating a copy of old block with new size
    mm_free(oldptr); 				// to free the old block 
    return newptr;

	return NULL;
}

static void *coalescing(void *bp)   // combining neighbouring free blocks
	{ 
	size_t prev_allocated = GET_ALLOC(FTRP(PREV_BLKP(bp)));  // status of next block
	size_t next_allocated = GET_ALLOC(HDRP(NEXT_BLKP(bp)));  // status of previous free block                               
	size_t size = GET_SIZE(HDRP(bp));                    // size of the header                            

	if (prev_allocated && next_allocated) { return bp;  }         // both neighboring blocks are allocated
	else if (prev_allocated && !next_allocated) {                 // previous block is allocated & next block is free
	  	size += GET_SIZE(HDRP(NEXT_BLKP(bp)));        
                PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
                PUT(HDRP(bp), PACK(size, 0));
                

	}
	else if (!prev_allocated && next_allocated) {                  // previous block is free & next block is allocated
		size += GET_SIZE(HDRP(PREV_BLKP(bp))); 
		PUT(FTRP(bp), PACK(size, 0)); 
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
	 	bp = PREV_BLKP(bp); 
   	}	
	else if (!prev_allocated && !next_allocated) {                 // both neighboring blocks are free
		size += GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(HDRP(NEXT_BLKP(bp)));
                PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
                PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
                bp = PREV_BLKP(bp);

	}
	return bp;
}







