%{
# include "test.tab.h"
void yyerror(char*);
%}

/* you should implement the following between the first and the second %% */
/* 1. more operators and actions */
/* 2. how to match float numbers? */
%%
"+"	{ return ADD; }
"-"	{ return SUB; }
"*" { return MUL; }
"/" { return DIV; }
"**" { return EXP; }
"(" { return LPAR; }
")" { return RPAR; }
[0-9]+	{ yylval.val = atoi(yytext); return NUMBER; }
[0-9]+(/.[0-9]+)?  

[ \t]   { /* ignore white space */ }
.	{ yyerror("Unknown character!\n"); }
\n      { return EOL; }
%%