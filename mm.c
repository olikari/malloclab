 /* The list has the following form:
 *
 * begin                                                          end
 * heap                                                           heap  
 *  -----------------------------------------------------------------   
 * |  pad   | hdr(16:a) | ftr(16:a) | zero or more usr blks | hdr(8:a) |
 *  -----------------------------------------------------------------
 *          |       prologue      |                       | epilogue |
 *          |         block       |                       | block    |
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "mm.h"
#include "memlib.h"


/* Team structure */
team_t team = {
    "explisit free list", 
    "Olafur Kari Sigurbjornsson", "olafurks10",
    "Ásgeir Atlason", "asgeira13",
    "Pétur Bergmann Halldórsson", "peturh13"
}; 

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* word size (bytes) */  
#define DSIZE       8       /* doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* initial heap size (bytes) */
#define OVERHEAD    16       /* overhead of header and footer (bytes) */
#define ALIGNMENT	8

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)         (*(size_t *)(p))
#define PUT(p, val)    (*(size_t *)(p) = (val))  

/* (which is about 49/100).* Read the size and allocated fields from address p */
#define GET_SIZE(p)    (GET(p) & ~0x7)
#define GET_ALLOC(p)   (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)  
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr pb, compute address of predicessor and successor */
#define PRED(bp)		((char *)(bp))
#define SUCC(bp)		((char *)(bp) + WSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define ALIGN(size)		(((size) + (ALIGNMENT-1)) & ~0x7)
/* $end mallocmacros */

/* Global variables */
static char *heap_listp;  /* pointer to first block */  
static int freeblockcount;
//static int countblocks;

/* function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void *place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp); 
static void checkblock(void *bp);
static void insert_block(void *bp);
static void remove_block(void *bp);
static int block_in_free_list(void *bp);

/* 
 * mm_init - Initialize the memory manager 
 */
/* $begin mminit */
int mm_init(void) 
{
    /* create the initial empty heap */
    if ((heap_listp = mem_sbrk(6*WSIZE)) == NULL) {
        return -1;
    }
    PUT(heap_listp, 0);                       		 /* alignment padding */
    PUT(heap_listp+(1*WSIZE), PACK(OVERHEAD, 1));  	/* prologue header */
	PUT(heap_listp+(2*WSIZE), 0);
	PUT(heap_listp+(3*WSIZE), 0);
    PUT(heap_listp+(4*WSIZE), PACK(OVERHEAD, 1));  	/* prologue footer */ 
    PUT(heap_listp+(5*WSIZE), PACK(0, 1));   		/* epilogue header */
    heap_listp += (2*WSIZE);
	freeblockcount = 0;

	//mm_checkheap(1); 
	
	char* freeblock;
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if ((freeblock = extend_heap((CHUNKSIZE/WSIZE + 88))) == NULL) {
        return -1;
    }
	insert_block(freeblock);
	
	//mm_checkheap(1);

	/*printf("Remove-a block\n");
	remove_block(freeblock);
	mm_checkheap(1);*/

	/*char* anotherfree;
	anotherfree = extend_heap(CHUNKSIZE/WSIZE);
	printf("adda annari blokkinni\n");
	insert_block(anotherfree);*/

	/*char* temp;
	temp = extend_heap(CHUNKSIZE/WSIZE);
	printf("adda þriðju blokkinni\n");
	insert_block(temp);*/

	/*size_t predFirstBlock = GET(PRED(freeblock));
	size_t succFirstBlock = GET((char*)SUCC(freeblock));
	printf("FIRSTBLOCK, PRED: %x SUCC: %x\n", predFirstBlock, succFirstBlock);

	remove_block(freeblock);
	printf("Remove-a fremstu blokk\n");

	mm_checkheap(1);*/

	return 0;
}
/* $end mminit */

/* 
 * mm_malloc - Allocate a block with at least size bytes of payload 
 */
/* $begin mmmalloc */
void *mm_malloc(size_t size) 
{
    size_t asize;      /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    char *bp;      
	

    /* Ignore spurious requests */
    if (size <= 0) {
        return NULL;
    }

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE) {
        asize = DSIZE + OVERHEAD;
    }
    else {
        asize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);
    }
    
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap((extendsize/WSIZE + 88))) == NULL) {
        return NULL;
    }
    place(bp, asize);
	//mm_checkheap(0);
    return bp;
} 
/* $end mmmalloc */

/* 
 * mm_free - Free a block 
 */
/* $begin mmfree */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
	insert_block(coalesce(bp));
}
/* $end mmfree */

