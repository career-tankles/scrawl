#ifndef __febird_automata_string_as_dfa_hpp__
#define __febird_automata_string_as_dfa_hpp__

#include <string>
#include <limits.h>

class StringAsDFA {
public:
	std::string str;

	StringAsDFA() {}
	explicit StringAsDFA(const std::string& s) : str(s) {}

	typedef unsigned state_id_t;
	typedef unsigned transition_t;
//	struct state_t;
	static const state_id_t    null_state = UINT_MAX;
	static const int sigma = 256;

	size_t total_states() const { return str.size() + 1; }

	bool is_term(state_id_t s) const {
		assert(s <= str.size());
	   	return s == str.size();
	}

	template<class OP>
	void for_each_move(state_id_t curr, OP op) const {
		if (str.size() != curr)
			op(curr, curr+1, str[curr]);
	}
	template<class OP>
	void for_each_dest(state_id_t curr, OP op) const {
		if (str.size() != curr)
			op(curr, curr+1);
	}
	state_id_t add_move(state_id_t source, state_id_t target, unsigned char ch) {
		assert(source <= str.size());
		assert(target <= str.size());
		return target;
	}
#if 0
	state_id_t set_move(state_id_t source, state_id_t target, unsigned char ch) {
		assert(source <= str.size());
		assert(target <= str.size());
		assert(0);
		throw std::logic_error("calling StringAsDFA::set_move");
	}
#endif
	state_id_t state_move(state_id_t curr, unsigned char ch) {
		assert(curr <= str.size());
		if (curr < str.size() && ch == (unsigned char)str[curr])
			return curr + 1;
		else
			return null_state;
	}
	bool has_children(state_id_t s) const {
        assert(s <= str.size());
		return s != str.size();
	}

	void print_output(FILE* fp) {
		fprintf(fp, "%s\n", str.c_str());
	}
	void print_output(const char* fname) {
		FILE* fp = fopen(fname, "w");
		if (NULL == fp) {
			string_appender<> msg;
			msg << "StringAsDFA::print_output: " << fname;
			throw std::runtime_error(msg);
		}
		print_output(fp);
		fclose(fp);
	}

	bool is_pzip(size_t i) const {
		assert(i <= str.size());
		return false;
	}
	const unsigned char* get_zpath_data(size_t) const {
		assert(0);
		abort();
		return NULL;
	}

	struct dfs_walker {
		state_id_t curr;
		state_id_t nstr;
		void resize(size_t NumStates) {	nstr = NumStates; }
		void put_root(state_id_t RootState) { curr = RootState; }
		bool is_finished() const {
			return nstr < curr;
		}
		state_id_t next() {
			assert(curr <= nstr);
		   	return curr;
	   	}
		void putChildren(const StringAsDFA* au, state_id_t parent) {
			assert(au->str.size() >= curr);
			assert(au->str.size() == nstr);
		   	curr = parent + 1;
	   	}
	};
	typedef dfs_walker bfs_walker, pfs_walker;
};


#endif // __febird_automata_string_as_dfa_hpp__

