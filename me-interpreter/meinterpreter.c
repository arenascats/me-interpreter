#include <stdlib.h>
#include <stdio.h> /* For printf() */
#include <string.h>
#define KB 6
#define DEBUG 1
#ifdef DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#endif

//----------------------------------Base------------------------------------//
int interpreter(char *str);
int token;            // current token
char *src, *old_src;  // pointer to source code string;
int poolsize;         // default size of text/data/stack,use for malloc
int line;             // line number
					  /*---------------------------------------------------------------------------*/

					  //-------------------------------Virtual machine------------------------------//
int  *text;           // Text segment
int  *old_text;        // In order to dump text segment
int  *stack;           // stack for process function data
char *data;           // data segment
int *pc, *bp, *sp, ax, cycle;
int debug = 0;
//pc:program counter; bp:base address pointer; sp:pointer register; ax:universal register
//
enum {
	LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH,
	OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
	OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT
};
void program();
int cvm_init();
void next();
void expression(int level);
int readstr(char *str1, char *str2);

const int text_poolsize = KB *1024;
const int data_poolsize = KB *1024;
const int stack_poolsize = KB *1024;
int main()
{
	const char *test1 = " printf(\"hello\"); ";
	interpreter(test1);
	getchar();
}

/*---------------------------------------------------------------------------*/
int interpreter(char *str)
{
	int i;
	poolsize = 1024 * KB;
	PRINTF("INFO:Read str: %s\n", str);
	if (!(src = old_src = malloc(poolsize)))
	{
		printf("ERROR:Malloc failure.\n");
	}
	PRINTF("INFO:Malloc complete,size: %d\n", poolsize);
	i = readstr(src, str);
	src[i] = 0;//EOF flag
	PRINTF("INFO:Copy str to src: %s, size:%d\n", src, i);
	if (cvm_init() != 0)
	{
		PRINTF("ERROR:cvm init failure.\n");
	}
	i = 0;
	text[i++] = IMM;
	text[i++] = 10;
	text[i++] = PUSH;
	text[i++] = IMM;
	text[i++] = 20;
	text[i++] = ADD;
	text[i++] = PUSH;
	text[i++] = EXIT;
	pc = text;
	evalv2();
	program();
	return 0;

}
//---------------Init virtual machine------------------//
int cvm_init()
{
	// Allocate memory for virtual machine
	if (!(text = old_text = malloc(text_poolsize))) {
		printf("ERROR:could not malloc(%d) for text area\n", text_poolsize);
		return -1;
	}
	if (!(data = malloc(data_poolsize))) {
		printf("ERROR:could not malloc(%d) for data area\n", data_poolsize);
		return -1;
	}
	if (!(stack = malloc(stack_poolsize))) {
		printf("ERROR:could not malloc(%d) for stack area\n", stack_poolsize);
		return -1;
	}
	//Memory space Init
	memset(text, 0, text_poolsize);
	memset(data, 0, data_poolsize);
	memset(stack, 0, stack_poolsize);
	//Register Init
	bp = sp = (int *)((int)stack + stack_poolsize);//size defined by stack_poolsize
	ax = 0;
	PRINTF("INFO:Cvm init complete.\n");
	return 0;

}
void next() {
	token = *src++;
	return;
}
void expression(int level) {
	// do nothing
}
void program() {
	next();                  // get next token
	while (token > 0) {
		printf("token is: %c\n", token);
		next();
	}
}
//---------------------instruction set judge---------------------------------//
int eval() {
	int op, *tmp;
	while (1) {
		op = *pc++;
		if (op == IMM) { ax = *pc++; }                  // load immediate value to ax
		else if (op == LC) { ax = *(char *)ax; }            // load character to ax, address in ax
		else if (op == LI) { ax = *(int *)ax; }             // load integer to ax, address in ax
		else if (op == SC) { ax = *(char *)*sp++ = ax; }   // save character to address, value in ax, address on stack
		else if (op == SI) { *(int *)*sp++ = ax; }        // save integer to address, value in ax, address on stack
		else if (op == PUSH) { *--sp = ax; } 			//save ax value onto stack
		else if (op == JMP) { pc = (int *)*pc; } 	//jump command make pc register set as <addr>
		else if (op == JZ) { pc = ax ? pc + 1 : (int *)*pc; } 		//if ax>0,pc=pc+1,else (int*)*pc,jump;   brief:ax is 0,jump
		else if (op == JNZ) { pc = ax ? (int *)*pc : pc + 1; }		//if ax>0,pc=(int *)*pc,else pc = pc+1;  brief:ax is 1,jump
		else if (op == CALL) { *--sp = (int)(pc + 1); pc = (int *)*pc; }           // call subroutine
		else if (op == ENT) { *--sp = (int)bp; bp = sp; sp = sp - *pc++; }//save current stack pointer,make new stack frame
		else if (op == ADJ) { sp = sp + *pc++; }     	//clear stack data(cover way)
		else if (op == LEV) { sp = bp; bp = (int *)*sp++; pc = (int *)*sp++; }  // restore call frame and PC
		else if (op == LEA) { ax = (int)(bp + *pc++); }     // load address for arguments.
															//----------------------------oparator set judge-----------------------------------//
		else if (op == OR)  ax = *sp++ | ax;
		else if (op == XOR) ax = *sp++ ^ ax;
		else if (op == AND) ax = *sp++ & ax;
		else if (op == EQ)  ax = *sp++ == ax;
		else if (op == NE)  ax = *sp++ != ax;
		else if (op == LT)  ax = *sp++ < ax;
		else if (op == LE)  ax = *sp++ <= ax;
		else if (op == GT)  ax = *sp++ >  ax;
		else if (op == GE)  ax = *sp++ >= ax;
		else if (op == SHL) ax = *sp++ << ax;
		else if (op == SHR) ax = *sp++ >> ax;
		else if (op == ADD) ax = *sp++ + ax;
		else if (op == SUB) ax = *sp++ - ax;
		else if (op == MUL) ax = *sp++ * ax;
		else if (op == DIV) ax = *sp++ / ax;
		else if (op == MOD) ax = *sp++ % ax;
		//--------------------------------------inside function------------------------------------//
		else if (op == EXIT) { printf("exit(%d)", *sp); return *sp; }
		//else if (op == EXIT) { printf("PROCESS_EXIT()", *sp); return *sp;}//contiki specifical function
		else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
		else if (op == CLOS) { ax = close(*sp); }
		else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }
		else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
		else if (op == MALC) { ax = (int)malloc(*sp); }
		else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp); }
		else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp); }
		else {
			printf("unknown instruction:%d\n", op);
			return -1;
		}




	}
	return 0;
}
int evalv2() {
	int op, *tmp;
	cycle = 0;
	while (1) {
		cycle++;
		op = *pc++; // get next operation code

					// print debug info
		if (debug) {
			printf("%d> %.4s", cycle,
				&"LEA ,IMM ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PUSH,"
				"OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
				"OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT"[op * 5]);
			if (op <= ADJ)
				printf(" %d\n", *pc);
			else
				printf("\n");
		}
		if (op == IMM) { ax = *pc++; }                                     // load immediate value to ax
		else if (op == LC) { ax = *(char *)ax; }                               // load character to ax, address in ax
		else if (op == LI) { ax = *(int *)ax; }                                // load integer to ax, address in ax
		else if (op == SC) { ax = *(char *)*sp++ = ax; }                       // save character to address, value in ax, address on stack
		else if (op == SI) { *(int *)*sp++ = ax; }                             // save integer to address, value in ax, address on stack
		else if (op == PUSH) { *--sp = ax; }                                     // push the value of ax onto the stack
		else if (op == JMP) { pc = (int *)*pc; }                                // jump to the address
		else if (op == JZ) { pc = ax ? pc + 1 : (int *)*pc; }                   // jump if ax is zero
		else if (op == JNZ) { pc = ax ? (int *)*pc : pc + 1; }                   // jump if ax is zero
		else if (op == CALL) { *--sp = (int)(pc + 1); pc = (int *)*pc; }           // call subroutine
																				   //else if (op == RET)  {pc = (int *)*sp++;}                              // return from subroutine;
		else if (op == ENT) { *--sp = (int)bp; bp = sp; sp = sp - *pc++; }      // make new stack frame
		else if (op == ADJ) { sp = sp + *pc++; }                                // add esp, <size>
		else if (op == LEV) { sp = bp; bp = (int *)*sp++; pc = (int *)*sp++; }  // restore call frame and PC
		else if (op == LEA) { ax = (int)(bp + *pc++); }                         // load address for arguments.

		else if (op == OR)  ax = *sp++ | ax;
		else if (op == XOR) ax = *sp++ ^ ax;
		else if (op == AND) ax = *sp++ & ax;
		else if (op == EQ)  ax = *sp++ == ax;
		else if (op == NE)  ax = *sp++ != ax;
		else if (op == LT)  ax = *sp++ < ax;
		else if (op == LE)  ax = *sp++ <= ax;
		else if (op == GT)  ax = *sp++ >  ax;
		else if (op == GE)  ax = *sp++ >= ax;
		else if (op == SHL) ax = *sp++ << ax;
		else if (op == SHR) ax = *sp++ >> ax;
		else if (op == ADD) ax = *sp++ + ax;
		else if (op == SUB) ax = *sp++ - ax;
		else if (op == MUL) ax = *sp++ * ax;
		else if (op == DIV) ax = *sp++ / ax;
		else if (op == MOD) ax = *sp++ % ax;

		else if (op == EXIT) { printf("exit(%d)", *sp); return *sp; }
		else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
		else if (op == CLOS) { ax = close(*sp); }
		else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }
		else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
		else if (op == MALC) { ax = (int)malloc(*sp); }
		else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp); }
		else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp); }
		else {
			printf("unknown instruction:%d\n", op);
			return -1;
		}
	}
}
int readstr(char *str1, char *str2)
{
	strcpy(str1, str2);
	return strlen(str2);
}