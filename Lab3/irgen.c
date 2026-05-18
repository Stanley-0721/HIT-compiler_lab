#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "irgen.h"

static FILE* out;
static int varCount = 0;
static int tempCount = 0;
static int labelCount = 0;

/* ---- variable name mapping (scoped) ---- */
typedef struct VarEntry {
    char name[MAX_NAME];
    char irName[16];
    struct VarEntry* next;
} VarEntry;

typedef struct VarScope {
    VarEntry* entries;
    struct VarScope* outer;
} VarScope;

static VarScope* varScope = NULL;

static void enterVarScope(void) {
    VarScope* s = (VarScope*)malloc(sizeof(VarScope));
    s->entries = NULL;
    s->outer = varScope;
    varScope = s;
}

static void exitVarScope(void) {
    if (!varScope) return;
    VarScope* old = varScope;
    varScope = old->outer;
    VarEntry* e = old->entries;
    while (e) {
        VarEntry* next = e->next;
        free(e);
        e = next;
    }
    free(old);
}

static char* lookupVar(const char* name) {
    VarScope* s = varScope;
    while (s) {
        VarEntry* e = s->entries;
        while (e) {
            if (strcmp(e->name, name) == 0)
                return e->irName;
            e = e->next;
        }
        s = s->outer;
    }
    return NULL;
}

static char* registerVar(const char* name) {
    varCount++;
    VarEntry* e = (VarEntry*)malloc(sizeof(VarEntry));
    strncpy(e->name, name, MAX_NAME - 1);
    e->name[MAX_NAME - 1] = '\0';
    snprintf(e->irName, sizeof(e->irName), "v%d", varCount);
    e->next = varScope->entries;
    varScope->entries = e;
    return e->irName;
}

static char* newTemp(void) {
    tempCount++;
    char* t = (char*)malloc(16);
    snprintf(t, 16, "t%d", tempCount);
    return t;
}

static char* newLabel(void) {
    labelCount++;
    char* l = (char*)malloc(16);
    snprintf(l, 16, "label%d", labelCount);
    return l;
}

/* ---- forward declarations ---- */
static char* translateExp(Node* exp);
static void translateStmt(Node* stmt);
static void translateCompSt(Node* compSt);
static void translateExtDef(Node* extDef);
static void translateDef(Node* def);
static void translateCondition(Node* exp, char* l_true, char* l_false);
static void translateVarDec(Node* varDec);

/* ================================================================
 *  入口
 * ================================================================ */

void generateIR(Node* root, const char* outputFile) {
    varCount = 0;
    tempCount = 0;
    labelCount = 0;
    varScope = NULL;

    out = fopen(outputFile, "w");
    if (!out) {
        fprintf(stderr, "Cannot open output file: %s\n", outputFile);
        return;
    }

    enterVarScope();  // global scope

    if (root && strcmp(root->name, "Program") == 0 && root->num >= 1) {
        Node* cur = root->child[0];
        while (cur && cur->num > 0 && strcmp(cur->name, "ExtDefList") == 0) {
            translateExtDef(cur->child[0]);
            cur = (cur->num >= 2 && cur->child[1]) ? cur->child[1] : NULL;
        }
    }

    exitVarScope();
    fclose(out);
}

/* ================================================================
 *  函数 / 全局变量定义
 * ================================================================ */

static void translateExtDef(Node* node) {
    if (!node) return;
    // ExtDef: Specifier ExtDecList SEMI  (global vars)
    //      |  Specifier SEMI            (struct def)
    //      |  Specifier FunDec CompSt   (function)

    if (node->num == 2) {
        // struct definition or similar — skip
        return;
    }

    Node* kind = node->child[1];

    if (strcmp(kind->name, "FunDec") == 0) {
        // ===== Function definition =====
        char* funcName = kind->child[0]->val.str;
        fprintf(out, "FUNCTION %s :\n", funcName);

        enterVarScope();

        // Process parameters: FunDec → ID LP VarList RP  or  ID LP RP
        if (kind->num == 4) {
            Node* varList = kind->child[2];
            Node* cur = varList;
            while (cur && strcmp(cur->name, "VarList") == 0) {
                Node* paramDec = cur->child[0]; // ParamDec → Specifier VarDec
                Node* varDec = paramDec->child[1];
                translateVarDec(varDec);
                char* irName = lookupVar(varDec->child[0]->val.str);
                fprintf(out, "PARAM %s\n", irName);
                cur = (cur->num == 3) ? cur->child[2] : NULL;
            }
        }

        // Translate function body CompSt
        if (node->num >= 3 && node->child[2]) {
            translateCompSt(node->child[2]);
        }

        exitVarScope();
    }
}

/* ================================================================
 *  复合语句 (block)
 * ================================================================ */

