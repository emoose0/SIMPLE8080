#include "c8080.h"

#define setZ(result) (cpu.Z = (result == 0))
#define setS(result) (cpu.S = ((result & 0b10000000) > 0))
#define setP(result) (cpu.P = (parityTable[result]))

CPU cpu;
int cycles;
uint16_t current = 0; //where instructions are being written
uint16_t highestAdd = 0; //highest address of program
uint16_t lowestAdd = 0xFFFF; //lowest address of program

bool interrupted =  false;

static const uint8_t parityTable[256] = {
	1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,
	0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0
};

void resetCPU(){
	cpu.PC = 0;
	cpu.SP = 0xFFFF;
	cpu.a = cpu.b = cpu.c = cpu.d = cpu.e = cpu.h = cpu.l = 0;
	cpu.S = cpu.Z = cpu.AC = cpu.P = cpu.CY = 0;
	cycles = 0;
	cpu.running = true;
	cpu.enableInterrupt = false;

	for(int i = 0; i < 65536; i++){
		cpu.memory[i] = NOP;
	}
}

uint8_t getmem(uint16_t address){
	return cpu.memory[address];
}

void setmem(uint8_t mem[65536]){
	for(int i = 0; i < 65536; i++){
		cpu.memory[i] = mem[i];
	}
}

void setCurr(uint16_t address){ //set current
	current = address;
}

void incCurr(uint16_t inc){ //incrememnt current
	current += inc;
}

size_t getProgSize(){
	return (highestAdd - lowestAdd) + 1;
}

uint16_t getLowadd(){
	return lowestAdd;
}

void setLowadd(uint16_t add){
	lowestAdd = add;
}

uint16_t getHighadd(){
	return highestAdd;
}

void setHighadd(uint16_t add){
	highestAdd = add;
}

void printCPU(){
	printf("\n\na b c d e h l:\n0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n\n", cpu.a, cpu.b, cpu.c, cpu.d, cpu.e, cpu.h, cpu.l);
	printf("S Z AC P CY:\n%u %u %u %u %u\n\n", cpu.S, cpu.Z, cpu.AC, cpu.P, cpu.CY);
	printf("PC SP:\n0x%x 0x%x\n\n", cpu.PC, cpu.SP);
	printf("cycles: %d\n\n", cycles);
}

void writeByte(uint8_t data){
	if(current > highestAdd) highestAdd = current;
	if(current < lowestAdd) lowestAdd = current;
	cpu.memory[current++] = data;
}

uint8_t readByte(){
	uint8_t data = cpu.memory[cpu.PC];
	return data;
}

void setAllFlags(uint8_t result){ //set all flags except CY and AC
	setZ(result);
	setS(result);
	setP(result);
}
void setCYADD(uint8_t a, uint8_t b, bool carry){ //set CY when adding
	cpu.CY = (uint16_t)(a + b + carry) > 0xFF;
}
void setCYSUB(uint8_t a, uint8_t b, bool carry){ //set CY when subtracting
	cpu.CY = (a < (b + carry));
}
void setACADD(uint8_t a, uint8_t b, bool carry){ //set AC when adding
	cpu.AC = (((a & 0x0F) + (b & 0x0F) + carry) > 0x0F);
}
void setACSUB(uint8_t a, uint8_t b, bool carry){ //set AC when subtracting
	cpu.AC = ((a & 0x0F) < ((b & 0x0F) + carry));
}

uint16_t getBC(){
	return (cpu.b << 8) | cpu.c;
}
void setBC(uint16_t BC){
	cpu.c = BC;
	cpu.b = (BC >> 8);
}

uint16_t getDE(){
	return (cpu.d << 8) | cpu.e;
}
void setDE(uint16_t DE){
	cpu.e = DE;
	cpu.d = (DE >> 8);
}

uint16_t getHL(){
	return (cpu.h << 8) | cpu.l;
}
void setHL(uint16_t HL){
	cpu.l = HL;
	cpu.h = (HL >> 8);
}

uint8_t getM(){
	return cpu.memory[getHL()];
}

