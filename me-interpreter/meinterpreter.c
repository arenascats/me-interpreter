////http://lotabout.me/2015/write-a-C-interpreter-2/
//#include "contiki.h"
//#include "dev/serial-line.h"
//#include "dev/leds.h"
#include <stdlib.h>
#include <stdio.h> /* For printf() */
#include <string.h>
#define KB 8
#define DEBUG 1
#define SIMULATOR 1

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

//pc:program counter; bp:base address pointer; sp:pointer register; ax:universal register
//
enum {
	LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH,
	OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
	OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT
};
/*--------------------------------------------------------------------------------------*/

//---------------------------------The tokens ,which support classes---------------------//
enum {
	Num = 128, Fun, Sys, Glo, Loc, Id,
	Char, Else, Enum, If, Int, Return, Sizeof, While,
	Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};
//Fun:function; Sys: ; Glo:global; Loc:local; Id:identifier; Assign: = ; Eq: equivalent
int token_val;                // value of current token (mainly for number)
int *current_id,              // current parsed ID
*symbols;                 // symbol table
						  // fields of identifier
enum { Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize };
/*--------------------------------------------------------------------------------------*/


//------------------------------------types of variable/function---------------------//
enum { CHAR, INT, PTR };//char,int ,pointer
int *idmain;                  // find the `main` function
							  /*--------------------------------------------------------------------------------------*/

							  //--------------------------------global declaration-------------------------------------//
int basetype;    // the type of a declaration, make it global for convenience
int expr_type;   // the type of an expression

				 //--------------------------------the memory malloc size-------------------------------------//
const int text_poolsize = 1024;
const int data_poolsize = 1024;
const int stack_poolsize = 1024;
const int symbol_poolsize = 1024;
/*---------------------------------------------------------------------------*/
//---------------OPTION-------------------//
int debug = 0;//cvm debug flag
int assembly = 0;//next() function debug flag
 //---------------FUNCTION-------------------//
void match(int tk);
void next();
int interpreter(char *str);
void program();
int readstr(char *str1, char *str2);
void global_declaration();
void enum_declaration();
void expression(int level);
void statement();
void function_body();
void function_parameter();
void function_declaration();
void interpreformal();
//--------------------------------------------//
//PROCESS(interp_process, "interpreter.");
//AUTOSTART_PROCESSES(&interp_process);
///*---------------------------------------------------------------------------*/
//PROCESS_THREAD(interp_process, ev, data)
//{
//	const char *test1 = "void main(){\n"
//		" printf(\"hello,simulate run !!!\"); \n"
//		"}\n";
//	PROCESS_BEGIN();
//	interpreter(test1);
//	PROCESS_END();
//}

void main()
{
	const char *test1 = "void main()\n"
		"{\n"
		"int i;\n"
		"i = 20;\n"
		" printf(\"hello,simulate run !!!\\n\"); \n"
		"printf(\"Interpreter running ok ,test int:%d !!!\",i); \n"
		"}\n";
	interpreformal(test1);
}

void interpreformal(char *str)
{
	
	
	//interpreter(test1);
	//return;

	//--------------------------------------ORIGIN CODE--------------------------------------------//
		int i, fd;
		int *tmp;
		
		poolsize = 256 * 1024; // arbitrary size
		line = 1;

		
		// allocate memory
		if (!(text = malloc(poolsize))) {
			printf("could not malloc(%d) for text area\n", poolsize);
			return -1;
		}
		if (!(data = malloc(poolsize))) {
			printf("could not malloc(%d) for data area\n", poolsize);
			return -1;
		}
		if (!(stack = malloc(poolsize))) {
			printf("could not malloc(%d) for stack area\n", poolsize);
			return -1;
		}
		if (!(symbols = malloc(poolsize))) {
			printf("could not malloc(%d) for symbol table\n", poolsize);
			return -1;
		}

		memset(text, 0, poolsize);
		memset(data, 0, poolsize);
		memset(stack, 0, poolsize);
		memset(symbols, 0, poolsize);

		old_text = text;
		src = "char else enum if int return sizeof while "
			"open read close printf malloc memset memcmp exit void main";

		// add keywords to symbol table
		i = Char;
		while (i <= While) {
			next();
			current_id[Token] = i++;
		}

		// add library to symbol table
		i = OPEN;
		while (i <= EXIT) {
			next();
			current_id[Class] = Sys;
			current_id[Type] = INT;
			current_id[Value] = i++;
		}

		next(); current_id[Token] = Char; // handle void type
		next(); idmain = current_id; // keep track of main

		if (!(src = old_src = malloc(poolsize))) {
			printf("could not malloc(%d) for source area\n", poolsize);
			return -1;
		}
		// read the source file
		i = readstr(src, str);
		PRINTF("Sourcefile: %s\n", src);
		src[i] = 0; // add EOF character
		program();

		if (!(pc = (int *)idmain[Value])) {
			printf("main() not defined\n");
			return -1;
		}

		// dump_text();
		if (assembly) {
			// only for compile
			return 0;
		}

		// setup stack
		sp = (int *)((int)stack + poolsize);
		*--sp = EXIT; // call exit if main returns
		*--sp = PUSH; tmp = sp;
	//	*--sp = argc;
	//	*--sp = (int)argv;
		*--sp = (int)tmp;

		return evalv2();
	
}
/*---------------------------------------------------------------------------*/
///
///
///

