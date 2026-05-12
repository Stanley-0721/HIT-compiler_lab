Lab2 语义分析 — 关键数据结构与函数
====================================

/**
 * 基础类型枚举
 */
typedef enum {
    TYPE_INT,      /** 整型         */
    TYPE_FLOAT,    /** 浮点型       */
    TYPE_STRUCT    /** 结构体类型   */
} TypeKind;

/**
 * 类型描述符
 *
 * 支持基本类型（int/float）、结构体类型、以及多维数组。
 * 数组通过 elementType 递归链接，每层 isArray=1 表示一个维度。
 *
 * 例：int a[10][20]
 *   Type { kind=INT, isArray=1, arraySize=10,
 *          elementType -> Type { kind=INT, isArray=1, arraySize=20,
 *                                elementType -> Type { kind=INT } } }
 */
typedef struct Type_ {
    TypeKind kind;              /** 基本类型                   */
    char structName[MAX_NAME];  /** 结构体标签名（TYPE_STRUCT） */
    int isArray;                /** 是否数组                   */
    struct Type_* elementType;  /** 数组元素类型（递归链）      */
    int arraySize;              /** 本维长度                   */
} Type;

/**
 * 创建基本类型（TYPE_INT / TYPE_FLOAT）
 * @param kind  类型枚举值
 * @return      新分配的 Type*，调用者负责释放
 */
Type* createType(TypeKind kind);

/**
 * 创建结构体类型
 * @param name  结构体标签名
 * @return      kind=TYPE_STRUCT 的 Type*
 */
Type* createStructType(const char* name);

/**
 * 创建数组类型，包装已有元素类型
 * @param elemType  元素类型（接管所有权）
 * @param size      数组长度
 * @return          新数组类型，isArray=1
 */
Type* createArrayType(Type* elemType, int size);

/**
 * 深拷贝 Type（含 elementType 递归链）
 *
 * 用于 insertSymbol 前创建独立副本，
 * 避免多个符号共享同一 Type* 导致 double free。
 * @param type  源类型（可为 NULL）
 * @return      独立副本
 */
Type* copyType(Type* type);

/**
 * 递归释放 Type 及其 elementType 链
 * @param type  待释放的类型（可为 NULL）
 */
void freeType(Type* type);

/**
 * 符号种类
 */
typedef enum {
    SYM_VAR,      /** 变量 / 函数形参 / 结构体字段   */
    SYM_FUNC,     /** 函数                           */
    SYM_STRUCT    /** 结构体标签                     */
} SymKind;

/**
 * 符号表条目
 *
 * 不同 kind 使用的字段：
 *   SYM_VAR:    type 有效
 *   SYM_FUNC:   returnType、paramTypes、paramCount 有效
 *   SYM_STRUCT: fields、fieldCount 有效
 */
typedef struct Symbol_ {
    char name[MAX_NAME];        /** 标识符名                         */
    Type* type;                 /** 变量/参数的类型                  */
    SymKind kind;               /** 符号种类                         */
    int line;                   /** 定义所在行号                     */

    Type* returnType;           /** 函数返回值类型 (SYM_FUNC)        */
    Type** paramTypes;          /** 函数形参类型数组 (SYM_FUNC)      */
    int paramCount;             /** 形参个数                         */

    struct Symbol_** fields;    /** 结构体字段数组 (SYM_STRUCT)      */
    int fieldCount;             /** 字段个数                         */
} Symbol;

/**
 * 单层作用域
 *
 * 通过 outer 指针形成栈式作用域链：
 *   当前作用域 → 外层作用域 → ... → 全局作用域 (outer==NULL)
 *
 * lookupSymbol() 从当前作用域沿 outer 链向外查找。
 */
typedef struct Scope_ {
    Symbol** symbols;           /** 符号动态数组                     */
    int count;                  /** 当前符号数                       */
    int capacity;               /** 已分配容量                       */
    struct Scope_* outer;       /** 外层作用域，NULL = 全局          */
} Scope;

/**
 * 初始化符号表，创建全局作用域
 */
void initSymbolTable(void);

/**
 * 进入 / 离开一层作用域
 *
 * exitScope() 会释放该层所有符号及其类型、形参类型、字段。
 * 全局作用域不会被 exitScope 释放（需调用 freeSymbolTable）。
 */
void enterScope(void);
void exitScope(void);

/**
 * 在当前作用域插入符号
 *
 * 同名符号已存在时返回 NULL，不打印错误（由调用方 semantic.c 处理）。
 * 成功后接管 type 的所有权。
 *
 * @param name  符号名
 * @param type  类型（可为 NULL，如函数/结构体）
 * @param kind  符号种类
 * @param line  定义行号
 * @return      新 Symbol*，重复时返回 NULL
 */
Symbol* insertSymbol(const char* name, Type* type, SymKind kind, int line);

/**
 * 从当前作用域出发，沿 outer 链向外查找符号
 * @param name  符号名
 * @return      找到的 Symbol*，未找到返回 NULL
 */
Symbol* lookupSymbol(const char* name);

/**
 * 仅在当前作用域查找（不沿 outer 链向外）
 * @param name  符号名
 * @return      找到的 Symbol*，未找到返回 NULL
 */
Symbol* lookupSymbolInScope(const char* name);

/**
 * 在指定结构体的字段列表中查找字段
 * @param structName  结构体标签名
 * @param fieldName   字段名
 * @return            字段 Symbol*，未找到返回 NULL
 */
Symbol* lookupStructField(const char* structName, const char* fieldName);

/**
 * 释放整个符号表（所有作用域及符号）
 */
