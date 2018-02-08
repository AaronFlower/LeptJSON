#include "leptjson.h"
#include <assert.h> /* assert() */
#include <stdlib.h> /* NULL */
#include <string.h> /* strcpy */
#include <math.h> /* HUGE_VALF, HUGE_VAL, HUGE_VALL */

#define EXPECT(c, ch) do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9') 
#define ISTOOBIG(n) ((n) == HUGE_VAL || (n) == -HUGE_VAL)

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256 	// 使用者在编译时可以自行设置宏。
#endif

/**
 * 解析过程中上下文的临时变量。可以缓存已经解析的结果。
 */
typedef struct {
	const char* json;
	char* stack;
	size_t size; 	// 堆栈的容量
	size_t top; 	// 栈顶位置。 
} lept_context;

/** 解析字符时的堆栈操作 **/
/* 
 * lept_context_push 入栈操作, 在解析字符串时, 每次入栈的大小都是 1。
 * 入栈进先考虑栈的容量的是否还允许，不允许的话则按 1.5 倍扩充。
 * 最后返回入栈的元素指针，供外面赋值。
 * 
 * @param c 		解析时的上下文。
 * @param len 	入栈的长度。
 *
 * @return ret 	返回入栈后的元素指针，供外面赋值。
 */
static void* lept_context_push (lept_context* c, size_t len) {
	void* ret;
	assert(len > 0);

	if (c->top + len >= c->size) {
		if (c->size == 0) {
			c->size = LEPT_PARSE_STACK_INIT_SIZE;
		}
		while (c->top + len >= c->size) {
			c->size += c->size >> 1;
		}
		c->stack = (char *)realloc(c->stack, c->size);
	}

	ret = c->stack + c->top;
	c->top += len;

	return ret;
}

/**
 * lept_context_pop 从栈中弹出元素。
 *
 * @param c 		解析时上下文。
 * @param len 	弹出长度.
 */
static void* lept_context_pop (lept_context* c, size_t len) {
	assert(len <= c->size); // size_t 类型不能为负，所以只考虑一种条件即可.

	c->top -= len;
	return c->stack + c->top;

}

/**
 * 向栈中压入合法字符.  当然我们也可以仅定义成一个宏来使用.
 * #define PUTC(c,ch) do { *(char*)lept_context_push((c), sizeof(char)) = (ch); } while(0)
 *
 * @param c 		解析时上下文。
 * @param ch 		要压入的字符。
 */
static void put_c (lept_context* c, char ch) {
	char* pc = (char *)lept_context_push(c, sizeof(char));
	*pc = ch;
}

/**
 * 解析字符串。
 */
static int lept_parse_string (lept_context* c, lept_value* v) {
	size_t head = c->top, len;
	const char* p;
	EXPECT(c, '\"');
	
	p = c->json;
	for (;;) {
		char ch = *p++;

		switch (ch) {
			case '\"':
				len = c->top - head;
				lept_set_string(v, (const char*)lept_context_pop(c, len), len);
				c->json = p;
				return LEPT_PARSE_OK;
			case '\0':
				c->top = head;
				return LEPT_PARSE_MISS_QUOTATION_MARK;
			default: 
				put_c(c, ch);
		}
	}
}

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
	size_t i;
	switch (expect_value) {
		case LEPT_NULL:
			strcpy(literal, "null");
			break;
		case LEPT_FALSE:
			strcpy(literal, "false");
			break;
		case LEPT_TRUE:
			strcpy(literal, "true");
			break;
	}
	
	EXPECT(c, *literal);
	
	for (i = 0; literal[i + 1]; ++i) {
		if (c->json[i] != literal[i + 1]) {
			return LEPT_PARSE_INVALID_VALUE;	
		}
	}
	
	c->json += i; 

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

/**
 * 解析数字
 */
static int lept_parse_number(lept_context *c, lept_value* v) {
	char *end;
	int checkRet;
	if ((checkRet = lept_validate_number(c, end)) != LEPT_PARSE_OK) {
		return checkRet;
	}
	
	v->u.n = strtod(c->json, &end);

	if (c->json == end) {
		return LEPT_PARSE_INVALID_VALUE;
	}
	
	if (ISTOOBIG(v->u.n)) {
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
		case '"': return lept_parse_string(c, v);
		default: return lept_parse_number(c, v);
		case '\0': return LEPT_PARSE_EXPECT_VALUE;
	}
}

/**
 * 实现 API 函数 
 */
int lept_parse (lept_value* v, const char* json) {
	assert(v != NULL);

	int ret;
	
	// 每次解析时，创建一个上下文，并且初始化缓冲区。
	lept_context c;
	c.json = json;
	c.stack = NULL;
	c.size = c.top = 0;

	lept_init(v);
	lept_parse_whitespace(&c);

	if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
		lept_parse_whitespace(&c);
		if (*c.json != '\0') {
			return LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	
	// 最终释放堆栈, 释放时确保所有的数据都被弹出。
	assert(c.top == 0);
	free(c.stack);

	return ret; 
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}


double lept_get_number(const lept_value *v) {
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->u.n;
}

void lept_free (lept_value *v) {
	assert(v != NULL);

	if (v->type == LEPT_STRING) {
		free(v->u.s.s);
	}
	
	v->type = LEPT_NULL;
}

void lept_set_string (lept_value *v, const char *s, size_t len) {
	assert(v != NULL && (s != NULL || len == 0));

	lept_free(v);

	v->u.s.s = (char*)malloc(len + 1); // 要多放置一个 '\0' 的位置。
	memcpy(v->u.s.s, s, len);
	v->u.s.s[len] = '\0';
	v->u.s.len = len;
	v->type = LEPT_STRING;
}

const char* lept_get_string (const lept_value *v) {
	assert(v != NULL && v->type == LEPT_STRING);

	return v->u.s.s;
}

size_t lept_get_string_length (const lept_value *v) {
	assert(v != NULL && v->type == LEPT_STRING);

	return v->u.s.len;
}
