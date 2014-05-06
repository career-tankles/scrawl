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
		const char* szidx = argv[i];
		size_t idx = strtoul(szidx, NULL, 10);
		std::string word = dawg->nth_word(idx);
		printf("%08zd  %s\n", idx, word.c_str());
	}
	return 0;
}

