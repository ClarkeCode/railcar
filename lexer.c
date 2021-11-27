#include <stdio.h>
#include <assert.h>
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