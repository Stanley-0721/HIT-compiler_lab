#ifndef MAIN_H
#define MAIN_H
#define MAX_NAME 100

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

void yyerror(const char* msg);

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

#endif