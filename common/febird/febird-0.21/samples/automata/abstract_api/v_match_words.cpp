#include <febird/automata/dfa_interface.hpp>
#include <getopt.h>
#include <malloc.h>

struct OnMatch {
	void operator()(int len, int idx) const {
		printf("%08d  %.*s\n", idx, len, word);
	}
	const char* word;
};

int main(int argc, char* argv[]) {
	std::auto_ptr<DFA_Interface> dfa(DFA_Interface::load_from(stdin));
	DAWG_Interface* dawg = dynamic_cast<DAWG_Interface*>(dfa.get());
	if (NULL == dawg) {
		fprintf(stderr, "not a dawg\n");
		return 1;
	}
	OnMatch on_match;
	for (int i = 1; i < argc; ++i) {
		on_match.word = argv[i];
		int len0 = dawg->v_match_words(on_match.word, on_match);
		printf("len0=%d: %.*s\n", len0, len0, on_match.word);
	}
	return 0;
}

