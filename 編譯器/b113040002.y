%{
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stddef.h>
extern unsigned lineCount;
extern unsigned charCount;
extern size_t yyleng;
int is_loop = 0;
int in_scope = 0;
int stmt_is_block = 0;
int yylex();
void yyerror() {
    printf("Syntax error at line %d\n", lineCount);
};
typedef struct SymNode {
    char *name;
    struct SymNode *next;
} SymNode;

typedef struct Scope {
    SymNode *ids;    // 簡單 hash table
    struct Scope *prev;
} Scope;

Scope *current_scope;

void enter_scope() {
        Scope *s = malloc(sizeof(Scope));
        s->ids  = NULL;
        s->prev = current_scope;
        current_scope = s;
}

// 退出 scope，並 free 裡面所有節點
void exit_scope() {
    SymNode *n = current_scope->ids;
    SymNode *t = n;
    while (n) {
        t = n;
        n = n->next;
        free(t->name);
        free(t);
    }
    Scope *s = current_scope;
    current_scope = s->prev;
    free(s);
}

int add_symbol(const char *name) {
    SymNode * n = current_scope->ids;
    for (; n; n = n->next) {
        if (strcmp(n->name, name)==0) return -1;
    }
    n = malloc(sizeof(SymNode));
    n->name = strdup(name);
    n->next = current_scope->ids;
    current_scope->ids =  n;
    return 0;
}

int find_symbol(const char *name) {
    Scope *s = current_scope;
    while(s){
        for (SymNode *n = s->ids; n; n=n->next) {
            if (strcmp(n->name, name) == 0) {
                return 1;
            }
        }
        s = s->prev;
    }
    return -1;
}

%}
%locations
%union{
    float floatval;
    int intval;
    char *strval;
    struct {
     char *name;
     int col;
    }idinfo;
}
%token<floatval> NUMBER 
%token<strval> STRING
%token<idinfo> ID
%type<idinfo> var
//Operator
%token ADD INC MINUS DEC MUL DIV MOD
%token AND OR NOT EQ NE LT LE GT GE ASSIGN
//Symbol
%token COMMA COLON SEMICOLON LP RP LBRACKET RBRACKET LBRACE RBRACE
//Preserved words
%token TYPE BREAK_CONTINUE CASE_DEFAULT IF ELSE SWITCH FOR DO WHILE TRY CATCH FINALLY CLASS 
%token EXTENDS IMPLEMENTS ACCESS_MODIFIER STATIC FINAL CONST RETURN NEW THIS BOOLEAN_LITERAL MAIN PRINT


%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%nonassoc for_var_decl
%nonassoc for_method_decl
%right ASSIGN
%left ADD MINUS
%left MUL DIV MOD
%nonassoc EQ NE LT LE GT GE
%right INC DEC
%nonassoc LOWER_THAN_LBRAKET
%nonassoc LBRAKET

%%

program:
   {enter_scope();}
     class_list 
   {exit_scope();}
;

class_list: /*there must be at least a class*/
    class_list class_decl
  | class_decl
;

class_decl:
    CLASS ID 
    {
    	//printf("ds\n");
    	if ( add_symbol($2.name) < 0 ) {
            printf("ERROR*****Line %d, char: %d: '%s' is a duplicate identifier.\n", lineCount, charCount, $2.name);
            free($2.name);
        }
    }
    LBRACE
   {enter_scope();}
    class_body_opt
   {exit_scope();} 
    RBRACE
;
/*If there is any line in class*/
class_body_opt:
    /* empty */
  | nonempty_class_body
;
/*single or multiple lines*/
nonempty_class_body:
    nonempty_class_body member_decl 
  | member_decl
;
/*class menber*/
member_decl:
    var_decl 
  | STATIC var_decl 
  | FINAL var_decl 
  | CONST var_decl 
  | ACCESS_MODIFIER var_decl 
  | method_decl
  | FINAL method_decl
  | CONST method_decl
  | ACCESS_MODIFIER method_decl
  | STATIC method_decl
  | class_decl       /* 支援巢狀類別 */
;

