#include "railcar.h"
#include "rc_utilities.h"
#include "data_structures.h"
#include <assert.h>
#include <string.h>

extern Flags flags;

TOKEN_TYPE branching[] = {
	HEAD_READ,
	CHECK_ABILITY_TO_MOVE
};

TOKEN_TYPE openers[] = {
	OPEN_BLOCK,
	OPEN_CONDITIONAL
};

TOKEN_TYPE closers[] = {
	CLOSE_BLOCK,
	CLOSE_CONDITIONAL
};

TOKEN_TYPE loopers[] = {
	LOOP_UNTIL_END,
	LOOP_UNTIL_BEGINNING,
	LOOP_FIXED_AMOUNT,
	LOOP_DYNAMIC_AMOUNT
};

//Note: calling make_block increments the Token pointer //TODO NOW: it doesn't increment
//Note: If Token pointer is null, the block is the token start and token end
Instruction* make_block(Token** tk, size_t* blockId) {
	Instruction* block_start = calloc(1, sizeof(Instruction));
	Instruction* block_end   = calloc(1, sizeof(Instruction));
	BlockContext* block_ctx  = calloc(1, sizeof(BlockContext));
	block_ctx->senior = block_start;
	block_ctx->junior = block_end;
	block_start->type          = block_end->type          = BLOCK;
	block_start->block_context = block_end->block_context = block_ctx;
	block_start->block_id      = block_end->block_id      = (*blockId)++;
	block_start->next_unconditional = block_end;
	if (tk) {
		block_start->tk = (*tk)++;
	}
	else {
		//maybe do something?
	}
	return block_start;
}

//At the end of the function, instructions have been connected unconditionally:
//block_start -> [previously existing instructions...] -> new instruction -> block_end
void block_add_instruction(Instruction* block_start, Instruction* next) {
	BlockContext* ctx = block_start->block_context;

	assert(NUM_AST_TYPE == 2 && "Unhandled parsing option");
	if (next->type == INSTRUCTION) {
		next->block_context = ctx; //not if next is a block
		next->block_id = block_start->block_id;
		next->next_unconditional = ctx->junior; //Point to the end of the block
	}
	else if (next->type == BLOCK) {
		//Have the end of the nested block point to the end of the enclosing block
		next->block_context->junior->next_unconditional = ctx->junior;
	}

	if (ctx->firstInstruction == NULL) {
		ctx->senior->next_unconditional = next;
		ctx->firstInstruction = ctx->mostRecentAddition = next;
	}
	else {
		ctx->mostRecentAddition->next_unconditional = next;
		ctx->mostRecentAddition = next;
	}
}

//Note: calling make_instruction increments the Token pointer //TODO NOW: it doesn't increment
Instruction* make_instruction(Token** current) {
	Instruction* next = calloc(1, sizeof(Instruction));
	next->type = INSTRUCTION;
	next->tk = (*current)++;
	return next;
}

//Forward declaration
void populate_branch(Instruction* branch, size_t* blockId, Token** current, Token** stopper);

//Handle the opening of a new context block
void _handle_open_block(Stack* blockStack, size_t* blockId, Token** current) {
	blockStack->push(blockStack, make_block(current, blockId));
}
//Handle the closing of a context block
void _handle_close_block(Stack* blockStack, Token** tk) {
	Instruction* finished_block = blockStack->pop(blockStack);
	finished_block->block_context->junior->tk = (*tk)++;
	Instruction* wrapping_block = blockStack->top(blockStack);
	block_add_instruction(wrapping_block, finished_block);
}
//Handle instructions which do not create any branching
void _handle_no_branch(Stack* blockStack, Token** current) {
	Instruction* next  = make_instruction(current);
	Instruction* block = blockStack->top(blockStack);
	block_add_instruction(block, next);
}
//Handle instructions which do create branches in the execution path
void _handle_branch(Stack* blockStack, size_t* blockId, Token** current, Token** stopper) {
	Instruction* next  = make_instruction(current);
	Instruction* block = blockStack->top(blockStack);
	block_add_instruction(block, next);

	Instruction* false_block = next->next_if_false = make_block(current, blockId);
	Instruction* true_block  = next->next_if_true  = make_block(current, blockId);
	false_block->block_context->junior->next_unconditional = next;
	true_block->block_context->junior->next_unconditional = next;

	populate_branch(false_block, blockId, current, stopper);
	populate_branch(true_block,  blockId, current, stopper);
}

void _handle_loop(Stack* blockStack, size_t* blockId, Token** current, Token** stopper) {
	Instruction* next  = make_instruction(current);
	Instruction* enclosing_block = blockStack->top(blockStack);
	block_add_instruction(enclosing_block, next);

	next->next_if_false = next->next_unconditional;
	Instruction* looped_block = next->next_if_true = make_block(current, blockId);
	populate_branch(looped_block, blockId, current, stopper);
}

void populate_branch(Instruction* branch, size_t* blockId, Token** current, Token** stopper) {
	Stack* blocks = initializeStack(sizeof(Instruction*));
	blocks->push(blocks, branch);

	//Loop and populate
	while (*current < *stopper) {
		if (tk_in_type_arr(*current, openers, ARR_SZ(openers))) {
			_handle_open_block(blocks, blockId, current);
		}
		else if (tk_in_type_arr(*current, closers, ARR_SZ(closers))) {
			if (blocks->size == 1) {
				//We have found the end of this conditional branch and can stop processing
				//(or malformed pairs got past the Lexer)
				current++;
				break;
			}
			_handle_close_block(blocks, current);
		}
		else if (tk_in_type_arr(*current, loopers, ARR_SZ(loopers))) {
			_handle_loop(blocks, blockId, current, stopper);
		}
		else if (!tk_in_type_arr(*current, branching, ARR_SZ(branching))) {
			_handle_no_branch(blocks, current);
		}
		else {
			_handle_branch(blocks, blockId, current, stopper);
		}
		current++;
	}
}

void Railcar_Parser(Program* prog) {
	const char ERR_PREFIX[] = "PARSER";

	Token* tokens  = prog->tokens;
	Token* stopper = prog->tokens + prog->sz_tokens;

	size_t blockId = 0;

	assert(NUM_TOKEN_TYPE == 30 && "Unhandled Token");

	Instruction* full_ast = make_block(NULL, &blockId);	
	populate_branch(full_ast, &blockId, &tokens, &stopper);
	prog->instruction_tree = full_ast;
}