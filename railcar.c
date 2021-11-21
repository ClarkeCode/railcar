#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "railcar.h"
#include <string.h>

//Pass NULL as an argument to num_tokens if you do not want it to be incremented
//Pass 0 as an argument to val if the token does not requre a value
void create_token(size_t* num_tokens, Token* tk, TOKEN_TYPE tk_t, int val) {
	if (tk) {
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
		assert(NUM_TOKEN_TYPE == 22 && "Unhandled Token");
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
			create_token(&num_tokens, tk, REPEAT_MOVE, atoi(&c));
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

//TODO: try reworking to have an internal buffer and returning the buffer as a char*
void binary_str_from_byte(char byte, char* str, bool separateNibbles) {
	int writeLen = (separateNibbles) ? 8 : 7;
	char* currentChar = str;
	for (int x = 0; x < 8; x++) {
		if (separateNibbles && x == 4)
			(*currentChar++) = ' ';

		if ((byte >> 7-x) & 0x1)
			(*currentChar++) = '1';
		else
			(*currentChar++) = '0';
	}
}

char* human(TOKEN_TYPE ttype) {
	assert(NUM_TOKEN_TYPE == 22 && "Unhandled Token");

	switch (ttype) {
		case NO_OPERATION: return "NOP"; break;

		case HEAD_LEFT: return "H_LEFT"; break;
		case HEAD_RIGHT: return "H_RIGHT"; break;
		case HEAD_UP: return "H_UP"; break;
		case HEAD_DOWN: return "H_DOWN"; break;
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

		case STAKE_FLAG: return "STAKE_FLAG"; break;
		case RETURN_FLAG: return "RETURN_FLAG"; break;

		case UNKNOWN: return "UNKNOWN"; break;
	};
	return "unreachable";
}

//TODO: add sequence number to tokens to remove 'tkArr' argument
void dump_token(FILE* fp, Token* tk, Token* tkArr) {
	if (!tk) return;
	bool lineInfo = false;
	bool nextTk = true;
	if (lineInfo) fprintf(fp, "%s:%lu:%lu ", tk->loc.file, tk->loc.line, tk->loc.character);
	fprintf(fp, "TOKEN #%lu { %s",
		tk-tkArr, human(tk->type));
	if (tk->type == HEAD_WRITE || tk->type == REPEAT_MOVE) 
		fprintf(fp, " - '%d'", tk->value);
	if (nextTk) {
		if (tk->next_unconditional) fprintf(fp, " NU: %lu", tk->next_unconditional-tkArr);
		if (tk->next_if_false) fprintf(fp, " NF: %lu",tk->next_if_false-tkArr);
		if (tk->next_if_true) fprintf(fp, " NT: %lu",tk->next_if_true-tkArr);
	}
	fprintf(fp, "}\n");
}
void dump_tokens(FILE* fp, Token* tkArr, size_t num_tk) {
	for (size_t x = 0; x<num_tk; x++) {
		dump_token(fp, tkArr+x, tkArr);
	}
}

void usage(FILE* fp) {
	fprintf(fp, "USAGE: ./railcar.exe <subcommand> filename\n");
	fprintf(fp, "\nSUBCOMMANDS:\n");
	fprintf(fp, "	step     //step through tokens one-at-a-time");
}

typedef struct {
	bool step;
} Flags;

Token* _next_token_of_type(Token* current, Token* stopper, TOKEN_TYPE tk_t, bool doIncrement) {
	Token* test = current;
	while (test != stopper) {
		if (test->type == tk_t) return test;
		test += doIncrement ? 1 : -1;
	}
	return NULL;
}
Token* next_token_of_type(Token* current, Token* stopper, TOKEN_TYPE tk_t) {
	return _next_token_of_type(current, stopper, tk_t, true);
}

Token* r_next_token_of_type(Token* current, Token* stopper, TOKEN_TYPE tk_t) {
	return _next_token_of_type(current, stopper, tk_t, false);
}

void Railcar_Parser(Token* tokens, size_t numTokens) {
	Token* stopper = tokens + numTokens;
	Token* current = tokens;

	//TODO: add error reporting and syntax checking to call

	//TODO: lying here, need to handle all existing tokens
	assert(NUM_TOKEN_TYPE == 22 && "Unhandled Token");
	for (; current < stopper; current++) {
		//TODO: WARNING - does not properly check validity of lookahead/behind
		if (current->type == OPEN_BLOCK) current->next_unconditional = current+1;
		//TODO: handle nested if/reads
		if (current->type == HEAD_READ) {
			current->next_if_false = next_token_of_type(current, stopper, OPEN_CONDITIONAL)+1;
			current->next_if_true = next_token_of_type(current->next_if_false, stopper, OPEN_CONDITIONAL)+1;
		}
		if (current->type == NO_OPERATION) current->next_unconditional = current+1;
		if (current->type == HEAD_WRITE) current->next_unconditional = current+1;
		if (current->type == HEAD_LEFT) current->next_unconditional = current+1;
		if (current->type == HEAD_RIGHT) current->next_unconditional = current+1;


		//TODO: handle nested if/reads
		if (current->type == OPEN_CONDITIONAL) current->next_unconditional = next_token_of_type(current, stopper, CLOSE_CONDITIONAL)+1;
		if (current->type == CLOSE_CONDITIONAL) current->next_unconditional = current+1;

		if (current->type == LOOP_UNTIL_END) {
			current->next_if_false = r_next_token_of_type(current, tokens-1, OPEN_BLOCK)->next_unconditional;
			current->next_if_true = next_token_of_type(current, stopper, CLOSE_BLOCK);
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

void Railcar_Simulator(Token* firstTk) {
	//TODO: expand simulator to allow an arbitrary number of byte rows
	char input_byte = byte_from_binary_str("0000 0101", true);
	char output_byte = 0;

	size_t x_pos = 0;
	size_t y_pos = 0;

	Token* current = NULL;
	while (!current || current->next_unconditional || current->next_if_false || current->next_if_true) {
		char* selectedByte = y_pos == 0 ? &input_byte : &output_byte;
		
		char buff[10] = {0};
		printf("  "); for(size_t x = 0; x < 9; x++) {if(x==4)printf(" ");printf("%c", x == x_pos ? 'v' : ' ');}
		binary_str_from_byte(input_byte, buff, true);
		printf("\n%c %s\n", y_pos == 0 ? '>' : ' ', buff);
		// binary_str_from_byte(output_byte, buff, true);
		// printf("%c %s\n", y_pos == 1 ? '>' : ' ', buff);

		printf("\nExecuting: "); dump_token(stdout, current, firstTk);

		Token* nextTk = NULL;
		if (!current) {
			nextTk = firstTk;
			printf("\nNext TK: "); dump_token(stdout, nextTk, firstTk);
			printf("-----------\n");
			current = nextTk;
			continue;
		}
		else if (current->next_unconditional) {
			nextTk = current->next_unconditional;
			printf("Next TK: "); dump_token(stdout, nextTk, firstTk);
		}
		
		if (current->type == HEAD_READ) {
			bool result = readByteAtPosition(*selectedByte, x_pos);
			if (result) nextTk = current->next_if_true;
			else        nextTk = current->next_if_false;
			
			printf("Read head got '%d'\nNext TK: ", result); dump_token(stdout, nextTk, firstTk);
		}
		if (current->type == LOOP_UNTIL_END) {
			bool result = x_pos == 8;
			if (result) nextTk = current->next_if_true;
			else        nextTk = current->next_if_false;
			
			printf("At end got '%d'\nNext TK: ", result); dump_token(stdout, nextTk, firstTk);
		}
		if (current->type == HEAD_LEFT) {x_pos--;}
		if (current->type == HEAD_RIGHT) {x_pos++;}
		if (current->type == HEAD_WRITE) {
			if (current->value) *selectedByte = write1ToByte(*selectedByte, x_pos);
			else                *selectedByte = write0ToByte(*selectedByte, x_pos);
		}
		
		printf("-----------\n");

		// break;
		current = nextTk;
	}
	printf("No more tokens, Final state: \n");
	char buff[10] = {0};
	binary_str_from_byte(input_byte, buff, true);
	printf("%s\n", buff);
}

int main(int argc, char* argv[]) {
	Flags flags = {0};
	//TODO: better argv handling
	if (strcmp(argv[1], "step") == 0) flags.step = true;
	char* fileName = argv[flags.step ? 2 : 1];

	//Lexer
	printf("Lexing: %s\n", fileName);
	size_t num_tk = 0;
	Token* tkArr = Railcar_Lexer(fileName, &num_tk);
	
	//Parser
	Railcar_Parser(tkArr, num_tk);
	dump_tokens(stdout, tkArr, num_tk);

	//Simulator
	if (flags.step) {
		printf("Stepper\n");
		Railcar_Simulator(tkArr);
	}

	if (false) {
		//TODO: work on use of ANSI escapes
		//Resources:
		//	https://solarianprogrammer.com/2019/04/08/c-programming-ansi-escape-codes-windows-macos-linux-terminals/
		//	https://www.lihaoyi.com/post/BuildyourownCommandLinewithANSIescapecodes.html#cursor-navigation
		//	https://rosettacode.org/wiki/Terminal_control/Cursor_positioning#C.2FC.2B.2B
		//	https://youtu.be/kLj-H1K317U?t=4855
		printf("ABCDEF");printf("\x1b[4dGHIJK\n");
		printf("\x1b[32mHello, World\n");
		// printf("\x1b[2J");  //Clear the screen
		// Reset colors to defaults
		printf("\x1b[0mtesting\n");
	}
	return 0;
}