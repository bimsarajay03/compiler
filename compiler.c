#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>

// ============================================================================
// CONSTANTS AND DEFINES
// ============================================================================

#define MAX_TOKENS 1000
#define MAX_LEN 100
#define MAX_RUNTIME_VARS 100

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

// Token Types
typedef enum {
    KEYWORD,
    BUILT_FUNCTION,
    IDENTIFIER,
    NUMBER,
    OPERATOR,
    SYMBOL,
    END
} TokenType;

// Token Structure
typedef struct {
    TokenType type;
    char value[MAX_LEN];
} Token;

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
    TokenType tokenType;
    char value[MAX_LEN];
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *next;
} ASTNode;

// Symbol table row
typedef struct {
    char name[MAX_LEN];
    char type[MAX_LEN];
    int isInitialized;
    int scope;
    int lineNumber;
} SymbolEntry;

// Symbol table structure
typedef struct {
    SymbolEntry symbols[MAX_TOKENS];
    int count;
} SymbolTable;

// Runtime variable entry
typedef struct {
    char name[MAX_LEN];
    char type[MAX_LEN];
    union {
        int intValue;
        double floatValue;
    } value;
    int isDefined;
} RuntimeVar;

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Lexer globals
Token tokenArray[MAX_TOKENS];
int tokenCount = 0;

const char *KEYWORDS[] = {"int", "float"};
const char *BUILT_FUNCTIONS[] = {"print"};
const char *OPERATORS[] = {"+", "="};
const char *SYMBOLS[] = {"(", ")", ";"};

const size_t KEYWORD_COUNT = 2;
const size_t BUILT_FUNCTION_COUNT = 1;
const size_t OPERATOR_COUNT = 2;
const size_t SYMBOL_COUNT = 3;

// Parser globals
int currentToken = 0;

// Semantic globals
static SymbolTable symbolTable;
static int lineCounter = 0;

// Execute globals
static RuntimeVar runtimeTable[MAX_RUNTIME_VARS];
static int runtimeCount = 0;

// ============================================================================
// LEXER 
// ============================================================================

static int matches_list(const char *value, const char *list[], size_t count){
    for(size_t i = 0; i < count; i++){
        if(strcmp(value, list[i]) == 0){
            return 1;
        }
    }
    return 0;
}

static int is_operator_start(int c){
    for(size_t i = 0; i < OPERATOR_COUNT; i++){
        if(OPERATORS[i][0] == (char)c){
            return 1;
        }
    }
    return 0;
}

static int is_symbol_char(int c){
    for(size_t i = 0; i < SYMBOL_COUNT; i++){
        if(SYMBOLS[i][0] == (char)c){
            return 1;
        }
    }
    return 0;
}

static void add_token(TokenType type, const char *value){
    if(tokenCount >= MAX_TOKENS){
        return;
    }
    tokenArray[tokenCount].type = type;
    strncpy(tokenArray[tokenCount].value, value, MAX_LEN - 1);
    tokenArray[tokenCount].value[MAX_LEN - 1] = '\0';
    tokenCount++;
}

const char *token_type_name(TokenType type){
    switch(type){
        case KEYWORD: return "KEYWORD";
        case BUILT_FUNCTION: return "BUILT_FUNCTION";
        case IDENTIFIER: return "IDENTIFIER";
        case NUMBER: return "NUMBER";
        case OPERATOR: return "OPERATOR";
        case SYMBOL: return "SYMBOL";
        case END: return "END";
        default: return "UNKNOWN";
    }
}

void print_tokens(void){
    for(int i = 0; i < tokenCount; i++){
        printf("%s: %s\n", token_type_name(tokenArray[i].type), tokenArray[i].value);
    }
}

