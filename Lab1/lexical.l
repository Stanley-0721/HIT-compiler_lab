%{
#include <stdio.h>    
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "syntax.tab.h"
extern int has_lexical_error;
%}

%option yylineno
SPACE [ \r\t\n]+
INT ([1-9][0-9]*|0)
FLOAT [0-9]+\.[0-9]+
ID [a-zA-Z_][a-zA-Z0-9_]*
SEMI ;
COMMA ,
ASSIGNOP =
RELOP >|<|>=|<=|==|!=
PLUS \+
MINUS \-
STAR \*
DIV \/
AND &&
OR \|\|
DOT \.
NOT !
TYPE int|float
LP \(
RP \)
LB \[
RB \]
LC \{
RC \}
STRUCT struct
RETURN return
IF if
ELSE else
WHILE while
%%

{SPACE} {}

{INT}        {
    yylval.tree = createTree("INT", yylineno);
    yylval.tree->val.ival = atoi(yytext);
    return INT;
}

{FLOAT}      {
    yylval.tree = createTree("FLOAT", yylineno);
    yylval.tree->val.fval = atof(yytext);
    return FLOAT;
}

{SEMI}       { yylval.tree = createTree("SEMI", yylineno); return SEMI; }
{COMMA}      { yylval.tree = createTree("COMMA", yylineno); return COMMA; }
{ASSIGNOP}   { yylval.tree = createTree("ASSIGNOP", yylineno); return ASSIGNOP; }
{RELOP}      { yylval.tree = createTree("RELOP", yylineno); return RELOP; }
{PLUS}       { yylval.tree = createTree("PLUS", yylineno); return PLUS; }
{MINUS}      { yylval.tree = createTree("MINUS", yylineno); return MINUS; }
{STAR}       { yylval.tree = createTree("STAR", yylineno); return STAR; }
{DIV}        { yylval.tree = createTree("DIV", yylineno); return DIV; }
{AND}        { yylval.tree = createTree("AND", yylineno); return AND; }
{OR}         { yylval.tree = createTree("OR", yylineno); return OR; }
{DOT}        { yylval.tree = createTree("DOT", yylineno); return DOT; }
{NOT}        { yylval.tree = createTree("NOT", yylineno); return NOT; }

{TYPE}       {
    yylval.tree = createTree("TYPE", yylineno);
    strncpy(yylval.tree->val.str, yytext, MAX_NAME-1);
    yylval.tree->val.str[MAX_NAME-1] = '\0';
    return TYPE;
}

{LP}         { yylval.tree = createTree("LP", yylineno); return LP; }
{RP}         { yylval.tree = createTree("RP", yylineno); return RP; }
{LB}         { yylval.tree = createTree("LB", yylineno); return LB; }
{RB}         { yylval.tree = createTree("RB", yylineno); return RB; }
{LC}         { yylval.tree = createTree("LC", yylineno); return LC; }
{RC}         { yylval.tree = createTree("RC", yylineno); return RC; }
{STRUCT}     { yylval.tree = createTree("STRUCT", yylineno); return STRUCT; }
{RETURN}     { yylval.tree = createTree("RETURN", yylineno); return RETURN; }
{IF}         { yylval.tree = createTree("IF", yylineno); return IF; }
{ELSE}       { yylval.tree = createTree("ELSE", yylineno); return ELSE; }
{WHILE}      { yylval.tree = createTree("WHILE", yylineno); return WHILE; }

{ID}         {
    yylval.tree = createTree("ID", yylineno);
    strncpy(yylval.tree->val.str, yytext, MAX_NAME-1);
    yylval.tree->val.str[MAX_NAME-1] = '\0';
    return ID;
}

. {
    printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext);
    has_lexical_error = 1;
}

%%