uint8_t* getregPTR(uint8_t opcode){ //getter helper function
	switch(opcode){
		case 0: return &cpu.b;
		case 1: return &cpu.c;
		case 2: return &cpu.d;
		case 3: return &cpu.e;
		case 4: return &cpu.h;
		case 5: return &cpu.l;
		case 6: return &cpu.memory[getHL()]; //M
		case 7: return &cpu.a;
		default: return 0; //something has horribly gone wrong
	}
}

void mov(uint8_t instruction){
	uint8_t* source = getregPTR(instruction & 0x07);
	uint8_t* destination = getregPTR((instruction >> 3) & 0x07);

	if(((instruction & 0x07) == 6) || ((instruction >> 3) & 0x07) == 6){ //if source or destination is M
		cycles += 7;
	}else{
		cycles += 5;
	}

	*destination = *source;
}

void alu(uint8_t instruction){ //executing all alu instructions
	uint8_t aluINS = (instruction >> 3) & 0x07;
	uint8_t source = *getregPTR(instruction & 0x07);
	uint8_t oldA = cpu.a;

	switch(aluINS){
		case 0: //ADD
			cpu.a += source;
			setAllFlags(cpu.a);
			setACADD(oldA, source, 0);
			setCYADD(oldA, source, 0);
		break;
		case 1: //ADC
			cpu.a += (source + cpu.CY);
			setAllFlags(cpu.a);
			setACADD(oldA, source, cpu.CY);
			setCYADD(oldA, source, cpu.CY);
		break;
		case 2: //SUB
			cpu.a -= source;
			setAllFlags(cpu.a);
			setACSUB(oldA, source, 0);
			setCYSUB(oldA, source, 0);
		break;
		case 3: //SBB
			cpu.a -= (source + cpu.CY);
			setAllFlags(cpu.a);
			setACSUB(oldA, source, cpu.CY);
			setCYSUB(oldA, source, cpu.CY);
		break;
		case 4: //ANA
			cpu.a &= source;
			setAllFlags(cpu.a);
			cpu.CY = 0;
			cpu.AC = ((oldA | source) & 0x08) != 0; //8080 sets AC to logical OR of bit 3 in AND operations
		break;
		case 5: //XRA
			cpu.a ^= source;
			setAllFlags(cpu.a);
			cpu.CY = cpu.AC = 0;
		break;
		case 6: //ORA
			cpu.a |= source;
			setAllFlags(cpu.a);
			cpu.CY = cpu.AC = 0;
		break;
		case 7:{ //CMP
			uint8_t result = cpu.a - source; //for updating flags
			setAllFlags(result);
			setCYSUB(oldA, source, 0);
			setACSUB(oldA, source, 0);
		break;}

	}
	if((instruction & 0x07) == 6){
		cycles += 7;
	}else{
		cycles += 4;
	}
}

void RCC(bool condition){
	if(condition){
		cpu.PC = (cpu.memory[cpu.SP+1] << 8) | cpu.memory[cpu.SP];
		cpu.SP += 2;
		cycles += 6;
	} else{
		cpu.PC++;
	}
	cycles += 5;
}
void JCC(bool condition){
	if(condition){
		cpu.PC++;
		uint8_t datlo = readByte();
		cpu.PC++;
		uint8_t dathi = readByte();
		cpu.PC = (dathi << 8) | datlo;
	} else{
		cpu.PC += 3;
	}
	cycles += 10;
}
void CCC(bool condition){
	if(condition){
		cpu.memory[cpu.SP-1] = ((cpu.PC+3) >> 8);
		cpu.memory[cpu.SP-2] = (cpu.PC+3) & 0xFF;
		cpu.SP -= 2;
		
		cpu.PC++;
		uint8_t datlo = readByte();
		cpu.PC++;
		uint8_t dathi = readByte();
		cpu.PC = (dathi << 8) | datlo;
		cycles += 6;
	} else{
		cpu.PC += 3;
	}
	cycles += 11;
}

