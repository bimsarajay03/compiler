#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "lexer.h"
#include "parser.h"

// Symbol table row
typedef struct {
    char name[MAX_LEN];           // Variable name
    char type[MAX_LEN];           // Data type 
    int isInitialized;            // 1 if initialized, 0 otherwise
    int scope;                    // Scope level (0 = global, 1+ = local) | not really used for this test
    int lineNumber;               // Line where declared (for error messages)
} SymbolEntry;

// Symbol table structure
typedef struct {
    SymbolEntry symbols[MAX_TOKENS];   
    int count;                    
} SymbolTable;

//function declarations
void check_semantics(ASTNode* root);
void init_symbol_table(SymbolTable *table);
int add_symbol(SymbolTable *table, const char *name, const char *type, int isInitialized, int scope, int line);
SymbolEntry* lookup_symbol(SymbolTable *table, const char *name);
void print_symbol_table(SymbolTable *table);

#endif
