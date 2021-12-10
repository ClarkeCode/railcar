#ifndef RAILCAR_UTILITIES
#define RAILCAR_UTILITIES
#include "railcar.h"
#include <stdbool.h>
#include <stdio.h>

//Note: prefix is optional, pass NULL to ignore
void reportError(Location* errorLoc, const char* prefix, const char* fmessage, ...);

void shellEcho(const char* command);

char* human(TOKEN_TYPE ttype);
bool type_in(TOKEN_TYPE t, TOKEN_TYPE* desiredTypes, size_t sz);

void dump_tokens_to_dotfile(FILE* fp, Token* tkArr, size_t num_tk);



//START: Stack data structure
#define SK_UNPACK(sk_ele_t) *(sk_ele_t*) 
typedef struct _STK{
	size_t size;

	void (*push)(struct _STK*, void*);
	void* (*top)(struct _STK*);
	void* (*pop)(struct _STK*);

	//Size in bytes of elements contained by the stack
	size_t _element_size;
	size_t _capacity;
	void* _data;

} Stack;

Stack* initializeStack(size_t elementSize);
void freeStack(Stack* stack_p);
//END: Stack data structure


#endif