int interpreter(char *str)
{
	int i;
	int *tmp;
	poolsize = 1024 * KB;
	line = 1;
	if (cvm_init() != 0)//----CVM init
	{
		PRINTF("ERROR:cvm init failure.\n");
	}
	PRINTF("INFO:Read str: %s\n", str);
	if (!(src = old_src = malloc(poolsize)))
	{
		printf("ERROR:Malloc failure.\n");
	}
	PRINTF("INFO:Malloc complete,size: %d\n", poolsize);
	i = readstr(src, str);
	src[i] = 0;//EOF flag
	PRINTF("INFO:Copy str to src: %s, size:%d\n", src, i);
	
	old_text = text;
	//----------------------------'while' 'for' and so on token judge-------------------------------//
	src = "char else enum if int return sizeof while "
		"open read close printf malloc memset memcmp exit void main";
	// add keywords to symbol table
	PRINTF("INFO:start judge 'while'\n");
	i = Char;
	while (i <= While) {
		next();
		current_id[Token] = i++;
	}
	//----------------------------------------------------------------------//
	//#if SIMULATOR
	//----------------------------File token judge----------------------------------//
	PRINTF("INFO:start judge file token \n");
	i = OPEN;
	while (i <= EXIT) {
		next();
		current_id[Class] = Sys;
		current_id[Type] = INT;
		current_id[Value] = i++;
	}

	next(); current_id[Token] = Char; // handle void type
	next(); idmain = current_id; // keep track of main
								 //----------------------------------------------------------------------//
								 //#endif
	program();
	if (!(pc = (int *)idmain[Value])) {
		printf("main() not defined\n");
		return -1;
	}

	// dump_text();
	if (assembly) {
		// only for compile
		return 0;
	}
	PRINTF("INFO:Setup stack.\n");
	sp = (int *)((int)stack + stack_poolsize);
	*--sp = EXIT; // call exit if main returns
	*--sp = PUSH; tmp = sp;
	//    *--sp = argc;
	//    *--sp = (int)argv;
	*--sp = (int)tmp;

	return evalv2();


}
void cvmtest1()
{
	int i = 0;
	i = 0;
	text[i++] = IMM;
	text[i++] = 10;
	text[i++] = PUSH;
	text[i++] = IMM;
	text[i++] = 20;
	text[i++] = SUB;
	text[i++] = PUSH;
	text[i++] = EXIT;
	pc = text;
	evalv2();
}

