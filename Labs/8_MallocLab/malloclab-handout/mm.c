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
	"CSAPP_KH",
	/* First member's full name */
	"KethersHao",
	/* First member's email address */
	"kethershao@foxmail.com",
	/* Second member's full name (leave blank if none) */
	"",
	/* Second member's email address (leave blank if none) */
	""
};

typedef struct block
{
	size_t header;
#if __SIZEOF_POINTER__ == 4
	unsigned char padding[4];
#endif
	union 
	{
		unsigned char payload[0];
		struct block* next;
	};
	struct block* prev;
	
} block_t;

unsigned char get_segrelist_level_by_size(size_t payload_size);
void insert_block_to_segrelist(block_t* blkp, unsigned char level_idx);
void* extend_heap(size_t payload_size);
void* coalesce(block_t* p);
void* find_fit(size_t payload_size);
void* split_block(block_t* p, size_t payload_size);
int mm_check(void);
int check_traverse_free_lists();
int check_traverse_mem_heap();
int check_every_free_block_in_list();

// #define MM_CHECK_DEBUG
#ifdef MM_CHECK_DEBUG
	#define MM_CHECK(str)	if (mm_check()) \
							{\
								printf("in func: %s\n", __func__);\
								printf("%s\n", str);\
								fflush(stdout);\
								exit(0);\
							}\

#else
	#define MM_CHECK(str)
#endif	

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define MIN_PAYLOAD_SIZE 	(2 * sizeof(block_t *) + SIZE_T_SIZE)
#define MIN_BLOCK_SIZE		(2 * (sizeof(block_t *) + SIZE_T_SIZE))

#define WSIZE SIZE_T_SIZE
#define DSIZE (2 * SIZE_T_SIZE)
#define CHUNKSIZE (1<<12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define PACK(size, alloc)	((size) | (alloc))

#define GET(p)			(*(size_t *)(p))
#define PUT(p, val)		(*(size_t *)(p) = (val))

#define GET_SIZE(p)			(GET(p) & ~0x7)
#define GET_ALLOC(p)		(GET(p) & 0x1)
#define GET_PREV_ALLOC(p)	((GET(p) & 0x2) >> 1)

#define HDRP(bp)		((char *)(bp) - SIZE_T_SIZE)
#define FTRP(bp)		((char *)(bp) + GET_SIZE(HDRP(bp)) - 2 * SIZE_T_SIZE)

#define NEXT_NEAR_BLKP(p)	((char *)(p) + GET_SIZE(p))
#define PREV_NEAR_BLKP(p)	((char *)(p) - GET_SIZE((char*)p - SIZE_T_SIZE))

// segrelist_root_pointers[0] points to [1, 16] bytes payload, 
// the next in array points to [17, 32] bytes, and so on.
// Except that the last one points to (pagesize + 1, inf)
block_t** segrelist_root_pointers;
unsigned char segrelist_root_pointers_count;

inline void write_header(void* addr, size_t block_size, size_t pre_alloc, size_t cur_alloc)
{
	*(size_t*)addr = ((block_size >> 3) << 3) + (pre_alloc << 1) + cur_alloc;
}

inline void change_header_size(size_t* addr, size_t newsize)
{
	*addr = (*addr & 0x7) | (newsize >> 3) << 3;
}

inline void change_header_alloc(size_t* addr, size_t cur_alloc)
{
	*addr = (*addr & ~0x1) | cur_alloc;
}

inline void change_header_prealloc(size_t* addr, size_t pre_alloc)
{
	*addr = (*addr & ~0x2) | (pre_alloc << 1);
}

inline void place_alloc(block_t* blkp, size_t block_size, size_t pre_alloc)
{
	write_header(blkp, block_size, pre_alloc, 1);
	change_header_prealloc((char*)blkp + block_size, 1);
}

inline void place_free(block_t* blkp, size_t block_size, size_t pre_alloc)
{
	write_header(blkp, block_size, pre_alloc, 0);
	write_header(FTRP(blkp->payload), block_size, pre_alloc, 0);
	change_header_prealloc((char*)blkp + block_size, 0);
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	size_t pg_size = mem_pagesize();
	segrelist_root_pointers_count = 1;
	for (size_t i = MIN_PAYLOAD_SIZE; i <= pg_size; i <<= 1)
	{
		++segrelist_root_pointers_count;
	}
	segrelist_root_pointers = (block_t **)malloc(segrelist_root_pointers_count * sizeof(block_t *));
	for (size_t i = 0; i < segrelist_root_pointers_count; i++)
	{
		segrelist_root_pointers[i] = NULL;
	}
	
	block_t* prologue;
	if ((prologue = mem_sbrk(3 * SIZE_T_SIZE)) == (void*)-1)
		return -1;

	write_header(prologue, 2 * SIZE_T_SIZE, 1, 1);
	write_header(FTRP(prologue->payload), 2 * SIZE_T_SIZE, 1, 1);
	write_header((char*)prologue + GET_SIZE(prologue), 0, 1, 1);	// set epilogue's header
	
	MM_CHECK("");

	return 0;
}