void freeSymbolTable(void);

/**
 * 语义分析入口
 *
 * 初始化符号表 → 递归遍历 AST → 进行语义检查 → 释放符号表
 * @param root  语法树根节点
 */
void semanticAnalysis(Node* root);

/**
 * 判断语义分析阶段是否检测到错误
 * @return  非零 = 有错误
 */
int hasSemanticError(void);

/**
 * AST 递归遍历核心
 *
 * 按节点名分派：
 *   ExtDef → 全局定义     → processExtDef()
 *   Def    → 局部变量定义 → processDef()
 *   CompSt → 嵌套块       → enterScope() / exitScope()
 *   Exp    → 表达式       → checkExp()
 *   Stmt   → 语句         → RETURN 返回类型检查
 *   其他   → 递归处理子节点
 *
 * @param node  当前 AST 节点
 */
static void traverse(Node* node);

/**
 * 表达式类型推断 & 语义错误检测
 *
 * 按表达式结构分派，返回表达式计算结果类型：
 *
 *   Exp → ID               变量引用 → 查符号表 → 未找到: Error type 1
 *   Exp → INT / FLOAT      字面量   → 返回对应类型
 *   Exp → ID LP Args RP    函数调用 → 未找到: Error type 2
 *                                     → 不是函数: Error type 11
 *                                     → 实参不匹配: Error type 9
 *   Exp → Exp ASSIGNOP Exp 赋值     → 左值非法: Error type 6
 *                                     → 类型不匹配: Error type 5
 *   Exp → Exp OP Exp       二元运算 → 类型不匹配: Error type 7
 *   Exp → Exp LB Exp RB    数组下标 → 不是数组: Error type 10
 *                                     → 下标非int: Error type 12
 *   Exp → Exp DOT ID       成员访问 → 非法使用.: Error type 13
 *                                     → 字段不存在: Error type 14
 *   Exp → LP Exp RP        括号     → 返回内部表达式类型
 *   Exp → MINUS/NOT Exp    单目运算 → 返回操作数类型
 *
 * @param exp  表达式 AST 节点
 * @return     表达式结果类型；出错返回 NULL（抑制级联错误）
 */
static Type* checkExp(Node* exp);

/**
 * 从 Specifier 节点提取类型
 *
 *   TYPE "int"              → TYPE_INT
 *   TYPE "float"            → TYPE_FLOAT
 *   StructSpecifier (定义)   → 先调用 processStructDef() 注册结构体
 *   StructSpecifier (引用)   → 查符号表 → 未找到: Error type 17
 *
 * @param spec  Specifier AST 节点
 * @return      提取的类型
 */
static Type* getTypeFromSpecifier(Node* spec);

/**
 * 处理全局定义 ExtDef
 *
 *   Specifier FunDec CompSt    → 注册函数 (Error type 4: 重定义)
 *                               → enterScope → 收集形参 → 遍历函数体 → exitScope
 *   Specifier ExtDecList SEMI  → 全局变量声明
 *   Specifier SEMI             → 结构体定义 (Error type 16: 结构体名重复)
 *
 * @param node  ExtDef AST 节点
 */
static void processExtDef(Node* node);

/**
 * 处理局部变量定义 Def → Specifier DecList SEMI
 *
 * 提取类型 → 遍历 DecList 链 → 逐个 processVarDec() 注册变量
 * @param node  Def AST 节点
 */
static void processDef(Node* node);

/**
 * 从 VarDec 提取变量名并注册到符号表
 *
 *   VarDec → ID                   简单变量 → insertSymbol() (Error type 3: 重定义)
 *   VarDec → VarDec LB INT RB     数组     → 递归，每层 createArrayType()
 *
 * 插入前先 copyType() 创建独立副本，避免多个变量共享同一 Type*。
 * @param varDec  VarDec AST 节点
 * @param type    变量类型
 */
static void processVarDec(Node* varDec, Type* type);

/**
 * 处理结构体定义
 *
 *   StructSpecifier → STRUCT OptTag LC DefList RC
 *
 * 注册结构体符号 → 遍历 DefList → collectStructFields() 收集字段
 * @param structSpec  StructSpecifier AST 节点
 */
static void processStructDef(Node* structSpec);

/**
 * 递归遍历 DecList 链，为每个字段创建 Symbol 并加入结构体
 *
 *   DecList → Dec | Dec COMMA DecList
 *
 * 检查字段重复 (Error type 15)，通过后创建字段符号，存入 structSym->fields。
 * @param decList     DecList AST 节点
 * @param fieldType   字段类型
 * @param structSym   所属结构体符号
 */
static void collectStructFields(Node* decList, Type* fieldType, Symbol* structSym);

/**
 * 错误类型速查
 * ============================================================================
 *   1   变量未定义         Undefined variable
 *   2   函数未定义         Undefined function
 *   3   变量重复定义       Redefined variable
 *   4   函数重复定义       Redefined function
 *   5   赋值类型不匹配     Type mismatched for assignment
 *   6   左值非法           LHS of assignment must be a variable
 *   7   运算类型不匹配     Type mismatched for operands
 *   8   返回类型不匹配     Type mismatched for return
 *   9   实参不匹配         Function not applicable for arguments
 *  10   不是数组           Not an array
 *  11   不是函数           Not a function
 *  12   下标非整数         Index not integer
 *  13   .号非法使用        Illegal use of "."
 *  14   字段不存在         Non-existent field
 *  15   字段重复定义       Redefined field
 *  16   结构体名重复       Duplicated name
 *  17   结构体未定义       Undefined structure
 * ============================================================================
 */
