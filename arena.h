/* See end of file for LICENSE information */

/// Arena.h
// This file implements a memory arena that dynamically grows using the provided
// ARENA_MALLOC and ARENA_FREE functions. You cannot free individual allocations
// but you can free the entire arena all at once. To use this library you must
// #define ARENA_IMPLEMENTATION **once** in any C file of your choosing.

#ifndef _arena_h_included_
#define _arena_h_included_

#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>
#include <stdbool.h>

#include <stdlib.h> /* Remove if not using malloc() and free() in the configuration */

/// Configuration //////////////////////////////////////////////////////////////
// This defines how to ask for x bytes of memory
#define ARENA_MALLOC(x) malloc((x))

// This defines how to free a pointer p
#define ARENA_FREE(p) free((p))

// Default alignment for arena_alloc
#define ARENA_DEF_ALIGN (alignof(max_align_t))

// How much to grow the arena (relative to required size) when making new blocks
#define ARENA_GROW_FACTOR 1.15

// Helper macro, you can safely remove it
#define arena_alloc(ar_ptr, T, n) \
	arena_alloc_raw((ar_ptr), (sizeof(T) * (n)), alignof(T))

/// Declarations ///////////////////////////////////////////////////////////////
#define byte unsigned char

struct ArenaBlock {
	byte* data;
	size_t offset;
	size_t capacity;

	struct ArenaBlock* next;
};

struct ArenaAllocator {
	size_t block_count;
	struct ArenaBlock* head;
};

struct ArenaAllocator arena_create(size_t capacity);
void arena_destroy(struct ArenaAllocator* ar);
void* arena_alloc_raw(struct ArenaAllocator* ar, size_t nbytes, size_t alignment);
void arena_reset(struct ArenaAllocator* ar);
size_t arena_total_capacity(struct ArenaAllocator const* ar);

/// Implementation /////////////////////////////////////////////////////////////
#ifdef ARENA_IMPLEMENTATION
static uintptr_t 
align_forward_ptr(uintptr_t p, uintptr_t a){
	uintptr_t mod = p % a;

	if(mod > 0){
		p += (a - mod);
	}

	return p;
}

static size_t 
align_forward_size(size_t p, size_t a){
	size_t mod = p % a;

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
		.head = blk,
	};

	exit:
	return ar;
}

static bool
arena_push_block(struct ArenaAllocator* ar, size_t capacity){
	struct ArenaBlock *blk = arena_block_create(capacity);
	if(blk == NULL){ return false; }

	blk->next = ar->head;
	ar->head = blk;

	ar->block_count += 1;

	return true;
}

static uintptr_t
arena_block_get_required(uintptr_t cur, size_t nbytes, size_t alignment){
	uintptr_t aligned   = align_forward_ptr(cur, alignment);
	uintptr_t padding   = (aligned - cur);

	uintptr_t required  = padding + nbytes;
	return required;
}

static void*
arena_block_alloc_raw(struct ArenaBlock* blk, size_t nbytes, size_t alignment){
	uintptr_t base = (uintptr_t)blk->data;
	uintptr_t cur = base + blk->offset;

	uintptr_t available = blk->capacity - (cur - base);
	uintptr_t required  = arena_block_get_required(cur, nbytes, alignment);

	if(required > available){
		return NULL;
	}

	blk->offset += required;
	return &blk->data[blk->offset - nbytes];
}

void*
arena_alloc_raw(struct ArenaAllocator* ar, size_t nbytes, size_t alignment){
	if(nbytes == 0){ return NULL; }
	struct ArenaBlock* blk = ar->head;
	
	while(blk != NULL){
		void* p = arena_block_alloc_raw(blk, nbytes, alignment);
		if(p != NULL){ return p; }
		blk = blk->next;
	}

	// No block with enough space found, create new one
	size_t new_cap = align_forward_size(nbytes, alignment);
	bool ok = arena_push_block(ar, new_cap * ARENA_DEF_ALIGN);
	if(ok){
		return arena_alloc_raw(ar, nbytes, alignment);
	} else {
		return NULL;
	}
}

void
arena_reset(struct ArenaAllocator* ar){
	struct ArenaBlock* cur = ar->head;
	while(cur->next != NULL){
		cur->offset = 0;
		cur = cur->next;
	}
	cur->offset = 0;
}

static void
arena_block_destroy(struct ArenaBlock* b){
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
}

size_t arena_total_capacity(struct ArenaAllocator const* ar){
	struct ArenaBlock* blk = ar->head;
	size_t total = 0;

	while(blk != NULL){
		total += blk->capacity;
		blk = blk->next;
	}

	return total;
}

#undef byte

#endif /* ARENA_IMPLEMENTATION */
#endif /* Include guard */


// Copyright 2023 marcs-feh
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the “Software”), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
