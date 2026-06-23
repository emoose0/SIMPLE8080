#include "symbol.h"

symbol SYMBOLS[SYMMAX];
int symCount = 0;

int getData(token T){
	int i = 0;

	switch(T.type){
		case SYMBOL:
			i = returnSymData(T.attribute.name);
		break;
		case NUMBER:
			i = T.attribute.data;
		break;
		default: return -1;		
	}
	return i;
}

int searchSym(const char* name){
	for(int i = 0; i < SYMMAX; i++){
		if(SYMBOLS[i].name != NULL && strcmp(name, SYMBOLS[i].name) == 0) return i;
	}
	return -1;
}

int addSym(const char* name, bool isConst, token T, definition type){
	if(symCount >= SYMMAX-1){
		fprintf(stderr, "ERR: RAN OUT OF SPACE FOR SYMBOLS, CANNOT CREATE SYMBOL: %s\n\n", name);
		return -1;
	}

	int data = getData(T);
	if(data == -1){
		fprintf(stderr, "ERR: INVALID OPERAND FOR CREATING SYMBOL %s\n\n", name);
		return -1;
	}

	SYMBOLS[symCount].name = name;
	SYMBOLS[symCount].isConst = isConst;
	SYMBOLS[symCount].data = data;
	SYMBOLS[symCount].type = type;
	SYMBOLS[symCount].isDefined = false;
	symCount++;

	return 0;
}

int modifySym(int i, token T){
	if(i == -1){
		return i;
	}
	if(SYMBOLS[i].isConst || SYMBOLS[i].type != SET){
		fprintf(stderr, "ERR: REDEFINITION OF SYMBOL %s\n\n", SYMBOLS[i].name);
		return -1;
	}
	int data = getData(T);
	if(data == -1){
		fprintf(stderr, "ERR: INVALID REDEFINITION OF SYMBOL %s\n\n", SYMBOLS[i].name);
		return -1;
	}
	SYMBOLS[i].data = data;
	return 0;
}

int setSym(const char* name, bool isConst, token T, definition type){
	int i = searchSym(name);
	if(i == -1){ //if symbol not in table, create one
		i = addSym(name, isConst, T, type);
	} else{
		i = modifySym(i, T);
	}
	return i;
}

int returnSymData(const char* name){
	int i = searchSym(name);
	if(i == -1){
		fprintf(stderr, "ERR: SYMBOL %s HAS NOT BEEN DEFINED\n\n", name);
		return -1;
	}
	symbol SYM = returnSym(i);
	return SYM.data;
}

int defineSym(int i){
	if(i == -1) return i;
	if(SYMBOLS[i].isDefined){
		fprintf(stderr, "ERR: SYMBOL %s BEING REDEFINED\n\n", SYMBOLS[i].name);
		return -1;
	}
	SYMBOLS[i].isDefined = true;
	return 0;
}

symbol returnSym(int i){
	return SYMBOLS[i];
}