void PUSHRP(uint16_t RP){
	cpu.memory[cpu.SP-1] = (RP >> 8);
	cpu.memory[cpu.SP-2] = RP;
	cpu.SP -= 2;
	cycles += 11;
}
void PSWPUSH(){
	cpu.memory[cpu.SP-1] = cpu.a;
	cpu.memory[cpu.SP-2] = (cpu.S << 7) | (cpu.Z << 6) | (0 << 5) | (cpu.AC << 4) | (0 << 3) | (cpu.P << 2) | (1 << 1) | (cpu.CY); //SP-2 = (cpu.S)(cpu.Z)0(cpu.AC)0(cpu.P)1(cpu.cy)
	cpu.SP -= 2;
	cycles += 11;
}

uint16_t POPRP(){
	return (uint16_t)((cpu.memory[cpu.SP+1] << 8) | cpu.memory[cpu.SP]);
}
void PSWPOP(){
	cpu.CY = (cpu.memory[cpu.SP] & 0b00000001);
	cpu.P = ((cpu.memory[cpu.SP] & 0b00000100) >> 2);
	cpu.AC = ((cpu.memory[cpu.SP] & 0b00010000) >> 4);
	cpu.Z = ((cpu.memory[cpu.SP] & 0b01000000) >> 6);
	cpu.S = ((cpu.memory[cpu.SP] & 0b10000000) >> 7);
	cpu.a = cpu.memory[cpu.SP+1];
	cpu.SP += 2;
	cycles += 10;
}

