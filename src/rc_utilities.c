#include "rc_utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
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


char* human(TOKEN_TYPE ttype) {
	assert(NUM_TOKEN_TYPE == 28 && "Unhandled Token");

	switch (ttype) {
		case NO_OPERATION: return "NOP"; break;

		case HEAD_LEFT: return "LEFT"; break;
		case HEAD_RIGHT: return "RIGHT"; break;
		case HEAD_UP: return "UP"; break;
		case HEAD_DOWN: return "DOWN"; break;
		case REPEAT_MOVE: return "REPEAT_MOVE"; break;
		case REPEAT_MOVE_MAX: return "REPEAT_MOVE_MAX"; break;
		case RELATIVE_MOVE: return "RELATIVE_MOVE"; break;
		case CHECK_ABILITY_TO_MOVE: return "CHECK_ABILITY_TO_MOVE"; break;

		case HEAD_READ: return "H_READ"; break;
		case HEAD_WRITE: return "H_WRITE"; break;

		case NUMBER: return "NUMBER"; break;
		case STRING: return "STRING"; break;

		case OPEN_CONDITIONAL: return "OPEN_CONDITIONAL"; break;
		case CLOSE_CONDITIONAL: return "CLOSE_CONDITIONAL"; break;
		case OPEN_BLOCK: return "OPEN_BLOCK"; break;
		case CLOSE_BLOCK: return "CLOSE_BLOCK"; break;

		case GOTO_BLOCK_START: return "GOTO_BLOCK_START"; break;
		case GOTO_BLOCK_END: return "GOTO_BLOCK_END"; break;

		case LOOP_UNTIL_END: return "LOOP_E"; break;
		case LOOP_UNTIL_BEGINNING: return "LOOP_B"; break;
		case LOOP_FIXED_AMOUNT: return "LOOP_FIXED_AMOUNT"; break;

		case STAKE_FLAG: return "STAKE_FLAG"; break;
		case RETURN_FLAG: return "RETURN_FLAG"; break;

		case PRINT_BYTE_AS_CHAR: return "PRINT_BYTE_AS_CHAR"; break;
		case PRINT_BYTE_AS_NUM: return "PRINT_BYTE_AS_NUM"; break;
		case END_OF_PROGRAM: return "END_OF_PROGRAM"; break;
		case UNKNOWN: return "UNKNOWN"; break;
	};
	return "unreachable";
}

bool type_in(TOKEN_TYPE t, TOKEN_TYPE* desiredTypes, size_t sz) {
	for (size_t x = 0; x < sz; x++) {
		if (*(desiredTypes+x) == t)
			return true;
	}
	return false;
}




extern Flags flags;
void dump_tokens_to_dotfile(FILE* fp, Token* tkArr, size_t num_tk) {
	const bool showConditionals = flags.graphviz_conditionals;
	const bool showPairedTokens = flags.graphviz_pairs;
	const bool showPrefixedTokens = flags.graphviz_prefixed;
	fprintf(fp, "digraph {\n");
	fprintf(fp, "compound=true;\n");
	// fprintf(fp, "\tlayout=neato;\n");
	Token* current;

	for (size_t x = 0, block_num = 0; x<num_tk; x++) {
		current = tkArr+x;
		if (current->type == OPEN_BLOCK) {
			fprintf(fp, "subgraph cluster_%lu{\ncolor=blue;\nlabel=\"Block #%lu\";\n", block_num, block_num);
			for(Token* sub = current; sub <= current->pair->junior; sub++) {
				fprintf(fp, "%lu ", sub->id);
			}
			fprintf(fp, ";\n}\n");
			block_num++;
		}

	}

	for (size_t x = 0; x<num_tk; x++) {
		current = tkArr+x;

		if (!showConditionals && (current->type == OPEN_CONDITIONAL || current->type == CLOSE_CONDITIONAL)) continue;
		if (!showPrefixedTokens && current->prefix_member && current == current->prefix_member->junior) continue;

		fprintf(fp, "%d [label=\"(%d) %s", current->id, current->id, human(current->type));
		if (current->str_value) {
			fprintf(fp, " '%s'", current->str_value);
		}
		if (current->type == CHECK_ABILITY_TO_MOVE || current->type == REPEAT_MOVE_MAX || current->type == REPEAT_MOVE) {
			fprintf(fp, " - %s", human((current+1)->type));
		}

		//Don't display the value of 0 except for the write operation
		if (current->value != 0 || current->type == HEAD_WRITE) { fprintf(fp, ": %d", current->value); }
		fprintf(fp, "\"]\n");
		if (current->next_unconditional)
			fprintf(fp, "%d -> %d\n", current->id, current->next_unconditional->id);
		if (current->next_if_true)
			fprintf(fp, "%d -> %d [label=\"True\"]\n", current->id, current->next_if_true->id);
		if (current->next_if_false)
			fprintf(fp, "%d -> %d [label=\"False\"]\n", current->id, current->next_if_false->id);
		if (showConditionals && current->conditional) {
			fprintf(fp, "%d -> %d [label=\"EndTrue\"]\n", current->id, current->conditional->end_true->id);
			fprintf(fp, "%d -> %d [label=\"EndFalse\"]\n", current->id, current->conditional->end_false->id);
		}
		if (showPairedTokens && current->pair && current == current->pair->senior) {
			fprintf(fp, "%d -> %d [color=\"green\" dir=\"both\"]\n", current->id, current->pair->junior->id);
		}
		if (showPrefixedTokens && current->prefix_member && current == current->prefix_member->senior) {
			fprintf(fp, "%d -> %d [color=\"orange\"]\n", current->prefix_member->senior->id, current->prefix_member->junior->id);
		}
	}
	fprintf(fp, "}\n");
}