unsigned char get_segrelist_level_by_size(size_t payload_size)
{
	unsigned char levelIdx = 0;

	for (size_t i = (payload_size - 1) / MIN_PAYLOAD_SIZE; i > 0; i >>= 1)
	{
		++levelIdx;
	}
	
	return (unsigned char)MIN(levelIdx, segrelist_root_pointers_count - 1);
}

/// @brief insert block of level_idx into the segregated lists by address-order
/// @param blkp pointer to the block
/// @param level_idx level index of the segregated list that indicates the size range
void insert_block_to_segrelist(block_t* blkp, unsigned char level_idx)
{
	block_t *target_blk = NULL, *last_blk = NULL;
	if (segrelist_root_pointers[level_idx] == NULL)	// insert at the head of the list
	{
		blkp->prev = blkp->next = NULL;
		segrelist_root_pointers[level_idx] = blkp;
		return;
	}
	for (block_t* i = segrelist_root_pointers[level_idx]; i != NULL; i = i->next)
	{
		last_blk = i;
		if (i > blkp)
		{
			target_blk = i;
			break;
		}
	}

	if (target_blk != NULL)
	{
		blkp->next = target_blk;
		if (blkp != target_blk->prev)	// in case of self-reference
			blkp->prev = target_blk->prev;
		if (blkp->prev)
			blkp->prev->next = blkp;
		else
			segrelist_root_pointers[level_idx] = blkp;
		blkp->next->prev = blkp;
	}
	else	// insert at the tail of the list
	{
		if (last_blk != blkp)
		{
			blkp->prev = last_blk;
			last_blk->next = blkp;
		}
		blkp->next = NULL;
	}

	char info[100];
	sprintf(info, "block addr: %p, level id: %d", blkp, level_idx);
	MM_CHECK(info);
}

void remove_block_from_segrelist(block_t* blkp, unsigned char level_idx)
{
	if (blkp == NULL || GET_ALLOC(blkp))
		return;

	if (segrelist_root_pointers[level_idx] == blkp)
	{
		segrelist_root_pointers[level_idx] = blkp->next;
	}
	if (blkp->prev && blkp->prev <= mem_heap_hi())
		blkp->prev->next = blkp->next;
	if (blkp->next && blkp->next <= mem_heap_hi())
		blkp->next->prev = blkp->prev;
	blkp->next = blkp->prev = NULL;

	change_header_alloc(blkp, 1);
	change_header_prealloc((char*)blkp + GET_SIZE(blkp), 1);
}

void* extend_heap(size_t payload_size)
{
	size_t pg_size = mem_pagesize();
	size_t blk_size = pg_size + SIZE_T_SIZE;
	
	unsigned char list_level_idx = get_segrelist_level_by_size(payload_size);
	if (list_level_idx == segrelist_root_pointers_count - 1)
	{
		blk_size = ((payload_size + pg_size - 1) / pg_size) * pg_size + SIZE_T_SIZE;
	}
	block_t *p = (char*)mem_sbrk(blk_size);
	if (p == (void *)-1)
		return NULL;

	p = (char*)p - SIZE_T_SIZE;
	memcpy((char*)p + blk_size, p, SIZE_T_SIZE);	// move the epilogue
	unsigned int prealloc = GET_PREV_ALLOC(p);
	place_alloc(p, blk_size, prealloc);

	char info[100];
	sprintf(info, "before split_block() -- block addr: %p, size: %d, pre_alloc: %d, alloc: %d", 
		p, GET_SIZE(p), GET_PREV_ALLOC(p), GET_ALLOC(p));
	MM_CHECK(info);

	return split_block(p, payload_size);
}

