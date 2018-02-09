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
#define EXPECT_EQ_NULL(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.10g")
#define EXPECT_EQ_STRING(expect, actual, len) EXPECT_EQ_BASE(strncmp((expect), (actual), (len)) == 0, expect, actual, "%s")

#define EXPECT_EQ_BOOLEAN(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

#define TEST_LITERAL(expect, json)\
	do {\
		lept_value v;\
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
		EXPECT_EQ_INT(expect, lept_get_type(&v));\
	} while(0)


/**
 * static 函数的意思是指，该函数只作用于编译单元中，
 * 如果没有调用能被发现的, 编译时会报 [-Wunused-function] 。
 */
static void test_parse_literal () {
	TEST_LITERAL(LEPT_NULL, "null");
	TEST_LITERAL(LEPT_FALSE, "false");
	TEST_LITERAL(LEPT_TRUE, "true");
}

#define TEST_NUMBER(expect, json)\
	do {\
		lept_value v;\
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
		EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));\
	  EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));\
	} while(0)

static void test_parse_number () {
	TEST_NUMBER(0.0f, "0");
	TEST_NUMBER(0.0f, "-0");
	TEST_NUMBER(0.0f, "-0.0");
	TEST_NUMBER(1.0, "1");
	TEST_NUMBER(-1.0, "-1.0");
	TEST_NUMBER(1.5, "1.5");
	TEST_NUMBER(-1.5, "-1.5");
	TEST_NUMBER(3.1415, "3.1415");
	TEST_NUMBER(1E10, "1E10");
	TEST_NUMBER(1e10, "1e10");
	TEST_NUMBER(1E+10, "1E+10");
	TEST_NUMBER(1E-10, "1E-10");
	TEST_NUMBER(-1E10, "-1E10");
	TEST_NUMBER(-1e10, "-1e10");
	TEST_NUMBER(-1E+10, "-1E+10");
	TEST_NUMBER(-1E-10, "-1E-10");
	TEST_NUMBER(1.234E+10, "1.234E+10");
	TEST_NUMBER(1.234E-10, "1.234E-10");
	TEST_NUMBER(0.0f, "1e-10000"); /* must underflow  */
	TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
	TEST_NUMBER( 5.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
	TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
	TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
	TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
	TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
	TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
	TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
	TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_ERROR(error, json)\
	do {\
		lept_value v;\
		v.type = LEPT_FALSE;\
		EXPECT_EQ_INT(error, lept_parse(&v, json));\
		EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));\
	} while(0)


static void test_parse_invalid_value () {
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE,  "?");

#if 1
	/* invalid number */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE,  "+0");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE,  "+1");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE,  ".123");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE,  "1.");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE,  "INF");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE,  "inf");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE,  "NAN");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE,  "nan");
#endif
} 

static void test_parse_root_not_singular () {
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");

#if 1
	/* invalid number */

	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero shoule be '.' or nothing */
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");

#endif
}


static void test_parse_number_too_big() {
#if 1
	TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
	TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
#endif
}

#define TEST_STRING(expect, json) \
	do {\
		lept_value v;\
		lept_init(&v);\
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
		EXPECT_EQ_INT(LEPT_STRING, lept_get_type(&v));\
		EXPECT_EQ_STRING(expect, lept_get_string(&v), lept_get_string_length(&v));\
		lept_free(&v);\
	} while(0)

static void test_access_string () {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "", 0);
	EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v));
	lept_set_string(&v, "Hello", 5);
	EXPECT_EQ_STRING("Hello", lept_get_string(&v), lept_get_string_length(&v));
	lept_free(&v);
}

static void test_access_null () {
	lept_value v;
	lept_init(&v);
	EXPECT_EQ_NULL(LEPT_NULL, lept_get_null(&v));
}

static void test_access_boolean () {
	lept_value v;
	lept_init(&v);
	lept_set_boolean(&v, 1);
	EXPECT_EQ_BOOLEAN(LEPT_TRUE, lept_get_boolean(&v));
	lept_set_boolean(&v, 0);
	EXPECT_EQ_BOOLEAN(LEPT_FALSE, lept_get_boolean(&v));
}

static void test_access_number () {
	lept_value v;
	lept_init(&v);
	lept_set_number(&v, 3.1415);
	EXPECT_EQ_DOUBLE(3.1415, lept_get_number(&v));
	lept_set_number(&v, 2.718);
	EXPECT_EQ_DOUBLE(2.718, lept_get_number(&v));
}

static void test_parse_string () {
	TEST_STRING("", "\"\"");
	TEST_STRING("Hello", "\"Hello\"");
}

static void test_parse_expect_value () {
	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}

static void test_parse () {
	test_parse_literal();
	test_parse_number();
	test_parse_expect_value();
	test_parse_invalid_value();
	test_parse_root_not_singular();
	test_parse_number_too_big();
	test_parse_string();


	test_access_string();
	test_access_boolean();
	test_access_number();
	test_access_null();
}

int main () {
	test_parse();
	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
	return main_ret;
}