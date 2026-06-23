#include "parser.h"
#include "symbol.h"
parser* p;
int lines = 1;

int insBytes(instructionType ins){ //get how many bytes an instruction is
	switch(ins){
		//1 byte instructions
		case iNOP:
		case iRRC:
		case iRAL:
		case iRAR:
		case iDAA:
		case iCMA:
		case iCMC:
		case iHLT:
		case iMOV:
		case iSTAX:
		case iINX:
		case iINR:
		case iDCR:
		case iDAD:
		case iLDAX:
		case iDCX:
		case iADD:
		case iADC:
		case iSUB:
		case iSBB:
		case iANA:
		case iXRA:
		case iORA:
		case iCMP:
		case iRNZ:
		case iRZ:
		case iRNC:
		case iRC:
		case iRPO:
		case iRPE:
		case iRM:
		case iRP:
		case iRET:
		case iPOP:
		case iPUSH:
		case iRST:
		case iXTHL:
		case iPCHL:
		case iXCHG:
		case iDI:
		case iSPHL:
		case iEI: return 1;
		//2 byte instructions
		case iMVI:
		case iADI:
		case iACI:
		case iSUI:
		case iSBI:
		case iANI:
		case iXRI:
		case iORI:
		case iCPI:
		case iOUT: return 2;
		//3 byte instructions
		case iLXI:
		case iSHLD:
		case iLHLD:
		case iSTA:
		case iLDA:
		case iJNZ:
		case iJZ:
		case iJNC:
		case iJC:
		case iJPO:
		case iJPE:
		case iJM:
		case iJP:
		case iJMP:
		case iCNZ:
		case iCZ:
		case iCNC:
		case iCC:
		case iCPO:
		case iCPE:
		case iCM:
		case iCP:
		case iCALL: return 3;
		default: return 0;
		
	}
}

int initParser(token* tokens, int tcount){
	p = (parser*)malloc(sizeof(parser));
	if(p == NULL){
		fprintf(stderr, "ERR: NOT ENOUGH MONEY TO PARSE\n\n");
		return -1;
	}	

	p->tokens = tokens;
	p->ti = 0;
	p->currentToken = p->tokens[p->ti];
	p->tcount = tcount;

	p->i = 0;
	p->icount = 0;
	p->icapacity = 10;
	p->ins = (instruction*)malloc(sizeof(instruction)*p->icapacity);
	if(p->ins == NULL){
		fprintf(stderr, "ERR: NOT ENOUGH MEMORY TO PARSE\n\n");
		return -1;
	}

	return 0;
}

int addInstruction(instruction ins){
	p->icount++;
	if(p->icount >= p->icapacity){
		p->icapacity += 10;
		instruction* tmp = (instruction*)realloc(p->ins, p->icapacity*sizeof(instruction));
		if(tmp == NULL){
			fprintf(stderr, "ERR: NOT ENOUGH MEMORY TO CREATE INSTRUCTION\n\n");
			return -1;
		}
		p->ins = tmp;
	}
	p->ins[p->i++] = ins;

	return 0;
}

token peekToken(){
	return p->tokens[p->ti+1];
}

int addOperand(instruction* ins,token T){
	ins->numops++;
	if(ins->numops > ins->op->reqOperands && ins->op->ins != aDW && ins->op->ins != aDB){
		fprintf(stderr, "ERR: INVALID INSTRUCTION ON LINE %d\n\n", lines);
		return -1;
	}
	if(ins->numops > ins->op->reqOperands && (ins->op->ins == aDW || ins->op->ins == aDB)){ //DW and DB can have more than 1 operand, in this case, we reallocate memory to account for this
		token* tmp = (token*)realloc(ins->operands, sizeof(token)*ins->numops);
		if(tmp == NULL){
			fprintf(stderr, "ERR: NOT ENOUGH SPACE FOR OPERANDS ON LINE %d\n\n", lines);
			return -1;
		}
		ins->operands = tmp;
	}
	ins->operands[ins->numops-1] = T;
	return 0;
}

