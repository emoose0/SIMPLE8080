#include "lexer.h"

lexer* l;
int linecount = 1;

int gettcapacity(){
	return l->tcapacity;
}

int gettcount(){
	return l->tcount;
}

static const operator operators[] = {
	{"NOP", iNOP, 0, NOP},
	{"LXI", iLXI, 2, LXIB}, //because B and BC are 000, the default opcodes are equivalent to subtituting B/BC in an opcode
	{"STAX", iSTAX, 2, STAXB},
	{"INX", iINX, 1, INXB},
	{"INR", iINR, 1, INRB},
	{"DCR", iDCR, 1, DCRB},
	{"MVI", iMVI, 2, MVIB},
 	{"DAD", iDAD, 2, DADB},
	{"LDAX", iLDAX, 1, LDAXB},
	{"DCX", iDCX, 1, DCXB},
	{"RLC", iRLC, 0, RLC},
	{"RRC", iRRC, 0, RRC},
	{"RAL", iRAL, 0, RAL},
	{"RAR", iRAR, 0, RAR},
	{"SHLD", iSHLD, 1, SHLD},
	{"DAA", iDAA, 0, DAA},
	{"LHLD", iLHLD, 1, LHLD},
	{"CMA", iCMA, 0, CMA},
	{"STA", iSTA, 1, STA},
	{"STC", iSTC, 0, STC},
	{"LDA", iLDA, 1, LDA},
	{"CMC", iCMC, 0, CMC},
	{"MOV", iMOV, 2, MOVBB},
	{"HLT", iHLT, 0, HLT},
	{"ADD", iADD, 1, ADDB},
	{"ADC", iADC, 1, ADCB},
	{"SUB", iSUB, 1, SUBB},
	{"SBB", iSBB, 1, SBBB},
	{"ANA", iANA, 1, ANAB},
	{"XRA", iXRA, 1, XRAB},
	{"ORA", iORA, 1, ORAB},
	{"CMP", iCMP, 1, CMPB},
	{"RNZ", iRNZ, 0, RNZ},
	{"RZ", iRZ, 0, RZ},
	{"RNC", iRNC, 0, RNC},
	{"RC", iRC, 0, RC},
	{"RPO", iRPO, 0, RPO},
	{"RPE", iRPE, 0, RPE},
	{"RM", iRM, 0, RM},
	{"RP", iRP, 0, RP},
	{"POP", iPOP, 1, POPB},
	{"JNZ", iJNZ, 1, JNZ},
	{"JZ", iJZ, 1, JZ},
	{"JNC", iJNC, 1, JNC},
	{"JC", iJC, 1, JC},
	{"JPO", iJPO, 1, JPO},
	{"JPE", iJPE, 1, JPE},
	{"JM", iJM, 1, JM},
	{"JP", iJP, 1, JP},
	{"JMP", iJMP, 1, JMP},
	{"CNZ", iCNZ, 1, CNZ},
	{"CZ", iCZ, 1, CZ},
	{"CNC", iCNC, 1, CNC},
	{"CC", iCC, 1, CC},
	{"CPO", iCPO, 1, CPO},
	{"CPE", iCPE, 1, CPE},
	{"CM", iCM, 1, CM},
	{"CP", iCP, 1, CP},
	{"PUSH", iPUSH, 1, PUSHB},
	{"ADI", iADI, 1, ADI},
	{"ACI", iACI, 1, ACI},
	{"SUI", iSUI, 1, SUI},
	{"SBI", iSBI, 1, SBI},
	{"XRI", iXRI, 1, XRI},
	{"ORI", iORI, 1, CPI},
	{"CPI", iCPI, 1, CPI},
	{"RST", iRST, 1, RST0},
	{"RET", iRET, 0, RET},
	{"CALL", iCALL, 1, CALL},
	{"OUT", iOUT, 1, OUT},
	{"IN", iIN, 1, IN},
	{"XTHL", iXTHL, 0, XTHL},
	{"PCHL", iPCHL, 0, PCHL},
	{"XCHG", iXCHG, 0, XCHG},
	{"DI", iDI, 0, DI},
	{"SPHL", iSPHL, 0, SPHL},
	{"EI", iEI, 0, EI},
	{"DW", aDW, 1, 0},
	{"DB", aDB, 1, 0},
	{"DS", aDS, 1, 0},
	{"EQU", aEQU, 1, 0},
	{"SET", aSET, 1, 0},
	{"ORG", aORG, 1, 0}
};

void printtokens(){	
	for(int i = 0; i < l->tcount; i++){
   		 printf("%d: type=%d", i, l->tokens[i].type);

		if(l->tokens[i].type == INSTRUCTION)
			printf(" (%s)", l->tokens[i].attribute.op->str);

   		 printf("\n");
	}	
}

