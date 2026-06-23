#ifndef LEXER_H
#define LEXER_H
#include "c8080.h"

typedef enum parserResult{
	PARSERSUCCESS, TOKENSUCCESS,
	PARSERFAIL, TOKENFAIL
}parserResult;

typedef enum tokenType{
	UNKNOWN,
	
	INSTRUCTION,

	COMMA,
	COLON,
	NEWLINE,

	NUMBER,
	REGISTER,
	SYMBOL,

	STRINGLIT,

	END
}tokenType;

typedef enum REG{
	REGB, REGC, REGD, REGE, REGH, REGL, REGM, REGA,
	REGBC, REGDE, REGHL,
	REGSP, REGPSW,
	REGERROR //couldnt identify a register for some reason
}REG;

typedef enum instructionType{
	iNOP, iLXI, iSTAX, iINX, iINR, iDCR, iMVI, iDAD, iLDAX, iDCX, //instructions for the CPU
	iRLC, iRRC, iRAL, iRAR, iSHLD, iDAA, iLHLD, iCMA, iSTA, iSTC,
	iLDA, iCMC, iMOV, iHLT, iADD, iADC, iSUB, iSBB, iANA, iXRA, iORA, iCMP,
	iRNZ, iRZ, iRNC, iRC, iRPO, iRPE, iRP, iRM, iRET, iPOP, iJNZ,
	iJZ, iJNC, iJC, iJPO, iJPE, iJP, iJM, iJMP, iCNZ, iCZ, iCNC, iCC,
	iCPO, iCPE, iCP, iCM, iCALL, iPUSH, iADI, iACI, iSUI, iSBI, iANI,
	iXRI, iORI, iCPI, iRST, iOUT, iIN, iXTHL, iPCHL, iXCHG, iDI, iSPHL, iEI,

	aDW, aDB, aDS,//assembler directives
	aEQU, aSET,
	aORG
}instructionType;

typedef struct STRLIT{
	int len; //length of string literal
	char* string; //string literal itself
}STRLIT;

typedef struct operator{
	const char* str; //used for turning string into token
	instructionType ins;
	int reqOperands;
	opcode defaultOP; //default opcode
}operator;

typedef struct token{
	tokenType type;

	union attribute{
		uint16_t data; //data if number
		REG reg;
		const operator* op; //operator type if an instruction
		char* name; //name of symbol
		STRLIT STRL; //string literal
	}attribute;
}token;

typedef struct instruction{
	const char* label;
	const operator* op;
	token* operands;
	int numops; //number of operands
}instruction;

typedef struct lexer{
	token* tokens;
	int ti; //token index
	int tcount; //token count
	int tcapacity;

	const char* content;
	char c; //current character
	int i; //content index 
}lexer;

int initLexer(const char* contents);
parserResult tokenize();
token* returnTokens();
void freeLexer();
void printtokens();
int gettcount();
int gettcapacity();

#endif