bool expectOperand(tokenType type){
	switch(type){
		case COMMA:
		case INSTRUCTION:
		case COLON:
		case NEWLINE:
		case END:
		case UNKNOWN: return false;
		default: return true;
	}
}

bool expectEnd(tokenType type){
	switch(type){
		case NEWLINE:
		case END: return true;
		default: return false;
	}
}

void advanceToken(){
	p->currentToken = p->tokens[++p->ti];
}

void skipNEWLINE(){
	while(p->currentToken.type == NEWLINE){
		advanceToken();
		lines++;
	}
}

bool isDirective(token T){
	if(T.type == INSTRUCTION){
		return (T.attribute.op->ins == aDB || T.attribute.op->ins == aDW || T.attribute.op->ins == aDS || T.attribute.op->ins == aSET || T.attribute.op->ins == aEQU);
	}
	return false;
}

parserResult createInstruction(){ //turning tokens into Label: MNEMONIC OP1, OP2
	while(p->currentToken.type != END){
		const char* label = NULL;
		instruction ins = {0};
		int operands = 0;

		if(p->currentToken.type == NEWLINE){
			advanceToken();
			lines++;
			continue;
		}

		if(p->currentToken.type == SYMBOL){ //symbol definition
			label = p->currentToken.attribute.name;
			advanceToken();
			if(p->currentToken.type == COLON){ //label that points to address: X: [DIRECTIVE] [OPERAND]
				advanceToken();
				skipNEWLINE();
				if(p->currentToken.type != INSTRUCTION){ 
					fprintf(stderr, "ERR: EXPECTED VALID DIRECTIVE AFTER SYMBOL DECLARATION %s ON LINE %d\n\n", label, lines);
					return PARSERFAIL;
				}
			} else if(!isDirective(p->currentToken)){ //if a label isnt followed by ":" then it must be followed by a valid directive
				fprintf(stderr, "ERR: EXPECTED VALID DIRECTIVE AFTER SYMBOL DECLARATION %s on LINE %d\n\n", label, lines);
			}
		}

		if(p->currentToken.type != INSTRUCTION){
			fprintf(stderr, "EXPECTED INSTRUCTION ON LINE %d\n\n", lines);
			return PARSERFAIL;
		}

		ins.op = p->currentToken.attribute.op;
		advanceToken();
		if(ins.op->reqOperands == 0){
			ins.operands = NULL;
		} else{ 
			 ins.operands = (token*)malloc(sizeof(token)*ins.op->reqOperands);
			if(ins.operands == NULL){
				fprintf(stderr, "ERR: NOT ENOUGH MEMORY TO MAKE INSTRUCTIONS\n\n");
			}
		}

		while(expectOperand(p->currentToken.type)){
			int result = addOperand(&ins, p->currentToken);
			if(result == -1) return PARSERFAIL;
			advanceToken();

			if(p->currentToken.type == COMMA){
				advanceToken();
				if(!expectOperand(p->currentToken.type)){
					fprintf(stderr, "ERR: EXPECTED OPERAND AFTER COMMA ON LINE %d\n\n", lines);
					return PARSERFAIL;
				}
				continue;
			}
		}

		if(!expectEnd(p->currentToken.type)){
			fprintf(stderr, "ERR: INVALID INSTRUCTION ON LINE %d, EXPECTED END OF INSTRUCTION\n\n", lines);
			return PARSERFAIL;
		}
		if(p->currentToken.type == NEWLINE){
			advanceToken();
			lines++;
		}
		ins.label = label;
		addInstruction(ins);
	}
	return PARSERSUCCESS;
}

bool matchOperand(tokenType type1, tokenType type2){
	return type1 == type2;
}

bool matchNum(token T){
	if(T.type == STRINGLIT) return (T.attribute.STRL.len == 1 || T.attribute.STRL.len == 2);
	return (T.type == SYMBOL || T.type == NUMBER);
}

