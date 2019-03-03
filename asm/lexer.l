%{
 
#include "parser.h"
#include <stdio.h>

unsigned long column = 0;

void unrecognized_token(char * token) {
  FILE * file = fopen(filename, "r");
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  int count = 0;
  int length = strlen(token);

  while ((read = getline(&line, &len, file)) != -1) {
    count++;

    if (count <= yylineno) {
      fprintf(stderr, ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET, line);
    }
  }

  fprintf(stderr, ANSI_COLOR_RED "%*s^\n" ANSI_COLOR_RESET, (int)column - 1, "");
  fprintf(stderr, ANSI_COLOR_RED "Lexing error - unrecognized token: %s\n" ANSI_COLOR_RESET, token);

  if ((strcmp(token, "--") == 0) || (strcmp(token, "-") == 0)) {
    fprintf(stderr, ANSI_COLOR_CYAN "hint: " ANSI_COLOR_GREEN "the data separator is three dashes: ---\n" ANSI_COLOR_RESET);
  }
  exit(1);
}

void increment_column() {
  column = column + strlen(yytext);
}
 
%}

%option outfile="./asm/lexer.c" header-file="./asm/lexer.h"
%option warn noyywrap

%%

#.*[\n|\r] {
  yylineno++;
  column = 0;
}

\"[^\"]*\" {
  int length = strlen(yytext);
  increment_column();
  int new_length = length - 1;
  char * string = malloc(new_length);

  for (int i=0; i < (new_length); i++) {
    string[i] = yytext[i+1];
  }

  string[new_length - 1] = '\0';

  yylval.string = strdup(string);
  return STRING;
}

[a-zA-Z][._0-9a-zA-Z]* {
  yylval.string = strdup(yytext);
  increment_column();
  return SYMBOL;
}

[0-9]+\.[0-9]+ {
  yylval._double = atof(yytext);
  increment_column();
  return FLOAT;
}

[0-9]+ {
  yylval.integer = atoi(yytext);
  increment_column();
  return INTEGER;
}

--- { increment_column(); return SEPERATOR; }
-- { increment_column(); unrecognized_token("--"); }
- { increment_column(); unrecognized_token("-"); }
: { increment_column(); return ':'; }
= { increment_column(); return '='; }
[\n\r] {
  yylineno++;
  column = 0;
  return NEWLINE;
}
[ \t] { increment_column(); }// ignore whitespace

. { unrecognized_token(yytext); }
