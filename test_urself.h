#ifndef _test_urself_h_include_
#define _test_urself_h_include_

#include <stdio.h>
#include <stdbool.h>

#ifndef TEST_URSELF_COLORLESS
#define TEST_COL_FAIL  "\e[0;31m"
#define TEST_COL_PASS  "\e[0;32m"
#define TEST_COL_TITLE "\e[1;37m"
#define TEST_COL_RESET "\e[0m"
#else
#define TEST_COL_FAIL  ""
#define TEST_COL_PASS  ""
#define TEST_COL_TITLE ""
#define TEST_COL_RESET ""
#endif

struct tu_Test {
	const char* title;
	int tests_count;
	int error_count;
};

static
void tu_displayHeader(struct tu_Test* t){
	printf("[" TEST_COL_TITLE "%s" TEST_COL_RESET "]\n", t->title);
}

static
void tu_assert(struct tu_Test* t, bool expr, const char* msg){
	t->tests_count += 1;
	if(!expr){
		t->error_count += 1;
		printf("  Failed: %s\n", msg);
	}
}

static
void tu_displayResults(struct tu_Test* t){
	if(t->error_count > 0){
		printf(TEST_COL_FAIL "FAIL" TEST_COL_RESET);
	} else {
		printf(TEST_COL_PASS "PASS" TEST_COL_RESET);
	}
	printf(" ok in %u/%u\n", t->tests_count - t->error_count, t->tests_count);
}

#define Test_Begin(title_) \
	struct tu_Test _test_ = { .title = title_, .tests_count = 0, .error_count = 0}; \
	tu_displayHeader(&_test_);

#define Test_End() tu_displayResults(&_test_); \
	return _test_.error_count;

#define Test_Log(fmt, ...) printf("  >> " fmt "\n", __VA_ARGS__);

// Test predicate.
#define Tp(expr) { tu_assert(&_test_, (expr), #expr); }

#endif /* Include guard */