/*
 * mm_realloc - naive implementation of mm_realloc. The realloc() function shall change the 
 * size of the memory object pointed to by ptr to the size specified by size. The contents 
 * of the object shall remain unchanged up to the lesser of the new and old sizes. If the 
 * new size of the memory object would require movement of the object, the space for the 
 * previous instantiation of the object is freed. If the new size is larger, the contents 
 * of the newly allocated portion of the object are unspecified.
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *newp;
	void *oldp = ptr;
    size_t oldSize = GET_SIZE(HDRP(oldp));
	//size_t newSize = ALIGN(size);
	size_t asize;
	//size_t isPrevFree = GET_ALLOC(FTRP(PREV_BLKP(oldp)));
	//size_t isNextFree = GET_ALLOC(HDRP(NEXT_BLKP(oldp)));
	//size_t prevSize = GET_SIZE(FTRP(PREV_BLKP(oldp)));
	//size_t nextSize = GET_SIZE(HDRP(NEXT_BLKP(oldp)));

	//mm_checkheap(0);

	if(ptr == NULL)
		return mm_malloc(size);
	else if(size == 0){
		mm_free(ptr);
		return NULL;
	} 
	else if(size == GET_SIZE(HDRP(ptr)))
		return ptr;		
	
    if (size <= DSIZE)
        asize = DSIZE + OVERHEAD;
    else 
       asize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);
	
	if(asize <= oldSize){
	  //printf("OldSize: %zu , asize: %zu\n", oldSize, asize); 
		
	  if(oldSize - asize < OVERHEAD){
			//printf("FER HINGAÐ\n");
			return oldp;
		}
		PUT(HDRP(oldp), PACK(asize, 1));
		PUT(FTRP(oldp), PACK(asize, 1));
		//printf("alloc minnkar og nýtir rest í fría blokk\n");
		PUT(HDRP(NEXT_BLKP(oldp)), PACK(oldSize - asize, 0));
		PUT(FTRP(NEXT_BLKP(oldp)), PACK(oldSize - asize, 0));
		insert_block(NEXT_BLKP(oldp));
	}
	/*else if(!isNextFree && (oldSize + nextSize) >= asize){
		if((oldSize + nextSize) - asize < OVERHEAD){
			PUT(HDRP(oldp), PACK(oldSize + nextSize, 1));
			PUT(FTRP(oldp), PACK(oldSize + nextSize, 1));
		}
		else{
			PUT(HDRP(oldp), PACK(asize, 1));
			PUT(FTRP(oldp), PACK(asize, 1));
			//PUT(HDRP(NEXT_BLKP(oldp)), PACK()
		}
	}*/	

	// calculate the adjusted size of the request ????
	// ef adjustedSize <= oldsize, return ptr
	
	if ((newp = mm_malloc(size)) == NULL) {
        printf("ERROR: mm_malloc failed in mm_realloc\n");
        exit(1);
    }
    if (size < oldSize) {
        oldSize = size;
    }
    memcpy(newp, ptr, oldSize);
    mm_free(ptr);
    return newp;
}

/* 
 * mm_checkheap - Check the heap for consistency 
 */
void mm_checkheap(int verbose) 
{
	//printf("************ CHECK BEGIN ***************\n");
    char *bp = heap_listp;
	char *checkAll = heap_listp;
	int countblocks = -1;

    if (verbose) {
        printf("Heap: (%p)\n", heap_listp);
    }

    if ((GET_SIZE(HDRP(heap_listp)) != OVERHEAD) || !GET_ALLOC(HDRP(heap_listp))) {
        printf("Bad prologue header\n");
    }
    checkblock(heap_listp);
	
	
	 while(GET(SUCC(bp)) != 0){
		if(verbose == 1)
		 	printblock(bp);
		checkblock(bp);
		//countblocks++;
		bp = (char*)GET(SUCC(bp));
	}
	 if(verbose == 1)
		 printblock(bp);

    for (checkAll = heap_listp; GET_SIZE(HDRP(checkAll)) > 0; checkAll = NEXT_BLKP(checkAll)) {
		if(!GET_ALLOC(HDRP(checkAll)) && !block_in_free_list(checkAll)){
			printf("Block is marked free but not in free list\n");		
		}
        checkblock(checkAll);
		countblocks++;
    }

	//printf("Number of free blocks: %d\n", freeblockcount);
	//printf("Total number of blocks: %d\n", countblocks);
    
    if ((GET_SIZE(HDRP(checkAll)) != 0) || !(GET_ALLOC(HDRP(checkAll)))) {
        printf("Bad epilogue header\n");
    }
	//printf("****************** CHECK END ********************\n");
}

