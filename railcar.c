#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "railcar.h"
#include <string.h>

typedef struct {
	bool step;
	bool use_ansi;
	bool no_colour;
	bool graphviz;
} Flags;
Flags flags = {0};

//Pass NULL as an argument to num_tokens if you do not want it to be incremented
//Pass 0 as an argument to val if the token does not requre a value
void create_token(size_t* num_tokens, Token* tk, TOKEN_TYPE tk_t, int val) {
	if (tk) {
		tk->id = *num_tokens;
		tk->type = tk_t;
		if (val != 0) tk->value = val;
	}
	if (num_tokens)
		(*num_tokens)++;
}

char procureNextChar(FILE* fp, size_t* line_tracker, size_t* char_tracker) {
	char c = fgetc(fp);
	(*char_tracker)++;
	if (c == '\n') {
		(*line_tracker)++;
		(*char_tracker) = -1;
	}
	return c;
}
char fpeek(FILE* stream) {
	char temp = fgetc(stream);
	ungetc(temp, stream);
	return temp;
}
Token* Railcar_Lexer(char* fileName, size_t* tokenNum) {
	FILE* lexf = fopen(fileName, "r");
	if (!lexf) return NULL;

	printf("Opened %s\n", fileName);

	Token* tokens = calloc(128, sizeof(Token));
	size_t num_tokens = 0;
	size_t num_lines = 0;
	size_t num_chars = -1;

	char c;
	while ((c = procureNextChar(lexf, &num_lines, &num_chars)) != EOF) {
		assert(NUM_TOKEN_TYPE == 24 && "Unhandled Token");
		Token* tk = tokens+num_tokens;
		tk->loc.file = fileName;
		tk->loc.line = num_lines;
		tk->loc.character = num_chars;

		//Comment
		if (c == '/') {
			do { c = procureNextChar(lexf, &num_lines, &num_chars); if (c=='\n') break; } while (c != EOF);
			if (c == EOF) break;
		}
		if (c == '\n' || c == ' ' || c == '\t') continue;
		if (c >= '2' && c <= '9') {
			if (fpeek(lexf) == ']') create_token(&num_tokens, tk, LOOP_FIXED_AMOUNT, atoi(&c));
			else create_token(&num_tokens, tk, REPEAT_MOVE, atoi(&c));
			continue;
		}
		
		switch (c) {
			case '-': create_token(&num_tokens, tk, NO_OPERATION, 0);          break;
			case '>': create_token(&num_tokens, tk, HEAD_RIGHT, 0);            break;
			case '<': create_token(&num_tokens, tk, HEAD_LEFT, 0);             break;
			case '^': create_token(&num_tokens, tk, HEAD_UP, 0);               break;
			case 'v': create_token(&num_tokens, tk, HEAD_DOWN, 0);             break;
			case '?': create_token(&num_tokens, tk, CHECK_ABILITY_TO_MOVE, 0); break;

			case 'r': create_token(&num_tokens, tk, HEAD_READ, 0);             break;
			case 'w':
				if ((c = procureNextChar(lexf, &num_lines, &num_chars)) == EOF) return NULL;
				if (!(c == '0' || c == '1')) return NULL;
				create_token(&num_tokens, tk, HEAD_WRITE, c == '0' ? 0 : 1);
				break;

			case '(': create_token(&num_tokens, tk, OPEN_CONDITIONAL, 0);      break;
			case ')': create_token(&num_tokens, tk, CLOSE_CONDITIONAL, 0);     break;
			case '[': create_token(&num_tokens, tk, OPEN_BLOCK, 0);            break;
			case ']': create_token(&num_tokens, tk, CLOSE_BLOCK, 0);           break;
			case '#': create_token(&num_tokens, tk, GOTO_BLOCK_START, 0);      break;
			case '$': create_token(&num_tokens, tk, GOTO_BLOCK_END, 0);        break;
			case 'e': create_token(&num_tokens, tk, LOOP_UNTIL_END, 0);        break;
			case 'b': create_token(&num_tokens, tk, LOOP_UNTIL_BEGINNING, 0);  break;
			case '!': create_token(&num_tokens, tk, REPEAT_MOVE_MAX, 0);       break;
			case '&': create_token(&num_tokens, tk, STAKE_FLAG, 0);            break;
			case '@': create_token(&num_tokens, tk, RETURN_FLAG, 0);           break;

			default:  create_token(&num_tokens, tk, UNKNOWN, 0);
		}
	}
	create_token(&num_tokens, tokens+num_tokens, END_OF_PROGRAM, 0);
	*tokenNum = num_tokens;

	fclose(lexf);
	return tokens;
}