var_decl: 
    ID var_list SEMICOLON 
    {
        if ( find_symbol($1.name) < 0 ) {
            printf("ERROR*****Line %d, char: %d: '%s' is undeclared.\n", lineCount, $1.col, $1.name);
            free($1.name);
        }
    }
  | ID var_list {
       if ( find_symbol($1.name) < 0 ) {
            printf("ERROR*****Line %d, char: %d: '%s' is undeclared.\n", lineCount, $1.col, $1.name);
            free($1.name);
       }
       unsigned D = charCount - yyleng;
       printf("ERROR*****Line %d, char: %d: Expect ';' at end of declaration.\n", lineCount,charCount);
       yyerrok;
    }error /*No semicolon after*/
    
  | ID LBRACKET RBRACKET var_list SEMICOLON /*class object*/
  | TYPE var_list SEMICOLON 
  | TYPE LBRACKET RBRACKET var_list SEMICOLON /*declare array*/
  | TYPE var_list error var SEMICOLON /*no comma between variables*/
    {
       printf("ERROR*****Line %d, char: %d: Expect ',' before '%s'.\n", lineCount, $4.col, $4.name);
       free($4.name);
       yyerrok;
    } 
;
/*single or multiple variable*/
var_list:
    var_list COMMA var
  | var
;

var:
    ID
    {
    	if ( add_symbol($1.name) < 0 ) {
            printf("ERROR*****Line %d, char: %d: '%s' is a duplicate identifier.\n", lineCount, $1.col, $1.name);
            free($1.name);
        }
      $$ = $1;
    }
  | ID ASSIGN expression
    {
  	  if ( add_symbol($1.name) < 0 ) {
            printf("ERROR*****Line %d, char: %d: '%s' is a duplicate identifier.\n", lineCount, $1.col, $1.name);
            free($1.name);
        }
      $$ = $1;
    }
  | ID ASSIGN error /*no expr after "="*/
    {
  	  if ( add_symbol($1.name) < 0 ) {
            printf("ERROR*****Line %d, char: %d: '%s' is a duplicate identifier.\n", lineCount, $1.col, $1.name);
            free($1.name);
        }
      $$ = $1;
       printf("ERROR*****Line %d, char: %d: Illigel variable assigment.\n", lineCount, charCount);
       yyerrok;
    }
;

method_decl:
     TYPE ID LP param_list_opt RP block
    {
    	if ( add_symbol($2.name) < 0 ) {
            printf("ERROR*****Line %d, char: %d: '%s' is a duplicate identifier.\n", lineCount, $2.col, $2.name);
        }
        free($2.name);
    } 
  | ID LP param_list_opt RP block 
    {
    	if ( add_symbol($1.name) < 0 ) {
            printf("ERROR*****Line %d, char: %d: '%s' is a duplicate identifier.\n", lineCount, $1.col, $1.name);
        }
        free($1.name);
    }
  | MAIN LP param_list_opt RP block /*main with no return type*/
  | TYPE MAIN LP param_list_opt RP block /*Main with return type*/
;
/*multiple parameter*/
param_list_opt:
    /* empty */
  | param_list
;

param_list:
    param_list COMMA TYPE ID
    {
    	add_symbol($4.name);
        free($4.name);
    } 
  | TYPE ID
    {
    	add_symbol($2.name);
        free($2.name);
    } 
;

block:
    LBRACE
   {enter_scope();}
    block_items_opt
   {exit_scope(); }
    RBRACE
;
/*Whether there is any line in the scope*/
block_items_opt:
    /* empty */
  | nonempty_block_items
;
/*multi lines*/
nonempty_block_items:
    nonempty_block_items block_item
  | block_item
;
/*those can be in a block*/
block_item:
    var_decl 
  | statement
  | class_decl
;

