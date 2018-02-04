#include "leptjson.h"
#include <assert.h> /* assert() */
#include <stdlib.h> /* NULL */
#include <string.h> /* strcpy */

#define EXPECT(c, ch) do { assert(*c->json == (ch)); c->json++; } while(0)


typedef struct {
	const char* json;
} lept_context;

/**
 * 文本解析时过滤掉其中的空白, 将指针移动到非空白字符的位置。
 */
static void lept_parse_whitespace(lept_context* c) {
	const char *p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
		p++;
	}
	c->json = p;
}

static int lept_parse_literal (lept_context* c, int expect_value, lept_value *v) {
	/**
	 * 程序员调用该函数时，应该保证所以期望的值是下面三个值之一。
	 * 所以使用断言。
	 */
	assert(expect_value == LEPT_NULL || expect_value == LEPT_FALSE || expect_value == LEPT_TRUE);

	char literal[6];
	int len, i;
	switch (expect_value) {
		case LEPT_NULL:
			strcpy(literal, "null");
			len = 4;
			break;
		case LEPT_FALSE:
			strcpy(literal, "false");
			len = 5;
			break;
		case LEPT_TRUE:
			strcpy(literal, "true");
			len = 4;
			break;
	}
	
	EXPECT(c, *literal);
	
	for (i = 1; i < len; ++i) {
		if (c->json[i -1] != literal[i]) {
			return LEPT_PARSE_INVALID_VALUE;	
		}
	}
	
	c->json += len - 1;

	v->type = expect_value;
	return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context *c, lept_value* v) {
	switch (*c->json) {
		case 'n': return lept_parse_literal(c, LEPT_NULL, v);
		case 'f': return lept_parse_literal(c, LEPT_FALSE, v);
		case 't': return lept_parse_literal(c, LEPT_TRUE, v);
		case '\0': return LEPT_PARSE_EXPECT_VALUE;
		default: return LEPT_PARSE_INVALID_VALUE;
	}
}

/**
 * 实现 API 函数 
 */
int lept_parse (lept_value* v, const char* json) {
	lept_context c;
	int ret;
	assert(v != NULL);
	c.json = json;
	v->type = LEPT_NULL;
	lept_parse_whitespace(&c);
	if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
		lept_parse_whitespace(&c);
		if (*c.json != '\0') {
			return LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	return ret; 
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}

