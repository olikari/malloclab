/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * - Ætlum að styðjast við ecplicit list til að halda utan um fríar blokkur
 * - Æltum að styðjast við best fit í placement policy
 * - Til að sameina samliggjandi fríar blokkir ætlum við að notast við
 *   boundary tags (vera með header og footer) og þ.a.l. vera með doubly linked list.
 * - Til að byrja með verðum við með footer í allocated blokkum en kemur til greina
 *   að sleppa því til að spara pláss þegar lengra verður komið.
 * - Fríar blokkir hafa 16 byte OVERHEAD, header, pred pointer, succ pointer og footer
 *   svo min size á blokkum eru 16 byte.
 * - Alligned blokkir samanstanda af header, payload, padding, footer.
 * - Næsta skref er að útfæra mm_init() (sjá dummy kóða í falli) og keyra litla traceskrá
 *   sem búin var til  og sett í möppunar traces.
 * - mm_checkheap() var útfært sem error checker sem og hjálparföllin mm_printBlock() og
 *   mm_checkBlock().
 *
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
 * provide your team information in below _AND_ in the
 * struct that follows.
 *
 * Note: This comment is parsed. Please do not change the
 *       Format!
 *
 * === User information ===
 * Group: MallCats 
 * User 1: olafurks10
 * SSN: X  2611872569	
 * User 2: asgeira13 
 * SSN: X
 * User 3: 
 * SSN: X
 * === End User Information ===
 ********************************************************/
team_t team = {
    /* Group name */
    "MallCats",
    /* First member's full name */
    "Olafur Kari Sigurbjornsson",
    /* First member's email address */
    "olafurks10@ru.is",
    /* Second member's full name (leave blank if none) */
    "Ásgeir Atlason",
    /* Second member's email address (leave blank if none) */
    "asgeira13@ru.is",
    /* Third full name (leave blank if none) */
    "",
    /* Third member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 	8
#define WSIZE 		4
#define DSIZE		8
#define OVERHEAD 	16			/* overhead in allo or free blocks */
#define CHUNKSIZE	(1<12)		/* Extend heap by this amount (bytes) */
//const int ALIGNMENT = 8;

/* pack a size and allocated bit into a word */
#define PACK(size, alloc)	((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)				(*(size_t *)(p))
#define PUT(p, val)			(*(size_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE_BLOCK(p)	(GET(p) & ~0x7)
#define GET_ALLOC(p)		(GET(p) & 0x1)

/* Given block pointer bp, compute address of its header and footer */
#define HDRP(bp)			((char *)(bp) - WSIZE)
#define FTRP(bp)			((char *)(bp) + GET_SIZE_BLOCK(((char *)(bp) - DSIZE)))

/* Given block pointer bp, compute address of next and previous blocks */
#define NEXT_BLOCKP(bp)		((char *)(bp) + GET_SIZE_BLOCK(((char *)(bp) - WSIZE)))
#define PREV_BLOCKP(bp)		((char *)(bp) - GET_SIZE_BLOCK(((char *)(bp) - DSIZE)))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
// macroinn hér að ofan er sama og inline fallið hér að neðan
/*inline int align(int size){
	return (((size) + (ALIGNMENT-1)) & ~0x7);
}*/

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Global variables */
static char* heap_listp;		/* points to first free block */
static char* start_of_heap; 	/* points to start of heap */

/* helper functions */
static void mm_checkheap(int verbose);
static void mm_printblock(void* bp);
static void mm_checkblock(void* bp);
static void mm_insert(void* bp);
static void mm_remove(void* bp);
static void *mm_extend_heap(size_t words);
static void mm_simple_check(int verbose);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	if((heap_listp = mem_sbrk(DSIZE * 3)) == (void *)-1){
		return -1;
	}
	/* Set size á prologue block sem 8 byte. Prologe
	 * og epilogue eru sentinel nóður til að einfalda
	 * coalescing */
	// PADDING??: að vera 0 eða WSIZE, size á epilogue er 0, en tekur það
	// WSIZE í minni ???
	PUT(heap_listp, 0);			/* Alignment padding: 8byte boundary */
	PUT(heap_listp + (WSIZE), PACK(8, 1));		/* prologue header */
	PUT(heap_listp + (DSIZE), PACK(8, 1));		/* prologue footer */
	PUT(heap_listp + (3*WSIZE), PACK(0, 1));	/* Epilogue block */
	/* Látum heap_listp benda á prologue block */
	heap_listp += (2*WSIZE);
	start_of_heap = heap_listp + DSIZE;
	
	mm_simple_check(1);
	mm_checkheap(0);
	
	void *freeblock;
	/* Extendum heap-inn og setjum free blokk á heap.
	 * QUESTION: af hverju viljum við gera CHUNKSIZE/WSIZE ?? */
	if((freeblock = mm_extend_heap(CHUNKSIZE/WSIZE)) == NULL){
		return -1;
	}
	
	// þurfum að bæta freeblock inní free block list.... but dont know how !!
	
	printf("************************\n");
	
	mm_simple_check(1);
	// upphafsstillum global breytur
	// fyrsta blokk verður að vera 16 byte
	// notum sbrk hér til að allocate-a minni á heap
	// returns 0 if successfull, -1 otherwise
	// undirbúa breakpointerinn.... setja hann á réttan stað
	// búa til smá pláss til þess að við séum undirbúin að kalla á malloc
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 * Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
	


    void *p = mem_sbrk(newsize);
    if (p == (void *)-1) {
        return NULL;
    }
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 * Viljum stækka minnið í realloc, taka frá meira minni... skilar
 * pointer á það minni. Endum á realloc
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL) {
        return NULL;
    }
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize) {
        copySize = size;
    }
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

