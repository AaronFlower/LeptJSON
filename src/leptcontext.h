#ifndef LEPT_CONTEXT_H__
#define LEPT_CONTEXT_H__

#ifndef LEPT_PARSE_STACK_INIT_SIZE 
#define LEPT_PARSE_STACK_INIT_SIZE 256 	// 使用者在编译时可以自行设置宏。
#endif

#include <stddef.h> // size_t
/**
 * lept_conctext 定义JSON字符串在解析时的上下文.
 */
typedef struct {
	const char* json;
	char* stack; 		// 栈
	size_t top;			// 栈顶
	size_t size;		// 栈容量
} lept_context;

/**
 * 入栈操作，仅做栈指针的移动，入栈的内容需要根据返回的指针在外面初始化。
 */
void* lept_context_push(lept_context* c, size_t len);

/**
 * 出栈操作
 */
void* lept_context_pop(lept_context* c, size_t len);


/**
 * 压入单个字符
 */
void put_c(lept_context* c, char ch);

#endif
