#include "semantics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static SymbolTable symbolTable;
static int lineCounter = 0;

// Initialize symbol table
void init_symbol_table(SymbolTable *table){
    table->count = 0;
}

// Add symbol to table
int add_symbol(SymbolTable *table, const char *name, const char *type, int isInitialized, int scope, int line){
    // Check for redeclaration
    for(int i = 0; i < table->count; i++){
        if(strcmp(table->symbols[i].name, name) == 0 && table->symbols[i].scope == scope){
            fprintf(stderr, "Semantic error (line %d): variable '%s' already declared\n", line, name);
            return 0;
        }
    }
    
    // Add new symbol
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

// Lookup symbol in table
SymbolEntry* lookup_symbol(SymbolTable *table, const char *name){
    for(int i = 0; i < table->count; i++){
        if(strcmp(table->symbols[i].name, name) == 0){
            return &table->symbols[i];
        }
    }
    return NULL;
}

// Print symbol table
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

// Forward declarations
static const char* analyze_expr(ASTNode *node);

// Analyze a term (check if identifier is declared and return its type)
static const char* analyze_term(ASTNode *node){
    if(!node) return NULL;
    
    // Use metadata from lexer: only identifiers need declaration checking
    if(node->tokenType == IDENTIFIER){
        SymbolEntry *sym = lookup_symbol(&symbolTable, node->value);
        if(!sym){
            fprintf(stderr, "Semantic error: undeclared variable '%s'\n", node->value);
            exit(1);
        }
        return sym->type;
    } else if(node->tokenType == NUMBER){
        // Check if it's a float or int literal
        if(strchr(node->value, '.') != NULL){
            return "float";
        } else {
            return "int";
        }
    }
    
    return NULL;
}

// Handle arithmetic expressions: + /
static const char* analyze_arithmetic_expr(const char *leftType, const char *rightType, const char *op){
    // int op int = int
    if(strcmp(leftType, "int") == 0 && strcmp(rightType, "int") == 0){
        return "int";
    }
    // Any float involvement = float
    else if(strcmp(leftType, "float") == 0 || strcmp(rightType, "float") == 0){
        return "float";
    }
    
    fprintf(stderr, "Semantic error: invalid types for arithmetic operator '%s'\n", op);
    exit(1);
}





// Analyze an expression and return its type
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
        
        // Dispatch to appropriate handler based on operator
        if(strcmp(op, "+") == 0 ){
            return analyze_arithmetic_expr(leftType, rightType, op);
        }
        else {
            fprintf(stderr, "Semantic error: unknown operator '%s'\n", op);
            exit(1);
        }
    }
    
    return NULL;
}

// Analyze a single statement
static void analyze_stmt(ASTNode *stmt){
    if(!stmt) return;
    
    lineCounter++;
    
    switch(stmt->nodeType){
        case NODE_DECL: {
            // Get variable name from left child (which is a TERM node)
            const char *varName = stmt->left->value;
            const char *varType = stmt->value;  // "int" or "float"
            
            // Check if initialized (has right child with expression)
            int isInit = (stmt->right != NULL);
            
            // If initialized, check the expression type
            if(isInit){
                const char *exprType = analyze_expr(stmt->right);
                
                // Type checking for initialization
                if(strcmp(varType, "int") == 0 && strcmp(exprType, "float") == 0){
                    fprintf(stderr, "Semantic error (line %d): cannot assign float to int variable '%s'\n", 
                            lineCounter, varName);
                    exit(1);
                }
                // float = int is OK (implicit widening)
                // int = int is OK
                // float = float is OK
            }
            
            // Add to symbol table
            if(!add_symbol(&symbolTable, varName, varType, isInit, 0, lineCounter)){
                exit(1);
            }
            break;
        }
        
        case NODE_ASSIGN: {
            // Check if variable is declared
            const char *varName = stmt->value;
            SymbolEntry *sym = lookup_symbol(&symbolTable, varName);
            
            if(!sym){
                fprintf(stderr, "Semantic error (line %d): undeclared variable '%s'\n", lineCounter, varName);
                exit(1);
            }
            
            // Check the expression type
            const char *exprType = analyze_expr(stmt->left);
            
            // Type checking for assignment
            if(strcmp(sym->type, "int") == 0 && strcmp(exprType, "float") == 0){
                fprintf(stderr, "Semantic error (line %d): cannot assign float to int variable '%s'\n", 
                        lineCounter, varName);
                exit(1);
            }
            // float = int is OK (implicit widening)
            // int = int is OK
            // float = float is OK
            
            // Mark as initialized
            sym->isInitialized = 1;
            break;
        }
        
        case NODE_BUILT_IN_CALL: {
            // Check the expression argument
            analyze_expr(stmt->left);
            break;
        }
        
        default:
            break;
    }
}

// Main semantic analysis function
void check_semantics(ASTNode* root){
    if(!root) return;
    
    init_symbol_table(&symbolTable);
    lineCounter = 0;
    
    // Traverse statement list
    ASTNode *stmt = root->left;
    while(stmt){
        analyze_stmt(stmt);
        stmt = stmt->next;
    }
    
    print_symbol_table(&symbolTable);
}