result executeInstruction(uint8_t data){
//	printf("PC: %x    OP: %x\n\tPC+1: %x    OP: %x\n\tPC:2 %x    OP:%x\n", cpu.PC, cpu.memory[cpu.PC], cpu.PC+1, cpu.memory[cpu.PC+1], cpu.PC+2, cpu.memory[cpu.PC+2]);
//	fflush(stdout);
	switch(data){
		case NOP:
			cycles += 4;
		break;
		//LXI
		case LXIB:
			cpu.PC++;
			cpu.c = readByte();
			cpu.PC++;
			cpu.b = readByte();
			cycles += 10;
		break;
		case LXID:
			cpu.PC++;
			cpu.e = readByte();
			cpu.PC++;
			cpu.d = readByte();
			cycles += 10;
		break;
		case LXIH:
			cpu.PC++;
			cpu.l = readByte();
			cpu.PC++;
			cpu.h = readByte();
			cycles += 10;
		break;
		case LXISP:{
			cpu.PC++;
			uint8_t datlo = readByte();
			cpu.PC++;
			uint8_t dathi = readByte();
			cpu.SP = (dathi << 8) | datlo;
			cycles += 10;
		break;}
		//STAX
		case STAXB:
			cpu.memory[getBC()] = cpu.a;
			cycles += 7;
		break;
		case STAXD:
			cpu.memory[getDE()] = cpu.a;
			cycles += 7;
		break;
		//INX
		case INXB:
			setBC(getBC()+1);
			cycles += 5;
		break;
		case INXD:
			setDE(getDE()+1);
			cycles += 5;
		break;
		case INXH:
			setHL(getHL()+1);
			cycles += 5;
		break;
		case INXSP:
			cpu.SP++;
			cycles += 5;
		break;
		//INR
		case INRA:{
			uint8_t oldA = cpu.a;
			cpu.a++;
			setAllFlags(cpu.a);
			setACADD(oldA, 1, 0);
			cycles += 5;
		break;}
		case INRB:{
			uint8_t oldB = cpu.b;
			cpu.b++;
			setAllFlags(cpu.b);
			setACADD(oldB, 1, 0);
			cycles += 5;
		break;}
		case INRC:{
			uint8_t oldC = cpu.c;
			cpu.c++;
			setAllFlags(cpu.c);
			setACADD(oldC, 1, 0);
			cycles += 5;
		break;}
		case INRD:{
			uint8_t oldD = cpu.d;
			cpu.d++;
			setAllFlags(cpu.d);
			setACADD(oldD, 1, 0);
			cycles += 5;
		break;}
		case INRE:{
			uint8_t oldE = cpu.e;
			cpu.e++;
			setAllFlags(cpu.e);
			setACADD(oldE, 1, 0);
			cycles += 5;
		break;}
		case INRH:{
			uint8_t oldH = cpu.h;
			cpu.h++;
			setAllFlags(cpu.h);
			setACADD(oldH, 1, 0);
			cycles += 5;
		break;}
		case INRL:{
			uint8_t oldL = cpu.l;
			cpu.l++;
			setAllFlags(cpu.l);
			setACADD(oldL, 1, 0);
			cycles += 5;
		break;}
		case INRM:{
			uint8_t oldM = getM();
			cpu.memory[getHL()] += 1;
			setAllFlags(getM());
			setACADD(oldM, 1, 0);
			cycles += 10;
		break;}
		//DCR
		case DCRA:{
			uint8_t oldA = cpu.a;
			cpu.a--;
			setAllFlags(cpu.a);
			cpu.AC = !((cpu.a & 0x0F) == 0xF);
			cycles += 5;
		break;}
		case DCRB:{
			uint8_t oldB = cpu.b;
			cpu.b--;
			setAllFlags(cpu.b);
			cpu.AC = !((cpu.b & 0x0F) == 0xF);
			cycles += 5;
		break;}
		case DCRC:{
			uint8_t oldC = cpu.c;
			cpu.c--;
			setAllFlags(cpu.c);
			cpu.AC = !((cpu.c & 0x0F) == 0xF);
			cycles += 5;
		break;}
		case DCRD:{
			uint8_t oldD = cpu.d;
			cpu.d--;
			setAllFlags(cpu.d);
			cpu.AC = !((cpu.d & 0x0F) == 0xF);
			cycles += 5;
		break;}
		case DCRE:{
			uint8_t oldE = cpu.e;
			cpu.e--;
			setAllFlags(cpu.e);
			cpu.AC = !((cpu.e & 0x0F) == 0xF);
			cycles += 5;
		break;}
		case DCRH:{
			uint8_t oldH = cpu.h;
			cpu.h--;
			setAllFlags(cpu.h);
			cpu.AC = !((cpu.h & 0x0F) == 0xF);
			cycles += 5;
		break;}
		case DCRL:{
			uint8_t oldL = cpu.l;
			cpu.l--;
			setAllFlags(cpu.l);
			cpu.AC = !((cpu.l & 0x0F) == 0xF);
			cycles += 5;
		break;}
		case DCRM:{
			uint8_t oldM = getM();
			cpu.memory[getHL()] -= 1;
			setAllFlags(getM());	
			cpu.AC = !((getM() & 0x0F) == 0xF);
			cycles += 10;
		break;}
		//MVI
		case MVIA:
			cpu.PC++;
			cpu.a = readByte();
			cycles += 7;
		break;
		case MVIB:
			cpu.PC++;
			cpu.b = readByte();
			cycles += 7;
		break;
		case MVIC:
			cpu.PC++;
			cpu.c = readByte();
			cycles += 7;
		break;
		case MVID:
			cpu.PC++;
			cpu.d = readByte();
			cycles += 7;
		break;
		case MVIE:
			cpu.PC++;
			cpu.e = readByte();
			cycles += 7;
		break;
		case MVIH:
			cpu.PC++;
			cpu.h = readByte();
			cycles += 7;
		break;
		case MVIL:
			cpu.PC++;
			cpu.l = readByte();
			cycles += 7;
		break;
		case MVIM:
			cpu.PC++;
			cpu.memory[getHL()] = readByte();
			cycles += 10;
		break;
		//DAD
		case DADB:{
			uint32_t result = getHL() + getBC();
			setHL((uint16_t)result);
			cpu.CY = (result > 0xFFFF);
			cycles += 10;
		break;}
		case DADD:{
			uint32_t result = getHL() + getDE();
			setHL((uint16_t)result);
			cpu.CY = (result > 0xFFFF);
			cycles += 10;
		break;}
		case DADH:{
			uint32_t result = getHL() + getHL();
			setHL((uint16_t)result);
			cpu.CY = (result > 0xFFFF);
			cycles += 10;
		break;}
		case DADSP:{
			uint32_t result = getHL() + cpu.SP;
			setHL((uint16_t)result);
			cpu.CY = (result > 0xFFFF);
			cycles += 10;
		break;}
		//LDAX
		case LDAXB:
			cpu.a = cpu.memory[getBC()];
			cycles += 7;
		break;
		case LDAXD:
			cpu.a = cpu.memory[getDE()];
			cycles += 7;
		break;
		//DCX
		case DCXB:
			setBC(getBC()-1);
			cycles += 5;
		break;
		case DCXD:
			setDE(getDE()-1);
			cycles += 5;
		break;
		case DCXH:
			setHL(getHL()-1);
			cycles += 5;
		break;
		case DCXSP:
			cpu.SP--;
			cycles += 5;
		break;
		//RLC: rotate bits of A circularly left (wrap around)
		case RLC:{
			uint8_t bit7 = (cpu.a >> 7);
			cpu.a = (cpu.a << 1) | bit7;
			cpu.CY = bit7;
			cycles += 4;
		break;}
		//RRC: rotate other way
		case RRC:{
			uint8_t bit0 = (cpu.a & 1);
			cpu.a = (cpu.a >> 1) | (bit0 << 7);
			cpu.CY = bit0;
			cycles += 4;
		break;}
		//RAL: rotate linearly through carry	
		case RAL:{
			uint8_t bit7 = (cpu.a >> 7);
			cpu.a = (cpu.a << 1) | cpu.CY;
			cpu.CY = bit7;
			cycles += 4;
		break;}
		//RAR rotate linearly right
		case RAR:{
			uint8_t bit0 = (cpu.a & 1);
			cpu.a = (cpu.a >> 1) | (cpu.CY << 7);
			cpu.CY = bit0;
			cycles += 4;
		break;}
		//SHLD
		case SHLD:{
			cpu.PC++;
			uint8_t datlo = readByte();
			cpu.PC++;
			uint8_t dathi = readByte();
			uint16_t address = (dathi << 8) | datlo;
			cpu.memory[address] = cpu.l;
			cpu.memory[address+1] = cpu.h;
			cycles += 16;
		break;}
		//DAA
		case DAA:{
			bool carry = cpu.CY;
			uint8_t oldA = cpu.a;
			uint8_t adjustment = 0;
			//low adjustment
			if((cpu.a & 0x0F) > 9 || cpu.AC){
				adjustment |= 0x06;
			}
			if(cpu.a > 0x99 || cpu.CY){ //high adjustment
				adjustment |= 0x60;
				carry = true;
			}
			uint16_t result = cpu.a + adjustment;
			cpu.a = (result & 0xFF);
			cpu.CY = carry;
			setACADD(oldA, adjustment, 0);
			setAllFlags(cpu.a);
			cycles += 4;
		break;}
		//LHLD
		case LHLD:{
			cpu.PC++;
			uint8_t datlo = readByte();
			cpu.PC++;
			uint8_t dathi = readByte();
			uint16_t address = (dathi << 8) | datlo;
			cpu.l = cpu.memory[address];
			cpu.h = cpu.memory[address+1];
			cycles += 16;
		break;}
		//CMA
		case CMA:
			cpu.a = ~cpu.a;
			cycles += 4;
		break;
		//STA
		case STA:{
			cpu.PC++;
			uint8_t datlo = readByte();
			cpu.PC++;
			uint8_t dathi = readByte();
			uint16_t address = (dathi << 8) | datlo;
			cpu.memory[address] = cpu.a;
			cycles += 13;
		break;}
		//STC
		case STC:
			cpu.CY = 1;
			cycles += 4;
		break;
		//LDA
		case LDA:{
			cpu.PC++;
			uint8_t addlo = readByte();
			cpu.PC++;
			uint8_t addhi = readByte();
			uint16_t address = (addhi << 8) | addlo;
			cpu.a = cpu.memory[address];
			cycles += 13;
		break;}
		//CMC
		case CMC:
			cpu.CY = !cpu.CY;
			cycles += 4;
		break;
		//MOV and HLT
		case MOVAA:
		case MOVAB:
		case MOVAC:
		case MOVAD:
		case MOVAE:
		case MOVAH:
		case MOVAL:
		case MOVAM:
		case MOVBA:
		case MOVBB:
		case MOVBC:
		case MOVBD:
		case MOVBE:
		case MOVBH:
		case MOVBL:
		case MOVBM:
		case MOVCA:
		case MOVCB:
		case MOVCC:
		case MOVCD:
		case MOVCE:
		case MOVCH:
		case MOVCL:
		case MOVCM:
		case MOVDA:
		case MOVDB:
		case MOVDC:
		case MOVDD:
		case MOVDE:
		case MOVDH:
		case MOVDL:
		case MOVDM:
		case MOVEA:
		case MOVEB:
		case MOVEC:
		case MOVED:
		case MOVEE:
		case MOVEH:
		case MOVEL:
		case MOVEM:
		case MOVHA:
		case MOVHB:
		case MOVHC:
		case MOVHD:
		case MOVHE:
		case MOVHH:
		case MOVHL:
		case MOVHM:
		case MOVLA:
		case MOVLB:
		case MOVLC:
		case MOVLD:
		case MOVLE:
		case MOVLH:
		case MOVLL:
		case MOVLM:
		case MOVMA:
		case MOVMB:
		case MOVMC:
		case MOVMD:
		case MOVME:
		case MOVMH:
		case MOVML:
			mov(data);
		break;
		//HLT
		case HLT:
			interrupted = true;
			cycles += 7;
		break;
		//ALU operations
		case ADDA:
		case ADDB:
		case ADDC:
		case ADDD:
		case ADDE:
		case ADDH:
		case ADDL:
		case ADDM:
		case ADCA:
		case ADCB:
		case ADCC:
		case ADCD:
		case ADCE:
		case ADCH:
		case ADCL:
		case ADCM:
		case SUBA:
		case SUBB:
		case SUBC:
		case SUBD:
		case SUBE:
		case SUBH:
		case SUBL:
		case SUBM:
		case SBBA:
		case SBBB:
		case SBBC:
		case SBBD:
		case SBBE:
		case SBBH:
		case SBBL:
		case SBBM:
		case ANAA:
		case ANAB:
		case ANAC:
		case ANAD:
		case ANAE:
		case ANAH:
		case ANAL:
		case ANAM:
		case XRAA:
		case XRAB:
		case XRAC:
		case XRAD:
		case XRAE:
		case XRAH:
		case XRAL:
		case XRAM:
		case ORAA:
		case ORAB:
		case ORAC:
		case ORAD:
		case ORAE:
		case ORAH:
		case ORAL:
		case ORAM:
		case CMPA:
		case CMPB:
		case CMPC:
		case CMPD:
		case CMPE:
		case CMPH:
		case CMPL:
		case CMPM:
			alu(data);
		break;
		//RCC and RET
		case RNZ:
			RCC(!cpu.Z);
		return SUCCESS; //for RET, JMP, CALL, and RST, cpu.PC should not auto-increment, so after executing, we just return back to interpret()
		case RZ:
			RCC(cpu.Z);
		return SUCCESS;
		case RNC:
			RCC(!cpu.CY);
		return SUCCESS;
		case RC:
			RCC(cpu.CY);
		return SUCCESS;
		case RPO:
			RCC(!cpu.P);
		return SUCCESS;
		case RPE:
			RCC(cpu.P);
		return SUCCESS;
		case RP:
			RCC(!cpu.S);
		return SUCCESS;
		case RM:
			RCC(cpu.S);
		return SUCCESS;
		case RET:
			cpu.PC = (cpu.memory[cpu.SP+1] << 8) | cpu.memory[cpu.SP];
			cpu.SP += 2;
			cycles += 10;
		return SUCCESS;
		//JCC and JMP
		case JNZ:
			JCC(!cpu.Z);
		return SUCCESS;
		case JZ:
			JCC(cpu.Z);
		return SUCCESS;
		case JNC:
			JCC(!cpu.CY);
		return SUCCESS;
		case JC:
			JCC(cpu.CY);
		return SUCCESS;
		case JPO:
			JCC(!cpu.P);
		return SUCCESS;
		case JPE:
			JCC(cpu.P);
		return SUCCESS;
		case JP:
			JCC(!cpu.S);
		return SUCCESS;
		case JM:
			JCC(cpu.S);
		return SUCCESS;
		case JMP:{
			cpu.PC++;
			uint8_t datlo = readByte();
			cpu.PC++;
			uint8_t dathi = readByte();
			cpu.PC = (dathi << 8) | datlo;
			cycles += 10;
		return SUCCESS;}
		//CCC and CALL
		case CNZ:
			CCC(!cpu.Z);
		return SUCCESS;
		case CZ:
			CCC(cpu.Z);
		return SUCCESS;
		case CNC:
			CCC(!cpu.CY);
		return SUCCESS;
		case CC:
			CCC(cpu.CY);
		return SUCCESS;
		case CPO:
			CCC(!cpu.P);
		return SUCCESS;
		case CPE:
			CCC(cpu.P);
		return SUCCESS;
		case CP:
			CCC(!cpu.S);
		return SUCCESS;
		case CM:
			CCC(cpu.S);
		return SUCCESS;
		case CALL:{
			cpu.memory[cpu.SP-1] = ((cpu.PC+3) >> 8);
			cpu.memory[cpu.SP-2] = (cpu.PC+3) & 0xFF;
			cpu.SP -= 2;
			cpu.PC++;
			uint8_t datlo = readByte();
			cpu.PC++;
			uint8_t dathi = readByte();
			cpu.PC = (dathi << 8) | datlo;
			cycles += 17;
		return SUCCESS;}
		//PUSH
		case PUSHB:
			PUSHRP(getBC());
		break;
		case PUSHD:
			PUSHRP(getDE());
		break;
		case PUSHH:
			PUSHRP(getHL());
		break;
		case PUSHPSW:
			PSWPUSH();
		break;
		//POP
		case POPB:
			setBC(POPRP());
			cpu.SP += 2;
			cycles += 10;
		break;
		case POPD:
			setDE(POPRP());
			cpu.SP += 2;
			cycles += 10;
		break;
		case POPH:
			setHL(POPRP());
			cpu.SP += 2;
			cycles += 10;
		break;
		case POPPSW:
			PSWPOP();
		break;
		//ALU I instructions
		case ADI:{
			cpu.PC++;
			uint8_t data = readByte();
			uint8_t oldA = cpu.a;
			cpu.a += data;
			setACADD(oldA, data, 0);
			setCYADD(oldA, data, 0);
			setAllFlags(cpu.a);
			cycles += 7;
		break;}
		case ACI:{
			cpu.PC++;
			uint8_t data = readByte();
			uint8_t oldA = cpu.a;
			cpu.a += (data + cpu.CY);
			setACADD(oldA, data, cpu.CY);
			setCYADD(oldA, data, cpu.CY);
			setAllFlags(cpu.a);
			cycles += 7;
		break;}
		case SUI:{
			cpu.PC++;
			uint8_t data = readByte();
			uint8_t oldA = cpu.a;
			cpu.a -= data;
			setACSUB(oldA, data, 0);
			setCYSUB(oldA, data, 0);
			setAllFlags(cpu.a);
			cycles += 7;
		break;}
		case SBI:{
			cpu.PC++;
			uint8_t data = readByte();
			uint8_t oldA = cpu.a;
			cpu.a -= (data + cpu.CY);
			setACSUB(oldA, data, cpu.CY);
			setCYSUB(oldA, data, cpu.CY);
			setAllFlags(cpu.a);
			cycles += 7;
		break;}
		case ANI:{
			cpu.PC++;
			uint8_t data = readByte();
			uint8_t oldA = cpu.a;
			cpu.a &= data;
			cpu.CY = 0;
			cpu.AC = ((oldA | data) & 0x08) != 0;
			setAllFlags(cpu.a);
			cycles += 7;
		break;}
		case XRI:{
			cpu.PC++;
			uint8_t data = readByte();
			cpu.a ^= data;
			cpu.CY = cpu.AC = 0;
			setAllFlags(cpu.a);
			cycles += 7;
		break;}
		case ORI:{
			cpu.PC++;
			uint8_t data = readByte();
			cpu.a |= data;
			cpu.CY = cpu.AC = 0;
			setAllFlags(cpu.a);
			cycles += 7;
		break;}
		case CPI:{
			cpu.PC++;
			uint8_t data = readByte();
			uint8_t result = cpu.a - data;
			setAllFlags(result);
			setCYSUB(cpu.a, data, 0);
			setACSUB(cpu.a, data, 0);
			cycles += 7;
		break;}
		//RST
		case RST0:
		case RST1:
		case RST2:
		case RST3:
		case RST4:
		case RST5:
		case RST6:
		case RST7:
			cpu.memory[cpu.SP-1] = ((cpu.PC+1) >> 8); //move the next instruction into memory[SP]
			cpu.memory[cpu.SP-2] = (cpu.PC+1) & 0xFF;
			cpu.SP -= 2;
			cpu.PC = 8*((data >> 3) & 0x07);
			cycles += 11;
		return SUCCESS;
		//PORT COMMANDS
		case IN:{
			cpu.PC++;
			uint8_t portNUM = readByte();
			cpu.a = cpu.PORT[portNUM];
			cycles += 10;
		break;}
		case OUT:{
			cpu.PC++;
			uint8_t portNUM = readByte();
			cpu.PORT[portNUM] = cpu.a;
			cycles += 10;
		break;}
		//XTHL HL <-> SP (swap)
		case XTHL:{
			uint8_t oldH = cpu.h;
			uint8_t oldL = cpu.l;
			setHL((uint16_t)((cpu.memory[cpu.SP+1] << 8) | cpu.memory[cpu.SP]));
			cpu.memory[cpu.SP+1] = oldH;
			cpu.memory[cpu.SP] = oldL;
			cycles += 18;
		break;}
		//PCHL
		case PCHL:
			cpu.PC = getHL();
			cycles += 5;
		return SUCCESS;
		//XCHG
		case XCHG:{
			uint16_t oldHL = getHL();
			setHL(getDE());
			setDE(oldHL);
			cycles += 5;	
		break;}
		//SPHL
		case SPHL:
			cpu.SP = getHL();
			cycles += 5;
		break;
		//Disable Interrupt and Enable Interrupt
		case DI:
			cpu.enableInterrupt = false;
			cycles += 4;
		break;
		case EI:
			cpu.enableInterrupt = true;
			cycles += 4;
		break;
		default:
			fprintf(stderr, "ERR: UNKNOWN OPCODE %x  AT PC = %u\n\n", cpu.memory[cpu.PC], cpu.PC);
			return FAIL;			
		break; 
	}
	cpu.PC++;
	return SUCCESS;
}

