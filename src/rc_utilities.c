#include "rc_utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h> //For variadic functions
#include <string.h>

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
	assert(NUM_TOKEN_TYPE == 30 && "Unhandled Token");

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
		case LOOP_DYNAMIC_AMOUNT: return "LOOP_DYNAMIC_AMOUNT"; break;

		case INSERT_DATA_ITEM: return "INSERT_DATA_ITEM"; break;

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

void generate_subgraph_names(int tk_id, char* sg_name, char* sg_true, char* sg_false) {
	if (sg_name)  sprintf(sg_name,  "conditional_%d", tk_id);
	if (sg_true)  sprintf(sg_true,  "cluster_%d_T", tk_id);
	if (sg_false) sprintf(sg_false, "cluster_%d_F", tk_id);
}

void dump_token_node(FILE* fp, Token* current) {
	//BEGIN LABEL
	fprintf(fp, "%d [label=\"(%d) %s", current->id, current->id, human(current->type));
	if (current->str_value) { 
		fprintf(fp, " '%s'", current->str_value);
	}
	if (current->type == CHECK_ABILITY_TO_MOVE || current->type == REPEAT_MOVE_MAX || current->type == REPEAT_MOVE) {
		fprintf(fp, " - %s", human((current+1)->type));
	}
	//Don't display the value of 0 except for the write operation
	if (current->value != 0 || current->type == HEAD_WRITE) { fprintf(fp, ": %d", current->value); }
	fprintf(fp, "\"");
	//END LABEL

	if (current->type == HEAD_READ || current->type == CHECK_ABILITY_TO_MOVE) {
		fprintf(fp, "shape=diamond");
	}
	fprintf(fp, "]\n");
}
//Subgraph_name and edge_colour are optional, pass NULL to ignore
void dump_token_edges(FILE* fp, Token* current) {
	const bool showConditionals = flags.graphviz_conditionals;
	const bool showPairedTokens = flags.graphviz_pairs;
	const bool showPrefixedTokens = flags.graphviz_prefixed;
	if (current->next_unconditional)
		fprintf(fp, "%d -> %d\n", current->id, current->next_unconditional->id);
	
	// if (showConditionals && current->conditional) {
	// 	fprintf(fp, "%d -> %d [label=\"EndTrue\"", current->id, current->conditional->end_true->id);
	// 	fprintf(fp, "%d -> %d [label=\"EndFalse\"", current->id, current->conditional->end_false->id);
	// }
	if (showPairedTokens && current->pair && current == current->pair->senior) {
		fprintf(fp, "%d -> %d [color=\"green\" dir=\"both\"]\n", current->id, current->pair->junior->id);
	}
	if (showPrefixedTokens && current->prefix_member && current == current->prefix_member->senior) {
		fprintf(fp, "%d -> %d [color=\"orange\"]\n", current->prefix_member->senior->id, current->prefix_member->junior->id);
	}

	if (current->conditional) { //opens a conditional branch
		char sg_t_name[128] = "";
		char sg_f_name[128] = "";
		generate_subgraph_names(current->id, NULL, sg_t_name, sg_f_name);

		if (current->next_if_false) {
			fprintf(fp, "%d -> %d [label=\"False\" lhead=%s color=%s]\n", current->id, current->next_if_false->id, sg_f_name, "red");
		}
		if (current->next_if_true) {
			fprintf(fp, "%d -> %d [label=\"True\" lhead=%s color=%s]\n", current->id, current->next_if_true->id, sg_t_name, "green");
		}
	}
	else {
		if (current->next_if_true)
			fprintf(fp, "%d -> %d [label=\"True\"]\n", current->id, current->next_if_true->id);
		if (current->next_if_false)
			fprintf(fp, "%d -> %d [label=\"False\"]\n", current->id, current->next_if_false->id);
	}
}


Token* dump_token_until_stopper(FILE* fp, Token* current, Token* stopper);//Forward declaration



