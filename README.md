# SIMPLE 8080
An emulator + assembler for the Intel 8080 written in C using only standard libraries.

Currently, the emulator supports every 8080 instruction, as well as symbol definitions and assembler directives such as `DB` `DW` `DS` and `ORG`.

# Features
- **Passes `TST8080.com` and `8080PRE.com`**
- Can compile `.asm` files into `.com` and `.bin` files
- Can execute `.bin` and `.com` files
- Counts cycles after every instruction
- Features basic BDOS calls for `.com` files
    - Printing single character to stdout
    - Printing '$' terminating string to stdout
- Basic interrupting function when HLT is called

# USAGE
```
usage: s8080 [file.asm/bin/com] [ARGS]
ARGS:
	-b: export as bin file
	-c: export as com file
	-p: print final results
	-e: execute file
	-i: start with interrupts enables
```