void tockenise(char *fileName){
    char buffer[1024];
    char current[MAX_LEN];
    size_t cur_len = 0;
    regex_t ident_re;
    regex_t number_re;
    FILE *fptr = fopen(fileName, "r");
    if(fptr == NULL){
        perror("fopen");
        return;
    }

    //compile the regex
    if(regcomp(&ident_re, "^[A-Za-z_][A-Za-z0-9_]*$", REG_EXTENDED) != 0 ||
       regcomp(&number_re, "^[0-9]+(\\.[0-9]+)?$", REG_EXTENDED) != 0){
        fclose(fptr);
        return;
    }

    int c;
    while((c = fgetc(fptr)) != EOF){
        if(isspace(c)){
            if(cur_len > 0){
                current[cur_len] = '\0';
                if(matches_list(current, KEYWORDS, KEYWORD_COUNT)){
                    add_token(KEYWORD, current);
                } else if(matches_list(current, BUILT_FUNCTIONS, BUILT_FUNCTION_COUNT)){
                    add_token(BUILT_FUNCTION, current);
                } else if(regexec(&number_re, current, 0, NULL, 0) == 0){
                    add_token(NUMBER, current);
                } else if(regexec(&ident_re, current, 0, NULL, 0) == 0){
                    add_token(IDENTIFIER, current);
                } else {
                    fprintf(stderr, "Lexical error: invalid token '%s'\n", current);
                    exit(1);
                }
                cur_len = 0;
            }
            continue;
        }

        if(is_symbol_char(c)){
            if(cur_len > 0){
                current[cur_len] = '\0';
                if(matches_list(current, KEYWORDS, KEYWORD_COUNT)){
                    add_token(KEYWORD, current);
                } else if(matches_list(current, BUILT_FUNCTIONS, BUILT_FUNCTION_COUNT)){
                    add_token(BUILT_FUNCTION, current);
                } else if(regexec(&number_re, current, 0, NULL, 0) == 0){
                    add_token(NUMBER, current);
                } else if(regexec(&ident_re, current, 0, NULL, 0) == 0){
                    add_token(IDENTIFIER, current);
                } else {
                    fprintf(stderr, "Lexical error: invalid token '%s'\n", current);
                    exit(1);
                }
                cur_len = 0;
            }

            buffer[0] = (char)c;
            buffer[1] = '\0';
            add_token(SYMBOL, buffer);
            continue;
        }

        if(is_operator_start(c)){
            if(cur_len > 0){
                current[cur_len] = '\0';
                if(matches_list(current, KEYWORDS, KEYWORD_COUNT)){
                    add_token(KEYWORD, current);
                } else if(matches_list(current, BUILT_FUNCTIONS, BUILT_FUNCTION_COUNT)){
                    add_token(BUILT_FUNCTION, current);
                } else if(regexec(&number_re, current, 0, NULL, 0) == 0){
                    add_token(NUMBER, current);
                } else if(regexec(&ident_re, current, 0, NULL, 0) == 0){
                    add_token(IDENTIFIER, current);
                } else {
                    fprintf(stderr, "Lexical error: invalid token '%s'\n", current);
                    exit(1);
                }
                cur_len = 0;
            }

            char op[3];
            op[0] = (char)c;
            op[1] = '\0';
            op[2] = '\0';

            int next = fgetc(fptr);
            if(next != EOF){
                op[1] = (char)next;
                op[2] = '\0';
                if(!matches_list(op, OPERATORS, OPERATOR_COUNT)){
                    op[1] = '\0';
                    ungetc(next, fptr);
                }
            }

            if(matches_list(op, OPERATORS, OPERATOR_COUNT)){
                add_token(OPERATOR, op);
            }
            continue;
        }

        if(cur_len < MAX_LEN - 1){
            current[cur_len++] = (char)c;
        }
    }

    if(cur_len > 0){
        current[cur_len] = '\0';
        if(matches_list(current, KEYWORDS, KEYWORD_COUNT)){
            add_token(KEYWORD, current);
        } else if(matches_list(current, BUILT_FUNCTIONS, BUILT_FUNCTION_COUNT)){
            add_token(BUILT_FUNCTION, current);
        } else if(regexec(&number_re, current, 0, NULL, 0) == 0){
            add_token(NUMBER, current);
        } else if(regexec(&ident_re, current, 0, NULL, 0) == 0){
            add_token(IDENTIFIER, current);
        } else {
            fprintf(stderr, "Lexical error: invalid token '%s'\n", current);
            exit(1);
        }
    }

    regfree(&ident_re);
    regfree(&number_re);
    fclose(fptr);
}

