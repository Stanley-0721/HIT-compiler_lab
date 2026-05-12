#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"
#include "symbol_table.h"

static int error_count = 0;
static Symbol* currentFunc = NULL;

/* ---- forward declarations ---- */
static void traverse(Node* node);
static Type* getTypeFromSpecifier(Node* spec);
static void processExtDef(Node* node);
static void processDef(Node* node);
static void processDecList(Node* decList, Type* baseType);
static void processExtDecList(Node* extDecList, Type* baseType);
static void processVarDec(Node* varDec, Type* type);
static Type* checkExp(Node* exp);
static void processStructDef(Node* structSpec);
static void collectStructFields(Node* decList, Type* fieldType, Symbol* structSym);
static char* getBaseVarName(Node* varDec);

/* ================================================================
 *  对外接口
 * ================================================================ */

void semanticAnalysis(Node* root) {
    error_count = 0;
    initSymbolTable();
    traverse(root);
    freeSymbolTable();
}

int hasSemanticError(void) {
    return error_count;
}

/* ================================================================
 *  主遍历
 * ================================================================ */

static void traverse(Node* node) {
    if (!node) return;

    if (strcmp(node->name, "ExtDef") == 0) {
        processExtDef(node);
        return;
    }

    if (strcmp(node->name, "Def") == 0) {
        processDef(node);
        return;
    }

    if (strcmp(node->name, "CompSt") == 0) {
        // 嵌套块：进入新作用域
        enterScope();
        for (int i = 0; i < node->num; i++)
            traverse(node->child[i]);
        exitScope();
        return;
    }

    if (strcmp(node->name, "Exp") == 0) {
        Type* t = checkExp(node);
        freeType(t);
        return;
    }

    if (strcmp(node->name, "Stmt") == 0) {
        // Stmt → RETURN Exp SEMI
        if (node->num >= 2 && strcmp(node->child[0]->name, "RETURN") == 0) {
            Type* retType = checkExp(node->child[1]);
            if (currentFunc && currentFunc->returnType &&
                retType && retType->kind != currentFunc->returnType->kind) {
                printf("Error type 8 at Line %d: Type mismatched for return.\n",
                       node->line);
                error_count++;
            }
            freeType(retType);
            if (node->num >= 3) traverse(node->child[2]); // SEMI
            return;
        }
        // 其他 Stmt 类型：递归处理子节点
        for (int i = 0; i < node->num; i++)
            traverse(node->child[i]);
        return;
    }

    // 默认：递归处理子节点
    for (int i = 0; i < node->num; i++)
        traverse(node->child[i]);
}

/* ================================================================
 *  表达式检查
 * ================================================================ */

static const char* typeName(Type* t) {
    if (!t) return "?";
    switch (t->kind) {
        case TYPE_INT:   return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_STRUCT: return t->structName;
        default: return "?";
    }
}

