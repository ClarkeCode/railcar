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
#endif