// ============================================================================
// PARSER 
// ============================================================================

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

static ASTNode* parse_term(void){
    Token *t = peek();
    
    if(!t){
        fprintf(stderr, "Parse error: unexpected end of input\n");
        exit(1);
    }
    
    if(match(NUMBER)){
        ASTNode *node = create_node(NODE_TERM, t->value, NUMBER);
        consume();
        return node;
    }
    
    if(match(IDENTIFIER)){
        ASTNode *node = create_node(NODE_TERM, t->value, IDENTIFIER);
        consume();
        return node;
    }
    
    if(match_value("(")){
        consume();
        ASTNode *expr = parse_expr();
        if(!match_value(")")){
            fprintf(stderr, "Parse error: expected ')'\n");
            exit(1);
        }
        consume();
        return expr;
    }
    
    fprintf(stderr, "Parse error: expected term, got '%s'\n", t->value);
    exit(1);
}

static ASTNode* parse_expr(void){
    ASTNode *left = parse_term();
    
    while(match(OPERATOR) && match_value("+")){
        consume();
        ASTNode *right = parse_term();
        ASTNode *expr = create_node(NODE_EXPR, "+", OPERATOR);
        expr->left = left;
        expr->right = right;
        left = expr;
    }
    
    return left;
}

static ASTNode* parse_decl(void){
    Token *type = consume();
    
    if(!match(IDENTIFIER)){
        fprintf(stderr, "Parse error: expected identifier after type\n");
        exit(1);
    }
    
    Token *id = consume();
    
    ASTNode *node = create_node(NODE_DECL, type->value, KEYWORD);
    node->left = create_node(NODE_TERM, id->value, IDENTIFIER);
    
    if(match_value("=")){
        consume();
        ASTNode *expr = parse_expr();
        node->right = expr;
    }
    
    if(!match_value(";")){
        fprintf(stderr, "Parse error: expected ';' after declaration\n");
        exit(1);
    }
    consume();
    
    return node;
}

static ASTNode* parse_assign(void){
    Token *id = consume();
    
    if(!match_value("=")){
        fprintf(stderr, "Parse error: expected '=' in assignment\n");
        exit(1);
    }
    consume();
    
    ASTNode *expr = parse_expr();
    
    if(!match_value(";")){
        fprintf(stderr, "Parse error: expected ';' after assignment\n");
        exit(1);
    }
    consume();
    
    ASTNode *node = create_node(NODE_ASSIGN, id->value, IDENTIFIER);
    node->left = expr;
    return node;
}

static ASTNode* parse_built_in_call(void){
    Token *func = consume();
    
    if(!match_value("(")){
        fprintf(stderr, "Parse error: expected '(' after %s\n", func->value);
        exit(1);
    }
    consume();
    
    ASTNode *expr = parse_expr();
    
    if(!match_value(")")){
        fprintf(stderr, "Parse error: expected ')' in %s call\n", func->value);
        exit(1);
    }
    consume();
    
    if(!match_value(";")){
        fprintf(stderr, "Parse error: expected ';' after %s call\n", func->value);
        exit(1);
    }
    consume();
    
    ASTNode *node = create_node(NODE_BUILT_IN_CALL, func->value, BUILT_FUNCTION);
    node->left = expr;
    return node;
}

static ASTNode* parse_stmt(void){
    Token *t = peek();
    
    if(!t || t->type == END){
        return NULL;
    }
    
    if(match(BUILT_FUNCTION)){
        return parse_built_in_call();
    }
    
    if(match(KEYWORD)){
        return parse_decl();
    }
    
    if(match(IDENTIFIER)){
        return parse_assign();
    }
    
    fprintf(stderr, "Parse error: unexpected token '%s'\n", t->value);
    exit(1);
}

static ASTNode* parse_stmt_list(void){
    ASTNode *stmt = parse_stmt();
    if(!stmt) return NULL;
    
    ASTNode *next = parse_stmt_list();
    stmt->next = next;
    
    return stmt;
}