char byte_from_binary_str(char* str, bool separateNibbles) {
	char output = 0x0000;
	for (size_t x = 0; x < (separateNibbles ? 9 : 8); x++) {
		if (separateNibbles && x == 4) continue;
		output <<= 1;
		output |= (*(str+x) == '1' ? 0x0001 : 0x0000);
	}
	return output;
}

const char ANSI_RESET_STYLE[] = "\x1b[0m";

//TODO: try reworking to have an internal buffer and returning the buffer as a char*
//Note: pass NULL to ansiColourString to ignore colouring
void binary_str_from_byte(char byte, char* str, bool separateNibbles, int colouredPosition, char* ansiColourStr) {
	char* currentChar = str;
	for (int x = 0; x < 8; x++) {
		if (separateNibbles && x == 4)
			(*currentChar++) = ' ';

		if (ansiColourStr && x == colouredPosition) {
			strcat(str, ansiColourStr);
			currentChar += strlen(ansiColourStr);
		}

		if ((byte >> 7-x) & 0x1)
			(*currentChar++) = '1';
		else
			(*currentChar++) = '0';

		if (ansiColourStr && x == colouredPosition) {
			const char* reset = ANSI_RESET_STYLE;
			while (*reset != '\0') {
				(*currentChar++) = *reset++;
			}
		}
	}
}

char* human(TOKEN_TYPE ttype) {
	assert(NUM_TOKEN_TYPE == 24 && "Unhandled Token");

	switch (ttype) {
		case NO_OPERATION: return "NOP"; break;

		case HEAD_LEFT: return "LEFT"; break;
		case HEAD_RIGHT: return "RIGHT"; break;
		case HEAD_UP: return "UP"; break;
		case HEAD_DOWN: return "DOWN"; break;
		case REPEAT_MOVE: return "REPEAT_MOVE"; break;
		case REPEAT_MOVE_MAX: return "REPEAT_MOVE_MAX"; break;
		case CHECK_ABILITY_TO_MOVE: return "CHECK_ABILITY_TO_MOVE"; break;

		case HEAD_READ: return "H_READ"; break;
		case HEAD_WRITE: return "H_WRITE"; break;

		case NUMBER: return "NUMBER"; break;

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

		case END_OF_PROGRAM: return "END_OF_PROGRAM"; break;
		case UNKNOWN: return "UNKNOWN"; break;
	};
	return "unreachable";
}

bool type_in(TOKEN_TYPE t, TOKEN_TYPE* desiredTypes, size_t sz) {
	for (size_t x = 0; x < sz; x++) {
		if (*(desiredTypes+x) == t) return true;
	}
	return false;
}


//TODO: add sequence number to tokens to remove 'tkArr' argument
void dump_token(FILE* fp, Token* tk) {
	if (!tk) return;
	bool lineInfo = false;
	bool nextTk = true;
	if (lineInfo) fprintf(fp, "%s:%lu:%lu ", tk->loc.file, tk->loc.line, tk->loc.character);
	fprintf(fp, "TOKEN #%lu { %s",
		tk->id, human(tk->type));
	if (tk->type == HEAD_WRITE || tk->type == REPEAT_MOVE) 
		fprintf(fp, " - '%d'", tk->value);
	if (nextTk) {
		if (tk->next_unconditional) fprintf(fp, " NU: %lu", tk->next_unconditional->id);
		if (tk->next_if_false) fprintf(fp, " NF: %lu",tk->next_if_false->id);
		if (tk->next_if_true) fprintf(fp, " NT: %lu",tk->next_if_true->id);
	}
	fprintf(fp, "}\n");
}
void dump_tokens(FILE* fp, Token* tkArr, size_t num_tk) {
	for (size_t x = 0; x<num_tk; x++) {
		dump_token(fp, tkArr+x);
	}
}