int handleBDOS(){ //for the 8080TST.com rom
	switch(cpu.c){
		case 2: 
			printf("%c", cpu.e); 
			fflush(stdout);
			break;
		case 9:{
			uint16_t address = getDE();
			char c = cpu.memory[address];
			while(c != '$'){
				printf("%c", c);
				fflush(stdout);
				c = cpu.memory[++address];
			}
		break;}
		default: return -1; 
	}
	return 0;
}

result interpret(bool isCOM, bool interrupts){
	cpu.PC = lowestAdd;
	cpu.enableInterrupt = interrupts;
	CPU initState = cpu;

	if(isCOM){
		cpu.memory[0x0006] = 0xFE;
		cpu.memory[0x0007] = 0xFF;
	}
	while(cpu.running){
		if(interrupted && cpu.enableInterrupt){
			printf("\nCPU INTERRUPTED! Type 'i' and press enter to uninterrupt, 'r' to reset, anything else to exit\n\n");
			fflush(stdout);
			char c = getchar();

			if(c == 'i') interrupted = false;
			else if(c == 'r'){
				printf("\nCPU RESET BACK TO PC = %x\n\n", lowestAdd);
				fflush(stdout);
				cpu = initState;
				interrupted = false;
				cycles = 0;
			} else{
				cpu.running = false;
				printf("\nExiting...\n\n");
				fflush(stdout);
				break;
			}
		} else if(interrupted && !cpu.enableInterrupt){
			cpu.running = false;
			break;
		}


		if(isCOM && cpu.PC == 5){
			handleBDOS();
			//simulate RET
			cpu.PC = (cpu.memory[cpu.SP+1] << 8) | cpu.memory[cpu.SP];
			cpu.SP += 2;
			cycles += 10;
		}
		if(isCOM && cpu.PC == 0){
			cpu.running = false;
			break;
		}
		result r = executeInstruction(readByte());
		if(r == FAIL) return FAIL;
	}

	return SUCCESS;
}