static void translateCompSt(Node* compSt) {
    if (!compSt) return;
    // CompSt → LC DefList StmtList RC
    enterVarScope();
    for (int i = 0; i < compSt->num; i++) {
        Node* child = compSt->child[i];
        if (strcmp(child->name, "DefList") == 0) {
            Node* cur = child;
            while (cur && cur->num > 0 && strcmp(cur->name, "DefList") == 0) {
                translateDef(cur->child[0]);
                cur = (cur->num >= 2 && cur->child[1]) ? cur->child[1] : NULL;
            }
        } else if (strcmp(child->name, "StmtList") == 0) {
            Node* cur = child;
            while (cur && cur->num > 0 && strcmp(cur->name, "StmtList") == 0) {
                translateStmt(cur->child[0]);
                cur = (cur->num >= 2 && cur->child[1]) ? cur->child[1] : NULL;
            }
        }
    }
    exitVarScope();
}

/* ================================================================
 *  局部变量定义
 * ================================================================ */

static void translateDef(Node* def) {
    if (!def) return;
    // Def → Specifier DecList SEMI
    Node* decList = def->child[1];
    Node* cur = decList;
    while (cur && strcmp(cur->name, "DecList") == 0) {
        Node* dec = cur->child[0]; // Dec → VarDec | VarDec ASSIGNOP Exp
        translateVarDec(dec->child[0]);
        if (dec->num == 3) {
            // Has initializer: VarDec ASSIGNOP Exp
            char* varName = dec->child[0]->child[0]->val.str;
            char* irName = lookupVar(varName);
            char* rhs = translateExp(dec->child[2]);
            if (irName && rhs) {
                fprintf(out, "%s := %s\n", irName, rhs);
            }
            free(rhs);
        }
        cur = (cur->num == 3) ? cur->child[2] : NULL;
    }
}

/* ================================================================
 *  变量声明: 分配 v 编号
 * ================================================================ */

static void translateVarDec(Node* varDec) {
    if (!varDec) return;
    // VarDec → ID  |  VarDec LB INT RB
    if (varDec->num == 1) {
        registerVar(varDec->child[0]->val.str);
        return;
    }
    if (varDec->num == 4) {
        translateVarDec(varDec->child[0]);
        return;
    }
}

/* ================================================================
 *  语句翻译
 * ================================================================ */

static void translateStmt(Node* stmt) {
    if (!stmt) return;

    // Stmt → RETURN Exp SEMI
    if (stmt->num >= 2 && strcmp(stmt->child[0]->name, "RETURN") == 0) {
        char* val = translateExp(stmt->child[1]);
        if (val) {
            fprintf(out, "RETURN %s\n", val);
            free(val);
        }
        return;
    }

    // Stmt → IF LP Exp RP Stmt [ELSE Stmt]
    if (stmt->num >= 5 && strcmp(stmt->child[0]->name, "IF") == 0) {
        Node* cond = stmt->child[2];
        Node* thenStmt = stmt->child[4];
        int hasElse = (stmt->num == 7);
        Node* elseStmt = hasElse ? stmt->child[6] : NULL;

        char* l_true = newLabel();
        char* l_false = newLabel();

        translateCondition(cond, l_true, l_false);

        fprintf(out, "LABEL %s :\n", l_true);
        translateStmt(thenStmt);

        if (hasElse) {
            char* l_merge = newLabel();
            fprintf(out, "GOTO %s\n", l_merge);
            fprintf(out, "LABEL %s :\n", l_false);
            translateStmt(elseStmt);
            fprintf(out, "LABEL %s :\n", l_merge);
            free(l_merge);
        } else {
            fprintf(out, "LABEL %s :\n", l_false);
        }

        free(l_true);
        free(l_false);
        return;
    }

    // Stmt → WHILE LP Exp RP Stmt
    if (stmt->num >= 5 && strcmp(stmt->child[0]->name, "WHILE") == 0) {
        Node* cond = stmt->child[2];
        Node* body = stmt->child[4];

        char* l_start = newLabel();
        char* l_body = newLabel();
        char* l_end = newLabel();

        fprintf(out, "LABEL %s :\n", l_start);
        translateCondition(cond, l_body, l_end);

        fprintf(out, "LABEL %s :\n", l_body);
        translateStmt(body);
        fprintf(out, "GOTO %s\n", l_start);

        fprintf(out, "LABEL %s :\n", l_end);

        free(l_start);
        free(l_body);
        free(l_end);
        return;
    }

    // Stmt → Exp SEMI  (expression statement)
    if (stmt->num >= 1 && strcmp(stmt->child[0]->name, "Exp") == 0) {
        char* result = translateExp(stmt->child[0]);
        free(result);
        return;
    }

    // Stmt → CompSt  (nested block)
    if (stmt->num >= 1 && strcmp(stmt->child[0]->name, "CompSt") == 0) {
        translateCompSt(stmt->child[0]);
        return;
    }
}

