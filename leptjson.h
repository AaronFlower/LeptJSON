/**
 * 避免重复引入导致重复声明，利用宏加入 include 防范 (inclde guard).
 */
#ifndef LEPTJSON_H__
#define LEPTJSON_H__

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
	LEPT_PARSE_ROOT_NOT_SINGULAR 		// 空白还有其它字符。
};



/*
 * lept_parse - parse json 
 * @param lept_value* v : 接收解析后的树变量，由调用方传入。
 * @param const char* josn: C 字符串, 加上 const 避免函数更改。
 * 一般用法：
 		lept_value v;
 		const char json[] = ...;
 		int ret = lept_parse(&v, json);
 */
int lept_parse (lept_value* v, const char* json);


/**
 * lept_get_type
 * 获取 json value 的值。
 */
lept_type lept_get_type(const lept_value* v);

#endif
