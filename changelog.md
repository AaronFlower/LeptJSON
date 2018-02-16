## Tutorial 03
### 数据类型

重新定义 `lept_value` 类型：
```c
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
```

增加三种解析错误类型：
```c
    LEPT_PARSE_MISS_QUOTATION_MARK,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR
```

### 方法
新增方法
```c
    #define lept_init(v) do { (v)->lept_type = NULL; } while(0)
    #define lept_set_null(v) lept_free(v)
    void lept_free(lept_value* v);
    void lept_set_number(lept_value *v, double n);
    int lept_get_boolean(const lept_value* v);
    void lept_set_boolean(lept_value *v, int b);
    const char* lept_get_string(const lept_value* v);
    void lept_set_string(lept_value* v, const char* s, size_t len);
    size_t lept_get_string_length(const lept_value* v);
```

### Tutorial 04

Happy Spring Festival.