void dump_tokens_to_dotfile(FILE* fp, Token* tkArr, size_t num_tk) {
	const bool showConditionals = false;
	const bool showPairedTokens = true;
	const bool showPrefixTokens = true;
	fprintf(fp, "digraph {\n");
	// fprintf(fp, "\tlayout=neato;\n");
	Token* current;
	for (size_t x = 0; x<num_tk; x++) {
		current = tkArr+x;

		if (!showConditionals && (current->type == OPEN_CONDITIONAL || current->type == CLOSE_CONDITIONAL)) continue;

		fprintf(fp, "%d [label=\"(%d) %s", current->id, current->id, human(current->type));
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
		if (showPairedTokens && current->paired_token && current < current->paired_token) {
			fprintf(fp, "%d -> %d [color=\"green\" dir=\"both\"]\n", current->id, current->paired_token->id);
		}
		if (showPrefixTokens && current->prefix_token) {
			fprintf(fp, "%d -> %d [color=\"orange\"]\n", current->prefix_token->id, current->id);
		}
	}
	fprintf(fp, "}\n");
}

void usage(FILE* fp) {
	fprintf(fp, "USAGE: ./railcar.exe <subcommand> filename\n");
	fprintf(fp, "\nSUBCOMMANDS:\n");
	fprintf(fp, "	step     //step through tokens one-at-a-time");
}



Token* _find_next_token_of_type(Token* current, Token* stopper, TOKEN_TYPE tk_t, bool doIncrement, bool skipBranches, bool testEquality) {
	Token* test = current;
	while (test != stopper) {
		if ((testEquality ? test->type == tk_t : test->type != tk_t)) return test;
		if (skipBranches && test->conditional) {
			test = test->conditional->end_true;
		}
		test += doIncrement ? 1 : -1;
	}
	return NULL;
}
Token* find_next_token_of_type(Token* current, Token* stopper, TOKEN_TYPE tk_t) {
	return _find_next_token_of_type(current, stopper, tk_t, true, false, true);
}

Token* rfind_next_token_of_type(Token* current, Token* stopper, TOKEN_TYPE tk_t) {
	return _find_next_token_of_type(current, stopper, tk_t, false, false, true);
}