static Type* checkExp(Node* exp) {
    if (!exp) return NULL;

    // Exp → ID  —— 变量引用
    if (exp->num == 1 && strcmp(exp->child[0]->name, "ID") == 0) {
        char* name = exp->child[0]->val.str;
        Symbol* sym = lookupSymbol(name);
        if (!sym) {
            printf("Error type 1 at Line %d: Undefined variable \"%s\".\n",
                   exp->line, name);
            error_count++;
            return createType(TYPE_INT);
        }
        return copyType(sym->type);
    }

    // Exp → INT
    if (exp->num == 1 && strcmp(exp->child[0]->name, "INT") == 0)
        return createType(TYPE_INT);

    // Exp → FLOAT
    if (exp->num == 1 && strcmp(exp->child[0]->name, "FLOAT") == 0)
        return createType(TYPE_FLOAT);

    // Exp → ID LP RP       /  Exp → ID LP Args RP  —— 函数调用
    if (exp->num >= 3 &&
        strcmp(exp->child[0]->name, "ID") == 0 &&
        strcmp(exp->child[1]->name, "LP") == 0) {
        char* name = exp->child[0]->val.str;
        Symbol* sym = lookupSymbol(name);
        if (!sym) {
            printf("Error type 2 at Line %d: Undefined function \"%s\".\n",
                   exp->line, name);
            error_count++;
            return createType(TYPE_INT);
        }
        if (sym->kind != SYM_FUNC) {
            printf("Error type 11 at Line %d: \"%s\" is not a function.\n",
                   exp->line, name);
            error_count++;
            return createType(TYPE_INT);
        }

        // 收集实参类型
        int argCount = 0;
        Type* argTypes[64];
        if (exp->num == 4) {
            Node* cur = exp->child[2]; // Args
            while (cur && strcmp(cur->name, "Args") == 0) {
                if (argCount < 64)
                    argTypes[argCount++] = checkExp(cur->child[0]);
                cur = (cur->num == 3) ? cur->child[2] : NULL;
            }
        }

        // 检查参数数量
        int match = (argCount == sym->paramCount);
        if (match) {
            for (int i = 0; i < argCount; i++) {
                if (argTypes[i]->kind != sym->paramTypes[i]->kind)
                    { match = 0; break; }
            }
        }

        if (!match) {
            // 构造函数签名 "func(int)"
            char sig[512];
            snprintf(sig, sizeof(sig), "%s(", name);
            for (int i = 0; i < sym->paramCount; i++) {
                if (i > 0) strncat(sig, ", ", sizeof(sig) - strlen(sig) - 1);
                strncat(sig, typeName(sym->paramTypes[i]),
                        sizeof(sig) - strlen(sig) - 1);
            }
            strncat(sig, ")", sizeof(sig) - strlen(sig) - 1);

            // 构造实参类型 "(int, int)"
            char argStr[512];
            snprintf(argStr, sizeof(argStr), "(");
            for (int i = 0; i < argCount; i++) {
                if (i > 0) strncat(argStr, ", ", sizeof(argStr) - strlen(argStr) - 1);
                strncat(argStr, typeName(argTypes[i]),
                        sizeof(argStr) - strlen(argStr) - 1);
            }
            strncat(argStr, ")", sizeof(argStr) - strlen(argStr) - 1);

            printf("Error type 9 at Line %d: Function \"%s\" is not applicable"
                   " for arguments \"%s\".\n", exp->line, sig, argStr);
            error_count++;
        }

        for (int i = 0; i < argCount; i++) freeType(argTypes[i]);
        return copyType(sym->returnType);
    }

    // Exp → Exp ASSIGNOP Exp  —— 赋值
    if (exp->num == 3 && strcmp(exp->child[1]->name, "ASSIGNOP") == 0) {
        Node* lhs = exp->child[0];
        int isVar = (lhs->num == 1 && strcmp(lhs->child[0]->name, "ID") == 0)
                 || (lhs->num == 4 && strcmp(lhs->child[1]->name, "LB") == 0)
                 || (lhs->num == 3 && strcmp(lhs->child[1]->name, "DOT") == 0);
        if (!isVar) {
            printf("Error type 6 at Line %d: The left-hand side of an assignment must be a variable.\n",
                   exp->line);
            error_count++;
        }
        Type* left  = checkExp(lhs);
        Type* right = checkExp(exp->child[2]);
        if (isVar && left && right && left->kind != right->kind) {
            printf("Error type 5 at Line %d: Type mismatched for assignment.\n",
                   exp->line);
            error_count++;
        }
        freeType(right);
        return left;
    }

    // Exp → Exp OP Exp  —— 二元运算
    if (exp->num == 3 && (
        strcmp(exp->child[1]->name, "PLUS")   == 0 ||
        strcmp(exp->child[1]->name, "MINUS")  == 0 ||
        strcmp(exp->child[1]->name, "STAR")   == 0 ||
        strcmp(exp->child[1]->name, "DIV")    == 0 ||
        strcmp(exp->child[1]->name, "AND")    == 0 ||
        strcmp(exp->child[1]->name, "OR")     == 0 ||
        strcmp(exp->child[1]->name, "RELOP")  == 0)) {
        Type* left  = checkExp(exp->child[0]);
        Type* right = checkExp(exp->child[2]);
        if (left && right && left->kind != right->kind) {
            printf("Error type 7 at Line %d: Type mismatched for operands.\n",
                   exp->line);
            error_count++;
        }
        freeType(right);
        return left;
    }

    // Exp → Exp LB Exp RB  —— 数组下标
    if (exp->num == 4 && strcmp(exp->child[1]->name, "LB") == 0) {
        Type* arrType = checkExp(exp->child[0]);
        if (arrType && !arrType->isArray) {
            char* name = "?";
            Node* lhs = exp->child[0];
            if (lhs->num == 1 && strcmp(lhs->child[0]->name, "ID") == 0)
                name = lhs->child[0]->val.str;
            printf("Error type 10 at Line %d: \"%s\" is not an array.\n",
                   exp->line, name);
            error_count++;
        }
        Type* idxType = checkExp(exp->child[2]);
        if (idxType && idxType->kind != TYPE_INT) {
            char idxStr[64] = "expression";
            Node* idxNode = exp->child[2];
            if (idxNode->num == 1) {
                if (strcmp(idxNode->child[0]->name, "INT") == 0)
                    snprintf(idxStr, sizeof(idxStr), "%d",
                             idxNode->child[0]->val.ival);
                else if (strcmp(idxNode->child[0]->name, "FLOAT") == 0)
                    snprintf(idxStr, sizeof(idxStr), "%g",
                             idxNode->child[0]->val.fval);
                else if (strcmp(idxNode->child[0]->name, "ID") == 0)
                    snprintf(idxStr, sizeof(idxStr), "%s",
                             idxNode->child[0]->val.str);
            }
            printf("Error type 12 at Line %d: \"%s\" is not an integer.\n",
                   exp->line, idxStr);
            error_count++;
        }
        freeType(idxType);
        if (arrType && arrType->isArray && arrType->elementType) {
            Type* elem = copyType(arrType->elementType);
            freeType(arrType);
            return elem;
        }
        freeType(arrType);
        return createType(TYPE_INT);
    }

    // Exp → Exp DOT ID  —— 结构体成员访问
    if (exp->num == 3 && strcmp(exp->child[1]->name, "DOT") == 0) {
        Type* left = checkExp(exp->child[0]);
        if (left && left->kind != TYPE_STRUCT) {
            printf("Error type 13 at Line %d: Illegal use of \".\".\n",
                   exp->line);
            error_count++;
            freeType(left);
            return createType(TYPE_INT);
        }
        if (left) {
            char* fieldName = exp->child[2]->val.str;
            Symbol* field = lookupStructField(left->structName, fieldName);
            if (!field) {
                printf("Error type 14 at Line %d: Non-existent field \"%s\".\n",
                       exp->line, fieldName);
                error_count++;
                freeType(left);
                return NULL;
            }
            Type* fieldType = copyType(field->type);
            freeType(left);
            return fieldType;
        }
        freeType(left);
        return createType(TYPE_INT);
    }

    // Exp → LP Exp RP
    if (exp->num == 3 && strcmp(exp->child[0]->name, "LP") == 0)
        return checkExp(exp->child[1]);

    // Exp → MINUS Exp  /  Exp → NOT Exp  (unary)
    if (exp->num == 2)
        return checkExp(exp->child[1]);

    // 其他：二元运算 / 数组下标 / 结构体访问 -> 返回左子表达式类型
    Type* result = NULL;
    for (int i = 0; i < exp->num; i++) {
        Type* t = checkExp(exp->child[i]);
        if (t) {
            if (!result) result = t;
            else freeType(t);
        }
    }
    return result ? result : createType(TYPE_INT);
}

