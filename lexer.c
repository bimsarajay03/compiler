#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>

FILE *fptr;

Token tokenArray[MAX_TOKENS];
int tokenCount = 0;

const char *KEYWORDS[] = {
    "int",
    "float"
};

const char *BUILT_FUNCTIONS[] = {
    "print"
};

const char *OPERATORS[] = {
    "+",
    "="
};

const char *SYMBOLS[] = {
    "(",
    ")",
    ";"
};

const size_t KEYWORD_COUNT = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);
const size_t BUILT_FUNCTION_COUNT = sizeof(BUILT_FUNCTIONS) / sizeof(BUILT_FUNCTIONS[0]);
const size_t OPERATOR_COUNT = sizeof(OPERATORS) / sizeof(OPERATORS[0]);
const size_t SYMBOL_COUNT = sizeof(SYMBOLS) / sizeof(SYMBOLS[0]);

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
    fptr = fopen(fileName, "r");
    if(fptr == NULL){
        perror("fopen");
        return;
    }

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
