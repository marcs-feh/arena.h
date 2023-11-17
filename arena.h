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

/// Default way for arena to get memory
static void* arena_default_mem_alloc(void* _, size_t n){
	(void)_; // Just so the compiler shuts up about it not being used
	return malloc(n);
}

/// Default way for arena to release memory
static void arena_default_mem_free(void* _, void* p){
	(void)_; // Just so the compiler shuts up about it not being used
	free(p);
}

/// How much to grow the arena (relative to required size) when making new blocks
#define ARENA_GROW_FACTOR 1.15

/// Helper macro, you can safely remove it if you don't want to use it
#define arena_alloc(ar_ptr, T, n) \
	arena_alloc_raw((ar_ptr), (sizeof(T) * (n)), alignof(T))

/// Declarations ///////////////////////////////////////////////////////////////

#define byte unsigned char

typedef void* (*ArenaMemAllocProc) (void*, size_t);
typedef void (*ArenaMemFreeProc) (void*, void*);

struct ArenaBlock {
	byte* data;
	size_t offset;
	size_t capacity;

	struct ArenaBlock* next;
};

struct ArenaAllocator {
	ArenaMemAllocProc mem_alloc;
	ArenaMemFreeProc mem_free;
	struct ArenaBlock* head;
};

/// Creates an arena.
// Use alloc_proc = NULL and free_proc = NULL to use the default functions from
// the Configuration section
struct ArenaAllocator arena_create(ArenaMemAllocProc alloc_proc,
                                   ArenaMemFreeProc free_proc,
                                   size_t capacity);

/// Destroys an arena, freeing all blocks.
void arena_destroy(struct ArenaAllocator* ar);

/// Allocates a chunk of raw memory of size nbytes, pointer aligned to alignment
// Will try to grow arena if needed. Returns NULL on failed allocation.
void* arena_alloc_raw(struct ArenaAllocator* ar,
                      size_t nbytes,
                      size_t alignment);

/// Resets arena, marking all blocks as free.
// Does not release resources back
void arena_reset(struct ArenaAllocator* ar);

/// Get combined capacity of all memory blocks available in the arena.
size_t arena_total_capacity(struct ArenaAllocator const* ar);

/// Get how many memory blocks arr in the arena.
size_t arena_block_count(struct ArenaAllocator const* ar);

/// Push a new block to the arena. Can be used to preemptively reserve space.
// Returns false on failure.
bool arena_push_block(struct ArenaAllocator* ar, size_t capacity);

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
arena_block_create(struct ArenaAllocator* ar, size_t capacity){
	struct ArenaBlock *blk = ar->mem_alloc(NULL, sizeof(*blk));
	if(blk == NULL){ return blk; }

	void* data = ar->mem_alloc(NULL, capacity);
	if(data == NULL){
		ar->mem_free(NULL, blk);
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
arena_create(ArenaMemAllocProc mem_alloc_proc, ArenaMemFreeProc mem_free_proc, size_t capacity){
	ArenaMemAllocProc alloc_proc = mem_alloc_proc;
	ArenaMemFreeProc free_proc = mem_free_proc;

	if(alloc_proc == NULL){
		alloc_proc = arena_default_mem_alloc;
	}
	if(free_proc == NULL){
		free_proc = arena_default_mem_free;
	}

	struct ArenaAllocator ar = {
		.mem_alloc = alloc_proc,
		.mem_free = free_proc,
	};
	struct ArenaBlock* blk = arena_block_create(&ar, capacity);

	ar.head = blk;

	return ar;
}

bool
arena_push_block(struct ArenaAllocator* ar, size_t capacity){
	struct ArenaBlock *blk = arena_block_create(ar, capacity);
	if(blk == NULL){ return false; }

	blk->next = ar->head;
	ar->head = blk;

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
	bool ok = arena_push_block(ar, new_cap * ARENA_GROW_FACTOR);
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
arena_block_destroy(struct ArenaAllocator* ar, struct ArenaBlock* b){
	ar->mem_free(NULL, b->data);
	ar->mem_free(NULL, b);
}

void
arena_destroy(struct ArenaAllocator* ar){
	struct ArenaBlock* cur = ar->head;
	struct ArenaBlock* next = NULL;
	while(cur->next != NULL){
		next = cur->next;
		arena_block_destroy(ar, cur);
		cur = next;
	}
	arena_block_destroy(ar, cur);
	ar->head = NULL;
}

size_t
arena_block_count(struct ArenaAllocator const* ar){
	struct ArenaBlock* blk = ar->head;
	size_t total = 0;

	while(blk != NULL){
		total += 1;
		blk = blk->next;
	}

	return total;
}

size_t
arena_total_capacity(struct ArenaAllocator const* ar){
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

/*
	Copyright 2023 marcs-feh
	
	Permission is hereby granted, free of charge, to any person obtaining a copy of
	this software and associated documentation files (the “Software”), to deal in
	the Software without restriction, including without limitation the rights to
	use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
	of the Software, and to permit persons to whom the Software is furnished to do
	so.
	
	THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/