int retriveNum(token T, bool is8b){
	int num = -1;
	if(T.type != SYMBOL && T.type != NUMBER && T.type != STRINGLIT) return -1;
	
	if(T.type == SYMBOL){
		num = returnSymData(T.attribute.name);
		if(is8b && num > 0xFF){
			fprintf(stderr, "ERR: EXPECTED 8 BIT NUMBER, GOT 16 BIT INSTEAD\n");
		} if(num == -1){
			fprintf(stderr, "ERR: INVALID SYMBOL %s\n", T.attribute.name);
		}
	} else if(T.type == STRINGLIT){
		if(is8b){
			if(T.attribute.STRL.len != 1){
				fprintf(stderr, "ERR: EXPECTED 1 ASCII CHARACTER\n");
				return num;
			}
			num = T.attribute.STRL.string[0];
		} else{
			if(T.attribute.STRL.len == 1){
				num = T.attribute.STRL.string[0];
			} else{
				num = (T.attribute.STRL.string[0] << 8) | T.attribute.STRL.string[1];
			}
		}
	} else if(T.type == NUMBER){
		num = T.attribute.data;
		if(is8b && num > 0xFF){
			fprintf(stderr, "ERR: EXPECTED 8 BIT NUMBER, GOT 16 BIT INSTEAD\n");
		}
	}
	return num;
}

uint8_t returnreg(REG reg, bool is8b, bool usePSW){
	if(is8b){
		switch(reg){
			case REGB: return 0;
			case REGC: return 1;
			case REGD: return 2;
			case REGE: return 3;
			case REGH: return 4;
			case REGL: return 5;
			case REGM: return 6;
			case REGA: return 7;
			default:
				fprintf(stderr, "EXPECTED 8 BIT REGISTER\n"); 
				return 0xFF;
		}
	}
	if(usePSW){
		switch(reg){
			case REGB:
			case REGBC: return 0;
			case REGD:
			case REGDE: return 1;
			case REGH:
			case REGHL: return 2;
			case REGPSW: return 3;
			default: 
				fprintf(stderr, "EXPECTED 16 BIT REGISTER\n");
				return 0xFF;
		}
	}
	switch(reg){
		case REGB:
		case REGBC: return 0;
		case REGD:
		case REGDE: return 1;
		case REGH:
		case REGHL: return 2;
		case REGSP: return 3;
		default: 
			fprintf(stderr, "EXPECTED 16 BIT REGISTER\n");
			return 0xFF;
	}

}

