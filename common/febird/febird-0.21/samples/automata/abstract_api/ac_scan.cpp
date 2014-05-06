#include <febird/automata/dfa_interface.hpp>
#include <getopt.h>
#include <malloc.h>

struct OnHit {
	void operator()(size_t endpos, const uint32_t* words, size_t cnt) {
		for (size_t i = 0; i < cnt; ++i) {
			int wlen = ac->wlen(words[i]);
			size_t pos = endpos - wlen;
			printf("hit_endpos=%06zd : word_id=%06d : %.*s\n", endpos, words[i], wlen, text+pos);
		}
	}
	const AC_Scan_Interface* ac;
	const char* text;
};

int main(int argc, char* argv[]) {
	bool use_mmap = false;
	const char* finput = NULL;
	for (int opt=0; (opt = getopt(argc, argv, "d::i:m")) != -1; ) {
		switch (opt) {
			case '?':
				return 3;
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
	OnHit on_hit;
	on_hit.ac = dynamic_cast<AC_Scan_Interface*>(&*dfa);
	if (NULL == on_hit.ac) {
		fprintf(stderr, "file: %s is not a AC DFA\n", finput?finput:"stdin");
		return 1;
	}
	for (int i = optind; i < argc; ++i) {
		on_hit.text = argv[i];
		on_hit.ac->v_scan(on_hit.text, on_hit);
	}
	return 0;
}

