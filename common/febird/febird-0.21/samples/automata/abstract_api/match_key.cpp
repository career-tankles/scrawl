#include <febird/automata/dfa_interface.hpp>
#include <getopt.h>
#include <malloc.h>

struct OnMatch {
	void operator()(int len, int idx, fstring value) {
		if (strnlen(value.p, value.n) < value.size()) {
			printf("%-20.*s idx=%08d bin=", len, text, idx);
			for (int i = 0; i < value.n; ++i)
				printf("%02X", (byte_t)value.p[i]);
			printf("\n");
		}
		else {
			printf("%-20.*s idx=%08d val=%.*s\n"
				, len, text, idx, value.ilen(), value.data());
		}
		klen = len;
	}
	const char* text;
	int klen;
};

int main(int argc, char* argv[]) {
	int delim = '\t';
	bool use_mmap = false;
	const char* finput = NULL;
	for (int opt=0; (opt = getopt(argc, argv, "d::i:m")) != -1; ) {
		switch (opt) {
			case '?':
				return 3;
			case 'd':
				if (optarg)
					delim = optarg[0];
				else // no arg for -d, use binary key-val match
					delim = 256; // default delim for binary key-val match
				break;
			case 'm':
				use_mmap = true;
				break;
			case 'i':
				finput = optarg;
				break;
		}
	}
	std::auto_ptr<DFA_Interface> dfa;
	if (finput) {
		if (use_mmap)
			dfa.reset(DFA_Interface::load_mmap(finput));
		else
			dfa.reset(DFA_Interface::load_from(finput));
	}
	else {
		if (use_mmap)
			dfa.reset(DFA_Interface::load_mmap(STDIN_FILENO));
		else
			dfa.reset(DFA_Interface::load_from(stdin));
	}

	// explicit define a boost::function object avoid temporary function object
	// and improve performance
	OnMatch on_match0;
	boost::function<void(size_t keylen, size_t value_idx, fstring value)>
		on_match(boost::ref(on_match0));
	for (int i = optind; i < argc; ++i) {
		const char* text = argv[i];
		on_match0.text = text;
		on_match0.klen = 0;
		printf("%s ----------\n", text);
		int max_partial_match_len = dfa->match_key(delim, text, on_match);
		if (on_match0.klen != max_partial_match_len)
			printf("max_partial_match_len=%d: %.*s\n", max_partial_match_len, max_partial_match_len, text);
	}
	return 0;
}

