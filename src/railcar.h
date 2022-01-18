#ifndef RAILCAR_HEADER
#define RAILCAR_HEADER
#include <stdlib.h>
#include <stdbool.h>

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
	STRING,

	OPEN_CONDITIONAL,
	CLOSE_CONDITIONAL,
	OPEN_BLOCK,
	CLOSE_BLOCK,

	GOTO_BLOCK_START,
	GOTO_BLOCK_END,

	//TODO: Remove LOOP_E/LOOP_B in favour of using ?>($)(#) / ?<($)(#)
	LOOP_UNTIL_END,
	LOOP_UNTIL_BEGINNING,
	LOOP_FIXED_AMOUNT,
	LOOP_DYNAMIC_AMOUNT,

	INSERT_DATA_ITEM,

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
	int loop_counter;
	char* str_value;
	Location loc;
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

typedef struct HLocationMapping{
	char* key;
	HeadLocation value;
}HLocationMapping;




typedef struct {
	bool help;
	bool silent;
	bool show_lex;
	bool show_parse;
	bool step;
	bool step_interactive;
	bool step_after_line;
	size_t step_line;
	bool use_ansi;
	bool no_colour;
	bool graphviz;
	bool graphviz_conditionals;
	bool graphviz_pairs;
	bool graphviz_prefixed;
} Flags;


//FUNCTION_DEFINITION
//FUNCTION_CALL

typedef enum {
	BLOCK,
	INSTRUCTION,
	
	NUM_AST_TYPE
}
AST_TYPE;

struct Instruction;
typedef struct BlockContext {
	struct Instruction* senior;
	struct Instruction* junior;
	struct Instruction* firstInstruction;
	struct Instruction* mostRecentAddition;
} BlockContext;

typedef struct Instruction{
	AST_TYPE type;

	size_t block_id;
	//Will point to the context of the most specific block
	BlockContext* block_context;
	
	//Contains the Token information from the lexer
	Token* tk;

	//For normal commands, unconditional points to next instruction
	//For loops, unconditional and false point to next instruction after loop
	//    if_true points to the beginning of the looped block
	//For commands that create branches, if_true point to next instruction if fulfilled,
	//    and if_false points to next instruction if not fulfilled
	//    one branch gets chosen to be executed depending on the nature of the branching command
	//    after the chosen branch is executed, they will follow unconditional to the first instruction after the branches
	struct Instruction* next_unconditional;
	struct Instruction* next_if_true;
	struct Instruction* next_if_false;

} Instruction;




#define PROGRAM_MAX_TOKENS 512
#define PROGRAM_MAX_INSTRUCTIONS 512
typedef struct Program{
	DataStack stack;
	Token* tokens;
	size_t sz_tokens;

	Instruction* instruction_tree;

	HLocationMapping flag_values[128];
} Program;

Program* Railcar_Lexer(char* fileName);

void Railcar_Parser(Program* prog);

void Railcar_Simulator(Program* prog);

#endif