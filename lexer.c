#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "railcar.h"

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
char consumeRemainderOfLine(FILE* fp, size_t* line_tracker, size_t* char_tracker) {
	char c;
	do {
		c = procureNextChar(fp, line_tracker, char_tracker);
		if (c=='\n') break;
	} while (c != EOF);
	return c;
}
char fpeek(FILE* stream) {
	char temp = fgetc(stream);
	ungetc(temp, stream);
	return temp;
}
Program* Railcar_Lexer(char* fileName) {
	FILE* lexf = fopen(fileName, "r");
	if (!lexf) return NULL;

	Program* program = calloc(1, sizeof(Program));
	//TODO: arbitrary instruction length
	Token* tokens = calloc(128, sizeof(Token));
	size_t num_tokens = 0;
	size_t num_lines = 0;
	size_t num_chars = -1;

	char c;

	//Set initial state of the data stack
	char buff[128] = {0};
	while ((c = procureNextChar(lexf, &num_lines, &num_chars)) != EOF) {
		if (c == '/') {
			consumeRemainderOfLine(lexf, &num_lines, &num_chars);
			if (c == EOF) break;
		}
		if (c == 'o') {
			do {
				c = procureNextChar(lexf, &num_lines, &num_chars);
				if (c != '=' && c != 'o') {
					fprintf(stderr, "ERROR: Malformed railcar\n"); exit(EXIT_FAILURE);
				}
			} while (c != 'o' && c != EOF);
			if (c != 'o') {
				fprintf(stderr, "ERROR: Malformed railcar - expected 'o', got '%c'\n", c); exit(EXIT_FAILURE);
			}
			break;
		}
		if (isdigit(c)) {
			buff[0] = c;
			while ((c = procureNextChar(lexf, &num_lines, &num_chars)) != EOF && isdigit(c)) {
				buff[strlen(buff)] = c;
			}
			create_token(&(program->stack.sz_content), program->stack.content+program->stack.sz_content, NUMBER, atoi(buff));
			while (strlen(buff) != 0) { buff[strlen(buff)-1] = 0; }
		}
	}
	program->stack.max_dimensions.x = 8;
	program->stack.max_dimensions.y = program->stack.sz_content;


	//Get instructions
	while ((c = procureNextChar(lexf, &num_lines, &num_chars)) != EOF) {
		assert(NUM_TOKEN_TYPE == 26 && "Unhandled Token");
		Token* tk = tokens+num_tokens;
		tk->loc.file = fileName;
		tk->loc.line = num_lines;
		tk->loc.character = num_chars;

		//Comment
		if (c == '/') {
			consumeRemainderOfLine(lexf, &num_lines, &num_chars);
			if (c == EOF) break;
			continue;
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
			case 'c': create_token(&num_tokens, tk, PRINT_BYTE_AS_CHAR, 0);    break;
			case 'd': create_token(&num_tokens, tk, PRINT_BYTE_AS_NUM, 0);     break;

			default:  create_token(&num_tokens, tk, UNKNOWN, 0);
		}
	}
	create_token(&num_tokens, tokens+num_tokens, END_OF_PROGRAM, 0);

	program->instructions = tokens;
	program->sz_instructions = num_tokens;
	fclose(lexf);

	for (Token* test = tokens; test-tokens < num_tokens; test++) {
		if (test->type == UNKNOWN) {
			fprintf(stderr, "ERROR: unable to determine token, introduced at %s:%lu:%lu\n", test->loc.file, test->loc.line, test->loc.character);
			exit(EXIT_FAILURE);
		}
	}

	return program;
}