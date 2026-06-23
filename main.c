#include "common.h"
#include "parser.h"
#include "c8080.h"

typedef enum EXT{
	DOTASM,
	DOTBIN,
	DOTCOM,

	ERROR
}EXT;

EXT checkext(const char* filename){
	const char* dot = strrchr(filename, '.');
	
	if(dot == NULL) return ERROR;
	if(dot - filename <= 0) return ERROR; //invalid file

	if(strcmp(dot, ".asm") == 0 || strcmp(dot, ".ASM") == 0) return DOTASM;
	else if(strcmp(dot, ".bin") == 0 || strcmp(dot, ".BIN") == 0) return DOTBIN;
	else if(strcmp(dot, ".com") == 0 || strcmp(dot, ".COM")) return DOTCOM;

	return ERROR;
}

int getASM(const char* filename){
	FILE* file = fopen(filename, "r");
	int size = 10;
	int count = 0;
	char* code = (char*)malloc(size);
	int c;
	if(file == NULL){
		printf("file %s does not exist\n\n", filename);
		return -1;
	}

	while((c = fgetc(file)) != EOF){
		if(count >= size){
			size += 10;
			char* tmp = (char*)realloc(code, size);
			if(tmp == NULL){
				free(code);
				fclose(file);
				file = NULL;
				fprintf(stderr, "ERR: OUT OF MEMORY TO READ FILE\n\n");
				return -1;
			}
			code = tmp;
		}
		code[count++] = c;
	}

	//add string terminator
	if(count >= size){
		size++;
		char* tmp = (char*)realloc(code, size);
		if(tmp == NULL){
			free(code);
			fclose(file);
			file = NULL;
			fprintf(stderr, "ERR: OUT OF MEMORY TO READ FILE\n\n");
			return -1;
		}
		code = tmp;
	}
	code[count] = '\0';
	//final reallocation
	char* tmp = (char*)realloc(code, strlen(code)+1);
	if(tmp == NULL){
		free(code);
		fclose(file);
		file = NULL;
		fprintf(stderr, "ERR: OUT OF MEMORY TO READ FILE\n\n");
		return -1;
	}
	code = tmp;
	
	resetCPU();
	parserResult r = execute(code);

	free(code);
	code = NULL;
	fclose(file);
	file = NULL;

	return 0;
}

int getROM(const char* filename, bool isCOM){
	resetCPU();
	uint8_t memory[65536];
	size_t programsize;

	memset(memory, NOP, sizeof(memory));
	FILE* file = fopen(filename, "rb");
	if(file == NULL){
		fprintf(stderr, "ERR: FILE DOES NOT EXIST\n\n");
		return -1;
	}

	if(isCOM){ //.COM files start at 0x100
		setLowadd(0x100);
		programsize = fread(memory+0x100, 1, 0xFFFF-0x0100, file);
		setHighadd(0x100+programsize-1);
	} else{
		setLowadd(0);
		programsize = fread(memory, 1, 0xFFFF, file);
		setHighadd(programsize-1);
	}

	setmem(memory);
	setCurr(getHighadd());
	
	return 0;
}


int writeROM(const char* filename, const char* ext){
	const char* dot = strrchr(filename, '.');
	if(dot == NULL){
		fprintf(stderr, "ERR: INVALID FILE %s\n\n", filename);
		return -1;
	}
	size_t length = dot - filename;
	
	char* newfilename = (char*)malloc(length + strlen(ext) + 1);
	if(newfilename == NULL){
		fprintf(stderr, "ERR: UNABLE TO CREATE ROM FROM FILE");
		return -1;
	}
	memcpy(newfilename, filename, length);
	memcpy(newfilename + length, ext, strlen(ext));
	newfilename[length+strlen(ext)] = '\0';

	FILE* file = fopen(newfilename, "wb");
	if(file == NULL){
		fprintf(stderr, "ERR: UNABLE TO CREATE ROM %s\n\n", newfilename);
		free(newfilename);
		return -1;
	}

	for(int i = getLowadd(); i < getHighadd()+1; i++){
		uint8_t byte = getmem(i);
		fwrite(&byte, 1, 1, file);
	}
	
	fclose(file);
	file = NULL;

	free(newfilename);
	newfilename = NULL;

	return 0;
}

int makeROM(const char* filename, bool exportCOM, bool exportBIN){
	if(exportBIN) writeROM(filename, ".bin");

	if(exportCOM) writeROM(filename, ".com");

	return 0;
}

int main(int argc, const char* argv[]){
	const char* filename;
	int result = 0;
	bool isCOM = false;
	bool execute = false, exportBIN = false, exportCOM = false, print = false;
	bool interrupts = false;

	if(argc < 2){
		printf("usage: s8080 file.asm/bin/com\n");
		printf("OPTIONS:\n\n");
		printf("\n\t-b: export as bin file\n\t-c: export as com file\n\t-p: print final results\n\t-e: execute file\n\t-i: start with interrupts enables\n\n");
		return -1;
	}

	filename = argv[1];
	EXT ext = checkext(filename);
	if(ext == ERROR){
		printf("file %s has incorrected extension, expected file *.asm or *.bin/com *hex\n\n", filename);
		return -1;
	} 

	if(ext == DOTCOM) isCOM = true;

	if(argc == 2){ //if no args, export file as bin 
		if(ext == DOTASM) exportBIN = true;
		else if(ext == DOTBIN || ext == DOTCOM) execute = true;
	} else if (argc > 2){ //checking for flags
		for(int i = 2; i < argc; i++){
			if(strcmp(argv[i], "-b") == 0) exportBIN = true;
			else if(strcmp(argv[i], "-c") == 0) exportCOM = true;
			else if(strcmp(argv[i], "-p") == 0) print = true, execute = true;
			else if(strcmp(argv[i], "-e") == 0) execute = true;
			else if(strcmp(argv[i], "-i") == 0) interrupts = true;
		}
	}

	if(ext == DOTASM){
		result = getASM(filename);
	} else if(ext == DOTBIN || ext == DOTCOM){
		result = getROM(filename, isCOM);
	}

	if(result != 0) return result;

	if(exportBIN || exportCOM) makeROM(filename, exportCOM, exportBIN);
	if(execute) interpret(isCOM, interrupts);
	if(print) printCPU();
	printf("\n\n");

	return result;
}