/* ================================================================
 *  类型提取
 * ================================================================ */

static Type* getTypeFromSpecifier(Node* spec) {
    if (!spec || spec->num == 0) return createType(TYPE_INT);
    Node* child = spec->child[0];
    if (strcmp(child->name, "TYPE") == 0) {
        if (strcmp(child->val.str, "int") == 0)
            return createType(TYPE_INT);
        if (strcmp(child->val.str, "float") == 0)
            return createType(TYPE_FLOAT);
    }
    if (strcmp(child->name, "StructSpecifier") == 0) {
        // StructSpecifier → STRUCT Tag  (引用)
        if (child->num == 2)
            return createStructType(child->child[1]->child[0]->val.str);
        // StructSpecifier → STRUCT OptTag LC DefList RC  (定义)
        if (child->num == 5 && child->child[1])
            return createStructType(child->child[1]->child[0]->val.str);
    }
    return createType(TYPE_INT);
}

/* ================================================================
 *  全局定义处理 (ExtDef)
 * ================================================================ */

static void processExtDef(Node* node) {
    // child[0] = Specifier
    // child[1] = ExtDecList | FunDec | SEMI
    // child[2] = SEMI | CompSt (if present)

    if (node->num == 2) {
        // Specifier SEMI (如 struct 定义)
        Node* spec = node->child[0];
        if (spec->num > 0 && strcmp(spec->child[0]->name, "StructSpecifier") == 0)
            processStructDef(spec->child[0]);
        return;
    }

    Node* spec = node->child[0];
    Node* kind = node->child[1];

    if (strcmp(kind->name, "FunDec") == 0) {
        // ===== 函数定义: Specifier FunDec CompSt =====
        Type* returnType = getTypeFromSpecifier(spec);

        // 函数名
        char* funcName = kind->child[0]->val.str;
        Symbol* funcSym = insertSymbol(funcName, NULL, SYM_FUNC, kind->line);
        if (!funcSym) {
            printf("Error type 4 at Line %d: Redefined function \"%s\".\n",
                   kind->line, funcName);
            error_count++;
        } else {
            funcSym->returnType = returnType;
        }

        // 进入函数作用域（参数 + 函数体）
        Symbol* prevFunc = currentFunc;
        currentFunc = funcSym;
        enterScope();

        // 处理参数列表 FunDec → ID LP VarList RP  或  ID LP RP
        if (kind->num == 4) {
            Node* varList = kind->child[2]; // VarList
            Node* cur = varList;
            while (cur && strcmp(cur->name, "VarList") == 0) {
                Node* paramDec = cur->child[0]; // ParamDec → Specifier VarDec
                Type* pType = getTypeFromSpecifier(paramDec->child[0]);
                processVarDec(paramDec->child[1], pType);

                // 收集参数类型到函数符号
                if (funcSym) {
                    Type* paramCopy = copyType(pType);
                    funcSym->paramTypes = (Type**)realloc(funcSym->paramTypes,
                        (funcSym->paramCount + 1) * sizeof(Type*));
                    funcSym->paramTypes[funcSym->paramCount++] = paramCopy;
                }

                cur = (cur->num == 3) ? cur->child[2] : NULL;
            }
        }

        // 遍历函数体的 CompSt（已在函数作用域内，不重复 enterScope）
        if (node->num >= 3 && node->child[2]) {
            Node* compSt = node->child[2];
            for (int i = 0; i < compSt->num; i++)
                traverse(compSt->child[i]);
        }

        exitScope();
        currentFunc = prevFunc;

    } else if (strcmp(kind->name, "ExtDecList") == 0) {
        // ===== 全局变量: Specifier ExtDecList SEMI =====
        Type* baseType = getTypeFromSpecifier(spec);
        processExtDecList(kind, baseType);
    }
}