ASTNode* parse_program(void){
    ASTNode *root = create_node(NODE_PROGRAM, "program", END);
    root->left = parse_stmt_list();
    return root;
}

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

// ============================================================================
// SEMANTIC ANALYZER 
// ============================================================================

void init_symbol_table(SymbolTable *table){
    table->count = 0;
}

int add_symbol(SymbolTable *table, const char *name, const char *type, int isInitialized, int scope, int line){
    for(int i = 0; i < table->count; i++){
        if(strcmp(table->symbols[i].name, name) == 0 && table->symbols[i].scope == scope){
            fprintf(stderr, "Semantic error (line %d): variable '%s' already declared\n", line, name);
            return 0;
        }
    }
    
    if(table->count >= MAX_TOKENS){
        fprintf(stderr, "Symbol table overflow\n");
        return 0;
    }
    
    strncpy(table->symbols[table->count].name, name, MAX_LEN - 1);
    table->symbols[table->count].name[MAX_LEN - 1] = '\0';
    
    strncpy(table->symbols[table->count].type, type, MAX_LEN - 1);
    table->symbols[table->count].type[MAX_LEN - 1] = '\0';
    
    table->symbols[table->count].isInitialized = isInitialized;
    table->symbols[table->count].scope = scope;
    table->symbols[table->count].lineNumber = line;
    
    table->count++;
    return 1;
}

SymbolEntry* lookup_symbol(SymbolTable *table, const char *name){
    for(int i = 0; i < table->count; i++){
        if(strcmp(table->symbols[i].name, name) == 0){
            return &table->symbols[i];
        }
    }
    return NULL;
}

void print_symbol_table(SymbolTable *table){
    printf("%-15s %-10s %-12s %-8s %-8s\n", "Name", "Type", "Initialized", "Scope", "Line");
    printf("─────────────────────────────────────────────────────────────────\n");
    
    for(int i = 0; i < table->count; i++){
        printf("%-15s %-10s %-12s %-8d %-8d\n",
               table->symbols[i].name,
               table->symbols[i].type,
               table->symbols[i].isInitialized ? "Yes" : "No",
               table->symbols[i].scope,
               table->symbols[i].lineNumber);
    }
    printf("\n");
}

static const char* analyze_expr(ASTNode *node);

static const char* analyze_term(ASTNode *node){
    if(!node) return NULL;
    
    if(node->tokenType == IDENTIFIER){
        SymbolEntry *sym = lookup_symbol(&symbolTable, node->value);
        if(!sym){
            fprintf(stderr, "Semantic error: undeclared variable '%s'\n", node->value);
            exit(1);
        }
        return sym->type;
    } else if(node->tokenType == NUMBER){
        if(strchr(node->value, '.') != NULL){
            return "float";
        } else {
            return "int";
        }
    }
    
    return NULL;
}

static const char* analyze_arithmetic_expr(const char *leftType, const char *rightType, const char *op){
    if(strcmp(leftType, "int") == 0 && strcmp(rightType, "int") == 0){
        return "int";
    }
    else if(strcmp(leftType, "float") == 0 || strcmp(rightType, "float") == 0){
        return "float";
    }
    
    fprintf(stderr, "Semantic error: invalid types for arithmetic operator '%s'\n", op);
    exit(1);
}

static const char* analyze_expr(ASTNode *node){
    if(!node) return NULL;
    
    if(node->nodeType == NODE_TERM){
        return analyze_term(node);
    } else if(node->nodeType == NODE_EXPR){
        const char *leftType = analyze_expr(node->left);
        const char *rightType = analyze_expr(node->right);
        
        if(!leftType || !rightType){
            fprintf(stderr, "Semantic error: invalid expression\n");
            exit(1);
        }
        
        const char *op = node->value;
        
        if(strcmp(op, "+") == 0){
            return analyze_arithmetic_expr(leftType, rightType, op);
        }
        else {
            fprintf(stderr, "Semantic error: unknown operator '%s'\n", op);
            exit(1);
        }
    }
    
    return NULL;
}

