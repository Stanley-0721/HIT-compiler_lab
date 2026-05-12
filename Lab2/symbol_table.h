#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#ifndef MAX_NAME
#define MAX_NAME 100
#endif

/* ================================================================
 *  语义分析 — 类型系统
 * ================================================================ */

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRUCT
} TypeKind;

typedef struct Type_ {
    TypeKind kind;
    char structName[MAX_NAME];  // TYPE_STRUCT 时的结构体名
    int isArray;                // 是否为数组
    struct Type_* elementType;  // 数组元素类型
    int arraySize;              // 本维长度
} Type;

Type* createType(TypeKind kind);
Type* createStructType(const char* name);
Type* createArrayType(Type* elemType, int size);
void freeType(Type* type);

/* ================================================================
 *  语义分析 — 符号表
 * ================================================================ */

typedef enum {
    SYM_VAR,      // 变量 / 参数
    SYM_FUNC,     // 函数
    SYM_STRUCT    // 结构体标签
} SymKind;

typedef struct Symbol_ {
    char name[MAX_NAME];
    Type* type;             // 变量/参数的类型；函数/结构体可为 NULL
    SymKind kind;
    int line;

    // 函数专用
    Type* returnType;
    struct Symbol_** params;
    int paramCount;

    // 结构体专用：成员字段
    struct Symbol_** fields;
    int fieldCount;
} Symbol;

typedef struct Scope_ {
    Symbol** symbols;
    int count;
    int capacity;
    struct Scope_* outer;   // 外层作用域，NULL = 全局
} Scope;

/* ---- 符号表操作 ---- */
void initSymbolTable(void);
void enterScope(void);
void exitScope(void);

Symbol* insertSymbol(const char* name, Type* type, SymKind kind, int line);
Symbol* lookupSymbol(const char* name);            // 从内向外查
Symbol* lookupSymbolInScope(const char* name);     // 仅查当前作用域

Symbol* lookupStructField(const char* structName, const char* fieldName);

void freeSymbolTable(void);

#endif
