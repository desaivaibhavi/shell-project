/*In the following code, I have implemented an explicit free list allocator using
the functions like mm_init, mm_malloc, mm_free, realloc, print_heap, coalesce, first_fit.
I have used a struct called blk which contains pointers to next, previous blocks and size. 
I have also defined some macros for simplifying the code.
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
    "Symphony",
    /* First member's full name */
    "Vaibhavi Desai",
    /* First member's email address */
    "201101209@daiict.ac.in",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* Initial size of heap (for expanding) */
#define CHUNKSIZE (1<<12) 

/* Extra bytes used by header and footer */
#define OVERHEAD 8

#define WSIZE 4 
#define DSIZE 8 // header size and footer size
#define XSIZE 16

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)    ((size) | (alloc))

/* Read and write a word at address p */ 
#define GET(p)                   (*(size_t *)(p)) 
#define PUT(p, val)             (*(size_t *)(p) = (val))


#define GET_SIZE(p) (GET(p) & ~0x7)
#define HDRP(bp) ((char*)(bp)-DSIZE)
#define FTRP(bp) ((char*)(bp)+GET_SIZE(HDRP(bp))-XSIZE)

/* Given block ptr, compute address of next and previous blocks */

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - DSIZE))) 

#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - XSIZE))) 


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define BLK_HDR_SIZE ALIGN(sizeof(blockHdr))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1) 
 


typedef struct blk blockHdr;

// defining a global struct
struct blk{
	size_t size;
	blockHdr *next; 					// pointer to next free block
	blockHdr *prior; 					// pointer to previous free block
};


/*declaration of functions used in the code*/
void *first_fit(size_t size); 
void print_heap();
static void *coalesce(void *);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) 
{
    	blockHdr *pt = mem_sbrk(BLK_HDR_SIZE); 			// requesting heap space of blockheader size
	pt->size = BLK_HDR_SIZE; 				/* setting the parameters*/
	pt->next = pt;
	pt->prior = pt;
	return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int new_size = ALIGN(BLK_HDR_SIZE + size); 			//aligning the size
    blockHdr *bp = first_fit(new_size); 				// checking for a freeblock using first fit
    if(bp==NULL)  						// when sufficient free space is not available
    {
	bp = mem_sbrk(new_size); 
    
    if ((long)bp == -1) 					// when requested space is not in the memory
	return NULL;
    else                          				// memory has the requested space
	bp->size = new_size | 1;  				// modifying the size of bp
    }
    else{                       				// extending the heap
		bp->size |= 1;
		bp->prior->next = bp->next;
		bp->next->prior = bp->prior;
	}
	return (char *)bp + BLK_HDR_SIZE; 			// pointer to the payload
	
}
void print_heap()          					// prints the status,address and size of the blocks 
{                 
	blockHdr *bp = mem_heap_lo();
	while(bp<(blockHdr *)mem_heap_hi()){
		printf("%s Block at  %p,size %d\n",(bp->size&1)?"allocated":"free",bp ,(int)(bp->size & ~1));
		bp = (blockHdr *)((char *)bp +(bp->size & ~1));

	}
}
void *first_fit(size_t size) 					// finding the first free block available
{
	blockHdr *pt;
	for(pt = ((blockHdr *)mem_heap_lo())->next;
		pt != mem_heap_lo() && pt->size < size;
		pt = pt->next); 				// starting from block after the prologue,traversing till we get the first free block 
	if(pt!=mem_heap_lo())  					// when the block is available
		return pt;
	else 
		return NULL; 					// block not available
}
	
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)   					// freeing an allocated block 
{
	blockHdr *bp = ptr-BLK_HDR_SIZE, 
                 *head = mem_heap_lo();     

	/*  changing the status from allocated to free  */
	
	bp->size &= ~1;
	bp->next = head->next;
	bp->prior = head;
	head->next = bp;
	bp->next->prior = bp;
}



/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)  			// to reallocate an already allocated block 
{
    void *oldptr = ptr;
    void *newptr;
    size_t cpSize;
    
    newptr = mm_malloc(size);  					// allocate the same block with new size
    if (newptr == NULL) 					// enough space is not available in the memory
      return NULL;
    cpSize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);  	// aligning the new size
    if (size < cpSize) 					//requested size is lesser than the size of previously allocated block
      cpSize = size;   
    memcpy(newptr, oldptr, cpSize); 				// creating a copy of old block with new size
    mm_free(oldptr); 						// freeing the space of the old block 
    return newptr;

	return NULL;
}

static void *coalesce(void *bp)   				// to merge neighboring free blocks
	{ 
	size_t prev_alloc_st = GET_ALLOC(FTRP(PREV_BLKP(bp)));  	// getting the status of next block
	size_t next_alloc_st = GET_ALLOC(HDRP(NEXT_BLKP(bp)));  	// getting the status of previous free block                               
	size_t size_st = GET_SIZE(HDRP(bp));                    	// getting the size of the header                            

	if (prev_alloc_st && next_alloc_st) { return bp;  }         	// both neighboring blocks are allocated
	else if (prev_alloc_st && !next_alloc_st) {                 	// previous block allocated & next free
	  	size_st += GET_SIZE(HDRP(NEXT_BLKP(bp)));        
                PUT(FTRP(NEXT_BLKP(bp)), PACK(size_st, 0));
                PUT(HDRP(bp), PACK(size_st, 0));
                

	}
	else if (!prev_alloc_st && next_alloc_st) {                  // previous block free & next allocated
		size_st += GET_SIZE(HDRP(PREV_BLKP(bp))); 
		PUT(FTRP(bp), PACK(size_st, 0)); 
		PUT(HDRP(PREV_BLKP(bp)), PACK(size_st, 0));
	 	bp = PREV_BLKP(bp); 
   	}	
	else if (!prev_alloc_st && !next_alloc_st) {                 // both neighboring blocks are free
		size_st += GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(HDRP(NEXT_BLKP(bp)));
                PUT(FTRP(NEXT_BLKP(bp)), PACK(size_st, 0));
                PUT(HDRP(PREV_BLKP(bp)), PACK(size_st, 0));
                bp = PREV_BLKP(bp);

	}
	return bp;
}