/* The remaining routines are internal helper routines */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
/* $begin mmextendheap */
static void *extend_heap(size_t words) 
{
    char *bp;
    size_t size;
        
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((bp = mem_sbrk(size)) == (void *)-1) {
        return NULL;
    }

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* free block footer */
	PUT(PRED(bp), 0);					  /* predicessor */
	PUT(SUCC(bp), 0);					  /* successor */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */
	
    /* Coalesce if the previous block was free */
    return coalesce(bp);
	//return bp;
}
/* $end mmextendheap */

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
/* $begin mmplace */
/* $begin mmplace-proto */
static void *place(void *bp, size_t asize)
// $end mmplace-proto 
{
    size_t csize = GET_SIZE(HDRP(bp));
	void *alloBlock = bp;

    if ((csize - asize) >= (DSIZE + OVERHEAD)) { 
		PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
		// setjum free blokkina sem kom út úr splitti í free lista
		insert_block(bp);
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
	// skilum pointer á allocated blokkina
	return alloBlock;
}
/* $end mmplace */

/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
static void *find_fit(size_t asize)
{
    // first fit search
    void *bp = heap_listp;
	
	while(bp != 0){
	    if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
		// tökum blokkina sem fannst útúr free listanum
		remove_block(bp);
		// remove-um frekar í place fallinu
      	 return bp;
		}
    	bp = (void*)GET(SUCC(bp));
	}
    return NULL; // no fit
}

/*
 * coalesce - boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce(void *bp) 
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {  			// Case 1 
        //insert_block(bp);
		return bp;
    }
    else if (prev_alloc && !next_alloc) {      // Case 2 
		// ef next block er free tökum við hana úr free lista
		remove_block(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }
    else if (!prev_alloc && next_alloc) {  		// Case 3
		// ef prev blokk er frí tökum við hana úr free lista
		remove_block(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else {                                     // Case 4
		// ef next og prev blokkir fríar tökum við þær úr free lista
		remove_block(PREV_BLKP(bp));
		remove_block(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}


static void printblock(void *bp) 
{
    size_t hsize, halloc, fsize, falloc;

    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));  
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));  
    
    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }

    printf("%p: header: [%d:%c] footer: [%d:%c]\n", bp, 
           hsize, (halloc ? 'a' : 'f'), 
           fsize, (falloc ? 'a' : 'f'));
	
	if(!GET_ALLOC(HDRP(bp))){	
		printf("%p: Predicessor: [0x%x] Successor: [0x%x]\n", bp,
				GET(PRED(bp)), GET(SUCC(bp)));
	}
	if(bp == heap_listp){
		printf("%p: Predicessor: [0x%x] Successor: [0x%x]\n", bp,
			GET(PRED(bp)), GET(SUCC(bp)));
	}
}

static void checkblock(void *bp) 
{
    if ((size_t)bp % 8) {
        printf("Error: %p is not doubleword aligned\n", bp);
    }
    if (GET(HDRP(bp)) != GET(FTRP(bp))) {
        printf("Error: header does not match footer\n");
    }
	if(GET_ALLOC(HDRP(bp)) == 0){
		if(GET_SIZE(HDRP(bp)) < 16){
			printf("Free block is lesser than 16 bytes !\n");
		}
	}
}

static void insert_block(void *bp){
	
	if(GET(SUCC(heap_listp)) == 0){
		PUT(PRED(heap_listp), 0);
		PUT(SUCC(heap_listp), (size_t)bp);
		PUT(PRED(bp), (size_t)heap_listp);
		PUT(SUCC(bp), 0);
	}
	else{
		PUT(PRED(bp), (size_t)heap_listp);
		PUT(SUCC(bp), (size_t)GET(SUCC(heap_listp)));
		// fæ segfault á línuna hér fyrir neðan !!! ???
		PUT(PRED(GET(SUCC(heap_listp))), (size_t)bp);
		PUT(SUCC(heap_listp), (size_t)bp);
	}
	freeblockcount++;
}

/* Remove block from the free list */
static void remove_block(void *bp){
	// ef aðeins 1 block í listanum
	if(freeblockcount == 1){
		PUT(SUCC(heap_listp), 0);
	}
	// blokk í enda listans
	else if(GET(SUCC(bp)) == 0){
		//printf("FER Í RÉTTA LOOPU ÞEGAR FYRSTU BLOKK ER EITT !!!\n");
		PUT(SUCC(GET(PRED(bp))), 0);
	}
	// blokk í miðjum lista
	else if(GET(PRED(bp)) != 0 && GET(SUCC(bp)) != 0){
		PUT(GET(SUCC(bp)), (size_t)GET(PRED(bp)));
		PUT(SUCC(GET(PRED(bp))), (size_t)GET(SUCC(bp)));
	}
	freeblockcount--;
}

static int block_in_free_list(void *bp){
	// fara í gegnum heap_listp listann
	void* freelist = heap_listp;
	// fáum segfoult á loopuna hér að neðan
	do{
		if(freelist == bp)
			return 1;	
		freelist = (void*)GET(SUCC(freelist));
	} while(freelist != 0);

	return 0;
}
