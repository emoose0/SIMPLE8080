#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"

typedef struct parser{
	token* tokens;
	token currentToken;
	int ti;
	int tcount;

	instruction* ins;
	int i;
	int icapacity;
	int icount;
}parser;

int initParser(token* tokens, int tcount);
parserResult execute(char* contents);
bool isDirective(token T);
int freeParser();

#endif
