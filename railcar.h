#ifndef RAILCAR_HEADER
#define RAILCAR_HEADER

typedef enum {
	NO_OPERATION,

	HEAD_LEFT,
	HEAD_RIGHT,
	HEAD_UP,
	HEAD_DOWN,
	REPEAT_MOVE,
	REPEAT_MOVE_MAX, //Change to REPEAT, can be used as block prefix
	CHECK_ABILITY_TO_MOVE, //followed by conditional pair

	HEAD_READ, //followed by conditional pair
	HEAD_WRITE,

	NUMBER,

	OPEN_CONDITIONAL,
	CLOSE_CONDITIONAL,
	OPEN_BLOCK,
	CLOSE_BLOCK,

	GOTO_BLOCK_START,
	GOTO_BLOCK_END,

	LOOP_UNTIL_END,
	LOOP_UNTIL_BEGINNING,

	STAKE_FLAG,
	RETURN_FLAG,

	UNKNOWN,

	NUM_TOKEN_TYPE
}
TOKEN_TYPE;

typedef struct {
	char* file;
	size_t line;
	size_t character;
} Location;

struct Token;

typedef struct ConditionalBranch{
	struct Token* branch_end_false;
	struct Token* branch_end_true;
} ConditionalBranch;

typedef struct Token{
	int id;
	TOKEN_TYPE type;
	int value;
	Location loc;
	struct Token* next_unconditional;
	struct Token* next_if_true;
	struct Token* next_if_false;

	ConditionalBranch* conditional;
} Token;

// int Railcar_Lexer(char* fileName, Token** tokenArr);

#endif