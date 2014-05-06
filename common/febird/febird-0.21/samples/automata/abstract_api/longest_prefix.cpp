#include <febird/automata/dfa_interface.hpp>
#include <getopt.h>
#include <malloc.h>

int main(int argc, char* argv[]) {
	std::auto_ptr<DFA_Interface> dfa(DFA_Interface::load_from(stdin));
	DAWG_Interface* dawg = dynamic_cast<DAWG_Interface*>(dfa.get());
	if (NULL == dawg) {
		fprintf(stderr, "not a dawg\n");
		return 1;
	}
	for (int i = 1; i < argc; ++i) {
		fstring word = argv[i];
		size_t len, idx;
		if (dawg->longest_prefix(word, &len, &idx)) {
			printf("longest_prefix: len=%zd idx=%zd word=%.*s\n", len, idx, int(len), word.p);
		}
	}
	return 0;
}

