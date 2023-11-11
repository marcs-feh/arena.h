#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>
#include <stdbool.h>

#include <stdlib.h> /* Remove if not using malloc() and free()  in the configuration */

/// Configuration //////////////////////////////////////////////////////////////
// This defines how to ask for x bytes of memory
#define ARENA_MALLOC(x) malloc((x))

// This defines how to free a pointer p
#define ARENA_FREE(p) free((p))

// Default alignment for arena_alloc
#define ARENA_DEF_ALIGN (alignof(max_align_t))

// How much to grow the arena (relative to required size) when making new blocks
#define ARENA_GROW_FACTOR 1.15
////////////////////////////////////////////////////////////////////////////////

typedef uint8_t byte;

struct ArenaBlock {
	byte* data;
	size_t offset;
	size_t capacity;

	struct ArenaBlock* next;
};

struct ArenaAllocator {
	size_t block_count;
	size_t total_capacity;
	struct ArenaBlock* head;
};


static uintptr_t 
align_forward_ptr(uintptr_t p, uintptr_t a){
	uintptr_t mod = p % a;

	if(mod > 0){
		p += (a - mod);
	}

	return p;
}

static struct ArenaBlock*
arena_block_create(size_t capacity){
	struct ArenaBlock *blk = ARENA_MALLOC(sizeof(*blk));
	if(blk == NULL){ return blk; }

	void* data = ARENA_MALLOC(capacity);
	if(data == NULL){
		ARENA_FREE(blk);
		return NULL;
	}

	*blk = (struct ArenaBlock){
		.capacity = capacity,
		.offset = 0,
		.data = data,
		.next = NULL,
	};

	return blk;
}


struct ArenaAllocator
arena_create(size_t capacity){
	struct ArenaAllocator ar = {0};
	struct ArenaBlock* blk = arena_block_create(capacity);

	if(blk == NULL){ goto exit; }

	ar = (struct ArenaAllocator){
		.block_count = 1,
		.total_capacity = blk->capacity,
		.head = blk,
	};

	exit:
	return ar;
}

bool
arena_push(struct ArenaAllocator* ar, size_t capacity){
	struct ArenaBlock *blk = arena_block_create(capacity);
	if(blk == NULL){ return false; }

	blk->next = ar->head;
	ar->head = blk;

	ar->block_count += 1;
	ar->total_capacity += capacity;

	return true;
}

void*
arena_alloc_align(struct ArenaAllocator* ar, size_t nbytes, size_t alignment){
	if(nbytes == 0){ return NULL; }
	struct ArenaBlock* blk = ar->head;

	uintptr_t base     = (uintptr_t)blk->data;
	uintptr_t cur      = (uintptr_t)(base + blk->offset);
	uintptr_t aligned  = align_forward_ptr(cur, alignment);
	uintptr_t required = (aligned - base) + nbytes;

	// TODO: first-fit, best-fit and always-alloc

	// Out of memory
	if(required > blk->capacity){
		arena_push(ar, nbytes * ARENA_GROW_FACTOR);
		return arena_alloc_align(ar, nbytes, alignment);
	}

	blk->offset += (aligned - cur) + nbytes;
	return &blk->data[aligned];
}

void
arena_reset(struct ArenaAllocator* ar){
	struct ArenaBlock* cur = ar->head;
	struct ArenaBlock* next = NULL;
	while(cur->next != NULL){
		cur->offset = 0;
		cur = cur->next;
	}
	cur->offset = 0;
}

void arena_block_destroy(struct ArenaBlock* b){
	ARENA_FREE(b->data);
	ARENA_FREE(b);
}

void
arena_destroy(struct ArenaAllocator* ar){
	struct ArenaBlock* cur = ar->head;
	struct ArenaBlock* next = NULL;
	while(cur->next != NULL){
		next = cur->next;
		arena_block_destroy(cur);
		cur = next;
	}
	arena_block_destroy(cur);
	ar->head = NULL;
	ar->block_count = 0;
	ar->total_capacity = 0;
}

#define arena_alloc(ar_ptr, T, n) \
	arena_alloc_align((ar_ptr), (sizeof(T) * (n)), alignof(T))