void setup_conditional_branch(Token* current, Token* tokens, Token* stopper) {
	//TODO: can probably make an optimization for the special case of r(-)(statements)
	//      '(-)' can be recognized and change the next_if_false pointer to point to the end of the true branch
	ConditionalBranch* branch = current->conditional = malloc(sizeof(ConditionalBranch));
	branch->creator = current;
	branch->start_false = find_next_token_of_type(current, stopper, OPEN_CONDITIONAL);
	branch->end_false   = _find_next_token_of_type(branch->start_false, stopper, CLOSE_CONDITIONAL, true, true, true);
	branch->start_true  = _find_next_token_of_type(branch->end_false, stopper, OPEN_CONDITIONAL, true, true, true);
	branch->end_true    = _find_next_token_of_type(branch->start_true, stopper, CLOSE_CONDITIONAL, true, true, true);
	
	branch->start_false->branchMember = branch;
	branch->start_true->branchMember = branch;
	branch->end_false->branchMember = branch;
	branch->end_true->branchMember = branch;
	
	current->next_if_false = branch->start_false+1;
	current->next_if_true = branch->start_true+1;
}
Token* next_nonconditional_token(Token* current, Token* stopper) {
	Token* test = current+1;
	if (test == stopper) return NULL;
	while (test != stopper && test->branchMember) { test++; }
	if (test == stopper) return NULL;
	return test;
}
TOKEN_TYPE pair_type_lookup(TOKEN_TYPE actual, TOKEN_TYPE* firsts, TOKEN_TYPE* seconds, size_t sz) {
	if (!(type_in(actual, firsts, sz) || type_in(actual, seconds, sz))) return UNKNOWN;
	TOKEN_TYPE* search_area = type_in(actual, firsts, sz) ? firsts : seconds;
	for (size_t x = 0; x < sz; x++) {
		if (actual == search_area[x]) {
			return (type_in(actual, firsts, sz) ? seconds[x] : firsts[x]); 
		}
	}
	return UNKNOWN;
}
void attach_self_as_prefix(Token* prefix, Token* modifiedTk) { modifiedTk->prefix_token = prefix; }
void Railcar_Parser(Token* tokens, size_t numTokens) {
	Token* stopper = tokens + numTokens;
	Token* rstopper = tokens - 1;

	//TODO: add error reporting function and syntax checking to call

	//TODO: lying here, need to handle all existing tokens
	assert(NUM_TOKEN_TYPE == 24 && "Unhandled Token");

	TOKEN_TYPE pairedtokens[] = {OPEN_BLOCK, CLOSE_BLOCK, STAKE_FLAG, RETURN_FLAG};
	TOKEN_TYPE pairedopener[] = {OPEN_BLOCK, STAKE_FLAG};
	TOKEN_TYPE pairedcloser[] = {CLOSE_BLOCK, RETURN_FLAG};

	{
		Token* current = tokens; Token* last = stopper-1;
		for (; current < last; current++) {
			if (type_in(current->type, pairedcloser, sizeof(pairedcloser)) && !current->paired_token) {
				fprintf(stderr, "ERROR: unpaired closing token '%s' at %s:%lu:%lu",
					human(current->type), current->loc.file, current->loc.line, current->loc.character);
			}
			if (type_in(current->type, pairedopener, sizeof(pairedopener))) {
				last = rfind_next_token_of_type(last, rstopper,
					pair_type_lookup(current->type, pairedopener, pairedcloser, sizeof(pairedopener)));
				if (!last || last <= current) {
					fprintf(stderr, "ERROR: no pair found");
				}
				last->paired_token = current;
				current->paired_token = last--;
			}
		}
	}


	//Iterate backwards through tokens which create conditional branches to cover nested conditions
	for (Token* current = tokens + numTokens - 1; current != rstopper; current--) {
		if (current->type == HEAD_READ) {
			setup_conditional_branch(current, tokens, stopper);
			attach_self_as_prefix(current, current+1);
		}
		if (current->type == CHECK_ABILITY_TO_MOVE) {
			setup_conditional_branch(current, tokens, stopper);
			attach_self_as_prefix(current, current+1);
		}
	}

	//Sets the next value of the last instruction in each conditional branch to be the next instruction not in a conditional
	for (Token* current = tokens; current < stopper; current++) {
		if (!current->conditional) continue;
		Token* lastInstructionFalse = current->conditional->end_false-1;
		Token* lastInstructionTrue = current->conditional->end_true-1;
		Token* firstInstructionAfterBranch = current->conditional->end_true+1;
		while (firstInstructionAfterBranch->branchMember) { //Handles nested conditionals
			firstInstructionAfterBranch = firstInstructionAfterBranch->branchMember->end_true+1;
		}
		lastInstructionFalse->next_unconditional = lastInstructionTrue->next_unconditional = firstInstructionAfterBranch;
	}

	TOKEN_TYPE passthrough[] = {
		OPEN_BLOCK,
		CLOSE_BLOCK,
		NO_OPERATION,
		HEAD_WRITE,
		HEAD_LEFT,
		HEAD_RIGHT,
		HEAD_UP,
		HEAD_DOWN,
		REPEAT_MOVE,
		REPEAT_MOVE_MAX,
		OPEN_CONDITIONAL,
		CLOSE_CONDITIONAL,
		STAKE_FLAG
	};
	for (Token* current = tokens; current < stopper; current++) {
		//TODO: WARNING - does not properly check validity of lookahead/behin
		if (!current->next_unconditional && type_in(current->type, passthrough, sizeof(passthrough))) {
			current->next_unconditional = next_nonconditional_token(current, stopper);
			if (current->type == REPEAT_MOVE || current->type == REPEAT_MOVE_MAX) {
				attach_self_as_prefix(current, current+1);
			}
		}

		if (current->type == LOOP_UNTIL_END || current->type == LOOP_UNTIL_BEGINNING) {
			current->next_if_false = rfind_next_token_of_type(current, rstopper, OPEN_BLOCK)->next_unconditional;
			current->next_if_true = find_next_token_of_type(current, stopper, CLOSE_BLOCK);
		}
		if (current->type == LOOP_FIXED_AMOUNT) {
			if (!(current+1)->paired_token)
				fprintf(stderr, "ERROR: no pair to loop");
			current->next_if_false = (current+1)->paired_token;
			current->next_if_true = current+1;
		}

		if (current->type == RETURN_FLAG) {
			current->paired_token = rfind_next_token_of_type(current, rstopper, STAKE_FLAG);
			current->next_unconditional = next_nonconditional_token(current, stopper);
		}
		if (current->type == GOTO_BLOCK_END) {
			current->next_unconditional = find_next_token_of_type(current, stopper, CLOSE_BLOCK);
		}
		if (current->type == GOTO_BLOCK_START) {
			current->next_unconditional = rfind_next_token_of_type(current, rstopper, OPEN_BLOCK);
		}
		if (current->type == END_OF_PROGRAM && current != stopper-1) {
			fprintf(stderr, "ERROR: '%s' token discovered at invalid position, check Lexer\n", human(current->type));
		}
	}
}



