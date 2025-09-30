%{
#include<stdio.h>
#include <stdlib.h>
#include "y.tab.h"
#include <string.h>
#define MAXSIZE 10000
char this_line[MAXSIZE] = "";
unsigned charCount = 1, idCount = 0, lineCount = 1;
int thisline_pos = 0;
void reset_thisline() { 
	memset(this_line, 0, MAXSIZE);
	thisline_pos = 0; }
%}

integer_lit [+-]?[0-9]+
float_lit [+-]?([0-9]*\.[0-9]+|[0-9]+\.)([eE][+-]?[0-9]+)?[fFdD]?
String \"([^\"\\\n]|\\.)*\"
String3 \'([^\'\\\n]|\\.)*\'
id [a-zA-Z\$_][a-zA-Z\$_0-9]*
id2 [a-zA-Z\$_][a-zA-Z\$_0-9]*[\-@#%\^!?]+[a-zA-Z\$_0-9]*
operator \+\+|--|\+|-|\*|\/|%|==|!=|<=|>=|<|>|=|&&|\|\||!
space [ \t]+
eol \r?\n
comment_1 \/\/[^\n]*
comment_2 \/\*([^*]|\*+[^*/]|\n)*\*+\/
comment_3 \/\*([^*]|\*+[^*/]|\n)*
symbol [,:;{}\[\]()]
%%
{operator}     {
				strncat(this_line, yytext, yyleng);
                charCount += yyleng;
                if      (!strcmp(yytext,"+"))   return ADD;
                else if (!strcmp(yytext,"++"))  return INC;
                else if (!strcmp(yytext,"-"))   return MINUS;
                else if (!strcmp(yytext,"--"))  return DEC;
                else if (!strcmp(yytext,"*"))   return MUL;
                else if (!strcmp(yytext,"/"))   return DIV;
                else if (!strcmp(yytext,"%"))   return MOD;
                else if (!strcmp(yytext,"&&"))  return AND;
                else if (!strcmp(yytext,"||"))  return OR;
                else if (!strcmp(yytext,"!"))   return NOT;
                else if (!strcmp(yytext,"=="))  return EQ;
                else if (!strcmp(yytext,"!="))  return NE;
                else if (!strcmp(yytext,"<"))   return LT;
                else if (!strcmp(yytext,"<="))  return LE;
                else if (!strcmp(yytext,">"))   return GT;
                else if (!strcmp(yytext,">="))  return GE;
                else                    /*=*/   return ASSIGN;
                }

{space} {
	strncat(this_line, yytext, yyleng);
        charCount += yyleng;
}
{eol}   {
	strncat(this_line, yytext, yyleng);
        printf("line %d: %s", lineCount, this_line);	
        lineCount++;
	charCount = 1;
	reset_thisline();
	//return NEWLINE;
}
{comment_1} {

	printf("line %d: %s", lineCount, yytext);
	charCount += yyleng;
}
{comment_2} {
    for(int i=0; i<yyleng; ++i){
      char c = yytext[i];
      this_line[thisline_pos++] = c;
      if(c=='\n'){
        this_line[thisline_pos] = '\0';
        printf("line %d: %s", lineCount++, this_line);
        reset_thisline();
        charCount = 1;
      }
    }
    charCount += yyleng;
}
{comment_3} {
	int newlineCount = 0;
	for(int i=0; i<yyleng; ++i){
      char c = yytext[i];
      this_line[thisline_pos++] = c;
      if(c=='\n'){
        this_line[thisline_pos] = '\0';
        printf("line %d: %s", lineCount++, this_line);
        reset_thisline();
        charCount = 1;
      }
    }
    charCount += yyleng;
}
boolean|byte|char|double|float|int|long|short|string|void {strncat(this_line, yytext, yyleng); charCount += yyleng; return TYPE; }
break|continue           {strncat(this_line, yytext, yyleng); charCount += yyleng;  return BREAK_CONTINUE; }

case|default             {strncat(this_line, yytext, yyleng); charCount += yyleng;  return CASE_DEFAULT; }

if                       {strncat(this_line, yytext, yyleng); charCount += yyleng;  return IF; }
else                     {strncat(this_line, yytext, yyleng); charCount += yyleng;  return ELSE; }

switch                   {strncat(this_line, yytext, yyleng); charCount += yyleng;  return SWITCH; }

for                      {strncat(this_line, yytext, yyleng); charCount += yyleng;  return FOR; }
do                       {strncat(this_line, yytext, yyleng); charCount += yyleng;  return DO; }
while                    {strncat(this_line, yytext, yyleng); charCount += yyleng;  return WHILE; }

try                      {strncat(this_line, yytext, yyleng); charCount += yyleng;  return TRY; }
catch                    {strncat(this_line, yytext, yyleng); charCount += yyleng;  return CATCH; }
finally                  {strncat(this_line, yytext, yyleng); charCount += yyleng;  return FINALLY; }

class                    {strncat(this_line, yytext, yyleng); charCount += yyleng; return CLASS; }
extends                  {strncat(this_line, yytext, yyleng); charCount += yyleng;  return EXTENDS; }
implements               {strncat(this_line, yytext, yyleng); charCount += yyleng;  return IMPLEMENTS; }

private|protected|public {strncat(this_line, yytext, yyleng); charCount += yyleng;  return ACCESS_MODIFIER; }

static                   {strncat(this_line, yytext, yyleng); charCount += yyleng;  return STATIC; }
final                    {strncat(this_line, yytext, yyleng); charCount += yyleng;  return FINAL; }
const                    {strncat(this_line, yytext, yyleng); charCount += yyleng;  return CONST; }

return                   {strncat(this_line, yytext, yyleng); charCount += yyleng;  return RETURN; }

new                      {strncat(this_line, yytext, yyleng); charCount += yyleng;  return NEW; }
this                     {strncat(this_line, yytext, yyleng); charCount += yyleng;  return THIS; }

true|false               {strncat(this_line, yytext, yyleng); charCount += yyleng;  return BOOLEAN_LITERAL; }

main                     {strncat(this_line, yytext, yyleng); charCount += yyleng;  return MAIN; }

print                    {strncat(this_line, yytext, yyleng); charCount += yyleng;  return PRINT; }

 /* symbol */
","   {strncat(this_line, yytext, yyleng); charCount += yyleng;  return COMMA;      }
":"   {strncat(this_line, yytext, yyleng); charCount += yyleng;  return COLON;      }
";"   {strncat(this_line, yytext, yyleng); charCount += yyleng;  return SEMICOLON;  }
"("   {strncat(this_line, yytext, yyleng); charCount += yyleng;  return LP;     }
")"   {strncat(this_line, yytext, yyleng); charCount += yyleng;  return RP;     }
"["   {strncat(this_line, yytext, yyleng); charCount += yyleng;  return LBRACKET;   }
"]"   {strncat(this_line, yytext, yyleng); charCount += yyleng;  return RBRACKET;   }
"{"   {strncat(this_line, yytext, yyleng); charCount += yyleng;  return LBRACE;     }
"}"   {strncat(this_line, yytext, yyleng); charCount += yyleng;  return RBRACE;     }

{integer_lit} {
    strncat(this_line, yytext, yyleng);
    charCount += yyleng;
    yylval.floatval = atoi(yytext);            /* 對應到 %union 的成員 */
    return NUMBER;
}

{float_lit} {
	strncat(this_line, yytext, yyleng);
    charCount += yyleng;
    yylval.floatval = atof(yytext);
    return NUMBER;
}
{String} {

	strncat(this_line, yytext, yyleng);
	//printf("Line: %d, 1st char: %d, \"%s\" is a \"string\".\n", lineCount, charCount, yytext);
	charCount += yyleng;
    yylval.strval = strdup(yytext);
    return STRING;
}

{String3} {
	strncat(this_line, yytext, yyleng);
	yylval.strval = strdup(yytext);
	//printf("Line: %d, 1st char: %d, \"%s\" is a \"invalid string\".\n", lineCount, charCount, yytext);
	charCount += yyleng;
    return STRING;
}
{id} {
	strncat(this_line, yytext, yyleng);
    if(!(yytext[0] >= 65 && yytext[0] <= 90) &&
	   !(yytext[0] >= 97 && yytext[0] <= 122) &&
	   yytext[0] != 95 && yytext[0] != 36){
		printf("Line: %d, 1st char: %d, \"%s\" is an \"invalid identifier(1)\".\n", lineCount, charCount, yytext);
	        yylval.idinfo.col = charCount;
		charCount += yyleng;
		yylval.idinfo.name = strdup(yytext);
		
        return ID;
	}
    else {
	yylval.idinfo.col = charCount;
        charCount += yyleng;
        yylval.idinfo.name = strdup(yytext);
        return ID;
    }
}
{id2} {
	strncat(this_line, yytext, yyleng);
	//printf("Line: %d, 1st char: %d, \"%s\" is an \"invalid identifier(2)\".\n", lineCount, charCount, yytext);
	yylval.idinfo.col = charCount;
	charCount += yyleng;
	yylval.idinfo.name = strdup(yytext);
    return ID;
}
. {
	strncat(this_line, yytext, yyleng);
	//printf("Line: %d, 1st char: %d, \"%s\" is an \"Unknown token\".\n", lineCount, charCount, yytext);
	charCount += yyleng;
}
%%

int yywrap(){
    return 1;
}

/*
//特別處裡
//1. operator的前後token種類要求
//2. 減1 VS 負1
//3. prev_token
//4. 非法字元開頭的id
//5. 中間有違法字元的id
//6. comment 缺少右括號
//7. string 缺乏右括號
//8. 跨行comment的行數計算

//單引號之字串 >JAVA單引號為字元

Yacc bonus(未做)
    : package_decl
    | import_decl
    | class_decl (O)
    | interface_decl
    | enum_decl

	變數重複宣告
	變數未宣告 => while 無限迴圈
	break continue出現在loop外
	default case 出現在switch外
	ID[#]為identifier
*/
