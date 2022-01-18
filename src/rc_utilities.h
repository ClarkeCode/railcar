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

// void dump_tokens_to_dotfile(FILE* fp, Token* tkArr, size_t num_tk);
void dump_ast_to_dotfile(FILE* fp, Instruction* instruction_tree);

bool tk_of_type(Token* tk, TOKEN_TYPE tk_t);
bool tk_in_type_arr(Token* tk, TOKEN_TYPE* tk_tarr, size_t sz);
//Note: this macro only works for arrays with a fixed size at compile-time
#define ARR_SZ(arr) sizeof(arr)/sizeof(arr[0])

#endif