char write1ToByte(char byte, size_t position) {
	if (position > 7) return byte;
	return byte | (0x1 << (7 - position));
}
char write0ToByte(char byte, size_t position) {
	if (position > 7) return byte;
	char mask = ~(0x1 << (7 - position));
	return byte & mask;
}

//Note: Only returns 0 or 1
char readByteAtPosition(char byte, size_t position) {
	return (byte >> (7-position)) & 0x1;
}

void move_head(int* x_pos, int* y_pos, TOKEN_TYPE t, size_t repeats) {
	for (size_t i = 0; i < repeats; i++) {
		switch (t) {
			case HEAD_LEFT:  (*x_pos)--; break;
			case HEAD_RIGHT: (*x_pos)++; break;
			case HEAD_UP:    (*y_pos)--; break;
			case HEAD_DOWN:  (*y_pos)++; break;
		}
	}
}

void dump_program_state(FILE* fp, int x_pos, int y_pos, char* grid_bytes, size_t grid_size) {
	fprintf(fp, " %c", x_pos == -1 ? 'v' : ' '); 
	for(size_t x = 0; x < 9; x++) {if(x==4)fprintf(fp, " ");fprintf(fp, "%c", x == x_pos ? 'v' : ' ');}
	fprintf(fp, "\n");

	for (size_t y = 0; y < 3; y++) {
		char buff[64] = {0};
		binary_str_from_byte(*(grid_bytes+y), buff, true, x_pos,
			(flags.use_ansi && !flags.no_colour && y == y_pos ? "\x1b[32m" : "")
		);
		fprintf(fp, "%c %s\n", y_pos == y ? '>' : ' ', buff);
	}
}

void Railcar_Simulator(Token* firstTk) {
	//TODO: expand simulator to allow an arbitrary number of byte rows
	char grid_bytes[3] = {0};
	*(grid_bytes+0) = byte_from_binary_str("0101 1100", true);
	*(grid_bytes+1) = byte_from_binary_str("0011 0100", true);
	*(grid_bytes+2) = 0;

	int x_pos = 0;
	int y_pos = 0;

	if (flags.use_ansi) { printf("\x1b[s***\n"); }//Save cursor position
	Token* current = NULL;
	while (!current || current->next_unconditional || current->next_if_false || current->next_if_true) {
		char* selectedByte = grid_bytes + y_pos;
		
		dump_program_state(stdout, x_pos, y_pos, grid_bytes, sizeof(grid_bytes));

		printf("\nExecuting: "); dump_token(stdout, current);

		Token* nextTk = NULL;
		if (!current) {
			nextTk = firstTk;
			printf("\nNext TK: "); dump_token(stdout, nextTk);
			printf("-----------\n");
			current = nextTk;
			continue;
		}
		else if (current->next_unconditional) {
			nextTk = current->next_unconditional;
			printf("Next TK: "); dump_token(stdout, nextTk);
		}
		
		if (current->type == HEAD_READ) {
			bool result = readByteAtPosition(*selectedByte, x_pos);
			if (result) nextTk = current->next_if_true;
			else        nextTk = current->next_if_false;
			
			printf("Read head got '%d'\nNext TK: ", result); dump_token(stdout, nextTk);
		}
		if (current->type == LOOP_UNTIL_END) {
			bool result = x_pos == 8;
			if (result) nextTk = current->next_if_true;
			else        nextTk = current->next_if_false;
			
			printf("At end got '%d'\nNext TK: ", result); dump_token(stdout, nextTk);
		}
		if (current->type == LOOP_UNTIL_BEGINNING) {
			bool result = x_pos == -1;
			if (result) nextTk = current->next_if_true;
			else        nextTk = current->next_if_false;
			
			printf("At beginning got '%d'\nNext TK: ", result); dump_token(stdout, nextTk);
		}
		if (current->type == HEAD_LEFT) {x_pos--;}
		if (current->type == HEAD_RIGHT) {x_pos++;}
		if (current->type == HEAD_UP) {y_pos--;}
		if (current->type == HEAD_DOWN) {y_pos++;}
		if (current->type == REPEAT_MOVE) {
			move_head(&x_pos, &y_pos, current->next_unconditional->type, current->value);
			nextTk = current->next_unconditional->next_unconditional;
		}
		if (current->type == REPEAT_MOVE_MAX) {
			if (current->next_unconditional->type == HEAD_LEFT) {
				x_pos = 0;
				nextTk = current->next_unconditional->next_unconditional;
			}
			else if (current->next_unconditional->type == HEAD_RIGHT) {
				x_pos = 7;
				nextTk = current->next_unconditional->next_unconditional;
			}
			else fprintf(stderr, "ERROR: Undefined operand for REPEAT_MOVE_MAX, got '%s'", human(current->next_unconditional->id));

		}


		if (current->type == HEAD_WRITE) {
			if (current->value) *selectedByte = write1ToByte(*selectedByte, x_pos);
			else                *selectedByte = write0ToByte(*selectedByte, x_pos);
		}
		
		printf("-----------\n");

		// break;
		current = nextTk;
		char ch; scanf("%c", &ch);
		if (ch == 'q') break;
		
		if (flags.use_ansi) { printf("\x1b[u\x1b[0J"); } //Load cursor position and wipe
	}
	printf("No more tokens, Final state: \n");
	dump_program_state(stdout, x_pos, y_pos, grid_bytes, sizeof(grid_bytes));
}


