#include "rc_utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> //For variadic functions

//Note: prefix is optional, pass NULL to ignore
void reportError(Location* errorLoc, const char* prefix, const char* fmessage, ...) {
	fprintf(stderr, "%s:%lu:%lu ", errorLoc->file, errorLoc->line, errorLoc->character);
	if (prefix) fprintf(stderr, "[%s] ", prefix);
	fprintf(stderr, "ERROR: ");


	va_list args;
	va_start (args, fmessage);
	vfprintf (stderr, fmessage, args);
	va_end (args);

	exit(EXIT_FAILURE);
}


void shellEcho(const char* command) {
	fprintf(stdout, "Running: %s\n", command);
	int retcode = system(command);
	fprintf(stdout, "%s %s\n", retcode==EXIT_SUCCESS ? "Success:" : "Failure", command);
}