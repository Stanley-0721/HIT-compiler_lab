%{
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex(void);
extern int has_syntax_error;
Node* root = NULL;
%}

%union{
    struct Node* tree;
};

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

Program : ExtDefList {
    $$ = createTree("Program", ($1 != NULL) ? $1->line : 1);
    if($1) addChild($$, $1);
    root = $$;
};

ExtDefList : ExtDef ExtDefList {
    $$ = createTree("ExtDefList", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
}
| /* empty */ {
    $$ = NULL;
};

ExtDef : Specifier ExtDecList SEMI {
    $$ = createTree("ExtDef", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Specifier SEMI {
    $$ = createTree("ExtDef", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
}
| Specifier FunDec CompSt {
    $$ = createTree("ExtDef", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
};

ExtDecList : VarDec {
    $$ = createTree("ExtDecList", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
}
| VarDec COMMA ExtDecList {
    $$ = createTree("ExtDecList", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
};

Specifier : TYPE {
    $$ = createTree("Specifier", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
}
| StructSpecifier {
    $$ = createTree("Specifier", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
};

StructSpecifier : STRUCT OptTag LC DefList RC {
    $$ = createTree("StructSpecifier", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    if ($2) addChild($$, $2);
    addChild($$, $3);
    if ($4) addChild($$, $4);
    addChild($$, $5);
}
| STRUCT Tag {
    $$ = createTree("StructSpecifier", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
};

OptTag : ID {
    $$ = createTree("OptTag", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
}
| /* empty */ {
    $$ = NULL;
};

Tag : ID {
    $$ = createTree("Tag", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
};

VarDec : ID {
    $$ = createTree("VarDec", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
}
| VarDec LB INT RB {
    $$ = createTree("VarDec", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
};

FunDec : ID LP VarList RP {
    $$ = createTree("FunDec", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
}
| ID LP RP {
    $$ = createTree("FunDec", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
};

VarList : ParamDec COMMA VarList {
    $$ = createTree("VarList", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| ParamDec {
    $$ = createTree("VarList", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
};

ParamDec : Specifier VarDec {
    $$ = createTree("ParamDec", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
};

CompSt : LC DefList StmtList RC {
    $$ = createTree("CompSt", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    if ($2) addChild($$, $2);
    if ($3) addChild($$, $3);
    addChild($$, $4);
}
;

StmtList : Stmt StmtList {
    $$ = createTree("StmtList", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    if ($2) addChild($$, $2);
}
| /* empty */ {
    $$ = NULL;
};

Stmt : Exp SEMI {
    $$ = createTree("Stmt", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
}
| CompSt {
    $$ = createTree("Stmt", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
}
| RETURN Exp SEMI {
    $$ = createTree("Stmt", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| IF LP Exp RP Stmt {
    $$ = createTree("Stmt", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
    addChild($$, $5);
}
| IF LP Exp RP Stmt ELSE Stmt {
    $$ = createTree("Stmt", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
    addChild($$, $5);
    addChild($$, $6);
    addChild($$, $7);
}
| WHILE LP Exp RP Stmt {
    $$ = createTree("Stmt", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
    addChild($$, $5);
}
| error SEMI    { $$ = NULL; }
| error RC      { $$ = NULL; }
;

DefList : Def DefList {
    $$ = createTree("DefList", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    if ($2) addChild($$, $2);
}
| /* empty */ {
    $$ = NULL;
};

Def : Specifier DecList SEMI {
    $$ = createTree("Def", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
};

DecList : Dec {
    $$ = createTree("DecList", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
}
| Dec COMMA DecList {
    $$ = createTree("DecList", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
};

Dec : VarDec {
    $$ = createTree("Dec", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
}
| VarDec ASSIGNOP Exp {
    $$ = createTree("Dec", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
};

Exp : Exp ASSIGNOP Exp {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp AND Exp {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp OR Exp {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp RELOP Exp {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp PLUS Exp {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp MINUS Exp {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp STAR Exp {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp DIV Exp {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| LP Exp RP {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| MINUS Exp {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
}
| NOT Exp {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
}
| ID LP Args RP {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
}
| ID LP RP {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp LB Exp RB {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
    addChild($$, $4);
}
| Exp DOT ID {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| ID {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
}
| INT {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
}
| FLOAT {
    $$ = createTree("Exp", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
};

Args : Exp COMMA Args {
    $$ = createTree("Args", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
    addChild($$, $2);
    addChild($$, $3);
}
| Exp {
    $$ = createTree("Args", ($1 != NULL) ? $1->line : 1);
    addChild($$, $1);
};

%%