//---------------Init virtual machine------------------//
int cvm_init()
{
	// Allocate memory for virtual machine
	PRINTF("INFO:start init cvm.\n");
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
	if (!(symbols = malloc(poolsize))) {
		printf("could not malloc(%d) for symbol table\n", poolsize);
		return -1;
	}
	//Memory space Init
	PRINTF("INFO:cvm mem set (init).\n");
	memset(text, 0, text_poolsize);
	memset(data, 0, data_poolsize);
	memset(stack, 0, stack_poolsize);
	memset(symbols, 0, symbol_poolsize);
	// memset(symbols, 0, symbol_poolsize);
	//Register Init
	bp = sp = (int *)((int)stack + stack_poolsize);//size defined by stack_poolsize
	ax = 0;
	PRINTF("INFO:Cvm init complete.\n");
	return 0;

}
//Token analyse, classify word to token,form token stream
void next()
{
	char *last_pos;
	int hash;
	while (token = *src) {
		++src;//current src is point to token next

		if (token == '\n') {
			if (assembly) {
				// print compile info
				printf("%d: %.*s", line, src - old_src, old_src);
				old_src = src;

				while (old_text < text) {
					printf("%8.4s", &"LEA ,IMM ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PUSH,"
						"OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
						"OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT"[*++old_text * 5]);

					if (*old_text <= ADJ)
						printf(" %d\n", *++old_text);
					else
						printf("\n");
				}
			}
			++line;
		}
		else if (token == '#') {
			// skip macro, because we will not support it
			while (*src != 0 && *src != '\n') {
				src++;
			}
		}
		else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) {

			// parse identifier
			last_pos = src - 1;
			hash = token;

			while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
				hash = hash * 147 + *src;
				src++;
			}

			// look for existing identifier, linear search
			current_id = symbols;
			while (current_id[Token]) {
				if (current_id[Hash] == hash && !memcmp((char *)current_id[Name], last_pos, src - last_pos)) {
					//found one, return
					token = current_id[Token];
					return;
				}
				current_id = current_id + IdSize;
			}


			// store new ID
			current_id[Name] = (int)last_pos;
			current_id[Hash] = hash;
			token = current_id[Token] = Id;
			return;
		}
		else if (token >= '0' && token <= '9') {
			// parse number, three kinds: dec(123) hex(0x123) oct(017)
			token_val = token - '0';
			if (token_val > 0) {
				// dec, starts with [1-9]
				while (*src >= '0' && *src <= '9') {
					token_val = token_val * 10 + *src++ - '0';
				}
			}
			else {
				// starts with number 0
				if (*src == 'x' || *src == 'X') {
					//hex
					token = *++src;
					while ((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || (token >= 'A' && token <= 'F')) {
						token_val = token_val * 16 + (token & 15) + (token >= 'A' ? 9 : 0);
						token = *++src;
					}
				}
				else {
					// oct
					while (*src >= '0' && *src <= '7') {
						token_val = token_val * 8 + *src++ - '0';
					}
				}
			}

			token = Num;
			return;
		}
		else if (token == '/') {
			if (*src == '/') {
				// skip comments
				while (*src != 0 && *src != '\n') {
					++src;
				}
			}
			else {
				// divide operator
				token = Div;
				return;
			}
		}
		else if (token == '"' || token == '\'') {
			// parse string literal, currently, the only supported escape
			// character is '\n', store the string literal into data.
			last_pos = data;
			while (*src != 0 && *src != token) {
				token_val = *src++;
				if (token_val == '\\') {
					// escape character
					token_val = *src++;
					if (token_val == 'n') {
						token_val = '\n';
					}
				}

				if (token == '"') {
					*data++ = token_val;
				}
			}

			src++;
			// if it is a single character, return Num token
			if (token == '"') {
				token_val = (int)last_pos;
			}
			else {
				token = Num;
			}

			return;
		}
		else if (token == '=') {
			// parse '==' and '='
			if (*src == '=') {
				src++;
				token = Eq;
			}
			else {
				token = Assign;
			}
			return;
		}
		else if (token == '+') {
			// parse '+' and '++'
			if (*src == '+') {
				src++;
				token = Inc;
			}
			else {
				token = Add;
			}
			return;
		}
		else if (token == '-') {
			// parse '-' and '--'
			if (*src == '-') {
				src++;
				token = Dec;
			}
			else {
				token = Sub;
			}
			return;
		}
		else if (token == '!') {
			// parse '!='
			if (*src == '=') {
				src++;
				token = Ne;
			}
			return;
		}
		else if (token == '<') {
			// parse '<=', '<<' or '<'
			if (*src == '=') {
				src++;
				token = Le;
			}
			else if (*src == '<') {
				src++;
				token = Shl;
			}
			else {
				token = Lt;
			}
			return;
		}
		else if (token == '>') {
			// parse '>=', '>>' or '>'
			if (*src == '=') {
				src++;
				token = Ge;
			}
			else if (*src == '>') {
				src++;
				token = Shr;
			}
			else {
				token = Gt;
			}
			return;
		}
		else if (token == '|') {
			// parse '|' or '||'
			if (*src == '|') {
				src++;
				token = Lor;
			}
			else {
				token = Or;
			}
			return;
		}
		else if (token == '&') {
			// parse '&' and '&&'
			if (*src == '&') {
				src++;
				token = Lan;
			}
			else {
				token = And;
			}
			return;
		}
		else if (token == '^') {
			token = Xor;
			return;
		}
		else if (token == '%') {
			token = Mod;
			return;
		}
		else if (token == '*') {
			token = Mul;
			return;
		}
		else if (token == '[') {
			token = Brak;
			return;
		}
		else if (token == '?') {
			token = Cond;
			return;
		}
		else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':')
		{
			// directly return the character as token;
			return;
		}
	}
}
void match(int tk)
{
	if (token == tk)
	{
		next();
	}
	else
	{
		printf("%d: expected token: %d\n", line, tk);
#if SIMULATOR
		exit(-1);
#else 
		PROCESS_EXIT();

#endif
	}
}


