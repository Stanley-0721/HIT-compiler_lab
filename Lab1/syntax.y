%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lex.yy.c"
#define MAX_NAME 100

// 当前行号
extern int yylineno;
// 当前匹配的文本
extern char *yytext; 
// flex 提供的词法分析函数，供 bison 调用
int yylex(void);
// Bison 默认错误处理函数声明
void yyerror(const char* msg);

// 语法树
typedef struct Node {
    char* name;        // 节点名
    int line;          // 行号
    int num;           // 子节点数量
    struct Node** child; // 子节点

    // 节点的值（整数/浮点数/标识符）
    union {
        char str[MAX_NAME];  // 存变量名
        int ival;            // 存整数
        float fval;          // 存浮点数
    } val;

} Node;

/**
 * @brief 创建一个空的语法树节点
 * @param name 节点类型名称字符串
 * @param line 节点对应的源代码行号
 * @return 指向新创建节点的指针
 */
Node* createTree(const char* name, int line);

/**
 * @brief 向父节点添加一个子节点
 * @param parent 父节点
 * @param child 要添加的子节点
 * @note 会自动扩容子节点数组
 */
void addChild(Node* parent, Node* child);

/**
 * @brief 递归释放整棵语法树的内存
 * @param root 语法树根节点
 * @note 会递归释放所有子节点、名称、子节点数组，避免内存泄漏
 */
void freeTree(Node* root);

/**
 * @brief 以缩进格式递归打印语法树
 * @param root 语法树根节点
 * @param depth 当前打印深度（用于缩进，根节点传 0）
 * @note 会自动显示节点名、行号、节点值（INT/FLOAT/ID/TYPE）
 */
void printTree(Node* root, int depth);

%}

%union{
    struct Node* tree;
}

%token <tree> INT FLOAT ID
%token <tree> SEMI COMMA ASSIGNOP RELOP DOT NOT
%token <tree> PLUS MINUS STAR DIV
%token <tree> LP RP LB RB LC RC
%token <tree> TYPE STRUCT RETURN IF ELSE WHILE AND OR

%type <tree> Program ExtDefList ExtDef ExtDecList Specifier StructSpecifier OptTag Tag VarDec FunDec VarList ParamDec CompSt StmtList Stmt DefList Def DecList Dec Exp Args

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%left LP RP RB LB DOT
%nonassoc ELSE

%%

/* 这里以后写你的语法规则 */

%%

// 错误处理函数
void yyerror(const char* msg) {
    fprintf(stderr, "Error: %s at line %d, text: %s\n", msg, yylineno, yytext);
}


Node* createTree(const char* name, int line) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->name = strdup(name);
    node->line = line;
    node->num = 0;
    node->child = NULL;
    return node;
}

void addChild(Node* parent, Node* child) {
    if (!parent || !child) return;

    parent->child = (Node**)realloc(parent->child, (parent->num + 1) * sizeof(Node*));
    parent->child[parent->num++] = child;
}

void freeTree(Node* root) {
    if (!root) return;

    // 递归释放所有子节点
    for (int i = 0; i < root->num; i++) {
        freeTree(root->child[i]);
    }

    // 释放自身
    free(root->name);
    free(root->child);
    free(root);
}

void printTree(Node* root, int depth) {
    if (!root) return;

    // 缩进
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }

    // 打印节点名 + 行号
    printf("[%s] (line %d)", root->name, root->line);

    // 如果有值，一起打印
    if (strcmp(root->name, "INT") == 0) {
        printf(" = %d", root->val.ival);
    } else if (strcmp(root->name, "FLOAT") == 0) {
        printf(" = %.2f", root->val.fval);
    } else if (strcmp(root->name, "ID") == 0) {
        printf(" = %s", root->val.str);
    } else if (strcmp(root->name, "TYPE") == 0) {
        printf(" = %s", root->val.str);
    }

    printf("\n");

    // 递归打印子节点
    for (int i = 0; i < root->num; i++) {
        printTree(root->child[i], depth + 1);
    }
}