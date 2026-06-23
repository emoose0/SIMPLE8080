#ifndef SYMBOL_H
#define SYMBOL_H

#include "common.h"
#include "lexer.h"

#define SYMMAX 1000

typedef enum definition{
	DW, DB, DS,
	EQU, SET,
	NONE
}definition;

typedef struct symbol{
	const char* name;
	uint16_t data;
	bool isConst;
	bool isDefined;
	definition type;
}symbol;

int searchSym(const char* string);
int addSym(const char* string, bool isConst, token T, definition type);
int modifySym(int i, token T);
int returnSymData(const char* name);
symbol returnSym(int i);
int setSym(const char* name, bool isConst, token T, definition type);
int defineSym(int i);

#endif