Token* dump_conditional(FILE* fp, Token* current) {
	char sg_name[128] = "";
	char sg_t_name[128] = "";
	char sg_f_name[128] = "";
	
	generate_subgraph_names(current->id, sg_name, sg_t_name, sg_f_name);


	fprintf(fp, "subgraph %s {\n", sg_name);
	//list tokens until end or
	//if token that creates subgraph
	dump_token_node(fp, current);
	{
		fprintf(fp, "\tsubgraph %s {\ncolor=red;\nlabel=\"False\";\n", sg_f_name);
		dump_token_until_stopper(fp, current->next_if_false, current->conditional->end_false+1);
		fprintf(fp, "\t}\n");
	}
	{
		fprintf(fp, "\tsubgraph %s {\ncolor=green;\nlabel=\"True\";\n", sg_t_name);
		current = dump_token_until_stopper(fp, current->next_if_true, current->conditional->end_true+1);
		fprintf(fp, "\t}\n");
	}
	
	fprintf(fp, "}\n");
	return current;
}


//Returned pointer will point to the provided stopper
Token* dump_token_until_stopper(FILE* fp, Token* current, Token* stopper) {
	const bool showPrefixedTokens = flags.graphviz_prefixed;
	const bool showConditionals = flags.graphviz_conditionals;
	do {

		if (!showConditionals && (current->type == OPEN_CONDITIONAL || current->type == CLOSE_CONDITIONAL)) continue;
		if (!showPrefixedTokens && current->prefix_member && current == current->prefix_member->junior) continue;


		if (current->type == HEAD_READ) {
			current = dump_conditional(fp, current)-1;
		}
		else {
			dump_token_node(fp, current);
		}
		
	}
	while(++current != stopper);
	return current;
}

void dump_tokens_to_dotfile(FILE* fp, Token* tkArr, size_t num_tk) {
	const bool showConditionals = flags.graphviz_conditionals;
	const bool showPairedTokens = flags.graphviz_pairs;
	const bool showPrefixedTokens = flags.graphviz_prefixed;
	fprintf(fp, "digraph {\n");
	fprintf(fp, "compound=true;\n");
	// fprintf(fp, "\tlayout=neato;\n");
	Token* current = tkArr;
	Token* stopper = tkArr+num_tk;

	dump_token_until_stopper(fp, current, stopper);

	do {
		if (!showConditionals && (current->type == OPEN_CONDITIONAL || current->type == CLOSE_CONDITIONAL)) continue;
		if (!showPrefixedTokens && current->prefix_member && current == current->prefix_member->junior) continue;
		dump_token_edges(fp, current);
	} while (++current != stopper);

	fprintf(fp, "}\n");
}











#define RC_DATASTRUCTURE_DEFAULT_CAPACITY 16
typedef unsigned char BYTE_;

void stackPush(Stack* stack_p, void* newElement) {
	if (stack_p->size == stack_p->_capacity-1) {
		stack_p->_capacity *= 2;
		stack_p->_data = realloc(stack_p->_data, stack_p->_capacity * 8);
	}

	void* newPlace = (BYTE_*)stack_p->_data + stack_p->size;
	memcpy(newPlace, newElement, stack_p->_element_size);
	stack_p->size++;
}
void* stackTop(Stack* stack_p) {
	if (stack_p->size == 0) return NULL;
	return (BYTE_*)stack_p->_data + (stack_p->size - 1);
}
void* stackPop(Stack* stack_p) {
	void* oldTop = stackTop(stack_p);
	stack_p->size--;
	return oldTop;
}

//Public
Stack* initializeStack(size_t elementSize) {
	Stack* stack = calloc(1, elementSize);
	stack->_capacity = RC_DATASTRUCTURE_DEFAULT_CAPACITY;
	stack->_data = calloc(RC_DATASTRUCTURE_DEFAULT_CAPACITY, elementSize);
	stack->_element_size = elementSize;

	stack->push  = &stackPush;
	stack->top   = &stackTop;
	stack->pop   = &stackPop;

	return stack;
}
void freeStack(Stack* stack_p) {
	if (stack_p->_data) free(stack_p->_data);
	if (stack_p)        free(stack_p);
}