/* ================================================================
 *  条件跳转: 如果 exp 为 Exp RELOP Exp，直接用 IF；否则转成 != 0
 * ================================================================ */

static void translateCondition(Node* exp, char* l_true, char* l_false) {
    if (!exp) return;

    // Exp → Exp RELOP Exp
    if (exp->num == 3 && strcmp(exp->child[1]->name, "RELOP") == 0) {
        char* left = translateExp(exp->child[0]);
        char* right = translateExp(exp->child[2]);
        char* relop = exp->child[1]->val.str;
        fprintf(out, "IF %s %s %s GOTO %s\n", left, relop, right, l_true);
        fprintf(out, "GOTO %s\n", l_false);
        free(left);
        free(right);
        return;
    }

    // Exp → NOT Exp
    if (exp->num == 2 && strcmp(exp->child[0]->name, "NOT") == 0) {
        translateCondition(exp->child[1], l_false, l_true);
        return;
    }

    // Otherwise: evaluate expression, compare to #0
    char* val = translateExp(exp);
    if (val) {
        fprintf(out, "IF %s != #0 GOTO %s\n", val, l_true);
        fprintf(out, "GOTO %s\n", l_false);
        free(val);
    }
}

/* ================================================================
 *  表达式翻译 — 返回结果名称 (vN / tN / #N)
 *  调用者负责 free 返回值
 * ================================================================ */