void program() {
	next();                  // get next token
	while (token > 0) {
		global_declaration();//Global declarate
	}
}
//---------------------instruction set judge---------------------------------//

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
				"OPEN, READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT"[op * 5]);
			if (op <= ADJ)
				printf("INFO:eval: %d\n", *pc);
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
		else if (op == OR)  ax = *sp++ | ax;//----------------------------oparator set judge-----------------------------------//
		else if (op == XOR) ax = *sp++ ^ ax;
		else if (op == AND) ax = *sp++ & ax;
		else if (op == EQ)  ax = *sp++ == ax;
		else if (op == NE)  ax = *sp++ != ax;
		else if (op == LT)  ax = *sp++ < ax;
		else if (op == LE)  ax = *sp++ <= ax;
		else if (op == GT)  ax = *sp++ > ax;
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
#if SIMULATOR 
		else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
		else if (op == CLOS) { ax = close(*sp); }
		else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }
#else
		else if (op == OPEN) { ax = cfs_open((char *)sp[1], sp[0]); }
		else if (op == READ) { ax = cfs_read(sp[2], (char *)sp[1], *sp); }
		else if (op == CLOS) { ax = cfs_close(*sp); }
		//--------------hard ware control-------------------------//
		else if (op == LEDI) { leds_on(*sp++); }
		else if (op == LEDO) { leds_off(*sp++); }
#endif
		else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
		else if (op == MALC) { ax = (int)malloc(*sp); }
		else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp); }
		else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp); }
		//--------------------------------------personal design-----------------------------------//
		else {
			printf("ERROR:unknown instruction:%d\n", op);
			return -1;
		}
	}
}
int readstr(char *str1, char *str2)
{
	strcpy(str1, str2);
	return strlen(str2);
}

void global_declaration()
{
	// global_declaration ::= enum_decl | variable_decl | function_decl
	//
	// enum_decl ::= 'enum' [id] '{' id ['=' 'num'] {',' id ['=' 'num'} '}'
	//
	// variable_decl ::= type {'*'} id { ',' {'*'} id } ';'
	//
	// function_decl ::= type {'*'} id '(' parameter_decl ')' '{' body_decl '}'
	int type; // tmp, actual type for variable
	int i; // tmp
	basetype = INT;
	// parse enum, this should be treated alone.
	if (token == Enum) {
		// enum [id] { a = 10, b = 20, ... }
		match(Enum);
		if (token != '{') {
			match(Id); // skip the [id] part
		}
		if (token == '{') {
			// parse the assign part
			match('{');
			enum_declaration();
			match('}');
		}
		match(';');
		return;
	}
	// parse type information
	if (token == Int) {
		match(Int);
	}
	else if (token == Char) {
		match(Char);
		basetype = CHAR;
	}
	// parse the comma seperated variable declaration.
	while (token != ';' && token != '}')
	{
		type = basetype;
		// parse pointer type, note that there may exist `int ****x;`
		while (token == Mul) {
			match(Mul);
			type = type + PTR;
		}
		if (token != Id) {
			// invalid declaration
			printf("ERROR:line %d: bad global declaration\n", line);
			exit(-1);
		}
		if (current_id[Class]) {
			// identifier exists
			printf("ERROR:line %d: duplicate global declaration\n", line);
			exit(-1);
		}
		match(Id);
		current_id[Type] = type;
		//----analyse function
		if (token == '(') {
			current_id[Class] = Fun;
			current_id[Value] = (int)(text + 1); // the memory address of function
			function_declaration();
		}
		else {
			//--variable declaration
			current_id[Class] = Glo; // global variable
			current_id[Value] = (int)data; // assign memory address
			data = data + sizeof(int);
		}
		if (token == ',') {
			match(',');
		}
	}
	next();
}

