#include "execute.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RUNTIME_VARS 100

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

// Runtime memory (symbol table)
static RuntimeVar runtimeTable[MAX_RUNTIME_VARS];
static int runtimeCount = 0;

// Helper: Find variable in runtime table
static RuntimeVar* find_runtime_var(const char *name){
    for(int i = 0; i < runtimeCount; i++){
        if(strcmp(runtimeTable[i].name, name) == 0){
            return &runtimeTable[i];
        }
    }
    return NULL;
}

// Helper: Add or update variable in runtime table
static void set_runtime_var(const char *name, const char *type, double value){
    RuntimeVar *var = find_runtime_var(name);
    
    if(var){
        // Update existing variable
        if(strcmp(type, "int") == 0){
            var->value.intValue = (int)value;
        } else {
            var->value.floatValue = value;
        }
        var->isDefined = 1;
    } else {
        // Add new variable
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

// Helper: Get variable value as double
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

// Helper: Get variable type
static const char* get_runtime_var_type(const char *name){
    RuntimeVar *var = find_runtime_var(name);
    
    if(!var){
        fprintf(stderr, "Runtime error: variable '%s' not found\n", name);
        exit(1);
    }
    
    return var->type;
}

// Forward declaration
static double eval_expr(ASTNode *node, char *resultType);

// Evaluate a term (literal or variable)
static double eval_term(ASTNode *node, char *resultType){
    if(!node) {
        fprintf(stderr, "Runtime error: null term\n");
        exit(1);
    }
    
    // Variable lookup
    if(node->tokenType == IDENTIFIER){
        const char *varType = get_runtime_var_type(node->value);
        strcpy(resultType, varType);
        return get_runtime_var_value(node->value);
    }
    // Literal number
    else if(node->tokenType == NUMBER){
        // Check if it's a float literal
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

// Evaluate an expression recursively
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
        
        // Determine result type: float if either operand is float
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

// Execute a single statement
static void execute_stmt(ASTNode *stmt){
    if(!stmt) return;
    
    switch(stmt->nodeType){
        case NODE_DECL: {
            const char *varName = stmt->left->value;
            const char *varType = stmt->value;
            
            // Check if there's an initialization expression
            if(stmt->right){
                char exprType[MAX_LEN];
                double value = eval_expr(stmt->right, exprType);
                set_runtime_var(varName, varType, value);
            } else {
                // Declare without initialization (set to 0)
                set_runtime_var(varName, varType, 0.0);
                RuntimeVar *var = find_runtime_var(varName);
                var->isDefined = 0; // Mark as declared but not initialized
            }
            break;
        }
        
        case NODE_ASSIGN: {
            const char *varName = stmt->value;
            
            // Get variable type
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
                
                // Print based on type
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

// Main execution function
void execute_ast(ASTNode* root){
    if(!root) return;
    
    // Initialize runtime table
    runtimeCount = 0;
    
    // Traverse statement list
    ASTNode *stmt = root->left;
    while(stmt){
        execute_stmt(stmt);
        stmt = stmt->next;
    }
    
    printf("\n");
}