parserResult createSymbolTable(){
	int slines = 0; //errors when making symbols
	uint16_t PC = 0;
	int result = 0;
	for(int i = 0; i < p->i; i++){
		slines++;
		if(p->ins[i].label != NULL){
			switch(p->ins[i].op->ins){
				case aEQU:
					result = setSym(p->ins[i].label, 1, p->ins[i].operands[0], EQU);
				continue;
				case aSET:
					result = setSym(p->ins[i].label, 0, p->ins[i].operands[0], SET);
				continue;
				case aDB:{
					token T;
					T.type = NUMBER;
					T.attribute.data = PC;
					result = setSym(p->ins[i].label, 1, T, DB);
					if(result == -1) return PARSERFAIL;

					for(int j = 0; j < p->ins[i].numops; j++){
						if(p->ins[i].operands[j].type == STRINGLIT){
							 PC += p->ins[i].operands[j].attribute.STRL.len;
						} else if(matchNum(p->ins[i].operands[j])){
							int data = retriveNum(p->ins[i].operands[j], 1);
							if(data == -1) return PARSERFAIL;
							PC++;
						} else{
							fprintf(stderr, "ERR: INVALID OPERAND ON LINE %d, EXPECTED NUMBER\n\n", slines);
							return PARSERFAIL;
						}
					}
				continue;}
				case aDW:{
					token T;
					T.type = NUMBER;
					T.attribute.data = PC;
					result = setSym(p->ins[i].label, 1, T, DW);
					if(result == -1) return PARSERFAIL;

					for(int j = 0; j < p->ins[i].numops; j++){
						if(matchNum(p->ins[i].operands[j])){
							int data = retriveNum(p->ins[i].operands[j], 0);
							if(data == -1) return PARSERFAIL;
							PC += 2;
						} else{
							fprintf(stderr, "ERR: INVALID OPERAND ON LINE %d, EXPECTED NUMBER\n\n", slines);
							return PARSERFAIL;
						}
					}
				continue;}
				case aDS:{
					token T;
					T.type = NUMBER;
					T.attribute.data = PC;
					result = setSym(p->ins[i].label, 1, T, DS);
					if(result == -1) return PARSERFAIL;

					if(matchNum(p->ins[i].operands[0])){
						int data = retriveNum(p->ins[i].operands[0], 0);
						if(data == -1) return PARSERFAIL;
						PC += data;
					}
				continue;}
				default:{
					token T;
					T.type = NUMBER;
					T.attribute.data = PC;
					result = setSym(p->ins[i].label, 1, T, NONE);
				break;}

				if(result == -1) return PARSERFAIL;
			}
		} 
		if(p->ins[i].op->ins == aORG){
			if(matchNum(p->ins[i].operands[0])){
				int newPC = retriveNum(p->ins[i].operands[0], 0);
				PC = newPC;
				continue;
			} else{
				fprintf(stderr, "ERR: INVALID OPERAND ON LINE %d, EXPECTED NUM\n\n", slines);
				return PARSERFAIL;
			}
		} else{
			uint8_t inc = insBytes(p->ins[i].op->ins);
			if(inc == 0){
				fprintf(stderr, "ERR: UNKNOWN INSTRUCTION ON LINE %d\n\n", slines);
				return PARSERFAIL;
			}
			PC += inc;
		}

	}
	return PARSERSUCCESS;
}

