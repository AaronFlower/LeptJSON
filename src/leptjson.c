#include "leptjson.h"
#include <assert.h> /* assert() */
#include <stdlib.h> /* NULL */
#include <string.h> /* strcpy */
#include <math.h> /* HUGE_VALF, HUGE_VAL, HUGE_VALL */
#include <stdio.h>

#define EXPECT(c, ch) do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISHEX(ch) (ISDIGIT(ch) || ((ch) >= 'A' && (ch) <= ('F')))
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

#define IS_UNICODE_HEX(p) (ISHEX(*p) && ISHEX(*(p + 1)) && ISHEX(*(p + 2)) && ISHEX(*(p + 3))) 

/**
 * 将 \Uxxxx 的十六进制数转换成码点 (code point)。
 *
 * @param p			待解析的十六进制序列。
 * @param u 		用于接收码点值。
 *
 * @return 			正确返回下一个需要解析的字符，错误则返回 NULL.
 */
static const char* lept_parse_hex4(const char* p, unsigned* u) {
	assert(p != NULL && u != NULL);	

	int i;
	char ch;
	*u = 0;
	for(i = 0; i <= 3; ++i) {
		*u = *u << 4;
		ch = *p++;
		if (ch >= '0' && ch <= '9') {
			*u |= (ch - '0');
		} else if (ch >= 'A' && ch <= 'F') {
			*u |= (10 + (ch - 'A'));
		} else if (ch >= 'a' && ch <= 'f') {
			*u |= (10 + (ch - 'a'));
		} else {
			return NULL;
		}
	}	
	return p;
}

/**
 * 将解析出的码点 code point 转存到 lept_context 中。
 * @param c			解析上下文
 * @param u 		码点
 * @return			无返回值
 */
static void lept_encode_utf8(lept_context* c, unsigned u) {
	assert(u >= 0x0 && u <= 0x10FFFF);
	
	if (u <= 0x7F) {
		put_c(c, u & 0xFF);
	} else if (u >= 0x80 && u <= 0x7FF) {
		put_c(c, 0xC0 | (u >> 6));
		put_c(c, 0x80 | (u & 0x3F));
	} else if (u >=0x800 && u <= 0xFFFF) {
		put_c(c, 0xE0 | ((u >> 12) & 0xFF));	
		put_c(c, 0x80 | ((u >>  6) & 0x3F));
		put_c(c, 0x80 | (u & 0x3F));
	} else {
		put_c(c, 0xF0 | ((u >> 18) & 0xFF));
		put_c(c, 0xE0 | ((u >> 12) & 0x3F));	
		put_c(c, 0x80 | ((u >>  6) & 0x3F));
		put_c(c, 0x80 | (u & 0x3F));
	}
}

#define STRING_ERROR(c, ret) do { (c)->top = head; return (ret); } while (0);
/**
 * 解析字符串。
 */
static int lept_parse_string (lept_context* c, lept_value* v) {
	size_t head = c->top, len;
	const char* p;
	unsigned high, low, u;
	EXPECT(c, '\"');
	
	p = c->json;
	for (;;) {
		char ch = *p++;

		switch (ch) {
			case '\\':
				switch (*p++) {
					case '\"': put_c(c, '\"'); break;
					case '\\': put_c(c, '\\'); break;
					case '/': put_c(c, '/'); break;
					case 'b': put_c(c, '\b'); break;
					case 'f': put_c(c, '\f'); break;
					case 'n': put_c(c, '\n'); break;
					case 'r': put_c(c, '\r'); break;
					case 't': put_c(c, '\t'); break;
					case 'u':
						if (!(p = lept_parse_hex4(p, &high))) {
							STRING_ERROR(c, LEPT_PARSE_INVALID_UNICODE_HEX);
						}
						 
						// Surrogate Check
						if (high >= 0xD800 && high <= 0xDBFF) {
							if (*p == '\\' && *(p + 1) == 'u') {
								if (!(p = lept_parse_hex4(p, &low))) {
									STRING_ERROR(c, LEPT_PARSE_INVALID_UNICODE_SURROGATE);
								}
								if (low >= 0xDC00 && low <= 0xDFFF) {
									u = 0x10000 + (high - 0xD800) * 0x400 + (low - 0xDC00);
								} else {
									STRING_ERROR(c, LEPT_PARSE_INVALID_UNICODE_SURROGATE);
								}
							} else {
								STRING_ERROR(c, LEPT_PARSE_INVALID_UNICODE_SURROGATE);
							}	
						} else if (high >= 0xDC00 && high <= 0xDFFF) {
							STRING_ERROR(c, LEPT_PARSE_INVALID_UNICODE_SURROGATE);
						} else {
							u = high; 	
						}
						lept_encode_utf8(c, u);
						break;
					default:
						STRING_ERROR(c, LEPT_PARSE_INVALID_STRING_ESCAPE);
				};
				break;
			case '\"':
				len = c->top - head;
				lept_set_string(v, (const char*)lept_context_pop(c, len), len);
				c->json = p;
				return LEPT_PARSE_OK;
			case '\0':
				c->top = head;
				STRING_ERROR(c, LEPT_PARSE_MISS_QUOTATION_MARK);
			default:
				if ((unsigned char)ch < 0x20) {
					c->top = head;
					STRING_ERROR(c, LEPT_PARSE_INVALID_STRING_CHAR);
				}
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


void lept_set_number(lept_value* v, double d) {
	lept_free(v);

	v->type = LEPT_NUMBER;
	v->u.n = d;
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

int lept_get_boolean(const lept_value* v) {
	assert(v != NULL && (v->type == LEPT_FALSE || v->type == LEPT_TRUE));
	return v->type;
}

/**
 * 在类型改变时，应该尝试去释放之前可能分配的内存。
 * 释放内存的时候会用断言判断 v 是否为 NULL.
 */
void lept_set_boolean(lept_value* v, int b) {
	lept_free(v);
	v->type = b ? LEPT_TRUE : LEPT_FALSE;
}


lept_type lept_get_null(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_NULL);
	return LEPT_NULL;
}
