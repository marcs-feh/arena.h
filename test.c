#include "test_urself.h"
#include <stdio.h>

#define ARENA_IMPLEMENTATION
#include "arena.h"

int test_arena(){
	Test_Begin("Arena Allocator");
	{   // Single node
		struct ArenaAllocator ar = arena_create(0, 0, 200);
		// Test_Log("Blocks: %zu Total capacity: %zu", arena_block_count(&ar), arena_total_capacity(&ar));
		int n = 40;
		int* numbers = arena_alloc(&ar, int, n + 9);
		Tp(numbers != NULL);
		numbers[49-1] = 4;
		// printf("Base: %p Nums: %p Max: %p\n", ar.head->data, numbers, ar.head->data + ar.head->capacity);

		int* num0 = arena_alloc_raw(&ar, sizeof(int), alignof(int));
		Tp(num0 != NULL);
		*num0 = 69;
		// Test_Log("Blocks: %zu Total capacity: %zu", arena_block_count(&ar), arena_total_capacity(&ar));
		// printf("Base: %p Num0: %p Max: %p\n", ar.head->data, num0, ar.head->data + ar.head->capacity);

		int* num1 = arena_alloc_raw(&ar, sizeof(int), alignof(int));
		Tp(num1 != NULL);
		*num1 = 420;
		Tp(arena_block_count(&ar) == 2);
		// Test_Log("Blocks: %zu Total capacity: %zu", arena_block_count(&ar), arena_total_capacity(&ar));
		// printf("Base: %p Num1: %p Max: %p\n", ar.head->data, num1, ar.head->data + ar.head->capacity);

		arena_reset(&ar);
		Tp(ar.head->offset == 0);

		int* num2 = arena_alloc_raw(&ar, sizeof(int) * 30, alignof(int));
		Tp(num2 != NULL);
		Tp(arena_block_count(&ar) == 2);
		// Test_Log("Blocks: %zu Total capacity: %zu", arena_block_count(&ar), arena_total_capacity(&ar));
		*num2 = 1;

		arena_destroy(&ar);
	}

	Test_End();
}

int main(){
	int res = test_arena();
	return res;
}