/* ================================================================
 *  局部变量定义处理 (Def)
 * ================================================================ */

static void processDef(Node* node) {
    // Def → Specifier DecList SEMI
    Type* baseType = getTypeFromSpecifier(node->child[0]);
    processDecList(node->child[1], baseType);
}

/* ================================================================
 *  DecList 链遍历
 *  DecList → Dec | Dec COMMA DecList
 * ================================================================ */

static void processDecList(Node* decList, Type* baseType) {
    if (!decList) return;

    // Dec → VarDec | VarDec ASSIGNOP Exp
    Node* dec = decList->child[0];
    processVarDec(dec->child[0], baseType);

    // 有初始化表达式则检查
    if (dec->num == 3)
        traverse(dec->child[2]);

    // 递归处理逗号链
    if (decList->num == 3)
        processDecList(decList->child[2], baseType);
}

/* ================================================================
 *  ExtDecList 链遍历
 *  ExtDecList → VarDec | VarDec COMMA ExtDecList
 * ================================================================ */

static void processExtDecList(Node* extDecList, Type* baseType) {
    if (!extDecList) return;

    processVarDec(extDecList->child[0], baseType);

    if (extDecList->num == 3)
        processExtDecList(extDecList->child[2], baseType);
}

/* ================================================================
 *  变量声明 (VarDec)
 *  VarDec → ID | VarDec LB INT RB
 * ================================================================ */

