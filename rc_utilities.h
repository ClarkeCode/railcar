#ifndef RAILCAR_UTILITIES
#define RAILCAR_UTILITIES
#include "railcar.h"

//Note: prefix is optional, pass NULL to ignore
void reportError(Location* errorLoc, const char* prefix, const char* fmessage, ...);

void shellEcho(const char* command);
#endif