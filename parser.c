#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int currentToken = 0;

// Helper Functions
static ASTNode* create_node(ASTNodeType type, const char *value, TokenType tokType){
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    if(!node){
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    node->nodeType = type;
    node->tokenType = tokType;
    strncpy(node->value, value ? value : "", MAX_LEN - 1);
    node->value[MAX_LEN - 1] = '\0';
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    return node;
}

static Token* peek(void){
    if(currentToken < tokenCount){
        return &tokenArray[currentToken];
    }
    return NULL;
}

static Token* consume(void){
    if(currentToken < tokenCount){
        return &tokenArray[currentToken++];
    }
    return NULL;
}

static int match(TokenType type){
    Token *t = peek();
    return t && t->type == type;
}

static int match_value(const char *value){
    Token *t = peek();
    return t && strcmp(t->value, value) == 0;
}

// Forward declarations
static ASTNode* parse_expr(void);
static ASTNode* parse_stmt(void);

// <term> ::= <number> | <identifier> | "(" <expr> ")"
static ASTNode* parse_term(void){
    Token *t = peek();
    
    if(!t){
        fprintf(stderr, "Parse error: unexpected end of input\n");
        exit(1);
    }
    
    // <number>
    if(match(NUMBER)){
        ASTNode *node = create_node(NODE_TERM, t->value, NUMBER);
        consume();
        return node;
    }
    
    // <identifier>
    if(match(IDENTIFIER)){
        ASTNode *node = create_node(NODE_TERM, t->value, IDENTIFIER);
        consume();
        return node;
    }
    
    // "(" <expr> ")"
    if(match_value("(")){
        consume(); // consume "("
        ASTNode *expr = parse_expr();
        if(!match_value(")")){
            fprintf(stderr, "Parse error: expected ')'\n");
            exit(1);
        }
        consume(); // consume ")"
        return expr;
    }
    
    fprintf(stderr, "Parse error: expected term, got '%s'\n", t->value);
    exit(1);
}

// <expr> ::= <term> { "+" <term> }*
static ASTNode* parse_expr(void){
    ASTNode *left = parse_term();
    
    // { "+" <term> }*
    while(match(OPERATOR) && match_value("+")){
        consume(); // consume "+"
        ASTNode *right = parse_term();
        ASTNode *expr = create_node(NODE_EXPR, "+", OPERATOR);
        expr->left = left;
        expr->right = right;
        left = expr;
    }
    
    return left;
}

// <decl> ::= <type> <identifier> ";" | <type> <identifier> "=" <expr> ";"
static ASTNode* parse_decl(void){
    Token *type = consume(); // "int" or "float"
    
    if(!match(IDENTIFIER)){
        fprintf(stderr, "Parse error: expected identifier after type\n");
        exit(1);
    }
    
    Token *id = consume();
    
    ASTNode *node = create_node(NODE_DECL, type->value, KEYWORD);
    node->left = create_node(NODE_TERM, id->value, IDENTIFIER);
    
    // Optional: "=" <expr>
    if(match_value("=")){
        consume(); // consume "="
        ASTNode *expr = parse_expr();
        node->right = expr;
    }
    
    if(!match_value(";")){
        fprintf(stderr, "Parse error: expected ';' after declaration\n");
        exit(1);
    }
    consume(); // consume ";"
    
    return node;
}

// <assign> ::= <identifier> "=" <expr> ";"
static ASTNode* parse_assign(void){
    Token *id = consume();
    
    if(!match_value("=")){
        fprintf(stderr, "Parse error: expected '=' in assignment\n");
        exit(1);
    }
    consume(); // consume "="
    
    ASTNode *expr = parse_expr();
    
    if(!match_value(";")){
        fprintf(stderr, "Parse error: expected ';' after assignment\n");
        exit(1);
    }
    consume(); // consume ";"
    
    ASTNode *node = create_node(NODE_ASSIGN, id->value, IDENTIFIER);
    node->left = expr;
    return node;
}

// <built_in_call> ::= <built_in_name> "(" <expr> ")" ";"
static ASTNode* parse_built_in_call(void){
    Token *func = consume(); // consume built-in function name
    
    if(!match_value("(")){
        fprintf(stderr, "Parse error: expected '(' after %s\n", func->value);
        exit(1);
    }
    consume(); // consume "("
    
    ASTNode *expr = parse_expr();
    
    if(!match_value(")")){
        fprintf(stderr, "Parse error: expected ')' in %s call\n", func->value);
        exit(1);
    }
    consume(); // consume ")"
    
    if(!match_value(";")){
        fprintf(stderr, "Parse error: expected ';' after %s call\n", func->value);
        exit(1);
    }
    consume(); // consume ";"
    
    ASTNode *node = create_node(NODE_BUILT_IN_CALL, func->value, BUILT_FUNCTION);
    node->left = expr;
    return node;
}

// <stmt> ::= <decl> | <assign> | <built_in_call>
static ASTNode* parse_stmt(void){
    Token *t = peek();
    
    if(!t || t->type == END){
        return NULL;
    }
    
    // <built_in_call> - any built-in function
    if(match(BUILT_FUNCTION)){
        return parse_built_in_call();
    }
    
    // <decl>
    if(match(KEYWORD)){
        return parse_decl();
    }
    
    // <assign>
    if(match(IDENTIFIER)){
        return parse_assign();
    }
    
    fprintf(stderr, "Parse error: unexpected token '%s'\n", t->value);
    exit(1);
}

// <stmt_list> ::= <stmt> <stmt_list> | <stmt>
static ASTNode* parse_stmt_list(void){
    ASTNode *stmt = parse_stmt();
    if(!stmt) return NULL;
    
    // Check if there are more statements
    ASTNode *next = parse_stmt_list();
    stmt->next = next;
    
    return stmt;
}

// <program> ::= <stmt_list>
ASTNode* parse_program(void){
    ASTNode *root = create_node(NODE_PROGRAM, "program", END);
    root->left = parse_stmt_list();
    return root;
}
















//////////////////////// AST Printing ////////////////////////////////

static void print_ast_helper(ASTNode *node, const char *prefix, int isLast){
    if(!node) return;
    
    printf("%s", prefix);
    printf("%s", isLast ? "└── " : "├── ");
    
    switch(node->nodeType){
        case NODE_PROGRAM:
            printf("Program\n");
            break;
        case NODE_STMT_LIST:
            printf("Statement List\n");
            break;
        case NODE_DECL:
            printf("Declaration: %s\n", node->value);
            break;
        case NODE_ASSIGN:
            printf("Assignment: %s\n", node->value);
            break;
        case NODE_BUILT_IN_CALL:
            printf("Built-in Call: %s()\n", node->value);
            break;
        case NODE_EXPR:
            printf("Expression: %s\n", node->value);
            break;
        case NODE_TERM:
            printf("Term: %s\n", node->value);
            break;
    }
    
    char newPrefix[1024];
    snprintf(newPrefix, sizeof(newPrefix), "%s%s", prefix, isLast ? "    " : "│   ");
    
    if(node->left && node->right){
        print_ast_helper(node->left, newPrefix, 0);
        print_ast_helper(node->right, newPrefix, 1);
    } else if(node->left){
        print_ast_helper(node->left, newPrefix, 1);
    } else if(node->right){
        print_ast_helper(node->right, newPrefix, 1);
    }
    
    // Don't traverse next here - it's handled by print_ast()
}

void print_ast(ASTNode *root){
    printf("\n\n");
    
    if(root){
        printf("Program\n");
        ASTNode *stmt = root->left;
        while(stmt){
            print_ast_helper(stmt, "", stmt->next == NULL);
            stmt = stmt->next;
        }
    }
    printf("\n");
}

void free_ast(ASTNode *node){
    if(!node) return;
    free_ast(node->left);
    free_ast(node->right);
    free_ast(node->next);
    free(node);
}
