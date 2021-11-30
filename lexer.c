#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "railcar.h"


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

char procureNextChar(FILE* fp, Location* parse_location) {
	char c = fgetc(fp);
	if (parse_location) {
		if (c == '\n') {
			parse_location->line++;
			parse_location->character = -1;
		}
		parse_location->character++;
	}
	return c;
}
char consumeRemainderOfLine(FILE* fp, Location* parse_location) {
	char c;
	do {
		c = procureNextChar(fp, parse_location);
		if (c=='\n') break;
	} while (c != EOF);
	return c;
}
char fpeek(FILE* stream) {
	char temp = fgetc(stream);
	ungetc(temp, stream);
	return temp;
}

const char* _prefix_lex = "LEXER";
#define BUFF_LEN 512

char default_or_escaped_char(char c) {
	const char* _escape_char_lookup[] = {"n\n", "r\r", "t\t"};
	const char** str = _escape_char_lookup;
	do {
		const char* compare = *str;
		if (*compare == c) {
			return *(++compare);
		}
	} while (str++ != _escape_char_lookup+sizeof(_escape_char_lookup));
	return c;
}


char* consumeString(FILE* fp, Location* parse_location) {
	char buff[BUFF_LEN] = {0};
	char c;
	while ((c = procureNextChar(fp, parse_location)) != EOF && c != '"') {
		if (c == '\\') c = default_or_escaped_char(procureNextChar(fp, parse_location));
		buff[strlen(buff)] = c;
	}
	if (c == EOF) reportError(parse_location, _prefix_lex, "Malformed string literal - expected '\"', got END-OF-FILE\n");
	char* output = malloc(sizeof(R_BYTE)*BUFF_LEN);
	strcpy(output, buff);
	return output;
}

Program* Railcar_Lexer(char* fileName) {
	FILE* lexf = fopen(fileName, "r");
	if (!lexf) return NULL;

	Program* program = calloc(1, sizeof(Program));
	//TODO: arbitrary instruction length
	Token* tokens = calloc(128, sizeof(Token));
	size_t num_tokens = 0;

	Location parse_location = {.file = fileName, .line = 0, .character = -1};

	char c;

	//Set initial state of the data stack
	char buff[128] = {0};
	DataStack* stack = &(program->stack);
	while ((c = procureNextChar(lexf, &parse_location)) != EOF) {
		R_BYTE* selected_byte = stack->content + stack->sz_content;
		if (c == '/') {
			consumeRemainderOfLine(lexf, &parse_location);
			if (c == EOF) break;
		}
		if (c == 'o') {
			do {
				c = procureNextChar(lexf, &parse_location);
				if (c == EOF) reportError(&parse_location, _prefix_lex, "Malformed railcar - expected 'o', got END-OF-FILE\n");
				if (c != '=' && c != 'o') {
					reportError(&parse_location, _prefix_lex, "Malformed railcar - expected 'o', got '%c'\n", c);
				}
			} while (c != 'o' && c != EOF);
			break;
		}
		if (isdigit(c)) {
			buff[0] = c;
			while ((c = procureNextChar(lexf, &parse_location)) != EOF && isdigit(c)) {
				buff[strlen(buff)] = c;
			}
			*selected_byte = (R_BYTE)atoi(buff);
			stack->sz_content++;
			while (strlen(buff) != 0) { buff[strlen(buff)-1] = 0; } //clear buffer
		}
		if (c == '"') {
			char* str = consumeString(lexf, &parse_location);
			// *(selected_byte++) = (R_BYTE)strlen(str)+1; //Size of string
			strcat(selected_byte, str);
			stack->sz_content += strlen(str)+1;
		}
	}
	program->stack.max_dimensions.x = 8;
	program->stack.max_dimensions.y = program->stack.sz_content;


	//Get instructions
	while ((c = procureNextChar(lexf, &parse_location)) != EOF) {
		assert(NUM_TOKEN_TYPE == 27 && "Unhandled Token");
		Token* tk = tokens+num_tokens;
		tk->loc = parse_location;

		//Comment
		if (c == '/') {
			consumeRemainderOfLine(lexf, &parse_location);
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
				if ((c = procureNextChar(lexf, &parse_location)) == EOF) return NULL;
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
			case '~': create_token(&num_tokens, tk, RELATIVE_MOVE, 0);         break;

			default:  create_token(&num_tokens, tk, UNKNOWN, 0);
		}
	}
	create_token(&num_tokens, tokens+num_tokens, END_OF_PROGRAM, 0);

	program->instructions = tokens;
	program->sz_instructions = num_tokens;
	fclose(lexf);

	for (Token* test = tokens; test-tokens < num_tokens; test++) {
		if (test->type == UNKNOWN) {
			reportError(&test->loc, _prefix_lex, "Unable to determine token");
		}
	}

	return program;
}