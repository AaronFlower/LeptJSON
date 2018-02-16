/**
 * 避免重复引入导致重复声明，利用宏加入 include 防范 (inclde guard).
 */
#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stddef.h> // size_t

/**
 * JSON 数据类型。其中 true, false 分别当作一种类型。
 * 另外，因为 C 语言没有 C++ 的 namespace 概念，所以前面加上一个标识。
 */
typedef enum { 
	LEPT_NULL,
	LEPT_FALSE,
	LEPT_TRUE,
	LEPT_NUMBER,
	LEPT_STRING,
	LEPT_ARRAY,
	LEPT_OBJECT
} lept_type;

/**
 * JSON 数据树结构
 */
typedef struct {
	union {
		double n;
		struct {
			char* s;
			size_t len;
		} s;
	} u;

	lept_type type;
} lept_value;


// API

/**
 * 定义返回值类型。无错误返回 LEPT_PARSE_OK.
 */
enum {
	LEPT_PARSE_OK = 0,
	LEPT_PARSE_EXPECT_VALUE, 		// JSON 只含有空白。
	LEPT_PARSE_INVALID_VALUE,		// 非法的字面值。	
	LEPT_PARSE_ROOT_NOT_SINGULAR, 		// 空白还有其它字符。
	LEPT_PARSE_NUMBER_TOO_BIG,
	LEPT_PARSE_MISS_QUOTATION_MARK,
	LEPT_PARSE_INVALID_STRING_ESCAPE,
	LEPT_PARSE_INVALID_STRING_CHAR,
	LEPT_PARSE_INVALID_UNICODE_HEX,
	LEPT_PARSE_INVALID_UNICODE_SURROGATE
};

#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)
#define lept_set_null(v) lept_free(v)

/*
 * lept_parse - parse json 
 * @param lept_value* v : 接收解析后的树变量，由调用方传入。
 * @param const char* json: C 字符串, 加上 const 避免函数更改。
 * 一般用法：
 		lept_value v;
 		const char json[] = ...;
 		int ret = lept_parse(&v, json);
 */
int lept_parse (lept_value* v, const char* json);

/*
 * 释放内存
 */
void lept_free(lept_value* v);

/**
 * lept_get_type
 * 获取 json value 的值。
 */
lept_type lept_get_type(const lept_value* v);


/**
 * lept_gett_nubmer
 * 当 lept_type 为 LEPT_NUMBER 时返回 n;
 */
double lept_get_number (const lept_value* v);
void lept_set_number(lept_value *v, double n);

int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value *v, int b);

const char* lept_get_string(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);
size_t lept_get_string_length(const lept_value* v);

lept_type lept_get_null(const lept_value* v);
#endif
