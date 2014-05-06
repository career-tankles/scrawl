#include <febird/automata/dfa_interface.hpp>
#include <febird/automata/mre_match.hpp>
#include <febird/util/linebuf.hpp>
#include <febird/util/profiling.hpp>
#include <malloc.h>

int main(int argc, char* argv[]) {
	std::auto_ptr<DFA_Interface> dfa;
	const char* dfa_file = NULL;
	const char* txt_file = NULL;
	bool verbose = false;
	for (int opt=0; (opt = getopt(argc, argv, "f:i:v")) != -1; ) {
		switch (opt) {
		case '?':
			return 1;
		case 'i':
			dfa_file = optarg;
			break;
		case 'f':
			txt_file = optarg;
			break;
		case 'v':
			verbose = true;
			break;
		}
	}
	if (NULL == dfa_file) {
		std::auto_ptr<DFA_Interface> dfa(DFA_Interface::load_from(stdin));
		MultiRegexFullMatch fm(&*dfa);
		for (int i = optind; i < argc; ++i) {
			const char* text = argv[i];
			printf("-------- %s ----------\n", text);
			int match_len = fm.match(text, ::tolower);
			for(int j = 0; j < (int)fm.size(); ++j) {
				fstring str(text, match_len);
				printf("regex_id=%d: %.*s\n", fm[j], str.ilen(), str.data());
			}
			printf("match_len=%d: %.*s\n", match_len, match_len, text);
		}
	} else {
		FILE* fp = stdin;
		if (txt_file)
			fp = fopen(txt_file, "r");
		if (NULL == fp) {
			fprintf(stderr, "fopen(%s, r) = %m\n", txt_file);
			return 1;
		}
		std::auto_ptr<DFA_Interface> dfa(DFA_Interface::load_from(dfa_file));
		MultiRegexFullMatch fm(&*dfa);
		febird::profiling pf;
		long ts = pf.now();
		fm.warm_up();
		febird::LineBuf line;
		long t0 = pf.now();
		long lineno = 0;
		long matched = 0;
		while (line.getline(fp) > 0) {
			lineno++;
			line.chomp();
			fm.match(fstring(line.p, line.n), ::tolower);
			if (verbose && fm.size()) {
				printf("line:%ld:", lineno);
				for (size_t i = 0; i < fm.size(); ++i) {
					printf(" %d", fm[i]);
				}
				printf("\n");
			}
			if (fm.size())
				matched++;
		}
		long t1 = pf.now();
		printf("time(warm_up)=%f's\n", pf.sf(ts, t0));
		printf("time=%f's lines=%ld matched=%ld QPS=%f Latency=%f'us\n"
				, pf.sf(t0,t1)
				, lineno
				, matched
				, lineno/pf.sf(t0,t1)
				, pf.uf(t0,t1)/lineno
				);
		if (stdin != fp)
			fclose(fp);
	}
	malloc_stats();
	return 0;
}