static void mm_checkheap(int verbose){

	char* bp = heap_listp;
	char* freeBlock = start_of_heap;
	char* allBlocks = start_of_heap;
	if(verbose){
		printf("Heap: %p\n", heap_listp);
	}
	
	/* Loopan skoðar blockir í free listann og 
	 * athugar hvort reglum sé framfylgt í þeim */
	while(GET_SIZE_BLOCK(HDRP(freeBlock)) > 0){
		
		// Ekki nóg að vera með í while loopu fyrir neðan ?
		//mm_printblock(freeBlock);
		//mm_checkblock(freeBlock);
	
		/* Er einnhver block í free listanum
		 * sem er ekki merkt free ? */
		if(GET_ALLOC(freeBlock) != 0){
			printf("Error: Block in free list but not marked as free\n");
		}
		/* Er einnhver block í free lista með prev eða next block marked as free ? 
		 * WARNING: GÆTI FENGIÐ SEG FOULT Á NEXT_BLOCKP 
		 * WARNING: ALGJOR MACRO SÚPA, GÆTI VERIÐ EÐ FUCKED */
		/*if((GET_ALLOC(NEXT_BLOCKP(HDRP(freeBlock)) == 0)) || (GET_ALLOC(PREV_BLOCKP(HDRP(freeBlock)) == 0))){
			printf("Error: Adjacent blocks are free but not coalesced\n");
		}*/
		
		/*  Do the pointers in the free list point to valid free blocks?
		 * Pössum að blockir eru ekki minni en OVERHEAD */
		if(GET_SIZE_BLOCK(HDRP(freeBlock)) < OVERHEAD){
			printf("Error: Free block not valid, size < OVERHEAD\n");
		}

		// færum pointer á næstu block
		freeBlock = NEXT_BLOCKP(freeBlock);
	}

	/* Loopan skoðar allar blockir á heap og
	 * athugar hvort reglum sé framfylgt í þeim*/
	while(GET_SIZE_BLOCK(HDRP(allBlocks)) > 0){
		
		mm_printblock(allBlocks);
		mm_checkblock(allBlocks);

		/* Er jafn langt í frá head í foot og foot í head ? 
		 * WARNING: Gæti verið kolvitlaus útfærsla !!! */
		size_t head_foot_gap;
		head_foot_gap = FTRP(allBlocks) - HDRP(allBlocks);
		if((HDRP(allBlocks) + head_foot_gap) != FTRP(allBlocks)){
			printf("Error: gap between block header and footer not the same\n");
		}
		
		//  Do the pointers in a heap block point to valid heap addresses?
		
		/* Færum pointer á næstu block */
		allBlocks = NEXT_BLOCKP(allBlocks);
	}
}

