#include "leptjson.h"
#include <assert.h> /* assert() */
#include <stdlib.h> /* NULL */
#include <string.h> /* strcpy */
#include <math.h> /* HUGE_VALF, HUGE_VAL, HUGE_VALL */

#define EXPECT(c, ch) do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9') 
#define ISTOOBIG(n) ((n) == HUGE_VAL || (n) == -HUGE_VAL)


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

/**
 * 将指针移动到非数字位置
 */
static void lept_ignore_digit (const char** p) {
	assert(p != NULL);
	while (ISDIGIT(**p)) {
		++*p;
	}
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

/**
 * 检查函数是否正确。LEPT_PARSE_OK，LEPT_PARSE_INVALID_VALUE，LEPT_PARSE_ROOT_NOT_SINGULAR 。
 * @param c 
 * @param end - 数字结束位置
 */
static int lept_validate_number(lept_context *c, const char *end) {
	const char* p = c->json;
	
	// pass sign, 过滤掉符号
	if (*p == '-') {
		p ++;
	}

	// check integer part 整数部分
	if (ISDIGIT(*p)) {
		if (*p == '0') {
			++p;
		} else {
			lept_ignore_digit(&p);
		}
	} else {
		return LEPT_PARSE_INVALID_VALUE;
	}	

	if (*p == '.' || *p == 'e' || *p == 'E') {
		// check fractional part
		if (*p == '.') {
			++p;
			if (ISDIGIT(*p)) {
				lept_ignore_digit(&p);
			}	else {
				return LEPT_PARSE_INVALID_VALUE;		
			}
		}

		if (*p == 'e' || *p == 'E') {
			++p;
			if (*p == '+' || *p == '-') {
				++p;
			}
			if (ISDIGIT(*p)) {
				lept_ignore_digit(&p);
			} else {
				return LEPT_PARSE_INVALID_VALUE;
			}
		}
	}

	if (*p != '\0') {
		return LEPT_PARSE_ROOT_NOT_SINGULAR;
	}
	return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context *c, lept_value* v) {
	char *end;
	int checkRet;
	if ((checkRet = lept_validate_number(c, end)) != LEPT_PARSE_OK) {
		return checkRet;
	}
	
	v->n = strtod(c->json, &end);

	if (c->json == end) {
		return LEPT_PARSE_INVALID_VALUE;
	}
	
	if (ISTOOBIG(v->n)) {
		return LEPT_PARSE_NUMBER_TOO_BIG;
	}
	c->json = end;
	v->type = LEPT_NUMBER;
	return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context *c, lept_value* v) {
	switch (*c->json) {
		case 'n': return lept_parse_literal(c, LEPT_NULL, v);
		case 'f': return lept_parse_literal(c, LEPT_FALSE, v);
		case 't': return lept_parse_literal(c, LEPT_TRUE, v);
		default: return lept_parse_number(c, v);
		case '\0': return LEPT_PARSE_EXPECT_VALUE;
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


double lept_get_number(const lept_value *v) {
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->n;
}