static char* translateExp(Node* exp) {
    if (!exp) return NULL;

    // Exp → INT
    if (exp->num == 1 && strcmp(exp->child[0]->name, "INT") == 0) {
        char* s = (char*)malloc(32);
        snprintf(s, 32, "#%d", exp->child[0]->val.ival);
        return s;
    }

    // Exp → ID
    if (exp->num == 1 && strcmp(exp->child[0]->name, "ID") == 0) {
        char* irName = lookupVar(exp->child[0]->val.str);
        return irName ? strdup(irName) : strdup("???");
    }

    // Exp → ID LP RP   /   ID LP Args RP  (function call)
    if (exp->num >= 3 &&
        strcmp(exp->child[0]->name, "ID") == 0 &&
        strcmp(exp->child[1]->name, "LP") == 0) {
        char* funcName = exp->child[0]->val.str;

        // 内建 read()
        if (strcmp(funcName, "read") == 0) {
            char* t = newTemp();
            fprintf(out, "READ %s\n", t);
            return t;
        }

        // 内建 write(x)
        if (strcmp(funcName, "write") == 0) {
            Node* args = exp->child[2]; // Args
            char* arg = translateExp(args->child[0]);
            if (arg[0] == '#') {
                char* t = newTemp();
                fprintf(out, "%s := %s\n", t, arg);
                free(arg);
                arg = t;
            }
            fprintf(out, "WRITE %s\n", arg);
            free(arg);
            return strdup("#0");
        }

        // 普通函数调用
        if (exp->num == 4) {
            // 收集实参
            int argCount = 0;
            char* argList[64];
            Node* cur = exp->child[2]; // Args
            while (cur && strcmp(cur->name, "Args") == 0) {
                argList[argCount++] = translateExp(cur->child[0]);
                cur = (cur->num == 3) ? cur->child[2] : NULL;
            }
            // 逆序生成 ARG
            for (int i = argCount - 1; i >= 0; i--) {
                if (argList[i][0] == '#') {
                    char* t = newTemp();
                    fprintf(out, "%s := %s\n", t, argList[i]);
                    free(argList[i]);
                    argList[i] = t;
                }
                fprintf(out, "ARG %s\n", argList[i]);
                free(argList[i]);
            }
        }

        char* t = newTemp();
        fprintf(out, "%s := CALL %s\n", t, funcName);
        return t;
    }

    // Exp → Exp ASSIGNOP Exp
    if (exp->num == 3 && strcmp(exp->child[1]->name, "ASSIGNOP") == 0) {
        Node* lhs = exp->child[0];
        char* rhs = translateExp(exp->child[2]);

        char* lhsName = NULL;
        if (lhs->num == 1 && strcmp(lhs->child[0]->name, "ID") == 0) {
            lhsName = lookupVar(lhs->child[0]->val.str);
        }
        // TODO: array subscript, struct member, deref

        if (lhsName && rhs) {
            fprintf(out, "%s := %s\n", lhsName, rhs);
        }
        free(rhs);
        return lhsName ? strdup(lhsName) : NULL;
    }

    // Exp → LP Exp RP
    if (exp->num == 3 && strcmp(exp->child[0]->name, "LP") == 0) {
        return translateExp(exp->child[1]);
    }

    // Exp → MINUS Exp  (unary)
    if (exp->num == 2 && strcmp(exp->child[0]->name, "MINUS") == 0) {
        char* operand = translateExp(exp->child[1]);
        char* t = newTemp();
        fprintf(out, "%s := #0 - %s\n", t, operand);
        free(operand);
        return t;
    }

    // Exp → NOT Exp
    if (exp->num == 2 && strcmp(exp->child[0]->name, "NOT") == 0) {
        char* operand = translateExp(exp->child[1]);
        char* l1 = newLabel();
        char* l2 = newLabel();
        char* t = newTemp();
        fprintf(out, "IF %s == #0 GOTO %s\n", operand, l1);
        free(operand);
        fprintf(out, "%s := #0\n", t);
        fprintf(out, "GOTO %s\n", l2);
        fprintf(out, "LABEL %s :\n", l1);
        fprintf(out, "%s := #1\n", t);
        fprintf(out, "LABEL %s :\n", l2);
        free(l1);
        free(l2);
        return t;
    }

    // Exp → Exp RELOP Exp  (standalone comparison → reduce to 0/1)
    if (exp->num == 3 && strcmp(exp->child[1]->name, "RELOP") == 0) {
        char* left = translateExp(exp->child[0]);
        char* right = translateExp(exp->child[2]);
        char* relop = exp->child[1]->val.str;
        char* l1 = newLabel();
        char* l2 = newLabel();
        char* t = newTemp();
        fprintf(out, "IF %s %s %s GOTO %s\n", left, relop, right, l1);
        free(left);
        free(right);
        fprintf(out, "%s := #0\n", t);
        fprintf(out, "GOTO %s\n", l2);
        fprintf(out, "LABEL %s :\n", l1);
        fprintf(out, "%s := #1\n", t);
        fprintf(out, "LABEL %s :\n", l2);
        free(l1);
        free(l2);
        return t;
    }

    // Exp → Exp AND Exp
    if (exp->num == 3 && strcmp(exp->child[1]->name, "AND") == 0) {
        char* l_false = newLabel();
        char* l_merge = newLabel();
        char* t = newTemp();

        // Short-circuit: if left is false, result is 0
        translateCondition(exp->child[0], l_merge, l_false);
        // Actually, AND: left && right. If left is false → false.
        // If left is true → result = right != 0

        // Simpler approach: compute to 0/1
        // t = (left != 0) & (right != 0)
        // Using jumps:
        char* left = translateExp(exp->child[0]);
        fprintf(out, "IF %s == #0 GOTO %s\n", left, l_false);
        free(left);
        char* right = translateExp(exp->child[2]);
        fprintf(out, "IF %s == #0 GOTO %s\n", right, l_false);
        free(right);
        fprintf(out, "%s := #1\n", t);
        fprintf(out, "GOTO %s\n", l_merge);
        fprintf(out, "LABEL %s :\n", l_false);
        fprintf(out, "%s := #0\n", t);
        fprintf(out, "LABEL %s :\n", l_merge);

        free(l_false);
        free(l_merge);
        return t;
    }

    // Exp → Exp OR Exp
    if (exp->num == 3 && strcmp(exp->child[1]->name, "OR") == 0) {
        char* l_true = newLabel();
        char* l_merge = newLabel();
        char* t = newTemp();

        char* left = translateExp(exp->child[0]);
        fprintf(out, "IF %s != #0 GOTO %s\n", left, l_true);
        free(left);
        char* right = translateExp(exp->child[2]);
        fprintf(out, "IF %s != #0 GOTO %s\n", right, l_true);
        free(right);
        fprintf(out, "%s := #0\n", t);
        fprintf(out, "GOTO %s\n", l_merge);
        fprintf(out, "LABEL %s :\n", l_true);
        fprintf(out, "%s := #1\n", t);
        fprintf(out, "LABEL %s :\n", l_merge);

        free(l_true);
        free(l_merge);
        return t;
    }

    // Exp → Exp OP Exp  (binary arithmetic: PLUS MINUS STAR DIV)
    if (exp->num == 3) {
        char* opName = exp->child[1]->name;
        const char* op = NULL;
        if (strcmp(opName, "PLUS") == 0) op = "+";
        else if (strcmp(opName, "MINUS") == 0) op = "-";
        else if (strcmp(opName, "STAR") == 0) op = "*";
        else if (strcmp(opName, "DIV") == 0) op = "/";

        if (op) {
            char* left = translateExp(exp->child[0]);
            char* right = translateExp(exp->child[2]);
            char* t = newTemp();
            fprintf(out, "%s := %s %s %s\n", t, left, op, right);
            free(left);
            free(right);
            return t;
        }
    }

    return NULL;
}
