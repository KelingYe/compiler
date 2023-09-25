%{
#  include <stdio.h>
#  include <math.h>
#  include <stdlib.h>
#  include <stdbool.h>
#  include <string.h>
void yyerror(char *s);
int yylex();
%}

// you should define the return value in lexer here
%union {
	double val
}

// you should declare tokens and their value type here
%token <val> NUMBER
%token ADD SUB
%token EOL
%token MUL  DIV
%token EXP
%token LPAR RPAR



%type <val> term exp

%left ADD SUB
%left MUL DIV
%right EXP



// you should declare rules for parser between the first and the second %%
%%
calclist: 
	| calclist exp EOL { printf("= %f\n> ", $2);}
	| calclist EOL { printf("> "); } 
	;

exp: term
	| exp ADD exp { $$ = $1 + $3; }
	| exp SUB exp { $$ = $1 - $3; }
	| exp MUL exp { $$ = $1 * $3; }
	| exp DIV exp { $$ = $1 / $3; }
	| exp EXP exp { $$ =pow($1,$3); }
	| LPAR exp RPAR { $$ = $2; }
	;

term: NUMBER
	;


%%

void yyerror(char *s)
{
	fprintf(stderr, "Error: %s\n", s);
}

int main()
{
	printf("> "); 
	yyparse();
}