static void processVarDec(Node* varDec, Type* type) {
    if (!varDec) return;

    if (varDec->num == 1) {
        // 简单变量: ID
        char* name = varDec->child[0]->val.str;
        Type* ownType = copyType(type);
        Symbol* sym = insertSymbol(name, ownType, SYM_VAR, varDec->line);
        if (!sym) {
            printf("Error type 3 at Line %d: Redefined variable \"%s\".\n",
                   varDec->line, name);
            error_count++;
            freeType(ownType);
        }
        return;
    }

    if (varDec->num == 4) {
        // 数组: VarDec LB INT RB
        int size = varDec->child[2]->val.ival;
        Type* arrType = createArrayType(copyType(type), size);
        processVarDec(varDec->child[0], arrType);
        return;
    }
}

/* ================================================================
 *  结构体定义
 * ================================================================ */

static void processStructDef(Node* structSpec) {
    if (!structSpec || structSpec->num != 5) return;
    // StructSpecifier → STRUCT OptTag LC DefList RC
    if (!structSpec->child[1]) return; // 匿名结构体

    char* name = structSpec->child[1]->child[0]->val.str;
    Symbol* structSym = insertSymbol(name, NULL, SYM_STRUCT, structSpec->line);
    if (!structSym) {
        printf("Error type 16 at Line %d: Duplicated name \"%s\".\n",
               structSpec->line, name);
        error_count++;
        return;
    }

    // 遍历 DefList 收集字段
    Node* defList = structSpec->child[3];
    Node* cur = defList;
    while (cur && cur->num > 0 && strcmp(cur->name, "DefList") == 0) {
        Node* def = cur->child[0]; // Def → Specifier DecList SEMI
        Type* fieldType = getTypeFromSpecifier(def->child[0]);
        collectStructFields(def->child[1], fieldType, structSym);
        cur = (cur->num >= 2 && cur->child[1]) ? cur->child[1] : NULL;
    }
}

static void collectStructFields(Node* decList, Type* fieldType, Symbol* structSym) {
    if (!decList || !structSym) return;
    // DecList → Dec | Dec COMMA DecList
    Node* dec = decList->child[0]; // Dec → VarDec | VarDec ASSIGNOP Exp
    Node* varDec = dec->child[0];
    char* fieldName = getBaseVarName(varDec);
    if (!fieldName) return;

    // 检查字段是否重复定义
    for (int i = 0; i < structSym->fieldCount; i++) {
        if (strcmp(structSym->fields[i]->name, fieldName) == 0) {
            printf("Error type 15 at Line %d: Redefined field \"%s\".\n",
                   varDec->line, fieldName);
            error_count++;
            // 仍然递归处理后续字段
            if (decList->num == 3)
                collectStructFields(decList->child[2], fieldType, structSym);
            return;
        }
    }

    Symbol* field = (Symbol*)malloc(sizeof(Symbol));
    strncpy(field->name, fieldName, MAX_NAME - 1);
    field->name[MAX_NAME - 1] = '\0';
    field->type = copyType(fieldType);
    field->kind = SYM_VAR;
    field->line = varDec->line;
    field->returnType = NULL;
    field->paramTypes = NULL;
    field->paramCount = 0;
    field->fields = NULL;
    field->fieldCount = 0;

    structSym->fields = (Symbol**)realloc(structSym->fields,
        (structSym->fieldCount + 1) * sizeof(Symbol*));
    structSym->fields[structSym->fieldCount++] = field;

    if (decList->num == 3)
        collectStructFields(decList->child[2], fieldType, structSym);
}

/* ================================================================
 *  辅助：获取 VarDec 递归链的最底层变量名
 *  a[10] → "a",  a[10][20] → "a"
 * ================================================================ */

static char* getBaseVarName(Node* varDec) {
    while (varDec && varDec->num == 4)
        varDec = varDec->child[0];
    if (varDec && varDec->num == 1)
        return varDec->child[0]->val.str;
    return NULL;
}