statement: 
    IF LP expression RP statement       %prec LOWER_THAN_ELSE /*If but no else, statement contain block option*/
  | IF LP expression RP statement ELSE statement /*If but with else, statement contain block option. Since If is also in statement so else if will be include*/
  | ELSE statement /*else but no if*/
    {
      printf("ERROR*****Line %d: Else without if statement.\n", lineCount);
    }
  | WHILE LP expression RP /*statement contains block*/
    {is_loop = 1;}
    statement
    {is_loop = 0;}
  | WHILE LP error RP { /*it is un efficient to print all illegle boolean expr, so just set a error token*/
      {
        printf("ERROR*****Line %d, char: %d: Invalid boolean expression.\n", lineCount, charCount);
        yyerrok;
      }
    }statement
    
  | FOR LP for_init_opt SEMICOLON expression SEMICOLON for_update_opt RP /*Normol for loop*/
    {is_loop = 1;}
    statement
    {is_loop = 0;}
  | FOR LP for_init_opt SEMICOLON SEMICOLON for_update_opt RP /*for loop without end condition*/
    {printf("ERROR*****Line %d: For loop will not stop.\n", lineCount);
     is_loop = 1;}
    statement
    {is_loop = 0;}
  | RETURN expression SEMICOLON /*return*/
  | PRINT LP expression RP SEMICOLON /*print*/
  | expr_stmt /*For statement like a++;*/
  | block /*block*/
  | BREAK_CONTINUE SEMICOLON /*break continue*/
    {
        if(is_loop != 1) printf("Break / Continue is not in loop scope.\n");
    }
;

expr_stmt:
    expression SEMICOLON
  | expression error /*expression statment but no semicolon*/
    {
       printf("ERROR*****Line %d, char: %d: Expect ';' at end of line.\n", lineCount, charCount);
       yyerrok;
    }
;

for_init_opt:
    /* empty */
  | TYPE ID ASSIGN expression /*like int i = 0*/
    {
    	add_symbol($2.name);
        free($2.name);
    } 
  | expression 
;

for_update_opt:
    /* empty */
  | expression
;

expression:
    expression ADD  expression
  | expression MINUS expression
  | expression MUL   expression
  | expression DIV   expression
  | expression MOD   expression
  | expression EQ    expression
  | expression NE    expression
  | expression LT    expression
  | expression LE    expression
  | expression GT    expression
  | expression GE    expression
  | expression ASSIGN expression
  | expression ASSIGN error /*if no expr after "="*/
    {
       printf("ERROR*****Line %d, char: %d: Illigel variable assigment.\n", lineCount, charCount);
       yyerrok;
    }
  | expression INC
  | INC expression
  | expression DEC
  | DEC expression
  | primary
;

primary:
    ID %prec LOWER_THAN_LBRAKET /*it is easy for single ID to get in conflict so set a precedence*/
    {
        if ( find_symbol($1.name) < 0 ) {
            printf("ERROR*****Line %d, char: %d: '%s' is undeclared.\n", lineCount, $1.col, $1.name);
            free($1.name);
        }
    }
  | NUMBER
  | STRING
  | ID     
    {
        if ( find_symbol($1.name) < 0 ) {
            printf("ERROR*****Line %d, char: %d: '%s' is undeclared.\n", lineCount, $1.col, $1.name);
            free($1.name);
        }
    }LP arg_list_opt RP /*invoke a function*/
  | ID LBRACKET {
        if ( find_symbol($1.name) < 0 ) {
            printf("ERROR*****Line %d, char: %d: '%s' is undeclared.\n", lineCount, $1.col, $1.name);
            free($1.name);
        }
    } expression RBRACKET /*express a element in array*/
    
  | NEW ID /*new a new object*/
    {
        if ( find_symbol($2.name) < 0 ) {
            printf("ERROR*****Line %d, char: %d: '%s' is undeclared.\n", lineCount, $2.col, $2.name);
            free($2.name);
        }
    }
  | NEW ID 
    {
        if ( find_symbol($2.name) < 0 ) {
            printf("ERROR*****Line %d, char: %d: '%s' is undeclared.\n", lineCount, $2.col, $2.name);
            free($2.name);
        }
    }LP arg_list_opt RP
  | NEW TYPE LBRACKET expression RBRACKET 
  | LP expression RP 
;

arg_list_opt:
    /* empty */
  | arg_list
;

arg_list:
    arg_list COMMA expression
  | expression
;
%%
int main(){
    yyparse();
    return 0;
}

/*
	變數重複宣告
	break continue出現在loop外
    For 有沒有終止條件
    等號後什麼都沒有
*/