#include "railcar.h"
#include "rc_utilities.h"
#include <assert.h>
#include <string.h>

extern Flags flags;

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

bool tk_of_type(Token* tk, TOKEN_TYPE tk_t) { return tk->type == tk_t; }
bool tk_in_type_arr(Token* tk, TOKEN_TYPE* tk_tarr, size_t sz) { return type_in(tk->type, tk_tarr, sz); }

Token* rfind_next_token_in_type_arr(Token* current, Token* stopper, TOKEN_TYPE* tk_tarr, size_t sz) {
	while (current-- != stopper) {
		if (tk_in_type_arr(current, tk_tarr, sz)) return current;
	}
	return NULL;
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
BidirectionalLinkage* make_blink(Token* senior, Token* junior) {
	BidirectionalLinkage* relation = malloc(sizeof(BidirectionalLinkage));
	relation->senior = senior;
	relation->junior = junior;
	return relation;
}
void apply_prefix(Token* prefix, Token* modifiedTk) {
	prefix->prefix_member = modifiedTk->prefix_member = make_blink(prefix, modifiedTk);
}
void apply_pair(Token* opener, Token* closer) {
	opener->pair = closer->pair = make_blink(opener, closer);
}
#define _SIZE(arr) sizeof(arr)/sizeof(arr[0])
void Railcar_Parser(Program* prog) {
	const char ERR_PREFIX[] = "PARSER";

	Token* tokens = prog->instructions;
	size_t numTokens = prog->sz_instructions;
	Token* stopper = tokens + numTokens;
	Token* rstopper = tokens - 1;

	//TODO: add error reporting function and syntax checking to call

	//TODO: lying here, need to handle all existing tokens
	assert(NUM_TOKEN_TYPE == 28 && "Unhandled Token");

	TOKEN_TYPE pairedopener[] = {OPEN_BLOCK};
	TOKEN_TYPE pairedcloser[] = {CLOSE_BLOCK};

	//Connect tokens that must be paired

	{
		Token* check_closers = tokens;
		Token* last = stopper-1;
		Token* test_open = stopper-1;
		Token* closer = stopper-1;

		//Pair all 'opener' tokens
		test_open = rfind_next_token_in_type_arr(last, rstopper, pairedopener, _SIZE(pairedopener));
		do {
			if (!test_open) break; //No more opener pairs
			TOKEN_TYPE closer_t = pair_type_lookup(test_open->type, pairedopener, pairedcloser, _SIZE(pairedopener));

			closer = find_next_token_of_type(test_open, stopper, closer_t);
			while (closer && closer->pair) {
				closer = find_next_token_of_type(closer+1, stopper, closer_t);
			}

			if (!closer) reportError(&test_open->loc, ERR_PREFIX, "%s has no closing token. Expected '%s'\n", human(closer_t));
			apply_pair(test_open, closer);
			test_open = rfind_next_token_in_type_arr(test_open, rstopper, pairedopener, _SIZE(pairedopener));

		} while(test_open != rstopper);

		//Report error if any 'closers' are unpaired
		do {
			if (tk_in_type_arr(check_closers, pairedcloser, _SIZE(pairedcloser)) && !check_closers->pair) {
				reportError(&check_closers->loc, ERR_PREFIX, "%s has no opening token. Expected '%s'\n",
					human(check_closers->type),
					human(pair_type_lookup(check_closers->type, pairedopener, pairedcloser, _SIZE(pairedopener))));
			}
		} while (check_closers++ != stopper);
	}


	//Iterate backwards through tokens which create conditional branches to cover nested conditions
	for (Token* current = tokens + numTokens - 1; current != rstopper; current--) {
		if (current->type == HEAD_READ) {
			setup_conditional_branch(current, tokens, stopper);
			apply_prefix(current, current+1);
		}
		if (current->type == CHECK_ABILITY_TO_MOVE) {
			setup_conditional_branch(current, tokens, stopper);
			apply_prefix(current, current+1);
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
		OPEN_CONDITIONAL,
		CLOSE_CONDITIONAL,
		PRINT_BYTE_AS_CHAR,
		PRINT_BYTE_AS_NUM,
		STRING
	};
	for (Token* current = tokens; current < stopper; current++) {
		//TODO: WARNING - does not properly check validity of lookahead/behind
		if (!current->next_unconditional && type_in(current->type, passthrough, sizeof(passthrough))) {
			current->next_unconditional = next_nonconditional_token(current, stopper);
		}
	}

	for (Token* current = tokens; current < stopper; current++) {
		//TODO: WARNING - does not properly check validity of lookahead/behind

		if (current->type == REPEAT_MOVE || current->type == REPEAT_MOVE_MAX || current->type == RELATIVE_MOVE) {
			apply_prefix(current, current+1);
			current->next_unconditional = current->prefix_member->junior->next_unconditional;
		}

		if (current->type == LOOP_UNTIL_END || current->type == LOOP_UNTIL_BEGINNING) {
			// current->next_if_false = rfind_next_token_of_type(current, rstopper, OPEN_BLOCK)->next_unconditional;
			current->next_if_true = find_next_token_of_type(current, stopper, CLOSE_BLOCK);
			current->next_if_false = current->next_if_true->pair->senior->next_unconditional;
		}
		if (current->type == LOOP_FIXED_AMOUNT) {
			if (!(current+1)->pair)
				reportError(&current->loc, ERR_PREFIX, "no pair to loop");
			current->next_if_false = (current+1)->pair->senior;
			current->next_if_true = current+1;
		}

		if (current->type == STAKE_FLAG) {
			apply_prefix(current, current+1);
			if ((current+1)->type != STRING) reportError(&current->loc, ERR_PREFIX, "Invalid operand for %s, expected '%s' got '%s'", human(STAKE_FLAG), human(STRING), human((current+1)->type));

			char* candidate_key = current->str_value = current->prefix_member->junior->str_value;
			HLocationMapping* iter = prog->flag_values;
			while(iter->key && strcmp(candidate_key, iter->key) != 0) {
				++iter;
			}
			if (!iter->key) iter->key=calloc(strlen(candidate_key), sizeof(char));

			strcpy(iter->key, candidate_key);
			current->next_unconditional = current->prefix_member->junior->next_unconditional;
		}

		if (current->type == RETURN_FLAG) {
			apply_prefix(current, current+1);
			if ((current+1)->type != STRING) reportError(&current->loc, ERR_PREFIX, "Invalid operand for %s, expected '%s' got '%s'", human(STAKE_FLAG), human(STRING), human((current+1)->type));
			
			char* candidate_key = current->str_value = current->prefix_member->junior->str_value;
			HLocationMapping* iter = prog->flag_values;
			while(iter->key && strcmp(candidate_key, iter->key) != 0) {
				++iter;
			}


			if (iter->key) {
				prog->stack.current_location = iter->value;
			}

			current->next_unconditional = next_nonconditional_token(current+1, stopper);
		}
		if (current->type == GOTO_BLOCK_END) {
			current->next_unconditional = find_next_token_of_type(current, stopper, CLOSE_BLOCK);
		}
		if (current->type == GOTO_BLOCK_START) {
			current->next_unconditional = rfind_next_token_of_type(current, rstopper, OPEN_BLOCK);
		}
		if (current->type == END_OF_PROGRAM && current != stopper-1) {
			reportError(&current->loc, ERR_PREFIX, "'%s' token discovered at invalid position, check Lexer\n", human(current->type));
		}
	}
}