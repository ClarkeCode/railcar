#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "railcar.h"
#include "rc_utilities.h"


Flags flags = {0};
Flags empty_flags = {0};








void show_usage(FILE* fp) {
	fprintf(fp, "USAGE: ./railcar.exe [options] <subcommand> filename\n");
	fprintf(fp, "\nSUBCOMMANDS:\n");
	fprintf(fp, "	step           //step through tokens one-at-a-time");
	fprintf(fp, "\nOPTIONS:\n");
	fprintf(fp, "	--h, --help    //Display this help message\n");
	fprintf(fp, "	--s, --silent  //Do not display output messages\n");
	fprintf(fp, "	--show-lex     //Show the lexing of the file\n");
	fprintf(fp, "	--show-parse   //Show the parsing of the file\n");
	fprintf(fp, "	--i            //interactive stepping\n");
	fprintf(fp, "	--gv           //Generate Graphviz flow-control diagrams\n"); //TODO: may be subcommand
	fprintf(fp, "	--no-colour    //turn off the pretty colours\n");
	fprintf(fp, "	--no-ansi      //turn off all use of ANSI escape codes\n");
}

bool isNum(void* a, void* b) {
	return (*(int*)a) == (*(int*)b);
}
int main(int argc, char* argv[]){
	Deque* deque = initializeDeque(sizeof(int));

	int i = 12;
	deque->insert(deque, &i, 0);
	i = 13;
	deque->insert(deque, &i, 1);
	i = 14;
	deque->insert(deque, &i, 0);

	deque->erase_front(deque);
	deque->erase_back(deque);

	i = 17;
	deque->push_front(deque, &i);
	i = 22;
	deque->push_back(deque, &i);
	

	printf("DQ: %d %d\n", DQ_UNPACK(int) deque->front(deque), DQ_UNPACK(int) deque->back(deque));
	int y = 14;
	printf("14 = %d, 22 = %d\n", deque->in(deque, &y, isNum), deque->in(deque, &i, isNum));

	freeDeque(deque);
}

int main2(int argc, char* argv[]) {
	argc--; argv++; //Discard own program name

	char* fileName = NULL;
	flags.use_ansi = true;
	while (argc != 0) {
		char* item = *argv++; argc--;
		if      (strcmp(item, "--show-lex") == 0) flags.show_lex = true;
		else if (strcmp(item, "--show-parse") == 0) flags.show_parse = true;
		else if (strncmp(item, "--step", 6) == 0) {
			flags.step = true;
			if (strlen(item+6) != 0) {
				flags.step = flags.step_interactive = false;
				flags.step_after_line = true;
				flags.step_line = (size_t)atoi(item+6);
			}
		}
		else if ((strcmp(item, "--h") == 0 || strcmp(item, "--help") == 0)) flags.help = true;
		else if ((strcmp(item, "--s") == 0 || strcmp(item, "--silent") == 0)) flags.silent = true;
		else if (strcmp(item, "--i") == 0) flags.step_interactive = true;
		else if (strcmp(item, "--no-colour") == 0) flags.no_colour = true;
		else if (strcmp(item, "--no-ansi") == 0) flags.use_ansi = false;
		else if (strncmp(item, "--gv", 4) == 0) {
			flags.graphviz = true;
			if (strchr(item+4, 'C') != NULL) flags.graphviz_conditionals = true;
			if (strchr(item+4, 'P') != NULL) flags.graphviz_pairs = true;
			if (strchr(item+4, 'X') != NULL) flags.graphviz_prefixed = true;
		}

		else {
			fileName = item;
		}
	}
	if (!fileName || flags.help) {
		fprintf(stderr, "ERROR: No file provided\n");
		show_usage(stderr);
		exit(EXIT_FAILURE);
	}
	if (flags.silent) { flags = empty_flags; }
	

	//Lexer
	if (flags.show_lex) ("Lexing: %s\n", fileName);
	Program* prog = Railcar_Lexer(fileName);
	if (flags.show_lex) dump_program(stdout, prog);
	
	//Parser
	Railcar_Parser(prog);
	if (flags.show_parse) dump_program(stdout, prog);

	if (flags.graphviz) {
		FILE* fp = fopen("output.dot", "w");
		if (fp) dump_tokens_to_dotfile(fp, prog->instructions, prog->sz_instructions);
		fclose(fp);

		shellEcho(".\\vendors\\Graphviz\\bin\\dot.exe -Tpng output.dot -O");
		shellEcho(".\\vendors\\Graphviz\\bin\\dot.exe -Tsvg output.dot -O");
	}

	//Simulator
	if (flags.step) {
		printf("Stepper\n");
		// if (flags.use_ansi) { printf("\x1b[s***"); }//Save cursor position
	}
	Railcar_Simulator(prog);

	if (false) {
		//TODO: work on use of ANSI escapes
		//Resources:
		//	https://solarianprogrammer.com/2019/04/08/c-programming-ansi-escape-codes-windows-macos-linux-terminals/
		//	https://www.lihaoyi.com/post/BuildyourownCommandLinewithANSIescapecodes.html#cursor-navigation
		//	https://rosettacode.org/wiki/Terminal_control/Cursor_positioning#C.2FC.2B.2B
		//	https://youtu.be/kLj-H1K317U?t=4855
		printf("ABCDEF");
		// printf("\x1b[4C\x1b[4AGHIJK\n");
		printf("\x1b[32mHello, World\n");
		printf("TWELVE");
		printf("\x1b[1000D\x1b[0K");
		// printf("\x1b[2J");
		printf("Savage\n");
		// printf("\x1b[2J");  //Clear the screen
		// Reset colors to defaults
		printf("\x1b[0mtesting\n");
	}
	return EXIT_SUCCESS;
}