void enum_declaration() {
	// parse enum [id] { a = 1, b = 3, ...}
	int i;
	i = 0;
	while (token != '}') {
		if (token != Id) {
			printf("ERROR:line %d: bad enum identifier %d\n", line, token);
			exit(-1);
		}
		next();
		if (token == Assign) {
			// like {a=10}
			next();
			if (token != Num) {
				printf("ERROR:line %d: bad enum initializer\n", line);
				exit(-1);
			}
			i = token_val;
			next();
		}
		current_id[Class] = Num;
		current_id[Type] = INT;
		current_id[Value] = i++;
		if (token == ',') {
			next();
		}
	}
}
void function_declaration() {
	// type func_name (...) {...}
	//               | this part
	match('(');
	function_parameter();
	match(')');
	match('{');
	function_body();
	current_id = symbols;
	while (current_id[Token]) {
		if (current_id[Class] == Loc) {
			current_id[Class] = current_id[BClass];
			current_id[Type] = current_id[BType];
			current_id[Value] = current_id[BValue];
		}
		current_id = current_id + IdSize;
	}
}

int index_of_bp; // index of bp pointer on stack
void function_parameter() {
	int type;
	int params;
	params = 0;
	while (token != ')') {
		// int name, ...
		type = INT;
		if (token == Int) {
			match(Int);
		}
		else if (token == Char) {
			type = CHAR;
			match(Char);
		}
		// pointer type
		while (token == Mul) {
			match(Mul);
			type = type + PTR;
		}
		// parameter name
		if (token != Id) {
			printf("%d: bad parameter declaration\n", line);
			exit(-1);
		}
		if (current_id[Class] == Loc) {
			printf("%d: duplicate parameter declaration\n", line);
			exit(-1);
		}
		match(Id);
		// store the local variable
		current_id[BClass] = current_id[Class]; current_id[Class] = Loc;
		current_id[BType] = current_id[Type];  current_id[Type] = type;
		current_id[BValue] = current_id[Value]; current_id[Value] = params++;   // index of current parameter
		if (token == ',') {
			match(',');
		}
	}
	index_of_bp = params + 1;
}

void function_body() {
	// type func_name (...) {...}
	//                   -->|   |<--
	// ... {
	// 1. local declarations
	// 2. statements
	// }
	int pos_local; // position of local variables on the stack.
	int type;
	pos_local = index_of_bp;
	// ¢Ù
	while (token == Int || token == Char) {
		// local variable declaration, just like global ones.
		basetype = (token == Int) ? INT : CHAR;//judge int or char
		match(token);
		while (token != ';') {
			type = basetype;
			while (token == Mul) {
				match(Mul);
				type = type + PTR;
			}
			if (token != Id) {
				// invalid declaration
				printf("ERROR:line%d: bad local declaration\n", line);
				exit(-1);
			}
			if (current_id[Class] == Loc) {
				// identifier exists
				printf("ERROR:line %d: duplicate local declaration\n", line);
				exit(-1);
			}
			match(Id);
			// store the local variable
			current_id[BClass] = current_id[Class]; current_id[Class] = Loc;
			current_id[BType] = current_id[Type];  current_id[Type] = type;
			current_id[BValue] = current_id[Value]; current_id[Value] = ++pos_local;   // index of current parameter
			if (token == ',') {
				match(',');
			}
		}
		match(';');
	}
	// save the stack size for local variables
	*++text = ENT;
	*++text = pos_local - index_of_bp;
	// statements
	while (token != '}') {
		statement();
	}
	// emit code for leaving the sub function
	*++text = LEV;
}

//if,while,return function declaration
void statement() {
	int *a, *b; // bess for branch control
	PRINTF("INFO:Start judge 'if while'circle\n");
	if (token == If)
	{
		match(If);
		match('(');
		expression(Assign);//parse 'if' condition 
		match(')');
		*++text = JZ;
		b = ++text;
		statement();
		if (token == Else)
		{
			match(Else);
			*b = (int)(text + 3);
			*++text = JMP;
			b = ++text;
			statement();
		}
		*b = (int)(text + 1);
	}
	else if (token == While) {
		//
		// a:                     a:
		//    while (<cond>)        <cond>
		//                          JZ b
		//     <statement>          <statement>
		//                          JMP a
		// b:                     b:
		match(While);

		a = text + 1;

		match('(');
		expression(Assign);
		match(')');

		*++text = JZ;
		b = ++text;

		statement();

		*++text = JMP;
		*++text = (int)a;
		*b = (int)(text + 1);
	}
	else if (token == '{') {
		// { <statement> ... }
		match('{');

		while (token != '}') {
			statement();
		}

		match('}');
	}
	else if (token == Return) {
		// return [expression];
		match(Return);

		if (token != ';') {
			expression(Assign);
		}

		match(';');

		// emit code for return
		*++text = LEV;
	}
	else if (token == ';') {
		// empty statement
		match(';');
	}
	else {
		// a = b; or function_call();
		expression(Assign);
		match(';');
	}
}


