#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leptjson.h"

/**
 * test.c
 * 包含一个极简的单元测试框架
 */

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format)\
	do {\
		test_count++;\
		if (equality)\
			test_pass++;\
		else {\
			fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
			main_ret = 1;\
		}\
	} while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

/**
 * static 函数的意思是指，该函数只作用于编译单元中，
 * 如果没有调用能被发现的, 编译时会报 [-Wunused-function] 。
 */
static void test_parse_expect_value () {
	lept_value v;

	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v, ""));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));

	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v, " "));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

#define TEST_LITERAL(expect, json)\
	do {\
		lept_value v;\
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
		EXPECT_EQ_INT(expect, lept_get_type(&v));\
	} while(0)


static void test_parse_literal () {
	TEST_LITERAL(LEPT_NULL, "null");
	TEST_LITERAL(LEPT_FALSE, "false");
	TEST_LITERAL(LEPT_TRUE, "true");
}

static void test_parse_invalid_value () {
	lept_value v;

	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "nul"));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));

	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "?"));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}
  
static void test_parse_root_not_singular () {
	lept_value v;
	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_ROOT_NOT_SINGULAR, lept_parse(&v, "null x"));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse () {
	test_parse_literal();
	test_parse_expect_value();
	test_parse_invalid_value();
	test_parse_root_not_singular();
}

int main () {
	test_parse();
	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
	return main_ret;
}