parserResult encodeInstruction(){
	int plines = 1; //used for errors
	if (createSymbolTable() == PARSERFAIL) return PARSERFAIL; //first pass to create symbols
	for(int i = 0; i < p->i; i++){
		instruction currentins = p->ins[i];
		if(currentins.op->reqOperands == 0){
			writeByte(currentins.op->defaultOP);
		} else{
			//if theres an instruction that needs a label (db, dw, equ, set) and theres no label, throw an error
			if((currentins.op->ins == aDB || currentins.op->ins == aDW || currentins.op->ins == aEQU || currentins.op->ins == aSET) && currentins.label == NULL){
				fprintf(stderr, "ERR: INSTRUCTION ON LINE %d REQUIRES A LABEL\n\n", plines);
				return PARSERFAIL;
			}
			switch(currentins.op->ins){
				//special instructions
				//LXI
				case iLXI:
					if(matchOperand(currentins.operands[0].type, REGISTER) && matchNum(currentins.operands[1])){
						uint8_t reg = returnreg(currentins.operands[0].attribute.reg, 0, 0);
						int data = retriveNum(currentins.operands[1], 0);
						if(reg > 2 || data == -1){
							fprintf(stderr, "ERR: INVALID INSTRUCION ON LINE %d\n\n", plines);
							return PARSERFAIL;
						}
						writeByte((uint8_t)(LXIB | (reg << 4)));
						writeByte((uint8_t)(data & 0xFF)); //datlo
						writeByte((uint8_t)(data >> 8)); //dathi
					}else{
						fprintf(stderr, "ERR: INVALID OPERANDS ON LINE %d\n\n", plines);
						return PARSERFAIL;
					}
				break;
				//MVI
				case iMVI:
					if(matchOperand(currentins.operands[0].type, REGISTER) && matchNum(currentins.operands[1])){
						uint8_t reg = returnreg(currentins.operands[0].attribute.reg, 1, 0);
						int data = retriveNum(currentins.operands[1], 1);
						if(reg == 0xFF || data == -1){
							fprintf(stderr, "ERR: INVALID INSTRUCION ON LINE %d\n\n", plines);
							return PARSERFAIL;
						}
						writeByte((uint8_t)(MVIB | (reg << 3)));
						writeByte((uint8_t)(data & 0xFF));
					}else{
						fprintf(stderr, "ERR: INVALID OPERANDS ON LINE %d\n\n", plines);
						return PARSERFAIL;
					}
				break;
				//MOV
				case iMOV:
					if(matchOperand(currentins.operands[0].type, REGISTER) && matchOperand(currentins.operands[0].type, REGISTER)){
						uint8_t regd = returnreg(currentins.operands[0].attribute.reg, 1, 0);
						uint8_t regs = returnreg(currentins.operands[1].attribute.reg, 1, 0);
						if(regd == 0xFF || regs == 0xFF){
							fprintf(stderr, "ERR: INVALID INSTRUCION ON LINE, EXPECTED 8 BIT REGISTER %d\n\n", plines);
							return PARSERFAIL;
						}
						writeByte((uint8_t)(MOVBB | (regd << 3) | (regs)));
					}else{
						fprintf(stderr, "ERR: INVALID OPERANDS ON LINE %d, EXPECTED 8 BIT REGISTER\n\n", plines);
						return PARSERFAIL;
					}
				break;
				//RST
				case iRST:
					if(matchNum(currentins.operands[0])){
						int N = retriveNum(currentins.operands[0], 1);
						if(N == -1 || N > 7){
							fprintf(stderr, "ERR: INVALID OPERAND ON LINE %d, EXPECTED NUMBER FROM 0-7\n\n", plines);
							return PARSERFAIL;
						}
						writeByte((uint8_t)(RST0 | (N << 3)));
					} else{
						fprintf(stderr, "ERR: INVALID OPERAND ON LINE %d, EXPECTED NUMBER FROM 0-7\n\n", plines);
					}
				break;
				//instructions which can only take BC or DE as operands
				case iLDAX:
				case iSTAX:
					if(matchOperand(currentins.operands[0].type, REGISTER) && (currentins.operands[0].attribute.reg == REGB) || currentins.operands[0].attribute.reg == REGBC || currentins.operands[0].attribute.reg == REGD || currentins.operands[0].attribute.reg == REGDE){
						uint8_t reg = returnreg(currentins.operands[0].attribute.reg, 0, 0);
						writeByte((uint8_t)(currentins.op->defaultOP | (reg << 4)));
					} else{
						fprintf(stderr, "ERR: INVALID OPERANDS ON LINE %d, EXPECTED REGISTER BC OR DE\n", plines);
						return PARSERFAIL;
					}
				break;
				//instructions which only take RP as operands, excluding PSW
				case iINX:
				case iDAD:
				case iDCX:
					if(matchOperand(currentins.operands[0].type, REGISTER)){
						uint8_t reg = returnreg(currentins.operands[0].attribute.reg, 0, 0);
						if(reg == 0xFF){
							fprintf(stderr, "ERR: INCORRECT REGISTER ON LINE %d\n\n", plines);
							return PARSERFAIL;
						}
						writeByte((uint8_t)(currentins.op->defaultOP) | (reg << 4));
					} else{
						fprintf(stderr, "ERR: INVALID OPERANDS ON LINE %d, EXPECTED 16 BIT REGISTER\n\n", plines);
						return PARSERFAIL;
					}
				break;
				//instructions that take only RP as operands, including PSW
				case iPOP:
				case iPUSH:
					if(matchOperand(currentins.operands[0].type, REGISTER)){
						uint8_t reg = returnreg(currentins.operands[0].attribute.reg, 0, 1);
						if(reg == 0xFF){
							fprintf(stderr, "ERR: INCORRECT REGISTER ON LINE %d\n\n", plines);
							return PARSERFAIL;
						}
						writeByte((uint8_t)(currentins.op->defaultOP | (reg << 4)));
					} else{
						fprintf(stderr, "ERR: INVALID OPERANDS ON LINE %d, EXPECTED 16 BIT REGISTER\n\n", plines);
						return PARSERFAIL;
					}
				break;
				//instructions that only take 8bit regs as operands
				case iINR:
				case iDCR:
					if(matchOperand(currentins.operands[0].type, REGISTER)){
						uint8_t reg = returnreg(currentins.operands[0].attribute.reg, 1, 0);
						if(reg == 0xFF){
							fprintf(stderr, "ERR: INVALID OPERANDS ON LINE %d, EXPECTED 8 BIT REGISTER\n\n", plines);
							return PARSERFAIL;
						}
						writeByte((uint8_t)(currentins.op->defaultOP | (reg << 3)));
					} else{
						fprintf(stderr, "ERR: INVALID OPERANDS ON LINE %d, EXPECTED 8 BIT REGISTER\n\n", plines);
						return PARSERFAIL;
					}
				break;
				case iADD:
				case iADC:
				case iSUB:
				case iSBB:
				case iANA:
				case iXRA:
				case iORA:
				case iCMP:
					if(matchOperand(currentins.operands[0].type, REGISTER)){
						uint8_t reg = returnreg(currentins.operands[0].attribute.reg, 1, 0);
						if(reg == 0xFF){
							fprintf(stderr, "ERR: INVALID OPERANDS ON LINE %d, EXPECTED 8 BIT REGISTER\n\n", plines);
							return PARSERFAIL;
						}
						writeByte((uint8_t)(currentins.op->defaultOP | reg));
					} else{
						fprintf(stderr, "ERR: INVALID OPERANDS ON LINE %d, EXPECTED 8 BIT REGISTER\n\n", plines);
						return PARSERFAIL;
					}
				break;
				//instructions that take 16bit address as operand
				case iSHLD:
				case iLHLD:
				case iSTA:
				case iLDA:
				case iJMP:
				case iJNZ:
				case iJZ:
				case iJNC:
				case iJC:
				case iJPO:
				case iJPE:
				case iJM:
				case iJP:
				case iCALL:
				case iCNZ:
				case iCZ:
				case iCNC:
				case iCC:
				case iCPO:
				case iCPE:
				case iCM:
				case iCP:
					if(matchNum(currentins.operands[0])){
						uint16_t add = retriveNum(currentins.operands[0], 0);
						writeByte(currentins.op->defaultOP);
						writeByte((uint8_t)(add & 0xFF));
						writeByte((uint8_t)(add >> 8));
					} else{
						fprintf(stderr, "ERR: INVALID OPERAND ON LINE %d, EXPECTED ADDRESS\n\n", plines);
						return PARSERFAIL;
					}
				break;
				//instructions that take 8 bit data as operand
				case iADI:
				case iACI:
				case iSUI:
				case iSBI:
				case iANI:
				case iXRI:
				case iORI:
				case iCPI:
					if(matchNum(currentins.operands[0])){
						uint8_t data = retriveNum(currentins.operands[0], 1);
						writeByte(currentins.op->defaultOP);
						writeByte(data);
					} else{
						fprintf(stderr, "ERR: INVALID OPERAND ON LINE %d\n\n", plines);
						return PARSERFAIL;
					}
				break;
				//instructions that take a port as operand
				case iOUT:
				case iIN:
					if(matchNum(currentins.operands[0])){
						int data = retriveNum(currentins.operands[0], 1);
						if(data == -1) return PARSERFAIL;
						writeByte(currentins.op->defaultOP);
						writeByte((uint8_t)(data));
					} else{
						fprintf(stderr, "ERR: INVALID OPERAND ON LINE %d\n\n", plines);
						return PARSERFAIL;
					}
				break;
				//directives
				case aSET:
					if(matchNum(currentins.operands[0])){
						int result = modifySym(searchSym(currentins.label), currentins.operands[0]);
						if(result == -1) return PARSERFAIL;
					} else{
						fprintf(stderr, "ERR: INVALID OPERAND FOR SYMBOL DECLARATION %s ON LINE %d\n\n", currentins.label, plines);
						return PARSERFAIL;
					}
				break;
				case aEQU:{
					int sym = searchSym(currentins.label);
					if(sym != -1){
						int result = defineSym(sym);
						if(result == -1) return PARSERFAIL;
					} else{
						return PARSERFAIL;
					}
				break;}
				case aDB:{
					int sym = searchSym(currentins.label);
					if(sym != -1){
						int result = defineSym(sym);
						if(result == -1) return PARSERFAIL;
						for(int j = 0; j < p->ins[i].numops; j++){
							if(p->ins[i].operands[j].type == STRINGLIT){
								for(int k = 0; k < p->ins[i].operands[j].attribute.STRL.len; k++){
									writeByte(p->ins[i].operands[j].attribute.STRL.string[k]);
								}
							} else if(matchNum(p->ins[i].operands[j])){
								int data = retriveNum(p->ins[i].operands[j], 1);
								if(data == -1) return PARSERFAIL;
								writeByte(data);
							} else{
								fprintf(stderr, "ERR: INVALID OPERAND ON LINE %d, EXPECTED NUMBER\n\n", plines);
								return PARSERFAIL;
							}
						}
					} else{
						return PARSERFAIL;
					}
				break;}
				case aDW:{
					int sym = searchSym(currentins.label);
					if(sym != -1){
						int result = defineSym(sym);
						if(result == -1) return PARSERFAIL;
						for(int j = 0; j < p->ins[i].numops; j++){
							if(matchNum(p->ins[i].operands[j])){
								int data = retriveNum(p->ins[i].operands[j], 0);
								if(data == -1) return PARSERFAIL;
								writeByte((uint8_t)(data & 0xFF)); //datlo
								writeByte((uint8_t)(data >> 8)); //dathi
							} else{
								fprintf(stderr, "ERR: INVALID OPERAND ON LINE %d, EXPECTED NUMBER\n\n", plines);
								return PARSERFAIL;
							}
						}
					} else{
						return PARSERFAIL;
					}
				break;}
				case aDS:{
					int sym = searchSym(currentins.label);
					if(sym != -1){
						int result = defineSym(sym);
						if(result == -1) return PARSERFAIL;
						
						int data = retriveNum(p->ins[i].operands[0], 0);
						if(data == -1) return PARSERFAIL;
						incCurr(data);
					} else{
						return PARSERFAIL;
					}
				break;}
				case aORG:
					if(matchNum(currentins.operands[0])){
						int data = retriveNum(currentins.operands[0], 0);
						if(data == -1) return PARSERFAIL;
						setCurr(data);
					} else{
						fprintf(stderr, "ERR: INVALID OPERAND FOR INSTRUCTION ORG ON LINE %d\n\n", plines);
						return PARSERFAIL;
					}
				break;
				//unknown
				default:
					fprintf(stderr, "ERR: UNKNOWN INSTRUCTION ON LINE %d\n\n", plines);
				return PARSERFAIL;
	
			}
		}
		plines++; //since every line can only hold one instruction, reaching the end of an instruction means youre on the next line
	}
	return PARSERSUCCESS;
}

parserResult LEXERFAIL(){
	freeLexer();
	return PARSERFAIL;
}

parserResult end(parserResult RESULT){
	freeParser();
	freeLexer();
	return RESULT;
}

parserResult execute(char* contents){
	parserResult r1, r2, r3, r4 = PARSERFAIL;
//	printf("A\n");

	r1 = initLexer(contents);
	if(r1 == TOKENFAIL) return LEXERFAIL();

	r2 = tokenize();
	if(r2 == TOKENFAIL) return LEXERFAIL();	

	r3 = initParser(returnTokens(), gettcount());
	if(r3 == PARSERFAIL) return end(PARSERFAIL);

//	printf("B\n");

	r4 = createInstruction();
	if(r4 == PARSERFAIL) return end(PARSERFAIL);

	parserResult r5 = encodeInstruction();
	if(r5 == PARSERFAIL) return end(PARSERFAIL);

//	printf("C\n");
//	printf("freeparser start\n");
	freeParser();
//	printf("free parser end\n");
	freeLexer();
//	printf("free lexer end\n");

	return r5;
}

int freeParser(){
	free(p->ins);
	free(p);

	p = NULL;
	return 0;
}
