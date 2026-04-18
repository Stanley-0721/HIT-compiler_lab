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

/* 顶层定义 */
Program : ExtDefList {
    $$ = createTree("Program", yylineno);
    if($1) addChild($$, $1);  // ✅ 安全
};

ExtDefList : ExtDef ExtDefList {
    $$ = createTree("ExtDefList", yylineno);
    addChild($$, $1);
    addChild($$, $2);
}
| /* empty */ {
    $$ = NULL;
};

ExtDef : Specifier ExtDecList SEMI {
    $$ = createTree("ExtDef", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Specifier SEMI {
    $$ = createTree("ExtDef", yylineno);
    addChild($$, $1);
    addChild($$, $2);
}
| Specifier FunDec CompSt {
    $$ = createTree("ExtDef", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
};

ExtDecList : VarDec {
        $$ = createTree("ExtDecList", yylineno);
        addChild($$, $1);
    }
    | VarDec COMMA ExtDecList {
        $$ = createTree("ExtDecList", yylineno);
        addChild($$, $1);
        addChild($$, $2);
        addChild($$, $3);
    }
;
/* Specifiers */
Specifier : TYPE {
    $$ = createTree("Specifier", yylineno);
    addChild($$, $1);
}
| StructSpecifier {
    $$ = createTree("Specifier", yylineno);
    addChild($$, $1);
};

StructSpecifier : STRUCT OptTag LC DefList RC {
    $$ = createTree("StructSpecifier", yylineno);
    addChild($$, $1);
    if ($2) addChild($$, $2);
    addChild($$, $3);
    if ($4) addChild($$, $4);
    addChild($$, $5);
}
| STRUCT Tag {
    $$ = createTree("StructSpecifier", yylineno);
    addChild($$, $1);
    addChild($$, $2);
};

OptTag : ID {
    $$ = createTree("OptTag", yylineno);
    addChild($$, $1);
}
| /* empty */ {
    $$ = NULL;
};

Tag : ID {
    $$ = createTree("Tag", yylineno);
    addChild($$, $1);
};

VarDec : ID {
    $$ = createTree("VarDec", yylineno);
    addChild($$, $1);
}
| VarDec LB INT RB {
    $$ = createTree("VarDec", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
};


FunDec : ID LP VarList RP {
    $$ = createTree("FunDec", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
}
| ID LP RP {
    $$ = createTree("FunDec", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
};

VarList : ParamDec COMMA VarList {
    $$ = createTree("VarList", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| ParamDec {
    $$ = createTree("VarList", yylineno);
    addChild($$, $1);
};

ParamDec : Specifier VarDec {
    $$ = createTree("ParamDec", yylineno);
    addChild($$, $1);
    addChild($$, $2);
};


CompSt : LC DefList StmtList RC {
    $$ = createTree("CompSt", yylineno);
    addChild($$, $1);
    if ($2) addChild($$, $2);
    if ($3) addChild($$, $3);
    addChild($$, $4);
};

StmtList : Stmt StmtList {
    $$ = createTree("StmtList", yylineno);
    addChild($$, $1);
    if ($2) addChild($$, $2);
}
| /* empty */ {
    $$ = NULL;
};


Stmt : Exp SEMI {
    $$ = createTree("Stmt", yylineno);
    addChild($$, $1);
    addChild($$, $2);
}
| CompSt {
    $$ = createTree("Stmt", yylineno);
    addChild($$, $1);
}
| RETURN Exp SEMI {
    $$ = createTree("Stmt", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| IF LP Exp RP Stmt {
    $$ = createTree("Stmt", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
    addChild($$, $5);
}
| IF LP Exp RP Stmt ELSE Stmt {
    $$ = createTree("Stmt", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
    addChild($$, $5);
    addChild($$, $6);
    addChild($$, $7);
}
| WHILE LP Exp RP Stmt {
    $$ = createTree("Stmt", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
    addChild($$, $5);
};


DefList : Def DefList {
    $$ = createTree("DefList", yylineno);
    addChild($$, $1);
    if ($2) addChild($$, $2);
}
| /* empty */ {
    $$ = NULL;
};

Def : Specifier DecList SEMI {
    $$ = createTree("Def", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
};

DecList : Dec {
    $$ = createTree("DecList", yylineno);
    addChild($$, $1);
}
| Dec COMMA DecList {
    $$ = createTree("DecList", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
};

Dec : VarDec {
    $$ = createTree("Dec", yylineno);
    addChild($$, $1);
}
| VarDec ASSIGNOP Exp {
    $$ = createTree("Dec", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
};

Exp : Exp ASSIGNOP Exp {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp AND Exp {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp OR Exp {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp RELOP Exp {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp PLUS Exp {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp MINUS Exp {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp STAR Exp {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp DIV Exp {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| LP Exp RP {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| MINUS Exp {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
}
| NOT Exp {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
}
| ID LP Args RP {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
}
| ID LP RP {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp LB Exp RB {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
}
| Exp DOT ID {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| ID {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
}
| INT {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
}
| FLOAT {
    $$ = createTree("Exp", yylineno);
    addChild($$, $1);
};

Args : Exp COMMA Args {
    $$ = createTree("Args", yylineno);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp {
    $$ = createTree("Args", yylineno);
    addChild($$, $1);
};

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