//Run function,compiler function
void expression(int level) {
	int *id;
	int tmp;
	int *addr;
	if (!token) {
		printf("ERROR:line %d: unexpected token EOF of expression\n", line);
#if SIMULATOR
		exit(-1);
#else   
		PROCESS_EXIT();
#endif        
	}
	if (token == Num)
	{
		match(Num);
		// emit code
		*++text = IMM;
		*++text = token_val;
		expr_type = INT;
	}
	/*
	char *p;
	p = "first line"
	"second line";
	*/
	else if (token == '"')//char value judge
	{
		// emit code
		*++text = IMM;
		*++text = token_val;
		match('"');
		// store the rest strings
		while (token == '"') {
			match('"');//if "",ignored
		}
		// append the end of string character '\0', all the data are default
		// to 0, so just move data one position forward.
		data = (char *)(((int)data + sizeof(int)) & (-sizeof(int)));
		expr_type = PTR;
	}
	else if (token == Sizeof)//only support int,char and pointer,return int value
	{
		// sizeof is actually an unary operator
		// now only `sizeof(int)`, `sizeof(char)` and `sizeof(*...)` are
		// supported.
		match(Sizeof);
		match('(');
		expr_type = INT;
		if (token == Int) {
			match(Int);
		}
		else if (token == Char) {
			match(Char);
			expr_type = CHAR;
		}
		while (token == Mul) {
			match(Mul);
			expr_type = expr_type + PTR;
		}
		match(')');
		// emit code
		*++text = IMM;
		*++text = (expr_type == CHAR) ? sizeof(char) : sizeof(int);//type judge
		expr_type = INT;
	}
	else if (token == Id)//Identifier
	{
		// 1. make function call
		// 2. get Enum variable
		// 3. get global/local variable
		match(Id);
		id = current_id;
		if (token == '(') {
			// function call
			match('(');
			// enter stack in turn ,standard c is reversed
			// pass in arguments
			tmp = 0; // number of arguments
			while (token != ')') {
				expression(Assign);
				*++text = PUSH;
				tmp++;
				if (token == ',') {
					match(',');
				}
			}
			match(')');
			// judge system inside function or normal function
			// emit code
			if (id[Class] == Sys) {//system funciton,include malloc,printf,and so on
								   // system functions
				*++text = id[Value];
			}
			else if (id[Class] == Fun) {
				// function call
				*++text = CALL;
				*++text = id[Value];
			}
			else {
				printf("%d: bad function call\n", line);
				exit(-1);
			}
			// clean the stack for arguments,direct change value of stack pointer 
			if (tmp > 0) {
				*++text = ADJ;
				*++text = tmp;
			}
			expr_type = id[Type];
		}
		else if (id[Class] == Num) {
			// if enum variable,save to ax
			*++text = IMM;
			*++text = id[Value];
			expr_type = INT;
		}
		else
		{
			// global or local variable
			if (id[Class] == Loc) {
				*++text = LEA;
				*++text = index_of_bp - id[Value];
			}
			else if (id[Class] == Glo) {
				*++text = IMM;
				*++text = id[Value];
			}
			else {
				printf("%d: undefined variable\n", line);
				exit(-1);
			}
			/*whether global or local variables are ultimately loaded with the
			LC or LI depending on their type*/
			// emit code, default behaviour is to load the value of the
			// address which is stored in `ax`
			expr_type = id[Type];
			*++text = (expr_type == Char) ? LC : LI;
			//according to back of identifier,will delete or replace LC/LI
		}
	}
	else if (token == '(') //(* type):froced to turn to other type
	{
		// cast or parenthesis
		match('(');
		if (token == Int || token == Char) {
			tmp = (token == Char) ? CHAR : INT; // cast type
			match(token);
			while (token == Mul) {//means pointer type in here
				match(Mul);
				tmp = tmp + PTR;
			}
			match(')');
			expression(Inc); // cast has precedence as Inc(++)
			expr_type = tmp;
		}
		else {
			// normal parenthesis
			expression(Assign);
			match(')');
		}
	}
	else if (token == Mul)//*p:get value from pointer
	{
		// dereference *<addr>
		match(Mul);
		expression(Inc); // dereference has the same precedence as Inc(++)
		if (expr_type >= PTR) {
			expr_type = expr_type - PTR;
		}
		else {
			printf("%d: bad dereference\n", line);
			exit(-1);
		}
		*++text = (expr_type == CHAR) ? LC : LI;
	}
	else if (token == And)//&:get address
	{
		// get the address of
		match(And);
		expression(Inc); // get the address of
		if (*text == LC || *text == LI) {
			text--;
		}
		else {
			printf("%d: bad address of\n", line);
			exit(-1);
		}
		expr_type = expr_type + PTR;
	}
	else if (token == '!')//!bool:logic reverse
	{
		// not or reverse
		match('!');
		expression(Inc);
		// emit code, use <expr> == 0
		*++text = PUSH;
		*++text = IMM;
		*++text = 0;
		*++text = EQ;
		expr_type = INT;
	}
	else if (token == '~') //~val:bitwise reverse
	{
		// bitwise not
		match('~');
		expression(Inc);
		// emit code, use <expr> XOR -1
		*++text = PUSH;
		*++text = IMM;
		*++text = -1;
		*++text = XOR;
		expr_type = INT;
	}
	else if (token == Add)//positive number
	{
		// +var, do nothing
		match(Add);
		expression(Inc);
		expr_type = INT;
	}
	else if (token == Sub)//negative number
	{
		// -var
		match(Sub);//next
		if (token == Num) {
			*++text = IMM;
			*++text = -token_val;
			match(Num);
		}
		else {
			*++text = IMM;
			*++text = -1;
			*++text = PUSH;
			expression(Inc);
			*++text = MUL;
		}
		expr_type = INT;
	}
	else if (token == Inc || token == Dec)
	{
		tmp = token;
		match(token);
		expression(Inc);
		//the address used twice,save it
		if (*text == LC) {
			*text = PUSH;  // to duplicate the address 
			*++text = LC;
		}
		else if (*text == LI) {
			*text = PUSH;
			*++text = LI;
		}
		else {
			printf("%d: bad lvalue of pre-increment\n", line);
			exit(-1);
		}
		*++text = PUSH;
		*++text = IMM;
		// ¢Ú
		*++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
		*++text = (tmp == Inc) ? ADD : SUB;
		*++text = (expr_type == CHAR) ? SC : SI;
	}
	else
	{
		printf("%d: bad expression\n", line);
		exit(-1);
	}
	/// include  |, ^, &, ==, != <=, >=, <, >, <<, >>, +, -, *, /, %
	while (token >= level)
	{
		// handle according to current operator's precedence
		tmp = expr_type;
		if (token == Assign)// var = expr;
		{
			match(Assign);
			if (*text == LC || *text == LI) {
				*text = PUSH; // save the lvalue's pointer
			}
			else {
				printf("ERROR:line:%d: expression:bad lvalue in assignment\n", line);
				exit(-1);
			}
			expression(Assign);
			expr_type = tmp;
			*++text = (expr_type == CHAR) ? SC : SI;
		}
		else if (token == Cond) // expr ? a : b;
		{
			match(Cond);
			*++text = JZ;
			addr = ++text;
			expression(Assign);
			if (token == ':') {
				match(':');
			}
			else {
				printf("%d: missing colon in conditional\n", line);
				exit(-1);
			}
			*addr = (int)(text + 3);
			*++text = JMP;
			addr = ++text;
			expression(Cond);
			*addr = (int)(text + 1);
		}
		else if (token == Lor)// logic or
		{
			match(Lor);
			*++text = JNZ;
			addr = ++text;
			expression(Lan);
			*addr = (int)(text + 1);
			expr_type = INT;
		}
		else if (token == Lan)// logic and
		{
			match(Lan);
			*++text = JZ;
			addr = ++text;
			expression(Or);
			*addr = (int)(text + 1);
			expr_type = INT;
		}
		else if (token == Or)
		{
			// bitwise or
			match(Or);
			*++text = PUSH;
			expression(Xor);
			*++text = OR;
			expr_type = INT;
		}
		else if (token == Xor)
		{
			// bitwise xor
			match(Xor);
			*++text = PUSH;
			expression(And);
			*++text = XOR;
			expr_type = INT;
		}

		else if (token == And) {
			// bitwise and
			match(And);
			*++text = PUSH;
			expression(Eq);
			*++text = AND;
			expr_type = INT;
		}
		else if (token == Eq) {
			// equal ==
			match(Eq);
			*++text = PUSH;
			expression(Ne);
			*++text = EQ;
			expr_type = INT;
		}
		else if (token == Ne) {
			// not equal !=
			match(Ne);
			*++text = PUSH;
			expression(Lt);
			*++text = NE;
			expr_type = INT;
		}
		else if (token == Lt) {
			// less than
			match(Lt);
			*++text = PUSH;
			expression(Shl);
			*++text = LT;
			expr_type = INT;
		}
		else if (token == Gt) {
			// greater than
			match(Gt);
			*++text = PUSH;
			expression(Shl);
			*++text = GT;
			expr_type = INT;
		}
		else if (token == Le) {
			// less than or equal to
			match(Le);
			*++text = PUSH;
			expression(Shl);
			*++text = LE;
			expr_type = INT;
		}
		else if (token == Ge) {
			// greater than or equal to
			match(Ge);
			*++text = PUSH;
			expression(Shl);
			*++text = GE;
			expr_type = INT;
		}
		else if (token == Shl)//<<
		{
			// shift left
			match(Shl);
			*++text = PUSH;
			expression(Add);
			*++text = SHL;
			expr_type = INT;
		}
		else if (token == Shr)//>>
		{
			// shift right
			match(Shr);
			*++text = PUSH;
			expression(Add);
			*++text = SHR;
			expr_type = INT;
		}
		else if (token == Add)
		{
			// add
			match(Add);
			*++text = PUSH;
			expression(Mul);

			expr_type = tmp;
			if (expr_type > PTR) {// pointer add, and not `char *`
				*++text = PUSH;
				*++text = IMM;
				*++text = sizeof(int);
				*++text = MUL;
			}
			*++text = ADD;
		}
		else if (token == Sub)
		{
			// sub
			match(Sub);
			*++text = PUSH;
			expression(Mul);
			if (tmp > PTR && tmp == expr_type)  // pointer subtraction
			{
				*++text = SUB;
				*++text = PUSH;
				*++text = IMM;
				*++text = sizeof(int);
				*++text = DIV;
				expr_type = INT;
			}
			else if (tmp > PTR)// pointer movement
			{
				*++text = PUSH;
				*++text = IMM;
				*++text = sizeof(int);
				*++text = MUL;
				*++text = SUB;
				expr_type = tmp;
			}
			else
			{
				// numeral subtraction
				*++text = SUB;
				expr_type = tmp;
			}
		}
		else if (token == Mul)
		{
			// multiply
			match(Mul);
			*++text = PUSH;
			expression(Inc);
			*++text = MUL;
			expr_type = tmp;
		}
		else if (token == Div)
		{
			// divide
			match(Div);
			*++text = PUSH;
			expression(Inc);
			*++text = DIV;
			expr_type = tmp;
		}
		else if (token == Mod)
		{
			// Modulo
			match(Mod);
			*++text = PUSH;
			expression(Inc);
			*++text = MOD;
			expr_type = tmp;
		}
		else if (token == Inc || token == Dec)
		{
			// postfix inc(++) and dec(--)
			// we will increase the value to the variable and decrease
			// it on `ax` to get its original value.
			if (*text == LI) {
				*text = PUSH;
				*++text = LI;
			}
			else if (*text == LC) {
				*text = PUSH;
				*++text = LC;
			}
			else {
				printf("%d: bad value in increment\n", line);
				exit(-1);
			}

			*++text = PUSH;
			*++text = IMM;
			*++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
			*++text = (token == Inc) ? ADD : SUB;
			*++text = (expr_type == CHAR) ? SC : SI;
			*++text = PUSH;
			*++text = IMM;
			*++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
			*++text = (token == Inc) ? SUB : ADD;
			match(token);
		}
		else if (token == Brak) // var[xx]:array access 
		{
			match(Brak);
			*++text = PUSH;
			expression(Assign);
			match(']');

			if (tmp > PTR) {
				// pointer, `not char *`
				*++text = PUSH;
				*++text = IMM;
				*++text = sizeof(int);
				*++text = MUL;
			}
			else if (tmp < PTR) {
				printf("%d: pointer type expected\n", line);
				exit(-1);
			}
			expr_type = tmp - PTR;
			*++text = ADD;
			*++text = (expr_type == CHAR) ? LC : LI;
		}
		else
		{
			printf("ERROR:%d: compiler error, token = %d\n", line, token);
			exit(-1);
		}
	}
}