void shellEcho(char* command) {
	fprintf(stdout, "Running: %s\n", command);
	int retcode = system(command);
	fprintf(stdout, "%s %s\n", retcode==EXIT_SUCCESS ? "Success:" : "Failure", command);
}

int main(int argc, char* argv[]) {
	
	//TODO: better argv handling
	if (strcmp(argv[1], "step") == 0) flags.step = true;
	flags.use_ansi = true;
	flags.no_colour = false;
	flags.graphviz = true;
	char* fileName = argv[flags.step ? 2 : 1];

	//Lexer
	printf("Lexing: %s\n", fileName);
	size_t num_tk = 0;
	Token* tkArr = Railcar_Lexer(fileName, &num_tk);
	
	//Parser
	Railcar_Parser(tkArr, num_tk);
	dump_tokens(stdout, tkArr, num_tk);

	if (flags.graphviz) {
		FILE* fp = fopen("output.dot", "w");
		if (fp) dump_tokens_to_dotfile(fp, tkArr, num_tk);
		fclose(fp);

		shellEcho(".\\vendors\\Graphviz\\bin\\dot.exe -Tpng output.dot -O");
		shellEcho(".\\vendors\\Graphviz\\bin\\dot.exe -Tsvg output.dot -O");
	}

	//Simulator
	if (flags.step) {
		printf("Stepper\n");
		// if (flags.use_ansi) { printf("\x1b[s***"); }//Save cursor position

		Railcar_Simulator(tkArr);
	}

	if (false) {
		//TODO: work on use of ANSI escapes
		//Resources:
		//	https://solarianprogrammer.com/2019/04/08/c-programming-ansi-escape-codes-windows-macos-linux-terminals/
		//	https://www.lihaoyi.com/post/BuildyourownCommandLinewithANSIescapecodes.html#cursor-navigation
		//	https://rosettacode.org/wiki/Terminal_control/Cursor_positioning#C.2FC.2B.2B
		//	https://youtu.be/kLj-H1K317U?t=4855
		printf("ABCDEF");
		// printf("\x1b[4C\x1b[4AGHIJK\n");
		printf("\x1b[32mHello, World\n");
		printf("TWELVE");
		printf("\x1b[1000D\x1b[0K");
		// printf("\x1b[2J");
		printf("Savage\n");
		// printf("\x1b[2J");  //Clear the screen
		// Reset colors to defaults
		printf("\x1b[0mtesting\n");
	}
	return 0;
}