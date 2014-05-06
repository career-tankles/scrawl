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
		const char* word = argv[i];
		size_t idx = dawg->index(word);
		if (size_t(-1) == idx)
			printf("NOT FOUND %s\n", word);
		else
			printf("%08zd  %s\n", idx, word);
	}
	return 0;
}

