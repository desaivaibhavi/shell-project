/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
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
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

#define WSIZE 4    
#define PACK(size, alloc)    ((size) | (alloc))
#define CHUNKSIZE (1<<12) 
#define OVERHEAD 8
#define DSIZE 8
/* Read and write a word at address p */ 
#define GET(p)                   (*(size_t *)(p)) 
//#define PUT(p, val)             (*(size_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)        (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1) 
/* Given block ptr bp, compute address of its header and footer */

#define HDRP(bp)  ((char *)(bp) - WSIZE) 

#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) -WSIZE))) 

#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) 
#define MAX(a,b) (a>b)?a:b
static void place(void *, size_t);
static void *coalesce(void *);
static void *extend_heap(size_t);
static void *find_fit(size_t);
static char* heap_listp;

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define PUT(p, val) (*(size_t *)(p) = (val))

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*struct block{
	struct block *prev;
	struct block *next;
}*p1,*p2;*/


char *next,*prev;
//head = (char *)mem_heap_lo();

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == NULL)	
		return -1; 

	head = (char *)mem_heap_lo();
	bp = head + 8;
	head->next = bp+8;
	//foot = (char *)mem_heap_hi();
	/*PUT(head, 0);  
	PUT(head + WSIZE, PACK(OVERHEAD, 1)); 
   	PUT(head + DSIZE, PACK(OVERHEAD, 1)); 
   	PUT(head + WSIZE + DSIZE, PACK(0, 1)); 
	head += DSIZE; 
	*/

	
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL) 
	return -1; 
    return 0;
}



static void *extend_heap(size_t words) { 
	char *bp; 
	size_t size;
 	
	size = (words % 2) ? (words+1)*WSIZE :
	                      words*WSIZE; 
 	if ((int)(bp = mem_sbrk(size)) < 0) 
       	return NULL; 
	
   	/*head = (char *)mem_heap_lo();
	PUT(HDRP(bp), PACK(size, 0)); 
	PUT(FTRP(bp), PACK(size, 0)); 
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); */

	return coalesce(bp);
 } 




/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
  /* int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
	} */

        size_t asize, extendsize; 	 /* adjusted block size */
	char *bp;                  /* amount to extend heap if no fit */

	if (size <= 0) return NULL;   
	if (size <= DSIZE)           
		 asize = DSIZE+OVERHEAD; 
	else 
		 asize = DSIZE*((size+(OVERHEAD)+(DSIZE-1))/DSIZE); 

	if ((bp = find_fit(asize)) != NULL) { 
		place(bp, asize); 
		return bp; 
	}
	extendsize = MAX(asize,CHUNKSIZE); 
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL) 	
		return NULL; 
	place(bp, asize); 
	return bp; 


}
static void *find_fit(size_t asize) { 
	void *bp;
	 
	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; 
		  bp = NEXT_BLKP(bp)) 
		if (!GET_ALLOC(HDRP(bp)) 
       && (asize <= GET_SIZE(HDRP(bp))))
			return bp; 
	
	return NULL;    /* no fit */
} 

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
size_t size = GET_SIZE(HDRP(ptr)); 
	
	PUT(HDRP(ptr), PACK(size, 0)); 
   PUT(FTRP(ptr), PACK(size, 0)); 
	
	coalesce(ptr); 
}

static void place(void *bp, size_t asize) {
	 size_t csize = GET_SIZE(HDRP(bp));
	
    /* if new free block would be at least as big as min block 
       size, split */ 
	 if ((csize - asize) >= (DSIZE + OVERHEAD)) {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1)); 
		bp = NEXT_BLKP(bp); 
		PUT(HDRP(bp), PACK(csize-asize, 0)); 	
		PUT(FTRP(bp), PACK(csize-asize, 0)); 
	} 
    /* do not split */
	else { 
		PUT(HDRP(bp), PACK(csize, 1)); 
		PUT(FTRP(bp), PACK(csize, 1)); 
	} 
}

static void *coalesce(void *bp) { 
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); 
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); 
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc) { return bp;  }         
	else if (prev_alloc && !next_alloc) { 
	  	size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
                PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
                PUT(HDRP(bp), PACK(size, 0));
                

	}
	else if (!prev_alloc && next_alloc) {
		size += GET_SIZE(HDRP(PREV_BLKP(bp))); 
		PUT(FTRP(bp), PACK(size, 0)); 
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
	 	bp = PREV_BLKP(bp); 
   	}	
	else if (!prev_alloc && !next_alloc) { 
		size += GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(HDRP(NEXT_BLKP(bp)));
                PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
                PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
                bp = PREV_BLKP(bp);

	}
	return bp;
}



/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