void* coalesce(block_t* p)
{
	unsigned int pre_alloc = GET_PREV_ALLOC(p);
	unsigned int next_alloc = GET_ALLOC((char*)p + GET_SIZE(p));
	block_t* prev_near_blk = pre_alloc ? NULL : PREV_NEAR_BLKP(p);
	block_t* next_near_blk = NEXT_NEAR_BLKP(p);
	unsigned int prev_size = pre_alloc ? 0 : GET_SIZE(prev_near_blk);
	unsigned int next_size = GET_SIZE(next_near_blk);
	unsigned int cur_size = GET_SIZE(p);
	unsigned int new_size = 0;
	block_t* bp;
	unsigned char old_level_id = get_segrelist_level_by_size(cur_size - SIZE_T_SIZE);
	unsigned char new_level_id;

	char info[100];
	sprintf(info, "before coalescing -- block addr: %p, size: %d, pre_alloc: %d, alloc: %d", 
		p, GET_SIZE(p), GET_PREV_ALLOC(p), GET_ALLOC(p));
	MM_CHECK(info);
	
	if (pre_alloc && next_alloc)		// Case 1
	{
		new_level_id = old_level_id;

		change_header_prealloc(next_near_blk, 0);	// set next block's prealloc bit
		bp = p->payload;
	}
	else if (pre_alloc && !next_alloc)	// Case 2
	{
		new_size = cur_size + next_size;
		remove_block_from_segrelist(next_near_blk, get_segrelist_level_by_size(next_size - SIZE_T_SIZE));
		new_level_id = get_segrelist_level_by_size(new_size - SIZE_T_SIZE);

		place_free(p, new_size, 1);
		bp = p->payload;
	}
	else if (!pre_alloc && next_alloc)	// Case 3
	{
		new_size = prev_size + cur_size;
		remove_block_from_segrelist(prev_near_blk, get_segrelist_level_by_size(prev_size - SIZE_T_SIZE));
		new_level_id = get_segrelist_level_by_size(new_size - SIZE_T_SIZE);

		unsigned int prev_prev_alloc = GET_PREV_ALLOC(prev_near_blk);
		place_free(prev_near_blk, new_size, prev_prev_alloc);
		bp = prev_near_blk->payload;
	}
	else								// Case 4
	{
		new_size = prev_size + cur_size + next_size;
		remove_block_from_segrelist(prev_near_blk, get_segrelist_level_by_size(prev_size - SIZE_T_SIZE));
		remove_block_from_segrelist(next_near_blk, get_segrelist_level_by_size(next_size - SIZE_T_SIZE));
		new_level_id = get_segrelist_level_by_size(new_size - SIZE_T_SIZE);

		unsigned int prev_prev_alloc = GET_PREV_ALLOC(prev_near_blk);
		place_free(prev_near_blk, new_size, prev_prev_alloc);
		bp = prev_near_blk->payload;
	}

	insert_block_to_segrelist(HDRP(bp), new_level_id);
	
	sprintf(info, "after coalescing -- block addr: %p, size: %d, pre_alloc: %d, alloc: %d", 
		p, GET_SIZE(p), GET_PREV_ALLOC(p), GET_ALLOC(p));
	MM_CHECK(info);
	
	return bp;
}

/// @brief find block that satisfy the payload size, and remove it from the free list
/// @param payload_size 
/// @return 
void* find_fit(size_t payload_size)
{
	unsigned char min_level_idx = get_segrelist_level_by_size(payload_size);
	unsigned char target_level_idx = min_level_idx;
	block_t* fit_blk = NULL;
	size_t required_blk_size = MAX(payload_size + SIZE_T_SIZE, MIN_BLOCK_SIZE);
	for (unsigned char i = min_level_idx; i < segrelist_root_pointers_count; i++)
	{
		for (block_t* j = segrelist_root_pointers[i]; j != NULL; j = j->next)
		{
			size_t j_blk_size = GET_SIZE(j);
			if (j_blk_size >= required_blk_size)	// first fit
			{
				fit_blk = j;
				target_level_idx = i;
				break;
			}
			// if (j_blk_size >= required_blk_size)	// best fit
			// {
			// 	if (fit_blk == NULL || j_blk_size < GET_SIZE(fit_blk))
			// 	{
			// 		fit_blk = j;
			// 		target_level_idx = i;
			// 	}
			// }
		}
		if (fit_blk != NULL)
			break;
	}
	
	if (fit_blk == NULL)
		return NULL;

	remove_block_from_segrelist(fit_blk, target_level_idx);
	// if (target_level_idx != get_segrelist_level_by_size(GET_SIZE(fit_blk) - SIZE_T_SIZE))
	// {
	// 	printf("ERROR: different level id!");
	// 	exit(0);
	// }
	// if (fit_blk == segrelist_root_pointers[target_level_idx])
	// {
	// 	segrelist_root_pointers[target_level_idx] = fit_blk->next;
	// }
	// else
	// {
	// 	fit_blk->prev->next = fit_blk->next;
	// }
	// if (fit_blk->next)
	// {
	// 	fit_blk->next->prev = fit_blk->prev;
	// }
	// change_header_alloc(fit_blk, 1);
	// change_header_prealloc((char*)fit_blk + GET_SIZE(fit_blk), 1);
	MM_CHECK("");
	
	return fit_blk->payload;	
}