static void analyze_stmt(ASTNode *stmt){
    if(!stmt) return;
    
    lineCounter++;
    
    switch(stmt->nodeType){
        case NODE_DECL: {
            const char *varName = stmt->left->value;
            const char *varType = stmt->value;
            
            int isInit = (stmt->right != NULL);
            
            if(isInit){
                const char *exprType = analyze_expr(stmt->right);
                
                if(strcmp(varType, "int") == 0 && strcmp(exprType, "float") == 0){
                    fprintf(stderr, "Semantic error (line %d): cannot assign float to int variable '%s'\n", 
                            lineCounter, varName);
                    exit(1);
                }
            }
            
            if(!add_symbol(&symbolTable, varName, varType, isInit, 0, lineCounter)){
                exit(1);
            }
            break;
        }
        
        case NODE_ASSIGN: {
            const char *varName = stmt->value;
            SymbolEntry *sym = lookup_symbol(&symbolTable, varName);
            
            if(!sym){
                fprintf(stderr, "Semantic error (line %d): undeclared variable '%s'\n", lineCounter, varName);
                exit(1);
            }
            
            const char *exprType = analyze_expr(stmt->left);
            
            if(strcmp(sym->type, "int") == 0 && strcmp(exprType, "float") == 0){
                fprintf(stderr, "Semantic error (line %d): cannot assign float to int variable '%s'\n", 
                        lineCounter, varName);
                exit(1);
            }
            
            sym->isInitialized = 1;
            break;
        }
        
        case NODE_BUILT_IN_CALL: {
            analyze_expr(stmt->left);
            break;
        }
        
        default:
            break;
    }
}

void check_semantics(ASTNode* root){
    if(!root) return;
    
    init_symbol_table(&symbolTable);
    lineCounter = 0;
    
    ASTNode *stmt = root->left;
    while(stmt){
        analyze_stmt(stmt);
        stmt = stmt->next;
    }
    
    print_symbol_table(&symbolTable);
}

// ============================================================================
// EXECUTOR 
// ============================================================================

static RuntimeVar* find_runtime_var(const char *name){
    for(int i = 0; i < runtimeCount; i++){
        if(strcmp(runtimeTable[i].name, name) == 0){
            return &runtimeTable[i];
        }
    }
    return NULL;
}

static void set_runtime_var(const char *name, const char *type, double value){
    RuntimeVar *var = find_runtime_var(name);
    
    if(var){
        if(strcmp(type, "int") == 0){
            var->value.intValue = (int)value;
        } else {
            var->value.floatValue = value;
        }
        var->isDefined = 1;
    } else {
        if(runtimeCount >= MAX_RUNTIME_VARS){
            fprintf(stderr, "Runtime error: too many variables\n");
            exit(1);
        }
        
        strncpy(runtimeTable[runtimeCount].name, name, MAX_LEN - 1);
        runtimeTable[runtimeCount].name[MAX_LEN - 1] = '\0';
        
        strncpy(runtimeTable[runtimeCount].type, type, MAX_LEN - 1);
        runtimeTable[runtimeCount].type[MAX_LEN - 1] = '\0';
        
        if(strcmp(type, "int") == 0){
            runtimeTable[runtimeCount].value.intValue = (int)value;
        } else {
            runtimeTable[runtimeCount].value.floatValue = value;
        }
        
        runtimeTable[runtimeCount].isDefined = 1;
        runtimeCount++;
    }
}

static double get_runtime_var_value(const char *name){
    RuntimeVar *var = find_runtime_var(name);
    
    if(!var || !var->isDefined){
        fprintf(stderr, "Runtime error: variable '%s' not defined\n", name);
        exit(1);
    }
    
    if(strcmp(var->type, "int") == 0){
        return (double)var->value.intValue;
    } else {
        return var->value.floatValue;
    }
}

static const char* get_runtime_var_type(const char *name){
    RuntimeVar *var = find_runtime_var(name);
    
    if(!var){
        fprintf(stderr, "Runtime error: variable '%s' not found\n", name);
        exit(1);
    }
    
    return var->type;
}

