#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

// AST Node Types
typedef enum {
    NODE_PROGRAM,
    NODE_STMT_LIST,
    NODE_DECL,
    NODE_ASSIGN,
    NODE_BUILT_IN_CALL,
    NODE_EXPR,
    NODE_TERM
} ASTNodeType;

// AST Node Structure
typedef struct ASTNode {
    ASTNodeType nodeType;
    TokenType tokenType;                  // Original token type from lexer
    char value[MAX_LEN];
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *next;
} ASTNode;

// External declarations
extern int currentToken;

// Function declarations
ASTNode* parse_program(void);
void print_ast(ASTNode *root);
void free_ast(ASTNode *node);

#endif