void* split_block(block_t* p, size_t payload_size)
{
	size_t total_blk_size = GET_SIZE(p);
	size_t alloc_blk_size = payload_size + SIZE_T_SIZE;
	size_t free_blk_size = total_blk_size - alloc_blk_size;
	size_t pre_alloc = GET_PREV_ALLOC(p);

	if (free_blk_size < MIN_BLOCK_SIZE)
		return p->payload;

	block_t *free_blk = NULL, *alloc_blk = NULL;
	if (payload_size > 64)	// place at tail
	{
		alloc_blk = (char*)p + free_blk_size;
		free_blk = p;
		place_alloc(alloc_blk, alloc_blk_size, 0);
	}
	else	// place at front
	{
		pre_alloc = 1;
		alloc_blk = p;
		change_header_size(alloc_blk, alloc_blk_size);
		free_blk = (char*)p + alloc_blk_size;
	}
	place_free(free_blk, free_blk_size, pre_alloc);

	char info[100];
	sprintf(info, "after split_block() -- block addr: %p, size: %d, pre_alloc: %d, alloc: %d", 
		p, GET_SIZE(p), GET_PREV_ALLOC(p), GET_ALLOC(p));

	free_blk = HDRP(coalesce(free_blk));

	return alloc_blk->payload;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	if (size == 0)
		return NULL;

	size_t asize = ALIGN(size);
	if (size < MIN_PAYLOAD_SIZE)
		asize = MIN_PAYLOAD_SIZE;
	
	char *bp;
	if ((bp = find_fit(asize)) != NULL)
	{
		// remove_block_from_segrelist(HDRP(bp), get_segrelist_level_by_size(GET_SIZE(HDRP(bp)) - SIZE_T_SIZE));
		bp = split_block(HDRP(bp), asize);
		return bp;
	}

	if ((bp = extend_heap(asize)) == NULL)
	{
		print_free_list_blocks();
		print_mem_heap_blocks();
		check_traverse_mem_heap();
		check_every_free_block_in_list();
		return NULL;
	}
	
	return bp;
}

/*
 * mm_free - Freeing a block
 */
void mm_free(void *ptr)
{
	size_t size = GET_SIZE(HDRP(ptr));
	unsigned int pre_alloc = GET_PREV_ALLOC(HDRP(ptr));

	place_free(HDRP(ptr), size, pre_alloc);
	MM_CHECK("");
	coalesce(HDRP(ptr));
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	void *oldptr = ptr;
	void *newptr;
	size_t copy_size, old_blk_size;

	if (oldptr == NULL)
	{
		return mm_malloc(size);
	}
	else if (size == 0)
	{
		mm_free(oldptr);
		return NULL;
	}

	old_blk_size = GET_SIZE(HDRP(oldptr));
	copy_size = old_blk_size - SIZE_T_SIZE;
	copy_size = MIN(size, copy_size);
	size_t alloc_blk_size = ALIGN(size) + SIZE_T_SIZE;
	if (old_blk_size >= alloc_blk_size)	// shrink
	{
		newptr = oldptr;
		char* free_ptr = newptr + alloc_blk_size;
		size_t free_blk_size = old_blk_size - alloc_blk_size;
		if (free_blk_size >= MIN_BLOCK_SIZE)
		{
			change_header_size(HDRP(newptr), alloc_blk_size);
			place_free(HDRP(free_ptr), free_blk_size, 1);
			coalesce(free_ptr);
		}
	}
	else	// extend
	{
		block_t* nextblk = NEXT_NEAR_BLKP(HDRP(oldptr));
		size_t next_blk_size = GET_SIZE(nextblk);
		if (!GET_ALLOC(nextblk) && next_blk_size + old_blk_size >= alloc_blk_size)
		{
			size_t total_blk_size = next_blk_size + old_blk_size;
			size_t free_blk_size = total_blk_size - alloc_blk_size;
			remove_block_from_segrelist(nextblk, get_segrelist_level_by_size(next_blk_size - SIZE_T_SIZE));
			size_t old_pre_alloc = GET_PREV_ALLOC(HDRP(oldptr));
			if (free_blk_size >= MIN_BLOCK_SIZE)	// can be split
			{
				char *freebp = NULL, *allocbp = NULL;
				if (size > 64)	// place at tail
				{
					allocbp = (char*)oldptr + free_blk_size;
					freebp = oldptr;
					memmove(allocbp, oldptr, copy_size);
					place_alloc(HDRP(allocbp), alloc_blk_size, 0);
					place_free(HDRP(freebp), free_blk_size, old_pre_alloc);
				}
				else	// place at head
				{
					allocbp = oldptr;
					freebp = (char*)oldptr + alloc_blk_size;
					place_alloc(HDRP(allocbp), alloc_blk_size, old_pre_alloc);
					place_free(HDRP(freebp), free_blk_size, 1);
				}
				coalesce(HDRP(freebp));
				newptr = allocbp;
			}
			else
			{
				newptr = oldptr;
				place_alloc(HDRP(newptr), total_blk_size, old_pre_alloc);
			}
		}
		else
		{
			newptr = mm_malloc(size);
			memmove(newptr, oldptr, copy_size);
			mm_free(oldptr);
		}
	}
	
	return newptr;
}

