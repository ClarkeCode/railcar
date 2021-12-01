#include "railcar.h"
#include "rc_utilities.h"
#include <assert.h>
#include <stdio.h>

extern Flags flags;
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

char byte_from_binary_str(char* str, bool separateNibbles) {
	char output = 0x0000;
	for (size_t x = 0; x < (separateNibbles ? 9 : 8); x++) {
		if (separateNibbles && x == 4) continue;
		output <<= 1;
		output |= (*(str+x) == '1' ? 0x0001 : 0x0000);
	}
	return output;
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

void move_head(HeadLocation* location, TOKEN_TYPE t, size_t repeats) {
	for (size_t i = 0; i < repeats; i++) {
		switch (t) {
			case HEAD_LEFT:  (location->x)--; break;
			case HEAD_RIGHT: (location->x)++; break;
			case HEAD_UP:    (location->y)--; break;
			case HEAD_DOWN:  (location->y)++; break;
		}
	}
}


void dump_token(FILE* fp, Token* tk) {
	if (!tk) return;
	bool lineInfo = false;
	bool nextTk = true;
	if (lineInfo) fprintf(fp, "%s:%lu:%lu ", tk->loc.file, tk->loc.line, tk->loc.character);
	fprintf(fp, "TOKEN #%lu { %s",
		tk->id, human(tk->type));
	if (tk->type == HEAD_WRITE || tk->type == REPEAT_MOVE) 
		fprintf(fp, " - '%d'", tk->value);
	
	if (tk->str_value) {
		fprintf(fp, " '%s'", tk->str_value);
	}
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
void dump_datastack(FILE* fp, Program* prog) {
	DataStack* ds = &(prog->stack);
	fprintf(fp, " %c", ds->current_location.x == -1 ? 'v' : ' '); 
	for(size_t x = 0; x < 9; x++) {if(x==4)fprintf(fp, " ");fprintf(fp, "%c", x == ds->current_location.x ? 'v' : ' ');}
	fprintf(fp, "\n");

	for (size_t y = 0; y < ds->max_dimensions.y; y++) {
		char buff[64] = {0};
		binary_str_from_byte(*(ds->content+y), buff, true, ds->current_location.x,
			(flags.use_ansi && !flags.no_colour && y == ds->current_location.y ? "\x1b[32m" : "")
		);
		fprintf(fp, "%c %s", ds->current_location.y == y ? '>' : ' ', buff);

		HLocationMapping* hFlag = prog->flag_values;
		while (hFlag->key) {
			if (y == hFlag->value.y) {
				fprintf(fp, " '%s':(%lu, %lu)", hFlag->key, hFlag->value.x, hFlag->value.y);
			}
			hFlag++;
		}
		fprintf(fp, "\n");
	}
}
void dump_program(FILE* fp, Program* prog) {
	fprintf(fp, "Datastack:\n");
	dump_datastack(fp, prog);
	fprintf(fp, "\nInstructions:\n");
	dump_tokens(fp, prog->instructions, prog->sz_instructions);
}

void dump_tokens_to_dotfile(FILE* fp, Token* tkArr, size_t num_tk) {
	const bool showConditionals = flags.graphviz_conditionals;
	const bool showPairedTokens = flags.graphviz_pairs;
	const bool showPrefixedTokens = flags.graphviz_prefixed;
	fprintf(fp, "digraph {\n");
	// fprintf(fp, "\tlayout=neato;\n");
	Token* current;
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

void Railcar_Simulator(Program* prog) {
	const char ERR_PREFIX[] = "SIMULATOR";

	if (flags.step && flags.use_ansi) { printf("\x1b[s***\n"); }//Save cursor position
	Token* firstTk = prog->instructions;
	Token* current = NULL;
	while (!current || current->next_unconditional || current->next_if_false || current->next_if_true) {
		R_BYTE* selectedByte = &(prog->stack.content[prog->stack.current_location.y]);
		int x_pos = prog->stack.current_location.x;
		int y_pos = prog->stack.current_location.y;

		Token* nextTk = NULL;
		if (!current) {
			nextTk = firstTk;
			if (flags.step && flags.step_interactive) {
				printf("\nNext TK: "); dump_token(stdout, nextTk);
				printf("-----------\n");
			}
			current = nextTk;
			continue;
		}
		else if (current->type == OPEN_BLOCK || current->type == CLOSE_BLOCK) {
			current = current->next_unconditional; continue;
		}


		//Printing
		if (flags.step_after_line && current->loc.line+1 == flags.step_line) {
			flags.step = flags.step_interactive = true;
			flags.step_after_line = false;
		}
		if (flags.step && flags.step_interactive) {
			dump_datastack(stdout, &prog->stack);
			printf("\nExecuting: "); dump_token(stdout, current);
		}

		assert(NUM_TOKEN_TYPE == 28 && "Unhandled token");

		
		if (current->type == STAKE_FLAG) {
			
			HLocationMapping* saved_locations = prog->flag_values;
			while(strlen(saved_locations->key) != 0 && strcmp(current->str_value, saved_locations->key) != 0) { ++saved_locations; }
			if (!saved_locations->key) {
				saved_locations->key=calloc(strlen(current->str_value)+1, sizeof(char));
				strcpy(saved_locations->key, current->str_value);
			}

			saved_locations->value = prog->stack.current_location;
			nextTk = current->next_unconditional;
		}
		else if (current->type == RETURN_FLAG) {
			HLocationMapping* saved_locations = prog->flag_values;
			while(saved_locations->key != 0 && strcmp(current->str_value, saved_locations->key) != 0) { ++saved_locations; }
			if (!saved_locations->key) reportError(&current->loc, ERR_PREFIX, "Cannot move to position, named flag '%s' does not exist", current->str_value);

			prog->stack.current_location = saved_locations->value;
			nextTk = current->next_unconditional;
		}
		else if (current->next_unconditional) {
			nextTk = current->next_unconditional;
			if (flags.step && flags.step_interactive) { printf("Next TK: "); dump_token(stdout, nextTk); }
		}
		
		if (current->type == HEAD_READ) {
			bool result = readByteAtPosition(*selectedByte, x_pos);
			if (result) nextTk = current->next_if_true;
			else        nextTk = current->next_if_false;
			
			if (flags.step && flags.step_interactive) { printf("Read head got '%d'\nNext TK: ", result); dump_token(stdout, nextTk); }
		}
		if (current->type == CHECK_ABILITY_TO_MOVE) {
			bool can_move = false;
			TOKEN_TYPE direction = current->prefix_member->junior->type;
			if (direction == HEAD_UP || direction == HEAD_DOWN) {
				can_move = (direction == HEAD_UP ? y_pos > 0 : y_pos < prog->stack.max_dimensions.y-1);
			}
			else if (direction == HEAD_LEFT || direction == HEAD_RIGHT) {
				can_move = (direction == HEAD_LEFT ? x_pos > 0 : x_pos < 7);
			}

			if (can_move) nextTk = current->next_if_true;
			else          nextTk = current->next_if_false;
		}

		if (current->type == LOOP_UNTIL_END) {
			bool result = x_pos == 8;
			if (result) nextTk = current->next_if_true;
			else        nextTk = current->next_if_false;
			
			if (flags.step && flags.step_interactive) {printf("At end got '%d'\nNext TK: ", result); dump_token(stdout, nextTk);}
		}
		if (current->type == LOOP_UNTIL_BEGINNING) {
			bool result = x_pos == -1;
			if (result) nextTk = current->next_if_true;
			else        nextTk = current->next_if_false;
			
			if (flags.step && flags.step_interactive) {printf("At beginning got '%d'\nNext TK: ", result); dump_token(stdout, nextTk);}
		}
		if (current->type == LOOP_FIXED_AMOUNT) {
			bool result = (current->value--) == 0;
			if (result) nextTk = current->next_if_true;
			else        nextTk = current->next_if_false;
			if (flags.step && flags.step_interactive) {printf("Loop fixed has '%d' remaining\nNext TK: ", current->value); dump_token(stdout, nextTk);}
		}

		
		if (current->type == PRINT_BYTE_AS_CHAR) { printf("%c", *selectedByte); }
		if (current->type == PRINT_BYTE_AS_NUM) { printf("%d", *selectedByte); }

		if (current->type == HEAD_LEFT) { move_head(&(prog->stack.current_location), HEAD_LEFT, 1); }
		if (current->type == HEAD_RIGHT) { move_head(&(prog->stack.current_location), HEAD_RIGHT, 1); }
		if (current->type == HEAD_UP) { move_head(&(prog->stack.current_location), HEAD_UP, 1); }
		if (current->type == HEAD_DOWN) { move_head(&(prog->stack.current_location), HEAD_DOWN, 1); }
		if (current->type == REPEAT_MOVE) {
			move_head(&(prog->stack.current_location), (current+1)->type, current->value);
			nextTk = current->next_unconditional;
		}
		if (current->type == REPEAT_MOVE_MAX) {
			if (current->prefix_member->junior->type == HEAD_LEFT) { prog->stack.current_location.x = 0; }
			else if (current->prefix_member->junior->type == HEAD_RIGHT) { prog->stack.current_location.x = prog->stack.max_dimensions.x-1; }
			else if (current->prefix_member->junior->type == HEAD_UP) {prog->stack.current_location.y = 0; }
			else if (current->prefix_member->junior->type == HEAD_DOWN) {prog->stack.current_location.y = prog->stack.max_dimensions.y-1; }
			else reportError(&current->loc, ERR_PREFIX, "Undefined operand for REPEAT_MOVE_MAX, got '%s'\n", human(current->next_unconditional->type));
			nextTk = current->prefix_member->junior->next_unconditional;
		}
		if (current->type == RELATIVE_MOVE) {
			switch(current->prefix_member->junior->type) {
				case HEAD_UP: move_head(&(prog->stack.current_location), HEAD_UP, *selectedByte); break;
				case HEAD_DOWN: move_head(&(prog->stack.current_location), HEAD_DOWN, *selectedByte); break;
				case HEAD_LEFT: move_head(&(prog->stack.current_location), HEAD_LEFT, *selectedByte); break;
				case HEAD_RIGHT: move_head(&(prog->stack.current_location), HEAD_RIGHT, *selectedByte); break;
				default: reportError(&current->loc, ERR_PREFIX, "Undefined operand for RELATIVE_MOVE, got '%s'\n", human(current->next_unconditional->type));
			}
			nextTk = current->prefix_member->junior->next_unconditional;
		}


		if (current->type == HEAD_WRITE) {
			if (current->value) *selectedByte = write1ToByte(*selectedByte, x_pos);
			else                *selectedByte = write0ToByte(*selectedByte, x_pos);
		}
		
		//Move to the next instruction
		current = nextTk;

		if (flags.step && flags.step_interactive) {
			printf("-----------\n");
			char ch; scanf("%c", &ch);
			if (ch == 'q') break;
		}		
		if (flags.step && flags.use_ansi) { printf("\x1b[u\x1b[0J"); } //Load cursor position and wipe
	}
	if (flags.step) {
		printf("No more tokens, Final state: \n");
		dump_datastack(stdout, &prog->stack);
	}
}