static double eval_expr(ASTNode *node, char *resultType);

static double eval_term(ASTNode *node, char *resultType){
    if(!node) {
        fprintf(stderr, "Runtime error: null term\n");
        exit(1);
    }
    
    if(node->tokenType == IDENTIFIER){
        const char *varType = get_runtime_var_type(node->value);
        strcpy(resultType, varType);
        return get_runtime_var_value(node->value);
    }
    else if(node->tokenType == NUMBER){
        if(strchr(node->value, '.') != NULL){
            strcpy(resultType, "float");
            return atof(node->value);
        } else {
            strcpy(resultType, "int");
            return (double)atoi(node->value);
        }
    }
    
    fprintf(stderr, "Runtime error: invalid term\n");
    exit(1);
}

static double eval_expr(ASTNode *node, char *resultType){
    if(!node) {
        fprintf(stderr, "Runtime error: null expression\n");
        exit(1);
    }
    
    if(node->nodeType == NODE_TERM){
        return eval_term(node, resultType);
    }
    else if(node->nodeType == NODE_EXPR){
        char leftType[MAX_LEN], rightType[MAX_LEN];
        double leftVal = eval_expr(node->left, leftType);
        double rightVal = eval_expr(node->right, rightType);
        
        if(strcmp(leftType, "float") == 0 || strcmp(rightType, "float") == 0){
            strcpy(resultType, "float");
        } else {
            strcpy(resultType, "int");
        }
        
        const char *op = node->value;
        
        if(strcmp(op, "+") == 0){
            return leftVal + rightVal;
        }
        else {
            fprintf(stderr, "Runtime error: unknown operator '%s'\n", op);
            exit(1);
        }
    }
    
    fprintf(stderr, "Runtime error: invalid expression node\n");
    exit(1);
}

static void execute_stmt(ASTNode *stmt){
    if(!stmt) return;
    
    switch(stmt->nodeType){
        case NODE_DECL: {
            const char *varName = stmt->left->value;
            const char *varType = stmt->value;
            
            if(stmt->right){
                char exprType[MAX_LEN];
                double value = eval_expr(stmt->right, exprType);
                set_runtime_var(varName, varType, value);
            } else {
                set_runtime_var(varName, varType, 0.0);
                RuntimeVar *var = find_runtime_var(varName);
                var->isDefined = 0;
            }
            break;
        }
        
        case NODE_ASSIGN: {
            const char *varName = stmt->value;
            
            RuntimeVar *var = find_runtime_var(varName);
            if(!var){
                fprintf(stderr, "Runtime error: variable '%s' not declared\n", varName);
                exit(1);
            }
            
            char exprType[MAX_LEN];
            double value = eval_expr(stmt->left, exprType);
            set_runtime_var(varName, var->type, value);
            break;
        }
        
        case NODE_BUILT_IN_CALL: {
            if(strcmp(stmt->value, "print") == 0){
                char exprType[MAX_LEN];
                double value = eval_expr(stmt->left, exprType);
                
                if(strcmp(exprType, "int") == 0){
                    printf("%d\n", (int)value);
                } else {
                    printf("%g\n", value);
                }
            } else {
                fprintf(stderr, "Runtime error: unknown built-in function '%s'\n", stmt->value);
                exit(1);
            }
            break;
        }
        
        default:
            break;
    }
}

void execute_ast(ASTNode* root){
    if(!root) return;
    
    runtimeCount = 0;
    
    ASTNode *stmt = root->left;
    while(stmt){
        execute_stmt(stmt);
        stmt = stmt->next;
    }
    
    printf("\n");
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    // Lexical Analysis
    tockenise(argv[1]);
    print_tokens();

    // Syntax Analysis
    currentToken = 0;
    ASTNode *ast = parse_program();
    print_ast(ast);

    // Semantic Analysis
    check_semantics(ast);
    
    // Execution
    execute_ast(ast);
    
    // Cleanup
    free_ast(ast);

    return 0;
}