int mm_check(void)
{
	return check_traverse_free_lists()
	 		|| check_traverse_mem_heap();
}

int check_traverse_free_lists()
{
	void *heap_start = mem_heap_lo(), *heap_end = mem_heap_hi();
	for (size_t i = 0; i < segrelist_root_pointers_count; i++)
	{
		for (block_t* j = segrelist_root_pointers[i]; j != NULL; j = j->next)
		{
			if (GET_ALLOC(j) == 1)
			{
				printf("CHECK by %s\n", __func__);
				printf("find block in free list not freed:\n");
				printf("block addr: %p, size: %d, pre_alloc: %d, alloc: %d\n",
					j, GET_SIZE(j), GET_PREV_ALLOC(j), GET_ALLOC(j));
				return 1;
			}

			if (j > heap_end || j < heap_start)
			{
				printf("CHECK by %s\n", __func__);
				printf("pointer in the free list point to invalid addr:\n");
				printf("block addr: %p, size: %d, pre_alloc: %d, alloc: %d\n",
					j, GET_SIZE(j), GET_PREV_ALLOC(j), GET_ALLOC(j));
				printf("heap_lo: %p, heap_hi: %p\n\n", heap_start, heap_end);
				return 1;
			}
		}
	}

	return 0;
}

int check_traverse_mem_heap()
{
	void* heap_start = mem_heap_lo();
	void* heap_end = mem_heap_hi();
	char* blk = heap_start;
	size_t heapsize = mem_heapsize();

	while (blk <= heap_end && GET_SIZE(blk) != 0)
	{
		char* next_blk = NEXT_NEAR_BLKP(blk);
		size_t blk_size = GET_SIZE(blk);
		if (GET_ALLOC(blk) == 0 && GET_ALLOC(next_blk) == 0)
		{
			printf("CHECK by %s\n", __func__);
			printf("find contiguous free blocks not coalesced:\n");
			printf("block 0 -- addr: %p, size: %d, pre_alloc: %d, alloc: %d\n",
				blk, blk_size, GET_PREV_ALLOC(blk), GET_ALLOC(blk));
			printf("block 1 -- addr: %p, size: %d, pre_alloc: %d, alloc: %d\n\n",
				next_blk, GET_SIZE(next_blk), GET_PREV_ALLOC(next_blk), GET_ALLOC(next_blk));
			return 1;
		}

		if (blk_size > heapsize)
		{
			printf("CHECK by %s\n", __func__);
			printf("find invalid block size:\n");
			printf("addr: %p, size: %d, pre_alloc: %d, alloc: %d\n",
				blk, blk_size, GET_PREV_ALLOC(blk), GET_ALLOC(blk));
			return 1;
		}

		// block_t* bblk = blk;
		// if (GET_ALLOC(bblk) == 0 
		// 	&& segrelist_root_pointers[get_segrelist_level_by_size(GET_SIZE(bblk))] != bblk)
		// {
		// 	if (bblk->prev < heap_start || bblk->prev > heap_end
		// 	|| bblk->next < heap_start || bblk->next > heap_end)
		// 	{
		// 		printf("CHECK by %s\n", __func__);
		// 		printf("find free blocks that not in free list:\n");
		// 		printf("addr: %p, size: %d, pre_alloc: %d, alloc: %d\n",
		// 			blk, blk_size, GET_PREV_ALLOC(blk), GET_ALLOC(blk));
		// 	}
		// }

		blk = (char*)blk + blk_size;
	}

	return 0;
}

