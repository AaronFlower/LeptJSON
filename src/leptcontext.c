#include "leptcontext.h"
#include <assert.h> /* assert() */
#include <stdlib.h> /* NULL, realloc */
/* 
 * lept_context_push 入栈操作, 在解析JSON时, 根据每次入栈的大小,
 * 先考虑栈的容量的是否还允许，不允许的话则按 1.5 倍扩充。 
 * 函数仅仅使用 realloc 函数，因为 realloc 函数是支持初始值为 NULL 的。
 * 最后返回入栈的元素指针，供外面赋值。
 * 
 * @param c 		解析时的上下文。
 * @param len 	入栈的长度。
 *
 * @return 返回入栈后的元素指针，供外面赋值。
 */
void* lept_context_push(lept_context* c, size_t len) {
	if (c->top + len >= c->size) {
		if (c->size == 0) {
			c->size = LEPT_PARSE_STACK_INIT_SIZE;
		}
		
		while(c->top + len >= c->size) {
			c->size += c->size >> 1;
		}
		c->stack = (char*)realloc(c->stack, c->size);
	}
	
	c->top += len;

	return c->stack + c->top - len;
}

/**
 * lept_context_pop 从栈中弹出元素。
 *
 * @param c 		解析时上下文。
 * @param len 	弹出长度.
 */
void* lept_context_pop(lept_context* c, size_t len) {
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
void put_c(lept_context* c, char ch) {
	assert(c != NULL);
	*(char *)lept_context_push(c, sizeof(ch)) = ch;
}
