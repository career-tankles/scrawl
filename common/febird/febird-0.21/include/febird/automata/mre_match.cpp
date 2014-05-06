#include "mre_match.hpp"
#include <febird/lcast.hpp>
#include <febird/util/linebuf.hpp>
#include <febird/util/autoclose.hpp>

//MultiRegexSubmatch::MultiRegexSubmatch() {}

MultiRegexSubmatch::MultiRegexSubmatch(const DFA_Interface* dfa, int scanpass, FILE* regexMetaFile) {
	init(dfa, scanpass, regexMetaFile);
}

MultiRegexSubmatch::MultiRegexSubmatch(const DFA_Interface* dfa, int scanpass, const char* regexMetaFile) {
	febird::Auto_fclose f(fopen(regexMetaFile, "r"));
	init(dfa, scanpass, f);
}

MultiRegexSubmatch::MultiRegexSubmatch(const MultiRegexSubmatch& y)
  : dfa(y.dfa)
  , m_fullmatch_regex(y.m_fullmatch_regex)
  , cap_pos_data(y.cap_pos_data)
  , cap_pos_ptr(y.cap_pos_ptr)
  , pf_match1(y.pf_match1)
  , pf_match2(y.pf_match2)
  , m_first_pass(NULL)
{
	if (y.m_first_pass)
		m_first_pass = new MultiRegexFullMatch(*y.m_first_pass);
}

void MultiRegexSubmatch::init(const DFA_Interface* dfa, int scanpass, FILE* regexMetaFile) {
	this->dfa = dfa;
	set_func(dfa, scanpass);
	febird::LineBuf line;
	valvec<fstring> F;
	valvec<int> offsets;
	int total_caps = 0;
	while (line.getline(regexMetaFile) > 0) {
		line.chomp();
		line.split('\t', &F);
	//	int regex_id   = febird::lcast(F[0]);
		int n_submatch = febird::lcast(F[1]);
		int is_onepass = febird::lcast(F[2]);
		offsets.push_back(total_caps);
		total_caps += is_onepass ? 2*n_submatch : 2;
	}
	offsets.push_back(total_caps);
	cap_pos_data.resize_no_init(total_caps);
	cap_pos_ptr.resize(offsets.size());
	for (size_t i = 0; i < offsets.size(); ++i)
		cap_pos_ptr[i] = cap_pos_data.data() + offsets[i];

//	printf("MultiRegexSubmatch::init: cap_pos_data.size()=%zd\n", cap_pos_data.size());
//	printf("MultiRegexSubmatch::init: cap_pos_ptr.size()=%zd\n", cap_pos_ptr.size());
}

inline bool is_utf8_tail(unsigned char c) {
	return (c & 0xC0) == 0x80;
}

///@note text must be the text used in previous match(...)
void MultiRegexSubmatch::fix_utf8_boundary(fstring text) {
	for(int regex_id : m_fullmatch_regex) {
		int* cap_beg = cap_pos_ptr[regex_id+0];
		int* cap_end = cap_pos_ptr[regex_id+1];
		assert(cap_end - cap_beg >= 2);
		// cap[0,1] is the fullmatch, needn't to trace back to utf8 boundary
		// just for submatches:
		for (int* pi = cap_beg + 2; pi < cap_end; ++pi) {
			ptrdiff_t i = *pi;
			if (i == text.n)
			   	break;
			while (i > 0 && is_utf8_tail(text.p[i])) --i;
			*pi = (int)i;
		}
	}
}

void MultiRegexSubmatch::reset() {
	m_fullmatch_regex.erase_all();
	cap_pos_data.fill(-1); // indicate all are unmatched
	ptrdiff_t n = cap_pos_ptr.size() - 1;
	for (ptrdiff_t i = 0; i < n; ++i)
		 *cap_pos_ptr[i] = 0; // the full match start position
}

void MultiRegexSubmatch::reset(int regex_id) {
	ptrdiff_t n = cap_pos_ptr.size() - 1;
	assert(regex_id < n);
	int* beg = cap_pos_ptr[regex_id+0];
	int* end = cap_pos_ptr[regex_id+1];
	beg[0] = 0; // the full match start position
	for (int* p = beg+1; p < end; ++p) *p = -1;
}

void MultiRegexSubmatch::push_regex_info(int n_submatch) {
	int* oldptr  = cap_pos_data.data();
	int  oldsize = cap_pos_data.size();
	cap_pos_ptr.push_back(oldptr + oldsize);
	cap_pos_data.resize(cap_pos_data.size() + n_submatch*2, -1);
	int* newptr = cap_pos_data.data();
	if (newptr != oldptr) {
		for (size_t i = 0; i < cap_pos_ptr.size(); ++i)
			cap_pos_ptr[i] = newptr + (cap_pos_ptr[i] - oldptr);
	}
	newptr[oldsize] = 0; // the full match start position
}

void MultiRegexSubmatch::complete_regex_info() {
	cap_pos_ptr.push_back(cap_pos_data.end());
}

/////////////////////////////////////////////////////////////////////////


