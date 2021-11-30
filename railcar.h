#ifndef RAILCAR_HEADER
#define RAILCAR_HEADER
#include <stdlib.h>

typedef enum {
	NO_OPERATION,

	HEAD_LEFT,
	HEAD_RIGHT,
	HEAD_UP,
	HEAD_DOWN,
	REPEAT_MOVE,
	REPEAT_MOVE_MAX, //Change to REPEAT, can be used as block prefix
	RELATIVE_MOVE, //Read current byte, and move a corresponding number of positions
	CHECK_ABILITY_TO_MOVE, //followed by conditional pair

	HEAD_READ, //followed by conditional pair
	HEAD_WRITE,

	NUMBER,
	// STRING,

	OPEN_CONDITIONAL,
	CLOSE_CONDITIONAL,
	OPEN_BLOCK,
	CLOSE_BLOCK,

	GOTO_BLOCK_START,
	GOTO_BLOCK_END,

	LOOP_UNTIL_END,
	LOOP_UNTIL_BEGINNING,
	LOOP_FIXED_AMOUNT,

	STAKE_FLAG,
	RETURN_FLAG,

	PRINT_BYTE_AS_CHAR,
	PRINT_BYTE_AS_NUM,
	END_OF_PROGRAM,

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
	struct Token* creator;
	struct Token* start_false;
	struct Token* start_true;
	struct Token* end_false;
	struct Token* end_true;
} ConditionalBranch;

typedef struct BidirectionalLinkage{
	struct Token* senior;
	struct Token* junior;
} BidirectionalLinkage;
BidirectionalLinkage* make_blink(struct Token* senior, struct Token* junior);

typedef struct Token{
	int id;
	TOKEN_TYPE type;
	int value;
	char* str_value;
	Location loc;
	struct Token* next_unconditional;
	struct Token* next_if_true;
	struct Token* next_if_false;

	BidirectionalLinkage* prefix_member; //Senior is the modifier token ie: repeat/max_repeat, Junior is the modified token ie: up/left
	
	BidirectionalLinkage* pair; //Senior is the opening token ie: '[', Junior is the closing token ie: ']'

	ConditionalBranch* conditional; //Will be not NULL if this token creates a branch
	ConditionalBranch* branchMember; //If the token belongs to a branch, this points to the info
} Token;

typedef struct {
	size_t x, y;
} HeadLocation;
typedef unsigned char R_BYTE;
typedef struct DataStack{
	HeadLocation current_location;
	HeadLocation saved_location;
	HeadLocation max_dimensions;
	R_BYTE content[128]; //TODO: dynamic size data stack
	size_t sz_content;
} DataStack;

typedef struct Program{
	DataStack stack;
	Token* instructions;
	size_t sz_instructions;
} Program;

Program* Railcar_Lexer(char* fileName);

void Railcar_Parser(Program* prog);
#endif