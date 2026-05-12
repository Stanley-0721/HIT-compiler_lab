#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

/* ================================================================
 *  类型系统
 * ================================================================ */

Type* createType(TypeKind kind) {
    Type* t = (Type*)malloc(sizeof(Type));
    t->kind = kind;
    t->structName[0] = '\0';
    t->isArray = 0;
    t->elementType = NULL;
    t->arraySize = 0;
    return t;
}

Type* createStructType(const char* name) {
    Type* t = createType(TYPE_STRUCT);
    strncpy(t->structName, name, MAX_NAME - 1);
    t->structName[MAX_NAME - 1] = '\0';
    return t;
}

Type* createArrayType(Type* elemType, int size) {
    Type* t = (Type*)malloc(sizeof(Type));
    t->kind = elemType->kind;
    strncpy(t->structName, elemType->structName, MAX_NAME - 1);
    t->structName[MAX_NAME - 1] = '\0';
    t->isArray = 1;
    t->elementType = elemType;
    t->arraySize = size;
    return t;
}

void freeType(Type* type) {
    if (!type) return;
    if (type->isArray && type->elementType)
        freeType(type->elementType);
    free(type);
}

/* ================================================================
 *  符号表
 * ================================================================ */

static Scope* globalScope = NULL;
static Scope* currentScope = NULL;

void initSymbolTable(void) {
    Scope* gs = (Scope*)malloc(sizeof(Scope));
    gs->symbols = NULL;
    gs->count = 0;
    gs->capacity = 0;
    gs->outer = NULL;
    globalScope = gs;
    currentScope = gs;
}

void enterScope(void) {
    Scope* scope = (Scope*)malloc(sizeof(Scope));
    scope->symbols = NULL;
    scope->count = 0;
    scope->capacity = 0;
    scope->outer = currentScope;
    currentScope = scope;
}

void exitScope(void) {
    if (currentScope == NULL || currentScope == globalScope) return;
    Scope* old = currentScope;
    currentScope = currentScope->outer;

    for (int i = 0; i < old->count; i++) {
        Symbol* sym = old->symbols[i];
        if (sym->type) freeType(sym->type);
        if (sym->returnType) freeType(sym->returnType);
        if (sym->params) {
            for (int j = 0; j < sym->paramCount; j++) {
                if (sym->params[j]->type) freeType(sym->params[j]->type);
                free(sym->params[j]);
            }
            free(sym->params);
        }
        if (sym->fields) {
            for (int j = 0; j < sym->fieldCount; j++) {
                if (sym->fields[j]->type) freeType(sym->fields[j]->type);
                free(sym->fields[j]);
            }
            free(sym->fields);
        }
        free(sym);
    }
    free(old->symbols);
    free(old);
}

Symbol* insertSymbol(const char* name, Type* type, SymKind kind, int line) {
    for (int i = 0; i < currentScope->count; i++) {
        if (strcmp(currentScope->symbols[i]->name, name) == 0) {
            printf("Error type 3 at Line %d: Redefined %s \"%s\"\n",
                   line,
                   kind == SYM_FUNC ? "function" :
                   kind == SYM_STRUCT ? "struct" : "variable",
                   name);
            return NULL;
        }
    }

    Symbol* sym = (Symbol*)malloc(sizeof(Symbol));
    strncpy(sym->name, name, MAX_NAME - 1);
    sym->name[MAX_NAME - 1] = '\0';
    sym->type = type;
    sym->kind = kind;
    sym->line = line;
    sym->returnType = NULL;
    sym->params = NULL;
    sym->paramCount = 0;
    sym->fields = NULL;
    sym->fieldCount = 0;

    if (currentScope->count >= currentScope->capacity) {
        currentScope->capacity = currentScope->capacity ? currentScope->capacity * 2 : 8;
        currentScope->symbols = (Symbol**)realloc(
            currentScope->symbols,
            currentScope->capacity * sizeof(Symbol*));
    }
    currentScope->symbols[currentScope->count++] = sym;
    return sym;
}

Symbol* lookupSymbol(const char* name) {
    Scope* s = currentScope;
    while (s) {
        for (int i = 0; i < s->count; i++) {
            if (strcmp(s->symbols[i]->name, name) == 0)
                return s->symbols[i];
        }
        s = s->outer;
    }
    return NULL;
}

Symbol* lookupSymbolInScope(const char* name) {
    for (int i = 0; i < currentScope->count; i++) {
        if (strcmp(currentScope->symbols[i]->name, name) == 0)
            return currentScope->symbols[i];
    }
    return NULL;
}

Symbol* lookupStructField(const char* structName, const char* fieldName) {
    Symbol* structSym = lookupSymbol(structName);
    if (!structSym) return NULL;
    if (structSym->kind != SYM_STRUCT) return NULL;
    for (int i = 0; i < structSym->fieldCount; i++) {
        if (strcmp(structSym->fields[i]->name, fieldName) == 0)
            return structSym->fields[i];
    }
    return NULL;
}

void freeSymbolTable(void) {
    while (currentScope && currentScope != globalScope)
        exitScope();
    if (globalScope) {
        Scope* old = globalScope;
        for (int i = 0; i < old->count; i++) {
            Symbol* sym = old->symbols[i];
            if (sym->type) freeType(sym->type);
            if (sym->returnType) freeType(sym->returnType);
            if (sym->params) {
                for (int j = 0; j < sym->paramCount; j++) {
                    if (sym->params[j]->type) freeType(sym->params[j]->type);
                    free(sym->params[j]);
                }
                free(sym->params);
            }
            if (sym->fields) {
                for (int j = 0; j < sym->fieldCount; j++) {
                    if (sym->fields[j]->type) freeType(sym->fields[j]->type);
                    free(sym->fields[j]);
                }
                free(sym->fields);
            }
            free(sym);
        }
        free(old->symbols);
        free(old);
        globalScope = NULL;
        currentScope = NULL;
    }
}
