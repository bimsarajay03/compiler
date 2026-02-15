#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

#define MAX_TOKENS 1000
#define MAX_LEN 100

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

// External declarations
extern Token tokenArray[MAX_TOKENS];
extern int tokenCount;

// Function declarations
void tockenise(char *fileName);
void print_tokens(void);
const char *token_type_name(TokenType type);

#endif