static void mm_printblock(void *bp){
	
	size_t hsize, halloc, fsize, falloc;

	hsize = GET_SIZE_BLOCK(HDRP(bp));
	halloc = GET_ALLOC(HDRP(bp));
	fsize = GET_SIZE_BLOCK(FTRP(bp));
	falloc = GET_ALLOC(FTRP(bp));

	/* fatta ekki alveg þetta test, líklega að segja
	 * að við séum á enda heap-sins og síðasta block
	 * í listanum ?? */
	if(hsize == 0){
		printf("%p: EOL\n", bp);
		return;
	}
	
	/* prentar út stærð og hvort block sé laus eða ekki
	 * bæði header of foooter */
	printf("%p: header: [%d:%c] footer: [%d:%c]\n", bp,
			hsize, (halloc ? 'a' : 'f'),
			fsize, (falloc ? 'a' : 'f'));
	
	if(!GET_ALLOC(bp)){
		printf("Predecessor: %d ------ Successor: %d \n", GET(bp), GET(bp+WSIZE));
	}
}

/* Athugar hvort að reglum sé framfylgt í
 * aligned og free blockum */
static void mm_checkblock(void* bp){
		
	/* Er blockin aligned? */
	if((size_t)bp % 8){
		printf("Error: %p is not doubleword aligned\n", bp);
	}
	/* Er header og footer sá sami */
	if(GET(HDRP(bp)) != GET(FTRP(bp))){
		printf("Error: Size & Align in header and footer not the same !\n");
	}
	if(!GET_ALLOC(bp)){
		printf("Predecessor: %d ------ Successor: %d \n", GET(bp), GET(bp+WSIZE));
	}
}

static void mm_insert(void* bp){
	//PUT(bp, HDRP(mm_heap_listp) + WSIZE);
	//PUT(bp + WSIZE, )
}

static void mm_remove(void* bp){
	
}

static void *mm_extend_heap(size_t words){
	
	char *bp;
	size_t size;
	/* Pössum uppá 8 byte allignment */
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

	if((bp = mem_sbrk(size)) == (void *)-1){
		return NULL;
	}

	/* Frumstillum fríu blokkina og epilogue header 
	 * QUESTION: vill ég láta pred og succ benda á header
	 * á pred og succ blokkum ? Sjá línu 3 og 4 !
	 * og á ég ekki örugglega að kasta í size_t þar ?? */
	PUT(HDRP(bp), PACK(size, 0));			/* stillum header og footer */
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(bp) + WSIZE, (size_t)HDRP(PREV_BLOCKP(bp)));
	PUT(HDRP(bp) + DSIZE, (size_t)HDRP(NEXT_BLOCKP(bp)));
	PUT(HDRP(NEXT_BLOCKP(bp)), PACK(0,1));		/* nýr epilogue header */
	
	return(bp);
}

static void mm_simple_check(int verbose){
	char* bp;
	
	bp = heap_listp;

	//printf("Heap: %p\n", heap_listp);
	printf("Size of heap: %d\n", mem_heapsize());
		
	// skoðum hvort prologe header sé réttur
	if(GET_SIZE_BLOCK(HDRP(heap_listp)) != DSIZE || !GET_ALLOC(HDRP(heap_listp))){
		printf("Bad prologue header\n");
	}	
	mm_checkblock(heap_listp);

	for(bp = heap_listp; GET_SIZE_BLOCK(HDRP(bp)) > 0; bp = NEXT_BLOCKP(bp)){
		
		if(verbose){
			mm_printblock(bp);
		}
		mm_checkblock(bp);
	}
	if((GET_SIZE_BLOCK(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp)))){
		printf("Bad epilogue header\n");
	}
}
