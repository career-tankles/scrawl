#include <febird/automata/dfa_interface.hpp>
#include <febird/automata/mre_match.hpp>
#include <febird/util/linebuf.hpp>
#include <febird/util/profiling.hpp>
#include <malloc.h>
#include <boost/bind.hpp>
#include <tbb/tbb_thread.h>
#include <tbb/mutex.h>

long  g_lineno;
FILE* g_fp;
tbb::mutex g_mutex;

class MatchThread {
	MultiRegexFullMatch fm;
	febird::LineBuf   line;
public:
	struct Match {
		long lineno;
		long offset;
	};
	tbb::tbb_thread* thr;
	valvec<int> regex_id_vec;
	valvec<Match> matched;

	MatchThread(const MultiRegexFullMatch& src) : fm(src) {}
	void run() {
		matched.reserve(8*1024);
		regex_id_vec.reserve(32*1024);
		regex_id_vec.resize(0);
		for (;;) {
			Match m;
			{
				tbb::mutex::scoped_lock lock(g_mutex);
				line.getline(g_fp);
				if (line.n < 0) break;
				m.lineno = ++g_lineno;
			}
			line.chomp();
			fm.match(fstring(line.p, line.n), ::tolower);
			if (fm.size()) {
				m.offset = regex_id_vec.size();
				matched.push_back(m);
				regex_id_vec.append(fm);
			}
		}
		matched.push_back();
		matched.back().offset = regex_id_vec.size();
	}
};

int main(int argc, char* argv[]) {
	const char* dfa_file = NULL;
	const char* txt_file = NULL;
	bool verbose = false;
	int num_threads = 1;
	for (int opt=0; (opt = getopt(argc, argv, "f:i:T:v")) != -1; ) {
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
		case 'T':
			num_threads = atoi(optarg);
			if (num_threads <= 0)
				num_threads = 1;
			break;
		}
	}
	if (NULL == dfa_file) {
		fprintf(stderr, "-i dfa_file is required\n");
		return 1;
	}
	g_fp = stdin;
	if (txt_file)
		g_fp = fopen(txt_file, "r");
	if (NULL == g_fp) {
		fprintf(stderr, "fopen(%s, r) = %m\n", txt_file);
		return 1;
	}
	fprintf(stderr, "num_threads=%d\n", num_threads);
	std::auto_ptr<DFA_Interface> dfa(DFA_Interface::load_from(dfa_file));
	MultiRegexFullMatch fm(&*dfa);
	valvec<MatchThread*> threads(num_threads);
	febird::profiling pf;
	long t0 = pf.now();
	fm.warm_up();
	long t1 = pf.now();
	for (int i = 0; i < num_threads; ++i) {
		MatchThread* mt = new MatchThread(fm);
		tbb::tbb_thread* tt = new tbb::tbb_thread(
				boost::bind(&MatchThread::run, mt));
		mt->thr = tt;
		threads[i] = mt;
	}
	long matched = 0;
	for (int i = 0; i < num_threads; ++i) {
		MatchThread* mt = threads[i];
		mt->thr->join();
		for(size_t j = 0; j < mt->matched.size()-1; ++j) {
			long lineno = mt->matched[j+0].lineno;
			long klower = mt->matched[j+0].offset;
			long kupper = mt->matched[j+1].offset;
			printf("line:%ld:", lineno);
			for(long k = klower; k < kupper; ++k) {
				printf(" %d", mt->regex_id_vec[k]);
			}
			printf("\n");
		}
		matched += mt->matched.size()-1;
	}
	long t2 = pf.now();
	printf("time(warm_up)=%f's\n", pf.sf(t0, t1));
	printf("time=%f's lines=%ld matched=%ld QPS=%f Latency=%f'us\n"
			, pf.sf(t1,t2)
			, g_lineno
			, matched
			, g_lineno/pf.sf(t1,t2)
			, pf.uf(t1,t2)/g_lineno
			);
	if (stdin != g_fp)
		fclose(g_fp);
	malloc_stats();
	return 0;
}