int initLexer(const char* code){
	l = (lexer*)malloc(sizeof(lexer));
	if(l == NULL){
		return -1;
	}

	l->content = code;
	l->i = 0;
	l->c = l->content[l->i];

	l->tokens = (token*)malloc(sizeof(token)*10);
	if(l->tokens == NULL){
		fprintf(stderr, "ERR: NOT ENOUGH MEMORY FOR TOKENS");
		return -1;
	}
	l->ti = l->tcount = 0;
	l->tcapacity = 10;

	return 0;
}

bool isNum(char c){
	return (c >= '0' && c <= '9');
}
bool isHex(char c){
	return (isNum(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}
bool isAlpha(char c){
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

void advance(){
	l->i++;
	l->c = l->content[l->i];
}

int getNum(){
	int base = 10;
	int oldi = l->i;
	bool hasSuffix = false;
	uint16_t number = 0;
	uint32_t temp = 0; //used to check overflow
	while(isHex(l->c)){ //get suffix
		if(tolower(l->c) == 'b' && !isHex(l->content[l->i+1])){
			break;
		}
		advance();
	}
	if(!isNum(l->c)){
		switch(tolower(l->c)){
			case 'h':
				base = 16;
				hasSuffix = true;
			break;
			case 'o':
			case 'q': 
				base = 8;
				hasSuffix = true;
			break;
			case 'b':
				base = 2; 
				hasSuffix = true;
			break;
		}
	}
	l->c = l->content[oldi]; //go back to beginning and actually read the number

	while(oldi < l->i){
		uint8_t digit = 0;
		if(isNum(l->c)){
			digit = l->c - '0';
		} else if(l->c >= 'a' && l->c <= 'f'){
			digit = l->c - 'a' + 10;
		} else if(l->c >= 'A' && l->c <= 'F'){
			digit = l->c - 'A' + 10;
		}

		if(digit >= base){ //invalid combination of numbers with base system: eg 2b or 0Ad
			fprintf(stderr, "ERR: INVALID DIGIT %d IN BASE %d ON LINE %d\n\n", digit, base, linecount);
			return -1;
		}
		temp = (temp * base) + digit;
		oldi++;
		l->c = l->content[oldi];
	}
	if(temp > 0xFFFF){ //number too big!
		fprintf(stderr, "ERR: OVERFLOW, NUMBER ON LINE %d TOO BIG\n\n", linecount);
		return -1;
	}
	if(!hasSuffix) l->c = l->content[--l->i]; //go to last digit character if no suffix
	number = (uint16_t)(temp);
	return number;
}

char* getString(){
	int count = 0; //character count
	int size = 10;
	char* word = (char*)malloc(size);
	if(word == NULL){
		fprintf(stderr, "ERR: RAN OUT OF MEMORY\n\n");
		return NULL;
	}
	while(isAlpha(l->c) || isNum(l->c)){
		if(count >= size){
			size *= 2;
			char* tmp = (char*)realloc(word, size);
			if(tmp == NULL){
				free(word);
				fprintf(stderr, "ERR: RAN OUT OF MEMORY\n\n");
				return NULL;
			}
			word = tmp;
		}
		word[count++] = l->c;
		advance();
	}
	//adding null terminator
	if(count >= size){
		size++;
		char* tmp = (char*)realloc(word, size);
		if(tmp == NULL){
			free(word);
			fprintf(stderr, "ERR: RAN OUT OF MEMORY\n\n");
			return NULL;
		}
		word = tmp;
	}
	word[count] = '\0';
	l->c = l->content[--l->i]; //go back to last character
	return word;
}

token getStrlit(){
	token T = {0};
	int count = 0;
	int size = 10;
	char* word = (char*)malloc(size);
	char endQuote = l->c; //will be " if double or ' if single
	if(word == NULL){
		fprintf(stderr, "ERR: NOT ENOUGH MEMORY TO ALLOCATE STRING LITERAL\n\n");
		return T;
	}

	advance(); //go to the next character after the quote

	while(l->c != endQuote){
		if(l->c == '\n' || l->c == '\0'){
			free(word);
			fprintf(stderr, "ERR: NO MATCHING ' %c  ' QUOTE ON LINE %d\n\n", endQuote, linecount);
			return T;
		}
		if(count >= size){
			size *= 2;
			char* tmp = (char*)realloc(word, size);
			if(tmp == NULL){
				free(word);
				fprintf(stderr, "ERR: RAN OUT OF MEMORY\n\n");
				return T;
			}
			word = tmp;
		}
		word[count++] = l->c;
		advance();
	}

	if(count == 0){
		free(word);
		fprintf(stderr, "ERR: EMPTY STRING LITERAL ON LINE %d\n\n", linecount);
		return T;
	}

	T.type = STRINGLIT;
	T.attribute.STRL.len = count;
	T.attribute.STRL.string = word;

	return T;
}

token initToken(tokenType type, void* data){
	token T = {0};
	T.type = type;

	
	switch(T.type){
		case INSTRUCTION: T.attribute.op = (operator*)(data); break;
		case SYMBOL: T.attribute.name = (char*)(data); break; 
		default: break;
	}
	
	return T;
}

int addToken(token T){
	l->tcount++;
	if(l->tcount >= l->tcapacity){ //allocate tokens
		l->tcapacity += 10;
		token* tmp = (token*)realloc(l->tokens, l->tcapacity*sizeof(token));

		if(tmp == NULL){
			fprintf(stderr, "ERR: NOT ENOUGH SPACE TO MAKE TOKENS\n\n");
			return -1;
		}
		l->tokens = tmp;
	}
	l->tokens[l->ti++] = T;	

	return 0;
}

REG getreg(char* string){
	if(strcmp(string, "B") == 0) return REGB;
	else if(strcmp(string, "BC") == 0) return REGBC;
	else if(strcmp(string, "C") == 0) return REGC;
	else if(strcmp(string, "D") == 0) return REGD;
	else if(strcmp(string, "DE") == 0) return REGDE;
	else if(strcmp(string, "E") == 0) return REGE;
	else if(strcmp(string, "H") == 0) return REGH;
	else if(strcmp(string, "HL") == 0) return REGHL;
	else if(strcmp(string, "L") == 0) return REGL;
	else if(strcmp(string, "M") == 0) return REGM;
	else if(strcmp(string, "A") == 0) return REGA;
	else if(strcmp(string, "SP") == 0) return REGSP;
	else if(strcmp(string, "PSW") == 0) return REGPSW;
	return REGERROR;
}

token matchKeyword(char* string){
	char* upperString = (char*)malloc(strlen(string)+1);
	for(int i = 0; i < strlen(string)+1; i++){
		upperString[i] = toupper(string[i]);
	}

	int elements = sizeof(operators)/sizeof(operators[0]);
	bool assigned = false;
	token T = {0};
	//register
	if(strcmp(upperString, "B") == 0 || strcmp(upperString, "C") == 0 || strcmp(upperString, "D") == 0 || strcmp(upperString, "E") == 0 || strcmp(upperString, "H") == 0 || strcmp(upperString, "L") == 0 || strcmp(upperString, "A") == 0 ||
	strcmp(upperString, "M") == 0 || strcmp(upperString, "BC") == 0 || strcmp(upperString, "DE") == 0 || strcmp(upperString, "HL") == 0 || strcmp(upperString, "SP") == 0 || strcmp(upperString, "PSW") == 0){
		REG reg = getreg(upperString);
		T.type = REGISTER;
		T.attribute.reg = reg;
	}
	else{ 
		for(int i = 0; i < elements; i++){ //check if token is an operator
			if(strcmp(upperString, operators[i].str) == 0){
				T = initToken(INSTRUCTION,(operator*)(&operators[i]));
				assigned = true;			
			}
		}
		if(!assigned){ //else its a symbol
			T = initToken(SYMBOL, string);
		}
	}
	free(upperString);
	return T;
}

parserResult tokenize(){
	while(l->c != '\0'){
		token T = {0};
		if(isNum(l->c)){
			int number = getNum();
			if(number == -1) return TOKENFAIL;

			T.type = NUMBER;
			T.attribute.data = number;
		} else if(isAlpha(l->c)){
			char* string = getString();
			if(string == NULL) return TOKENFAIL;
			T = matchKeyword(string);
		}
		else{
			switch(l->c){
				case ':': T = initToken(COLON, 0); break;
				case ',': T = initToken(COMMA, 0); break;
				case '\'':
				case '\"':
					T = getStrlit();
					if(T.type == UNKNOWN) return TOKENFAIL;
				break;
				case '\n':
					if(l->content[l->i+1] == '\0'){
						advance();
						continue;
					}
					linecount++;
					T = initToken(NEWLINE, 0);
				break;
				case '\t':
				case ' ':
					while(l->c == ' ' || l->c == '\t'){
						advance();
					}
				continue;
				case ';': //comments
					while(l->c != '\n' && l->c != '\0'){
						advance();
					}
				continue;
				default:
					T = initToken(UNKNOWN, 0);
					fprintf(stderr, "ERR: UNKNOWN CHARACTER %c at line: %d char %d\n\n", l->c, linecount, l->i+1);
				return TOKENFAIL;
			}
		}
		addToken(T); 
		advance();
	}
	addToken(initToken(END, 0));

	return TOKENSUCCESS;
}

token* returnTokens(){
	return l->tokens;
}

void freeLexer(){
	for(int i = 0; i < l->ti; i++){
		if(l->tokens[i].type == SYMBOL) free(l->tokens[i].attribute.name);
		else if(l->tokens[i].type == STRINGLIT) free(l->tokens[i].attribute.STRL.string);
	}

	free(l->tokens);
	free(l);

	l = NULL;
}


