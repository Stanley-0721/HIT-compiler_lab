#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h" 
#include "syntax.tab.h"
extern FILE* yyin;
int has_lexical_error = 0; // 标记是否有词法错误
int has_syntax_error = 0;  // 标记是否有语法错误
// 错误处理函数
void yyerror(const char *msg) {
  fprintf(stderr, "Error type B at Line %d: syntax error.\n", yylineno);
  has_syntax_error = 1;
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

  if (strcmp(root->name, "INT") == 0) {
    printf("INT: %d", root->val.ival);
  } 
  else if (strcmp(root->name, "FLOAT") == 0) {
    printf("FLOAT: %.2f", root->val.fval);
  } 
  else if (strcmp(root->name, "ID") == 0) {
    printf("ID: %s", root->val.str);
  } 
  else if (strcmp(root->name, "TYPE") == 0) {
    printf("TYPE: %s", root->val.str);
  }
  else if (
    strcmp(root->name, "SEMI")==0 || strcmp(root->name, "COMMA")==0 ||
    strcmp(root->name, "ASSIGNOP")==0 || strcmp(root->name, "RELOP")==0 ||
    strcmp(root->name, "PLUS")==0 || strcmp(root->name, "MINUS")==0 ||
    strcmp(root->name, "STAR")==0 || strcmp(root->name, "DIV")==0 ||
    strcmp(root->name, "AND")==0 || strcmp(root->name, "OR")==0 ||
    strcmp(root->name, "DOT")==0 || strcmp(root->name, "NOT")==0 ||
    strcmp(root->name, "LP")==0 || strcmp(root->name, "RP")==0 ||
    strcmp(root->name, "LB")==0 || strcmp(root->name, "RB")==0 ||
    strcmp(root->name, "LC")==0 || strcmp(root->name, "RC")==0 ||
    strcmp(root->name, "STRUCT")==0 || strcmp(root->name, "RETURN")==0 ||
    strcmp(root->name, "IF")==0 || strcmp(root->name, "ELSE")==0 ||
    strcmp(root->name, "WHILE")==0
  ) {
    printf("%s", root->name);
  }
  else {
    printf("%s (%d)", root->name, root->line);
  }

  printf("\n");

  // 递归打印子节点
  for (int i = 0; i < root->num; i++) {
    printTree(root->child[i], depth + 1);
  }
}

int main(int argc,char** argv){
  if(argc>1){
    if(!(yyin=fopen(argv[1],"r"))){
      perror(argv[1]);
      return 1;
    }
  }

  yyparse();

  extern Node* root;
  if (has_lexical_error || has_syntax_error) {
    return 0;
  }

  if (root != NULL) {
      printTree(root, 0);
      freeTree(root);
  }
  return 0;
}