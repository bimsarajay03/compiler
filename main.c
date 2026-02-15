#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "semantics.h"
#include "execute.h"

int main(int argc, char *argv[]){

    if(argc != 2){
        fprintf(stderr, "invalid input\n");
        return 1;
    }

    //Lexical Analysis (Lexer)
    tockenise(argv[1]);
    print_tokens();

    //Syntax Analysis (Parser)
    currentToken = 0;
    ASTNode *ast = parse_program();
    print_ast(ast);

    //Semantic Analysis
    check_semantics(ast);
    
    //Execution
    execute_ast(ast);
    
    //Clean up
    free_ast(ast);

    return 0;

}