int check_every_free_block_in_list()
{
	char* blk = mem_heap_lo();
	void* heap_end = mem_heap_hi();

	while (blk <= heap_end && GET_SIZE(blk) != 0)
	{
		if (!GET_ALLOC(blk))
		{
			unsigned char level_id = get_segrelist_level_by_size(GET_SIZE(blk) - SIZE_T_SIZE);
			unsigned char is_in_list = 0;
			for (block_t* i = segrelist_root_pointers[level_id]; i != NULL; i = i->next)
			{
				if (i == blk)
				{
					is_in_list = 1;
					break;
				}
			}
			
			if (!is_in_list)
			{
				printf("CHECK by %s\n", __func__);
				printf("find free block not in list:\n");
				printf("addr: %p, size: %d, pre_alloc: %d, alloc: %d\n",
					blk, GET_SIZE(blk), GET_PREV_ALLOC(blk), GET_ALLOC(blk));
					
				printf("should be in segrelist_root_pointers[%u]\n\n", level_id);
				return 1;
			}
		}

		blk += GET_SIZE(blk);
	}

	return 0;
}

void print_free_list_blocks()
{
	printf("---------------- free list blocks start ----------------\n");

	size_t payload_size = MIN_PAYLOAD_SIZE;
	size_t total_free_blocks_count = 0;
	for (unsigned char i = 0; i < segrelist_root_pointers_count; i++)
	{
		size_t blocks_count = 0, free_blocks_count = 0, alloc_blocks_count = 0;
		size_t max_block_size = 0, min_block_size = 20 * (1 << 20);
		printf("list max payload size is %d bytes, max block size is %d bytes\n", 
			payload_size, payload_size + SIZE_T_SIZE);
		printf("{\n");
		payload_size <<= 1;
		for (block_t* j = segrelist_root_pointers[i]; j != NULL; j = j->next)
		{
			size_t block_size = GET_SIZE(j);
			printf("addr: %p, size: %d, pre_alloc: %d, alloc: %d\n",
				j, block_size, GET_PREV_ALLOC(j), GET_ALLOC(j));
			++blocks_count;
			if (GET_ALLOC(j))
				++alloc_blocks_count;
			else
				++free_blocks_count;
			max_block_size = MAX(max_block_size, block_size);
			min_block_size = MIN(min_block_size, block_size);
		}
		printf("total blocks: %d, free blocks: %d, alloc blocks: %d\n", 
			blocks_count, free_blocks_count, alloc_blocks_count);
		printf("min block size: %d, max block size: %d\n", min_block_size, max_block_size);
		printf("}\n\n");
		total_free_blocks_count += free_blocks_count;
	}
	printf("total free blocks count: %d\n", total_free_blocks_count);
	printf("---------------- free list blocks end ----------------\n\n\n");
	fflush(stdout);
}

void print_mem_heap_blocks()
{
	printf("---------------- heap blocks start ----------------\n");
	printf("{\n");
	block_t* blk = mem_heap_lo();
	char* heap_end = mem_heap_hi();
	size_t blocks_count = 0, free_blocks_count = 0, alloc_blocks_count = 0;
	while (blk <= heap_end && GET_SIZE(blk) != 0)
	{
		printf("addr: %p, size: %d, pre_alloc: %d, alloc: %d\n",
			blk, GET_SIZE(blk), GET_PREV_ALLOC(blk), GET_ALLOC(blk));
		++blocks_count;
		if (GET_ALLOC(blk))
			++alloc_blocks_count;
		else
			++free_blocks_count;
		blk = (char*)blk + GET_SIZE(blk);
	}
	printf("total blocks: %d, free blocks: %d, alloc blocks: %d\n", 
			blocks_count, free_blocks_count, alloc_blocks_count);
	printf("}\n");
	printf("---------------- heap blocks end ----------------\n");
	fflush(stdout);
}
