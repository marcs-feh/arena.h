#include "test_urself.h"
#include <stdio.h>
#include "arena.c"

int test_arena(){
	Test_Begin("Arena Allocator");
	{   // Single node
		struct ArenaAllocator ar = arena_create(200);
		Test_Log("Blocks:%zu Total capacity: %zu", ar.block_count, ar.total_capacity);
		int n = 40;
		int* numbers = arena_alloc(&ar, int, n + 9);
		Tp(numbers != NULL);

		int* num0 = arena_alloc_align(&ar, sizeof(int), alignof(int));
		Tp(num0 != NULL);

		int* num1 = arena_alloc_align(&ar, sizeof(int), alignof(int));
		Tp(num1 != NULL);
		Tp(ar.block_count == 2);
		Test_Log("Blocks:%zu Total capacity: %zu", ar.block_count, ar.total_capacity);

		arena_reset(&ar);
		Tp(ar.head->offset == 0);

		int* num3 = arena_alloc_align(&ar, sizeof(int) * 30, alignof(int));
		Test_Log("Blocks:%zu Total capacity: %zu", ar.block_count, ar.total_capacity);
		Tp(ar.block_count == 2);

		Tp(num3 != NULL);

		arena_destroy(&ar);
	}

	Test_End();
}

int main(){
	int res = test_arena();
	return res;
}
