#ifndef __febird_automata_mre_match_hpp__
#define __febird_automata_mre_match_hpp__

#include <assert.h>
#include <stdio.h>
#include <febird/fstring.hpp>
#include <febird/valvec.hpp>
#include <boost/function.hpp>

class DFA_Interface;

class MultiRegexSubmatch {
protected:
	typedef unsigned char byte_t;
	typedef boost::function<byte_t(byte_t)> ByteTR;
	const DFA_Interface* dfa;
	valvec<int > m_fullmatch_regex;
	valvec<int > cap_pos_data;
	valvec<int*> cap_pos_ptr;

	size_t (*pf_match1)(MultiRegexSubmatch*, fstring);
	size_t (*pf_match2)(MultiRegexSubmatch*, fstring, const ByteTR& tr);
	void set_func(const DFA_Interface* dfa, int scanpass);

	class MultiRegexFullMatch* m_first_pass;

	void init(const DFA_Interface* dfa, int scanpass, FILE* regrexMetaFile);
	void fix_utf8_boundary(fstring text);

public:
	struct CaptureBinVal { // binary value compiled in DFA
		int32_t regex_id;
		int32_t capture_id;
	};
//	MultiRegexSubmatch();
	MultiRegexSubmatch(const DFA_Interface* dfa, int scanpass, FILE* regrexMetaFile);
	MultiRegexSubmatch(const DFA_Interface* dfa, int scanpass, const char* regrexMetaFile);
	MultiRegexSubmatch(const MultiRegexSubmatch& y);
	void warm_up();

	size_t match(fstring text) {
		return pf_match1(this, text);
	}
	size_t match(fstring text, const ByteTR& tr) {
		return pf_match2(this, text, tr);
	}
	size_t match_utf8(fstring text) {
		size_t ret = pf_match1(this, text);
		fix_utf8_boundary(text);
		return ret;
	}
	size_t match_utf8(fstring text, const ByteTR& tr) {
		size_t ret = pf_match2(this, text, tr);
		fix_utf8_boundary(text);
		return ret;
	}

	void push_regex_info(int n_submatch);
	void complete_regex_info();
	
	void reset();
	void reset(int regex_id);

	int num_submatch(int regex_id) const {
		assert(regex_id >= 0);
		assert(regex_id < (int)cap_pos_data.size());
		int* p0 = cap_pos_ptr[regex_id+0];
		int* p1 = cap_pos_ptr[regex_id+1];
		int  ns = p1 - p0;
		assert(ns % 2 == 0);
		return ns / 2;
	}

	bool is_full_match(int regex_id) const {
		assert(regex_id >= 0);
		assert(regex_id < (int)cap_pos_data.size());
		return -1 != cap_pos_ptr[regex_id][1];
	}

	std::pair<int,int>
	get_match_range(int regex_id, int sub_id) const {
		assert(regex_id >= 0);
		assert(regex_id < (int)cap_pos_data.size());
		std::pair<int,int> ij;
		ij.first  = this->get(regex_id, 2*sub_id+0);
		ij.second = this->get(regex_id, 2*sub_id+1);
		return ij;
	}

	// easy to get a submatch
	fstring operator()(const char* base, int regex_id, int sub_id) const {
		assert(regex_id >= 0);
		assert(regex_id < (int)cap_pos_data.size());
		assert(sub_id*2 < cap_pos_ptr[regex_id+1] - cap_pos_ptr[regex_id]); 
		int beg = this->get(regex_id, 2*sub_id+0);
		int end = this->get(regex_id, 2*sub_id+1);
		if (-1 == beg || -1 == end)
			return "";
		if (end <= beg)
			return "";
		return fstring(base + beg, end - beg);
	}

	const valvec<int>& fullmatch_regex() const {
		return m_fullmatch_regex;
	}

protected:
	// for internal use
	void set_capture(int regex_id, int cap_id, int pos) {
		assert(regex_id >= 0);
		assert(regex_id < (int)cap_pos_data.size());
		int* pcap = cap_pos_ptr[regex_id];
		assert(pcap + cap_id < cap_pos_ptr[regex_id+1]);
	//	printf("set_capture: regex_id=%d cap_id=%d pos=%d\n", regex_id, cap_id, pos);
		if (1 == cap_id) { // full match
			if (-1 == pcap[cap_id]) // first meet full match
				m_fullmatch_regex.push_back(regex_id);
		}
		pcap[cap_id] = pos;
	}

	int get(int regex_id, int cap_id) const {
		assert(regex_id < (int)cap_pos_data.size());
		int* pcap = cap_pos_ptr[regex_id];
		assert(pcap + cap_id < cap_pos_ptr[regex_id+1]);
		return pcap[cap_id];
	}
};

class MultiRegexFullMatch : public valvec<int> {
protected:
	typedef unsigned char byte_t;
	typedef boost::function<byte_t(byte_t)> ByteTR;
	const DFA_Interface*  m_dfa;
	class DynDFA_Context* m_ctx;
	class DenseDFA_DynDFA_320* m_dyn;

	size_t (*pf_match1)(MultiRegexFullMatch*, fstring);
	size_t (*pf_match2)(MultiRegexFullMatch*, fstring, const ByteTR&);

public:
	MultiRegexFullMatch(const DFA_Interface* dfa);
	MultiRegexFullMatch(const MultiRegexFullMatch& y);
	~MultiRegexFullMatch();

	class DenseDFA_DynDFA_320* get_dyn_dfa() const { return m_dyn; }
	void warm_up();
	void set_maxmem(size_t maxmem);
	size_t get_maxmem() const;

	size_t match(fstring text) {
		return pf_match1(this, text);
	}
	size_t match(fstring text, const ByteTR& tr) {
		return pf_match2(this, text, tr);
	}
};

#endif // __febird_automata_mre_match_hpp__

