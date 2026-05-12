#ifndef MAIN_H
#define MAIN_H
#define MAX_NAME 100

#include "symbol_table.h"

// 当前行号
extern int yylineno;
// 当前匹配的文本
extern char *yytext;

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

/* ---- 语法树 / 词法 ---- */
void yyerror(const char* msg);

Node* createTree(const char* name, int line);
void addChild(Node* parent, Node* child);
void freeTree(Node* root);
void printTree(Node* root, int depth);

#endif
