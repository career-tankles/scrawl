#ifndef __febird_automata_dfa_algo_hpp__
#define __febird_automata_dfa_algo_hpp__

#if defined(__GNUC__)
	#if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 7
	#elif defined(__clang__)
	#else
		#error "Requires GCC-4.7+"
	#endif
#endif

#include <boost/function.hpp>
#include <string>

#include <febird/fstring.hpp>
#include <febird/valvec.hpp>
#include <febird/mempool.hpp>
#include <febird/num_to_str.hpp>

#include "dfa_interface.hpp"
#include "nfa.hpp"
#include "graph_walker.hpp"
#include "hopcroft.hpp"
#include "onfly_minimize.hpp"

template<class Au, class OnNthWord>
struct ForEachWord_DFS : private boost::noncopyable {
	const Au* au;
	std::string word;
	size_t      nth;
	OnNthWord on_nth_word;
	typedef typename Au::state_id_t state_id_t;
	ForEachWord_DFS(const Au* au1, OnNthWord op)
	 : au(au1), nth(0), on_nth_word(op) {
	}
	void start(state_id_t RootState, size_t SkipRootPrefixLen) {
		nth = 0;
		word.resize(0);
		enter(RootState, SkipRootPrefixLen);
		au->template for_each_move<ForEachWord_DFS&>(RootState, *this);
	}
	size_t enter(state_id_t tt, size_t SkipLen) {
		size_t slen = 0;
		if (au->is_pzip(tt)) {
			const byte_t* s2 = au->get_zpath_data(tt);
			slen = *s2++;
			assert(SkipLen <= slen);
			word.append((const char*)s2 + SkipLen, slen - SkipLen);
		} else {
			assert(0 == SkipLen);
		}
		if (au->is_term(tt)) {
			on_nth_word(nth, word, tt);
			++nth;
		}
		return slen;
	}
	void operator()(state_id_t, state_id_t tt, char_t ch) {
		word.push_back(ch);
		const size_t slen = enter(tt, 0) + 1;
		au->template for_each_move<ForEachWord_DFS&>(tt, *this);
		word.resize(word.size() - slen); // pop_back
	}
};

template<class StateID>
struct CompactReverseNFA {
	void clear() {
		index.clear();
		data.clear();
	}
	valvec<StateID> index; // parallel with Automata::states
	valvec<CharTarget<StateID> > data;
};

struct IdentityTranslator {
	byte_t operator()(byte_t c) const { return c; }
};
struct TableTranslator {
	const byte_t* tr_tab;
	byte_t operator()(byte_t c) const { return tr_tab[c]; }
	TableTranslator(const byte_t* tr_tab1) : tr_tab(tr_tab) {}
};

template<class Au>
class DFA_Algorithm {
	const Au* const au;

public:
	DFA_Algorithm(const Au* au1) : au(au1) {}

	typedef typename Au::state_id_t state_id_t;
	typedef typename Au::transition_t transition_t;

    static const state_id_t  max_state = Au:: max_state;
    static const state_id_t null_state = Au::null_state;
	
    enum class Color : char {
        White,
        Grey,
        Black,
    };
    bool has_cycle_loop(state_id_t curr, Color* mark) const {
        if (Color::Grey == mark[curr])
            return true;
        if (Color::Black == mark[curr])
            return false;
        mark[curr] = Color::Grey;
        bool hasCycle = false;
        au->for_each_dest(curr, [=,&hasCycle](state_id_t, state_id_t t) {
            hasCycle = hasCycle || this->has_cycle_loop(t, mark);
        });
        mark[curr] = Color::Black;
        return hasCycle;
    }

	/// @return paths_from_state[curr], SIZE_T_MAX indicate path num overflow
    size_t dag_pathes_loop(state_id_t curr, Color* mark
                       , size_t* paths_from_state) const {
	//	printf("curr=%ld\n", (long)curr);
		const size_t SIZE_T_MAX = std::numeric_limits<size_t>::max();
        assert(curr < (state_id_t)au->total_states());
        if (Color::Grey == mark[curr]) {
            paths_from_state[curr] = SIZE_T_MAX;
            paths_from_state[initial_state] = SIZE_T_MAX;
			assert(0);
			throw std::logic_error("DFA_Algorithm::dag_pathes: encountered a loop");
        }
        if (Color::Black == mark[curr]) {
			assert(paths_from_state[curr] != 0);
            return paths_from_state[curr];
        }
        mark[curr] = Color::Grey;
        size_t num = 0;
		try {
			au->for_each_dest(curr, [&](state_id_t curr, state_id_t dest) {
				size_t subs = dag_pathes_loop(dest, mark, paths_from_state);
				if (num + subs < num) { // overflow
					num = SIZE_T_MAX;
					throw int(0);
				}
                num += subs;
			});
		} catch (int) {
			goto Done;
		}
		if (num < SIZE_T_MAX) {
			if (au->is_term(curr))
				num++;
		}
	Done:
		assert(num >= 1);
	//	assert(num < SIZE_T_MAX);
	//	printf("**curr=%ld, num=%ld\n", (long)curr, (long)num);
        paths_from_state[curr] = num;
        mark[curr] = Color::Black;
		return num;
    }

public:
    bool is_dag() const {
        valvec<Color> mark(au->total_states(), Color::White);
        return !has_cycle_loop(initial_state, &mark[0]);
    }
    // return ~size_t(0) indicate there is some loop in the state graph
    size_t dag_pathes0() const {
		assert(au->num_used_states() >= 1);
        valvec<Color> mark(au->total_states(), Color::White);
        valvec<size_t> paths_from_state(au->total_states());
        return dag_pathes_loop(initial_state, &mark[0], &paths_from_state[0]);
    }
	size_t dag_pathes() const {
		valvec<size_t> pathes;
		return dag_pathes(pathes);
	}
	size_t dag_pathes(valvec<size_t>& pathes) const {
		assert(au->num_used_states() >= 1);
		pathes.resize_no_init(au->total_states());
#ifndef NDEBUG
		pathes.fill(size_t(-2));
#endif
		topological_sort([&](state_id_t parent) {
			assert(size_t(-2) == pathes[parent]);
			size_t np = au->is_term(parent) ? 1 : 0;
			au->for_each_dest(parent,
				[&](state_id_t, state_id_t child) {
					assert(size_t(-2) != pathes[child]);
					size_t nc = pathes[child];
					if (size_t(-1) != np) {
						if (size_t(-1) == nc || np + nc < np)
							np = size_t(-1);
						else
							np += pathes[child];
					}
				});
			pathes[parent] = np;
		});
		return pathes[0];
	}

    // No mark bits, optimized for tree shape Automata
    template<class Visitor>
    void bfs_tree(state_id_t RootState, Visitor vis) const {
        valvec<state_id_t> q1, q2;
        q1.push_back(RootState);
		do {
			for (size_t i = 0; i < q1.size(); ++i) {
				state_id_t source = q1[i];
				au->for_each_move(source,
					[&](state_id_t parent, state_id_t child, char_t ch) {
						vis(parent, child, ch);
						q2.push_back(child);
					});
			}
			q1.swap(q2);
			q2.erase_all();
		} while (!q1.empty());
    }

	template<class OnFinish>
	void topological_sort(OnFinish on_finish) const {
		topological_sort2(initial_state, on_finish);
	}
	template<class OnFinish>
	void topological_sort(state_id_t RootState, OnFinish on_finish) const {
		topological_sort2(RootState, on_finish);
	}
	template<class OnFinish, class OnBackEdge>
	void dfs_post_order_walk(state_id_t RootState, OnFinish on_finish, OnBackEdge on_back_edge) const {
		// non-recursive topological sort
		assert(RootState < au->total_states());
		valvec<Color> mark(au->total_states(), Color::White);
		valvec<state_id_t> stack;
		stack.reserve(1024);
		stack.push_back(RootState);
		while (!stack.empty()) {
			state_id_t curr = stack.back();
			switch (mark[curr]) {
			default:
			   	assert(0);
			   	break;
			case Color::White: // not expanded yet
				mark[curr] = Color::Grey;
				au->for_each_dest_rev(curr,
				[&](state_id_t parent, state_id_t child) {
					switch (mark[child]) {
					default:
						assert(0);
						break;
					case Color::White: // tree edge
						// child may have multiple copies in stack
						// but this doesn't matter, just 1 will be expanded
						stack.push_back(child);
						break;
					case Color::Grey: // back edge
						on_back_edge(parent, child);
						break;
					case Color::Black: // cross edge
						break;
					}
				});
				break;
			case Color::Grey:
				mark[curr] = Color::Black;
				on_finish(curr);
				// fall through:
			case Color::Black: // forward edge
				stack.pop_back();
				break;
			}
		}
	}

	// used for multiple-pass topological sort start from different root
	template<class ColorID, class OnFinish>
	void topological_sort( valvec<state_id_t>& stack
						 , valvec<ColorID>& color
						 , OnFinish on_finish
						 ) const {
		// multiple pass non-recursive topological sort
		assert(color.size() >= au->total_states()+1);
		assert(stack.size() >= 1); // could has multiple roots
		auto on_back_edge = [&](state_id_t parent, state_id_t child) {
			fstring func = BOOST_CURRENT_FUNCTION;
			if (parent == child)
				throw std::logic_error(func + ": self-loop");
			else
				throw std::logic_error(func + ": non-self-loop");
		};
		dfs_post_order_walk(stack, color, on_finish, on_back_edge);
	}

	// used for multiple-pass dfs_post_order_walk start from different root
	// color[0] is last ColorID sequencial number
	template<class ColorID, class OnFinish, class OnBackEdge>
	void dfs_post_order_walk( valvec<state_id_t>& stack
							, valvec<ColorID>& color
							, OnFinish on_finish
							, OnBackEdge on_back_edge
							) const {
		// multiple pass non-recursive post order walk
		assert(color.size() >= au->total_states()+1);
		assert(stack.size() >= 1); // could has multiple roots
	#ifndef NDEBUG
		for (size_t i = 0; i < stack.size(); ++i)
			assert(stack[i] < au->total_states());
	#endif
		ColorID last_id = color[0];
		while (!stack.empty()) {
			state_id_t curr = stack.back();
			assert(color[curr+1] <= last_id+2);
			if (color[curr+1] <= last_id) { // color is white
				color[curr+1]  = last_id+1; // set grey
				au->for_each_dest_rev(curr,
				[&](state_id_t parent, state_id_t child) {
					if (color[child+1] <= last_id) {
						// color is white: tree edge
						// child may have multiple copies in stack
						// but this doesn't matter, just 1 will be expanded
						stack.push_back(child);
					}
					else if (last_id+1 == color[child+1]) {
						// color is grey: back edge
						on_back_edge(parent, child);
					}
					else { // color must be black: cross edge
						assert(last_id+2 == color[child+1]);
					}
				});
			}
			else if (last_id+1 == color[curr+1]) {
				// case Color::Grey:
				color[curr+1] = last_id+2; // set black
				on_finish(curr);
				stack.pop_back();
			}
			else {
				// case Color::Black: // forward edge
				assert(last_id+2 == color[curr+1]);
				stack.pop_back();
			}
		}
		color[0] += 2; // every walk need 3 different color id
	}

	template<class OnFinish>
	void topological_sort2(state_id_t RootState, OnFinish on_finish) const {
		// non-recursive topological sort
		assert(RootState < au->total_states());
		auto on_back_edge = [&](state_id_t parent, state_id_t child) {
			fstring func = BOOST_CURRENT_FUNCTION;
			if (parent == child)
				throw std::logic_error(func + ": self-loop");
			else
				throw std::logic_error(func + ": non-self-loop");
		};
		dfs_post_order_walk(RootState, on_finish, on_back_edge);
	}
	template<class OnFinish>
	void topological_sort0(state_id_t RootState, OnFinish on_finish) const {
		assert(RootState < au->total_states());
		valvec<Color> mark(au->total_states(), Color::White);
		topological_sort_loop(&mark[0], on_finish, RootState);
	}
private:
	template<class OnFinish>
	void topological_sort_loop(Color* mark, OnFinish on_finish, state_id_t curr) const {
		mark[curr] = Color::Grey;
		au->for_each_dest(curr, [&](state_id_t, state_id_t child) {
				if (Color::White == mark[child])
					this->topological_sort_loop(mark, on_finish, child);
				else if (Color::Grey == mark[child])
					throw std::logic_error("DFA_Algorithm::topological_sort: found loop");
			});
		mark[curr] = Color::Black;
		on_finish(curr);
	}

private:
	static FILE* fopen_output(const char* fname) {
		FILE* fp;
	   	if (strcmp(fname, "-") == 0 ||
			strcmp(fname, "/dev/fd/1") == 0 ||
			strcmp(fname, "/dev/stdout") == 0) {
				fp = stdout;
		} else {
			fp = fopen(fname, "w");
			if (NULL == fp) {
				fprintf(stderr, "%s:%d can not open \"%s\", err=%s\n"
					   , __FILE__, __LINE__, fname, strerror(errno));
				throw std::runtime_error(fname);
			}
		}
		return fp;
	}

public:
	/// @param on_nth_word(size_t nth, fstring word, state_id_t accept_state)
	///    calling of on_nth_word is in lexical_graphical order,
	///    nth is the ordinal, 0 based
	/// @returns  number of all words
	template<class OnNthWord>
	size_t for_each_word(OnNthWord on_nth_word) const {
		ForEachWord_DFS<Au, OnNthWord> vis(au, on_nth_word);
		vis.start(initial_state, 0);
		return vis.nth;
	}
	template<class OnNthWord>
	size_t for_each_word(state_id_t RootState, OnNthWord on_nth_word) const {
		ForEachWord_DFS<Au, OnNthWord> vis(au, on_nth_word);
		vis.start(RootState, 0);
		return vis.nth;
	}

	/// @param on_suffix(size_t nth, fstring suffix, state_id_t accept_state)
	///    calling of on_nth_word is in lexical_graphical order,
	///    nth is the ordinal, 0 based
	/// @return number of suffixes
	template<class OnSuffix>
	size_t for_each_suffix(fstring prefix, OnSuffix on_suffix) const {
		return for_each_suffix<OnSuffix>(initial_state, prefix, on_suffix);
	}
	template<class OnSuffix, class TR>
	size_t for_each_suffix(fstring prefix, OnSuffix on_suffix, TR tr) const {
		return for_each_suffix<OnSuffix, TR>(initial_state, prefix, on_suffix, tr);
	}
	template<class OnSuffix>
	size_t for_each_suffix(state_id_t RootState, fstring prefix, OnSuffix on_suffix) const {
		return for_each_suffix<OnSuffix>(RootState, prefix, on_suffix, IdentityTranslator());
	}
	template<class OnSuffix, class TR>
	size_t for_each_suffix(state_id_t RootState, fstring prefix, OnSuffix on_suffix, TR tr) const {
		assert(RootState < au->total_states());
		ASSERT_isNotFree(RootState);
		state_id_t curr = RootState;
		size_t prelen = 0;
		for (size_t i = 0; ; ++i) {
			if (au->is_pzip(curr)) {
				const byte_t* s2 = au->get_zpath_data(curr);
				size_t n2 = *s2++;
				size_t n  = std::min<size_t>(n2, prefix.n-i);
				for (size_t j = 0; j < n; ++i, ++j)
					if ((byte_t)tr(prefix.p[i]) != s2[j]) return 0;
				prelen = n;
			} else
				prelen = 0;
			assert(i <= prefix.size());
			if (prefix.size() == i)
				break;
			state_id_t next = au->state_move(curr, tr(prefix.p[i]));
			if (null_state == next)
				return 0;
			assert(next < au->total_states());
			ASSERT_isNotFree(next);
			curr = next;
		}
		ForEachWord_DFS<Au, OnSuffix> vis(au, on_suffix);
		vis.start(curr, prelen);
		return vis.nth;
	}

    size_t print_output(const char* fname = "-", state_id_t RootState = initial_state) const {
        printf("print_output(\"%s\", RootState=%ld)\n", fname, (long)RootState);
		FILE* fp = fopen_output(fname);
        size_t cnt = for_each_word(RootState,
			[fp](size_t, fstring word, state_id_t) {
				fprintf(fp, "%s\n", word.c_str());
			});
		if (fp != stdout) fclose(fp);
		return cnt;
    }

public:
    void x_write_dot_file(FILE* fp) const {
	//	printf("%s:%d %s\n", __FILE__, __LINE__, BOOST_CURRENT_FUNCTION);
		fprintf(fp, "digraph G {\n");
		PFS_GraphWalker<state_id_t> dfs;
   		dfs.resize(au->total_states());
		dfs.put_root(initial_state);
		while (!dfs.is_finished()) {
			state_id_t s = dfs.next();
			au->dot_write_one_state(fp, s);
			dfs.putChildren(au, s);
        }
		dfs.resize(au->total_states());
		dfs.put_root(initial_state);
		while (!dfs.is_finished()) {
			long i = dfs.next();
            au->for_each_move(i,
				[fp,this](state_id_t s, const transition_t& t, char_t c) {
					au->dot_write_one_transition(fp, s, t, c);
				});
			dfs.putChildren(au, i);
        }
		fprintf(fp, "}\n");
    }

	bool accept(const fstring str) const {
		state_id_t curr = initial_state;
		for (size_t i = 0; ; ++i) {
			if (au->is_pzip(curr)) {
				const byte_t* s2 = au->get_zpath_data(curr);
				size_t n = *s2++;
				assert(n > 0);
				if (i + n > str.size())
					return false;
				size_t j = 0;
				do { // prefer do .. while for performance
					if ((byte_t)str.p[i++] != s2[j++])
						return false;
				} while (j < n);
			}
			assert(i <= str.size());
			if (str.size() == i)
				return au->is_term(curr);
			state_id_t next = au->state_move(curr, str.p[i]);
			if (null_state == next)
				return false;
			assert(next < au->total_states());
			ASSERT_isNotFree(next);
			curr = next;
		}
	}

	/// @param on_match(size_t endpos)
	/// @return max forward scaned chars
	template<class OnMatch>
	size_t match_prefix(fstring str, OnMatch on_match) const {
		return match_prefix<OnMatch>(str, on_match, IdentityTranslator());
	}

	/// @param on_match(size_t endpos)
	/// @param tr(byte_t c) translate c, such as 'tolower'
	/// @return max forward scaned chars
	template<class OnMatch, class Translator>
	size_t match_prefix(fstring str, OnMatch on_match, Translator tr) const {
		state_id_t curr = initial_state;
		for (size_t i = 0; ; ++i) {
			if (au->is_pzip(curr)) {
				const byte_t* s2 = au->get_zpath_data(curr);
				size_t n2 = *s2++;
				size_t n  = std::min<size_t>(n2, str.size()-i);
				for (size_t j = 0; j < n; ++i, ++j)
					if ((byte_t)tr(str.p[i]) != s2[j]) return i;
				if (n < n2)
					return str.size();
			}
			assert(i <= str.size());
			if (au->is_term(curr))
				on_match(i);
			if (str.size() == i)
				return i;
			state_id_t next = au->state_move(curr, tr(str.p[i]));
			if (null_state == next)
				return i;
			assert(next < au->total_states());
			ASSERT_isNotFree(next);
			curr = next;
		}
		assert(0);
	}

	/// @param on_match(size_t keylen, size_t value_idx, fstring value)
	/// @return max forward scaned chars
	template<class OnMatch>
	size_t match_key(char delim, fstring str, OnMatch on_match) const {
		return match_key<OnMatch, IdentityTranslator>(delim, str, on_match, IdentityTranslator());
	}

	/// @param on_match(size_t keylen, size_t value_idx, fstring value)
	/// @param tr(byte_t c) translate c, such as 'tolower'
	/// @return max forward scaned chars
	template<class OnMatch, class Translator>
	size_t match_key(char delim, fstring str, OnMatch on_match, Translator tr) const {
		state_id_t curr = initial_state;
		size_t i;
		auto on_suffix = [&](size_t nth, const std::string& value, size_t) {
			const size_t const_i = i; // keylen
			on_match(const_i, nth, value);
		};
        ForEachWord_DFS<Au, decltype(on_suffix)> vis(au, on_suffix);
		vis.word.reserve(63);
		for (i = 0; ; ++i) {
			if (au->is_pzip(curr)) {
				const byte_t* s2 = au->get_zpath_data(curr);
				size_t n2 = *s2++;
				size_t n  = std::min<size_t>(n2, str.n-i);
				for (size_t j = 0; j < n; ++i, ++j) {
					byte_t c1 = (byte_t)tr(str.p[i]);
					byte_t c2 = s2[j];
					if (c2 == delim) {
						vis.start(curr, j+1);
						return i;
					}
				   	else if (c1 != c2) {
					   	return i;
					}
				}
				if (n < n2) {
					if (delim == s2[n]) {
						vis.start(curr, n+1);
					}
					return str.size();
				}
			}
			assert(i <= str.size());
			state_id_t value_start = au->state_move(curr, delim);
			if (null_state != value_start) {
				vis.start(value_start, 0);
			}
			if (str.size() == i)
				return i;
			state_id_t next = au->state_move(curr, tr(str.p[i]));
			if (null_state == next)
				return i;
			assert(next < au->total_states());
			ASSERT_isNotFree(next);
			curr = next;
		}
		assert(0);
	}

	/// @param tr(byte_t c) translate c, such as 'tolower'
	template<class Translator>
	size_t match_path(fstring str, valvec<state_id_t>* path, Translator tr) const {
		return match_path<Translator>(initial_state, str, path, tr);
	}
	template<class Translator>
	size_t match_path(state_id_t RootState, fstring str
				    , valvec<state_id_t>* path, Translator tr) const {
		state_id_t curr = RootState;
		path->resize(0);
		for (size_t i = 0; ; ++i) {
			if (au->is_pzip(curr)) {
				const byte_t* s2 = au->get_zpath_data(curr);
				size_t n = std::min<size_t>(*s2++, str.size()-i);
				for (size_t j = 0; j < n; ++i, ++j) {
					path->push_back(curr);
					if ((byte_t)tr(str.p[i]) != s2[j]) return i;
				}
			}
			assert(i <= str.size());
			path->push_back(curr);
			if (str.size() == i)
				return i;
			state_id_t next = au->state_move(curr, tr(str.p[i]));
			if (null_state == next)
				return i;
			assert(next < au->total_states());
			ASSERT_isNotFree(next);
			curr = next;
		}
		assert(0);
	}

	/// @return pos <  len     : the mismatch pos
	///         pos == len     : matched all
	///         pos == len + 1 : matched state is not a final state
	size_t first_mismatch_pos(const fstring str) const {
		state_id_t curr = initial_state;
		for (size_t i = 0; i < str.size(); ++i) {
			if (au->is_pzip(curr)) {
				const byte_t* s2 = au->get_zpath_data(curr);
				size_t n = *s2++;
				assert(n > 0);
				size_t m = i + n > str.size() ? str.size() - i : n;
				size_t j = 0;
				do { // prefer do .. while for performance
					if ((byte_t)str.p[i] != s2[j])
						return i;
					++i; ++j;
				} while (j < m);
				if (str.size() == i)
					return au->is_term(curr) ? str.n : str.n + 1;
			}
			state_id_t next = au->state_move(curr, str.p[i]);
			if (null_state == next)
				return i;
			assert(next < au->total_states());
			ASSERT_isNotFree(next);
			curr = next;
		}
		return !au->is_pzip(curr) && au->is_term(curr) ? str.n : str.n + 1;
	}

    void compute_indegree(state_id_t RootState, valvec<state_id_t>& in) const {
		assert(RootState < au->total_states());
		ASSERT_isNotFree(RootState);
        in.resize(au->total_states(), 0);
		simplebitvec color(au->total_states(), 0);
		valvec<state_id_t> stack;
		stack.reserve(512);
		stack.push_back(RootState);
		color.set1(RootState);
		while (!stack.empty()) {
			state_id_t parent = stack.back(); stack.pop_back();
            au->for_each_dest_rev(parent, [&](state_id_t, state_id_t child) {
				ASSERT_isNotFree(child);
				if (color.is0(child)) {
					color.set1(child);
					stack.push_back(child);
				}
				in[child]++;
			});
        }
    }

	void get_final_states(valvec<state_id_t>* final_states) {
		return get_final_states(initial_state, final_states);
	}
	void get_final_states(state_id_t root, valvec<state_id_t>* final_states) {
		final_states->erase_all();
		PFS_GraphWalker<state_id_t> walker;
		walker.resize(au->total_states());
		walker.put_root(root);
		while (!walker.is_finished()) {
			state_id_t curr = walker.next();
			if (au->is_term(curr))
				final_states->push_back(curr);
			walker.putChildren(au, curr);
		}
	}

	// for adfa_minimize:
	template<class State>
	struct GetStateByRawArray {
		const State* states;
		const State& operator()(state_id_t s) const { return states[s]; }
		GetStateByRawArray(const State* states1) : states(states1) {}
	};
	/// 'this->au' must be Acyclic DFA
	template<class Au2>
	void adfa_minimize(Au2& au2) const {
		assert(au->num_zpath_states() == 0);
		au2.clear();
#ifdef NDEBUG
		valvec<state_id_t> Min; Min.resize_no_init(au->total_states());
#else
		valvec<state_id_t> Min(au->total_states(), null_state+0);
		valvec<state_id_t> L(au->total_states(), null_state+0); // linked list
#endif
		valvec<state_id_t> Heads;
		typedef gold_hash_tab<typename Au::state_t, state_id_t
			, typename Au::HashEqByMap
			, GetStateByRawArray<typename Au::state_t>
			, typename DFA_OnflyMinimize<Au>::Ordered_Real_Map_Register::Layout
			> reg_t;
		{
			reg_t R(
				typename Au::HashEqByMap(au, Min.data()),
				GetStateByRawArray<typename Au::state_t>(au->internal_get_states().data())
			);
			R.set_load_factor(0.7);
			R.reserve(au->total_states());
			topological_sort([&](state_id_t state_id) {
					std::pair<size_t, bool> sb = R.insert_i(state_id);
				#if !defined(NDEBUG)
					if (!sb.second) {
						state_id_t& head = R.elem_at(sb.first);
						L[state_id] = head; // insert to head of linked list
						head = state_id; // update head
					}
					else assert(null_state == L[R.elem_at(sb.first)]);
					assert(null_state == Min[state_id]);
				#endif
					Min[state_id] = sb.first; // state id mapping
				});
			Heads.assign(R.begin(), R.end());
		#if !defined(NDEBUG)
			for (size_t i = 0; i < Min.size(); ++i) assert(is_free(i) || null_state != Min[i]);
			// initial_state must have no any other equivalent states:
			assert(null_state == L[Heads.back()]);
			assert(R.end_i()-1== Min[initial_state]); // initial_state is the last
		#endif
		}
		size_t min_size = Heads.size();
		au2.resize_states(min_size);
		for (size_t i = 0; i < min_size; ++i) {
			state_id_t curr = Heads[i];
			state_id_t parent2 = min_size - 1 - i;
			if (au->is_term(curr))
				au2.set_term_bit(parent2);
			au->for_each_move(curr,
				[&](state_id_t, state_id_t child1, byte_t ch) {
					state_id_t child2 = min_size - 1 - Min[child1];
					state_id_t old = au2.add_move(parent2, child2, ch);
					assert(au2.null_state == old || old == child2); (void)old;
				});
		#if !defined(NDEBUG)
			while (null_state != (curr = L[curr])) {
				assert(Min[curr] == i);
				au->for_each_move(curr,
					[&](state_id_t, state_id_t child1, byte_t ch) {
						state_id_t child2 = min_size - 1 - Min[child1];
						state_id_t old = au2.state_move(parent2, ch);
						assert(old == child2);
					});
			}
		#endif
		}
	}

	valvec<CharTarget<state_id_t> > trie_reverse_dfa() const {
		const  bool HasFreeStates = (0 != au->num_free_states());
		const  auto bad_char = CharTarget<state_id_t>::bad_char;
		const  CharTarget<state_id_t> bad_ct(bad_char, null_state);
		valvec<CharTarget<state_id_t> > inv(au->total_states(), bad_ct);
		for (state_id_t s = 0; s < au->total_states(); ++s)
		  if (!(HasFreeStates && au->is_free(s)))
			au->for_each_move(s, [&](state_id_t s, state_id_t t, char_t c) {
				auto& ct = inv[t];
				assert(bad_char == ct.ch);
				assert(null_state == ct.target);
				ct.ch = c;
				ct.target = s;
			});
		return inv;
	}

	// optimized
	//
	// create the inverse nfa, inverse of a general dfa is a nfa
    //
	// index[i] to index[i+1] are the index range of 'data'
	//   for target set of state 'i' in the inverse nfa
	CompactReverseNFA<state_id_t> graph_reverse_nfa() const {
		const bool HasFreeStates = (0 != au->num_free_states());
		CompactReverseNFA<state_id_t> nfa;
		nfa.index.resize(au->total_states() + 1); // the extra element is a guard
		for (size_t s = 0; s < au->total_states(); ++s)
		  if (!(HasFreeStates && au->is_free(s)))
			au->for_each_dest(s, [&](state_id_t, state_id_t t) {
				nfa.index[t+1]++;
			});
		for (size_t i = 1; i < nfa.index.size(); ++i)
			nfa.index[i] += nfa.index[i-1];
		nfa.data.resize_no_init(nfa.index.back());
		valvec<state_id_t> ii = nfa.index;
		for (size_t s = 0; s < au->total_states(); ++s)
		  if (!(HasFreeStates && au->is_free(s)))
			au->for_each_move(s, [&](state_id_t s, state_id_t t, char_t c) {
				nfa.data[ii[t]++] = CharTarget<state_id_t>(c, s);
			});
		return nfa;
	}

	typedef smallmap<typename Hopcroft<state_id_t>::Splitter> ch_to_invset_t;

    template<class Au2>
    void graph_dfa_minimize(Au2& minimized) const {
		minimized.erase_all();
        assert(au->total_states() < (size_t)null_state);
        Hopcroft<state_id_t> H(*au);
		{
			CompactReverseNFA<state_id_t> inv(graph_reverse_nfa());
			H.refine([&inv](state_id_t curr, ch_to_invset_t& mE) {
				size_t low = inv.index[curr+0];
				size_t upp = inv.index[curr+1];
				for (size_t i = low; i < upp; ++i) {
					CharTarget<state_id_t>  ct = inv.data[i];
					mE.bykey(ct.ch).insert(ct.target);
				}
			});
		}
		collapse(H, minimized);
    }
    template<class Au2>
    void trie_dfa_minimize(Au2& minimized) const {
		minimized.erase_all();
        assert(au->total_states() < (size_t)null_state);
        Hopcroft<state_id_t> H(*au);
		{
			valvec<CharTarget<state_id_t> > inv(trie_reverse_dfa());
			H.refine([&inv](state_id_t curr, ch_to_invset_t& mE) {
				if (initial_state != curr) {
					auto ct = inv[curr];
					mE.bykey(ct.ch).insert(ct.target);
				}
			});
		}
		collapse(H, minimized);
    }
    template<class Au2>
    void collapse0(Hopcroft<state_id_t>& H, Au2& minimized) const
   	{
		// this function has redundant calls to add_move
		size_t min_size = H.P.size();
		H.P.clear(); // H.P is no longer used
		minimized.resize_states(min_size);
		const bool HasFreeStates = (0 != au->num_free_states());
		for (state_id_t s = 0; s < au->total_states(); ++s) {
			if (HasFreeStates && au->is_free(s)) continue;
			au->for_each_move(s, [&](state_id_t s, state_id_t t, char_t c) {
				const typename Au2::state_id_t old =
					minimized.add_move(H.L[s].blid, H.L[t].blid, c);
				assert(Au2::null_state == old || old == H.L[t].blid);
			});
            if (au->is_term(s))
                minimized.set_term_bit(H.L[s].blid);
		}
	}
    template<class Au2>
    void collapse1(Hopcroft<state_id_t>& H, Au2& minimized) const {
		// this function use a bitvec mark to avoid redundant add_move
		size_t min_size = H.P.size();
		H.P.clear(); // H.P is no longer used
		minimized.resize_states(min_size);
		const bool HasFreeStates = (0 != au->num_free_states());
		simplebitvec mark(au->total_states(), 0);
		for (state_id_t s = 0; s < au->total_states(); ++s) {
			if (HasFreeStates && au->is_free(s)) continue;
			if (mark.is1(s)) continue;
			state_id_t curr = s;
		//	size_t cnt = 0;
			do { // set all equivalent states as marked
				assert(mark.is0(curr));
				mark.set1(curr);
				curr = H.L[curr].next;
		//		assert(cnt++ < au->total_states());
			} while (s != curr);
			au->for_each_move(s, [&](state_id_t s, state_id_t t, char_t c) {
    			auto old = minimized.add_move(H.L[s].blid, H.L[t].blid, c);
				assert(Au2::null_state == old); (void)old;
			});
            if (au->is_term(s))
                minimized.set_term_bit(H.L[s].blid);
		}
	}
    template<class Au2>
    void collapse2(Hopcroft<state_id_t>& H, Au2& minimized) const {
		// this function use H.L[*].next as mark to avoid redundant add_move
		size_t min_size = H.P.size();
		H.P.clear(); // H.P is no longer used
		minimized.resize_states(min_size);
		const bool HasFreeStates = (0 != au->num_free_states());
		for (state_id_t s = 0; s < au->total_states(); ++s) {
			if (HasFreeStates && au->is_free(s)) continue;
			if (null_state == H.L[s].next) continue;
			au->for_each_move(s, [&](state_id_t s, state_id_t t, char_t c) {
    			auto old = minimized.add_move(H.L[s].blid, H.L[t].blid, c);
				assert(Au2::null_state == old); (void)old;
			});
            if (au->is_term(s))
                minimized.set_term_bit(H.L[s].blid);
			state_id_t curr = s;
			do { // set all equivalent states as marked: L[*].next = null
				assert(null_state != curr);
				state_id_t next = H.L[curr].next;
				H.L[curr].next = null_state;
				curr = next;
			} while (s != curr);
		}
	}
    template<class Au2>
    void collapse3(Hopcroft<state_id_t>& H, Au2& minimized) const {
		assert(H.L.size() == au->total_states());
		valvec<state_id_t> X; X.resize_no_init(H.P.size());
		valvec<state_id_t> M; M.resize_no_init(au->total_states());
		for (size_t i = 0; i < X.size(); ++i) X[i] = H.P[i].head;
		for (size_t i = 0; i < M.size(); ++i) M[i] = H.L[i].blid;
		H.P.clear();
		H.L.clear();
		minimized.resize_states(X.size());
		for(state_id_t s2 = 0; s2 < X.size(); ++s2) {
			state_id_t s1 = X[s2]; assert(M[s1] == s2);
			au->for_each_move(s1, [&](state_id_t, state_id_t t, char_t c) {
    			auto old = minimized.add_move(s2, M[t], c);
				assert(Au2::null_state == old); (void)old;
			});
            if (au->is_term(s1))
                minimized.set_term_bit(s2);
		}
	}
	template<class Au2>
	void collapse(Hopcroft<state_id_t>& H, Au2& minimized) const {
		collapse3(H, minimized);
	}

	bool is_free(size_t s) const { return au->is_free(s); }
};

template<class Au> DFA_Algorithm<Au> dfa_algo(      Au* au) { return  au; }
template<class Au> DFA_Algorithm<Au> dfa_algo(const Au* au) { return  au; }
template<class Au> DFA_Algorithm<Au> dfa_algo(const Au& au) { return &au; }

template<class Au>
class DFA_MutationAlogrithm {
	Au* const au;

public:
	DFA_MutationAlogrithm(Au* au1) : au(au1) {}

	typedef typename Au::state_id_t state_id_t;
	typedef typename Au::transition_t transition_t;

    static const state_id_t  max_state = Au:: max_state;
    static const state_id_t null_state = Au::null_state;

    bool add_word(fstring key) const { return add_word(initial_state, key); }
    bool add_word(size_t RootState, fstring key) const {
		assert(0 == au->num_zpath_states());
		assert(RootState < au->total_states());
        size_t curr = RootState;
        size_t next;
        size_t j = 0;
        while (j < key.size()) {
            next = au->state_move(curr, key.p[j]);
            if (au->null_state == next)
                goto AddNewStates;
            assert(next < au->total_states());
            curr = next;
            ++j;
        }
        // key could be the prefix of some existed key word
        if (au->is_term(curr)) {
            return false; // existed
        } else {
            au->set_term_bit(curr);
            return true;
        }
    AddNewStates:
        do {
            next = au->new_state();
            au->add_move_checked(curr, next, key.p[j]);
            curr = next;
            ++j;
        } while (j < key.size());
        // now curr == next, and it is a Terminal State
        au->set_term_bit(curr);
        return true;
    }

	///@param out path
	///@return
	///  - null_state   : the str from 'from' to 'to' is newly created
	///  - same as 'to' : the str from 'from' to 'to' has existed
	///  - others       : the str from 'from' to 'XX' has existed, XX is not 'to'
	///                   in this case, the Automata is unchanged
	///@note
	///  - This function don't change the Automata if str from 'from' to 'to'
	///    has existed. The caller should check the return value if needed.
	state_id_t make_path( state_id_t from, state_id_t to
						, const fstring str
						, valvec<state_id_t>* path) {
		assert(au->num_zpath_states() == 0);
		assert(null_state != from);
		assert(null_state != to);
		assert(NULL != path);
		assert(NULL != str.p);
	#ifdef NDEBUG
		if (0 == str.n) return false;
	#else
        assert(str.n > 0); // don't allow empty str
	#endif
		auto& p = *path;
		p.resize(str.n+1);
        p[0] = from;
        for (ptrdiff_t j = 0; j < str.n-1; ++j) {
            if (null_state == (p[j+1] = au->state_move(p[j], str.p[j]))) {
				do au->add_move(p[j], p[j+1] = au->new_state(), str.p[j]);
				while (++j < str.n-1);
				break;
			}
            assert(p[j+1] < au->total_states());
        }
		state_id_t old_to = au->add_move(p[str.n-1], to, str[str.n-1]);
		p[str.n] = (null_state == old_to) ? to : old_to;
		return old_to;
	}

	template<class Au2>
	state_id_t
	copy_from(const Au2& au2, typename Au2::state_id_t SrcRoot = initial_state) const {
		assert(0 == au->num_zpath_states());
		assert(SrcRoot < au2.total_states());
		valvec<state_id_t> idmap(au2.total_states(), (state_id_t)null_state);
		valvec<typename Au2::state_id_t> stack;
		stack.reserve(512);
		stack.push_back(SrcRoot);
		idmap[SrcRoot] = au->new_state(); // idmap also serve as "color"
		while (!stack.empty()) {
			auto parent = stack.back(); stack.pop_back();
			size_t old_stack_size = stack.size();
			assert(!au2.is_pzip(parent));
			assert(null_state != idmap[parent]);
			au2.for_each_move(parent,
				[&](typename Au2::state_id_t parent,
					typename Au2::state_id_t child, byte_t c)
			{
				if (idmap[child] == null_state) {
					idmap[child] = au->new_state();
					stack.push_back(child);
				}
				au->add_move(idmap[parent], idmap[child], c);
			});
			if (au2.is_term(parent))
				au->set_term_bit(idmap[parent]);
			std::reverse(stack.begin() + old_stack_size, stack.end());
		}
		return idmap[SrcRoot];
	}

	// dead states: states which are unreachable to any final states
	void remove_dead_states() const {
		Au clean;
		remove_dead_states(clean);
		au->swap(clean);
	}
	template<class CleanDFA>
	void remove_dead_states(CleanDFA& clean) const {
		assert(au->has_freelist());
		CompactReverseNFA<state_id_t> inv(dfa_algo(au).graph_reverse_nfa());
		valvec<state_id_t> stack;
		simplebitvec mark(au->total_states(), false);
		if (au->num_free_states()) {
			for (size_t root = 0, n = au->total_states(); root < n; ++root) {
				if (!au->is_free(root) && au->is_term(root))
					stack.push_back(root), mark.set1(root);
			}
		} else { // has no any free state
			for (size_t root = 0, n = au->total_states(); root < n; ++root) {
				if (au->is_term(root))
					stack.push_back(root), mark.set1(root);
			}
		}
		// DFS search from multiple final states
		while (!stack.empty()) {
			state_id_t parent = stack.back(); stack.pop_back();
			size_t beg = inv.index[parent+0];
			size_t end = inv.index[parent+1];
			assert(beg <= end);
			for (size_t i = beg; i < end; ++i) {
				state_id_t child = inv.data[i].target;
				assert(child < au->total_states());
				if (mark.is0(child))
					stack.push_back(child), mark.set1(child);
			}
		}
		size_t new_au_states = 0;
		typedef typename CleanDFA::state_id_t state_id_t2;
		valvec<state_id_t2> map(au->total_states(), (state_id_t2)clean.null_state);
		for (size_t s = 0, n = au->total_states(); s < n; ++s) {
			if (mark.is1(s))
				map[s] = new_au_states++;
		}
		clean.resize_states(new_au_states);
		for (size_t s = 0, n = au->total_states(); s < n; ++s) {
			if (map[s] == null_state)
			   	continue;
			au->for_each_move(s, [&](state_id_t src, state_id_t dst, char_t ch) {
				if (map[dst] != clean.null_state) {
					state_id_t2 src2 = map[src];
					state_id_t2 dst2 = map[dst];
					clean.add_move(src2, dst2, ch);
				}
			});
			if (au->is_term(s))
				clean.set_term_bit(map[s]);
		}
	}

	// sub graph rooted at RootState must has no intersection nodes
	// with other sub graphs
	void remove_sub(size_t RootState) const {
		assert(RootState < au->total_states());
		auto on_finish = [this](state_id_t s) { au->del_state(s); };
		auto on_back_edge = [&](state_id_t, state_id_t child){
			if (child == RootState) {
				string_appender<> msg;
				msg << BOOST_CURRENT_FUNCTION
				   	<< ": found cycle to RootState=" << RootState;
				throw std::logic_error(msg);
			}
		};
		dfa_algo(au).dfs_post_order_walk(RootState, on_finish, on_back_edge);
	}

    void graph_dfa_minimize() const {
        Au minimized;
        dfa_algo(au).graph_dfa_minimize(minimized);
        au->swap(minimized);
    }
    void trie_dfa_minimize() const {
        Au minimized;
        dfa_algo(au).trie_dfa_minimize(minimized);
        au->swap(minimized);
    }
    void adfa_minimize() const {
	   	Au minimized;
	   	dfa_algo(au).adfa_minimize(minimized);
	   	au->swap(minimized);
	}

	///@param on_fold:
	//        void on_fold.start(state_id_t RootState);
	///       void on_fold.on_value(size_t nth, const std::string& value);
	///       bool on_fold.finish(size_t num_values, std::string* folded_value);
	/// values are suffixes of prefixes after first delim
	///@effect
	///      if on_fold.finish returns true, values will be
	///      folded into a single value (such as sum of numbers)
	///@note this Automata must has no cycles in values
	template<class OnFold>
	void fold_values(char delim, OnFold on_fold) {
		assert(au->num_zpath_states() == 0);
		valvec<state_id_t> folds; folds.reserve(128);
		{
		//	BFS_MultiPassGraphWalker<state_id_t> walker; // BFS use more memory
		//	DFS_MultiPassGraphWalker<state_id_t> walker;
			DFS_GraphWalker<state_id_t> walker;
			simplebitvec in_folds(au->total_states(), 0);
			walker.resize(au->total_states());
			walker.put_root(initial_state);
			while (!walker.is_finished()) {
				state_id_t parent = walker.next();
				state_id_t vchild = au->state_move(parent, delim);
				if (null_state != vchild && in_folds.is0(vchild)) {
					in_folds.set1(vchild);
					folds.push_back(vchild);
				}
				walker.putChildren2(au, parent,
					[=](state_id_t, state_id_t child, bool) {
						// just fold at first delim bound
						return (child != vchild);
					});
			}
		#if !defined(NDEBUG)
			size_t n_reachable = 0;
			walker.resize(au->total_states());
			walker.put_root(initial_state);
			while (!walker.is_finished()) {
				state_id_t parent = walker.next();
				walker.putChildren(au, parent);
				n_reachable++;
			}
			assert(au->num_used_states() == n_reachable);
		#endif
		}
		valvec<state_id_t> indegree(au->total_states(), 0);
		dfa_algo(au).compute_indegree(initial_state, indegree);
		auto on_suffix = [&](size_t nth, const std::string& value, size_t) {
			on_fold.on_value(nth, value);
		};
        ForEachWord_DFS<Au, decltype(on_suffix)> vis(au, on_suffix);
		std::string folded_value;
		valvec<state_id_t> del_list;
		valvec<state_id_t> topological, stack, color;
		for (size_t j = 0; j < folds.size(); ++j) {
			state_id_t s = folds[j];
		#if !defined(NDEBUG)
			for (size_t i = 1; i < indegree.size(); ++i) {
				if (au->is_free(i)) continue;
				// if algorithm is correct, indegree[i] must > 0
				assert(indegree[i] > 0);
			}
		#endif
			on_fold.start(s);
			vis.start(s, 0); // will call on_fold.on_value(...)
			if (on_fold.finish(vis.nth, &folded_value)) {
				del_list.erase_all();
				topological.erase_all();
				assert(stack.empty());
				color.resize(au->total_states()+1);
				stack.push_back(s);
				dfa_algo(au).topological_sort(stack, color,
					   	[&](state_id_t x) { topological.push_back(x); });
				assert(topological.back() == s);
				topological.pop_back(); // pop s
				au->for_each_dest(s, [&](state_id_t, state_id_t child) {
						if (0 == --indegree[child])
							del_list.push_back(child);
					});
				for (ptrdiff_t i = topological.size()-1; i >= 0; --i) {
					state_id_t parent = topological[i];
					au->for_each_dest(parent,
						[&](state_id_t parent, state_id_t child) {
							if (0 == indegree[parent]) {
								if (0 == --indegree[child])
									del_list.push_back(child);
							}
						});
				}
				for (state_id_t x : del_list) au->del_state(x);
				au->del_all_children(s);
				fstring folded(folded_value);
				state_id_t curr = s;
				indegree.resize(std::max(au->total_states(), au->num_used_states()+folded.n));
				for (ptrdiff_t i = 0; i < folded.n; ++i) {
					state_id_t next = au->new_state();
					au->add_move(curr, next, folded.p[i]);
					indegree[next] = 1;
					curr = next;
				}
				au->set_term_bit(curr);
				assert(indegree.size() == au->total_states());
			}
		}
#if !defined(NDEBUG)
		for (size_t i = 1; i < indegree.size(); ++i) {
			if (au->is_free(i)) continue;
			// if algorithm is correct, indegree[i] must > 0
			assert(indegree[i] > 0);
		}
		// check new indegrees
		indegree.resize_no_init(au->total_states());
		indegree.fill(0);
		for (size_t i = 0; i < indegree.size(); ++i) {
			if (au->is_free(i)) continue;
			au->for_each_dest(i, [&](state_id_t, state_id_t child) {
					indegree[child]++;
				});
		}
		for (size_t i = 1; i < indegree.size(); ++i) {
			if (au->is_free(i)) continue;
			// if algorithm is correct, indegree[i] must > 0
			assert(indegree[i] > 0);
		}
#endif
	}
	
};

template<class Au> DFA_MutationAlogrithm<Au> dfa_mutate(Au* au) { return  au; }
template<class Au> DFA_MutationAlogrithm<Au> dfa_mutate(Au& au) { return &au; }


template<class Derived>
class DFA_VirtualAdapter : public DFA_Interface {
public:
	bool v_is_pzip(size_t s) const override final { return static_cast<const Derived*>(this)->is_pzip(s); }
	bool v_is_term(size_t s) const override final { return static_cast<const Derived*>(this)->is_term(s); }
	size_t  v_total_states() const override final { return static_cast<const Derived*>(this)->total_states(); }

	void v_for_each_move(size_t s, const boost::function<void(size_t parent, size_t child, byte_t ch)>& op)
	   	const override final {
		static_cast<const Derived*>(this)->template for_each_move<decltype(op)>(s, op);
	}
	void v_for_each_dest(size_t s, const boost::function<void(size_t parent, size_t child)>& op)
	   	const override final {
		static_cast<const Derived*>(this)->template for_each_dest<decltype(op)>(s, op);
	}
	void v_for_each_dest_rev(size_t s, const boost::function<void(size_t parent, size_t child)>& op)
	   	const override final {
		static_cast<const Derived*>(this)->template for_each_dest_rev<decltype(op)>(s, op);
	}
	const byte_t* v_get_zpath_data(size_t s) const override final {
	   	return static_cast<const Derived*>(this)->get_zpath_data(s);
   	}
	size_t v_null_state() const override final { return Derived::null_state; }
	size_t v_max_state() const override final { return Derived::max_state; }
	
	size_t for_each_word(size_t RootState, const boost::function<void(size_t nth, fstring)>& on_word)
	   	const override final {
		return dfa_algo(static_cast<const Derived*>(this)).for_each_word(RootState
			   	, [&](size_t nth, fstring w, size_t){ on_word(nth, w);}
				);
	}
	size_t for_each_suffix(size_t RootState, fstring prefix
			, const boost::function<void(size_t nth, fstring)>& on_word
			, const boost::function<byte_t(byte_t)>& tr
			)
	   	const override final {
		auto on_word3 = [&](size_t nth, fstring w, size_t){ on_word(nth, w); };
		return dfa_algo(static_cast<const Derived*>(this)).template
			for_each_suffix<decltype(on_word3), decltype(tr)>(RootState, prefix
			   	, on_word3, tr);
	}
	size_t for_each_suffix(size_t RootState, fstring prefix
			, const boost::function<void(size_t nth, fstring)>& on_word
			)
	   	const override final {
		return dfa_algo(static_cast<const Derived*>(this)).for_each_suffix(RootState, prefix
			   	, [&](size_t nth, fstring w, size_t){ on_word(nth, w);}
				);
	}

	size_t match_key(char delim, fstring str
			, const boost::function<void(size_t keylen, size_t value_idx, fstring value)>& on_match
			)
	   	const override final {
		return dfa_algo(static_cast<const Derived*>(this)).template
			match_key<decltype(on_match)>(delim, str, on_match);
	}

	size_t match_key(char delim, fstring str
			, const boost::function<void(size_t keylen, size_t value_idx, fstring value)>& on_match
			, const boost::function<byte_t(byte_t)>& tr
			)
	   	const override final {
		return dfa_algo(static_cast<const Derived*>(this)).template
			match_key<decltype(on_match), decltype(tr)>(delim, str, on_match, tr);
	}
};

template<class Derived>
class DFA_MutationVirtualAdapter : public DFA_MutationInterface {
public:
#ifdef FEBIRD_AUTOMATA_ENABLE_MUTATION_VIRTUALS
	using DFA_MutationInterface::add_word;
    virtual bool add_word(size_t RootState, fstring word) override {
		return dfa_mutate(static_cast<Derived*>(this)).add_word(RootState, word);
    }
	virtual size_t copy_from(size_t SrcRoot) override {
		Derived* const au = static_cast<Derived*>(this);
		return dfa_mutate(au).copy_from(*au, SrcRoot);
	}
	virtual void remove_dead_states() override {
		dfa_mutate(static_cast<Derived*>(this)).remove_dead_states();
	}
	virtual void remove_sub(size_t RootState) override {
		dfa_mutate(static_cast<Derived*>(this)).remove_sub(RootState);
	}
    virtual void graph_dfa_minimize() override {
		dfa_mutate(static_cast<Derived*>(this)).graph_dfa_minimize();
	}
    virtual void trie_dfa_minimize() override {
		dfa_mutate(static_cast<Derived*>(this)).trie_dfa_minimize();
	}
    virtual void adfa_minimize() override {
		dfa_mutate(static_cast<Derived*>(this)).adfa_minimize();
	}
#endif
// non-virtual
    void add_move_checked(size_t source, size_t target, char_t ch) {
		size_t old = this->add_move_imp(source, target, ch, false);
		assert(Derived::null_state == old);
		if (Derived::null_state != old) {
			throw std::logic_error(BOOST_CURRENT_FUNCTION);
		}
	}
    size_t add_move(size_t source, size_t target, char_t ch) {
		return this->add_move_imp(source, target, ch, false);
	}
	// overwrite existed
	/// @return null_state : if not existed
	///         otherwise  : the old target which was overwritten
    size_t set_move(size_t source, size_t target, char_t ch) {
		return this->add_move_imp(source, target, ch, true);
	}
};

template<class Derived>
class DAWG_VirtualAdapter : public DAWG_Interface {
public:
    size_t v_match_words(fstring str, const boost::function<void(size_t len, size_t nth)>& on_match) const override {
	   	return this->match_words(str, on_match);
	}
    size_t v_match_words(fstring str, const boost::function<void(size_t len, size_t nth)>& on_match, const boost::function<byte_t(byte_t)>& tr) const override {
	   	return this->match_words(str, on_match, tr);
	}
    size_t v_match_words(fstring str, const boost::function<void(size_t len, size_t nth)>& on_match, const byte_t* tr) const override {
		assert(NULL != tr);
	   	return this->match_words(str, on_match, [tr](byte_t ch){ return tr[ch]; });
	}

	struct LongestOnMatch {
		size_t cnt = 0;
		size_t len = 0;
		size_t nth = 0;
		void operator()(size_t len2, size_t nth2) {
			cnt++;
			len = len2;
			nth = nth2;
		}
		bool get_result(size_t* p_len, size_t* p_nth) const {
			if (cnt) {
				*p_len = len;
				*p_nth = nth;
				return true;
			}
			return false;
		}
	};
	bool longest_prefix(fstring str, size_t* len, size_t* nth, const boost::function<byte_t(byte_t)>& tr) const override {
		assert(NULL != len);
		assert(NULL != nth);
		LongestOnMatch on_match;
		match_words<LongestOnMatch&>(str, on_match, tr);
		return on_match.get_result(len, nth);
	}
	bool longest_prefix(fstring str, size_t* len, size_t* nth, const byte_t* tr) const override {
		assert(NULL != len);
		assert(NULL != nth);
		assert(NULL != tr);
		LongestOnMatch on_match;
		match_words<LongestOnMatch&>(str, on_match, TableTranslator(tr));
		return on_match.get_result(len, nth);
	}
	bool longest_prefix(fstring str, size_t* len, size_t* nth) const override {
		assert(NULL != len);
		assert(NULL != nth);
		LongestOnMatch on_match;
		this->match_words<LongestOnMatch&>(str, on_match);
		return on_match.get_result(len, nth);
	}

//-----------------------------------------------------------------------
	///@param on_hit(size_t match_end_pos, size_t word_id)
	template<class OnMatch>
    size_t match_words(fstring str, OnMatch on_match) const {
		return static_cast<const Derived*>(this)->
			match_words<const char*, OnMatch>(str.begin(), str.end(), on_match, IdentityTranslator());
	}

	///@param on_hit(size_t match_end_pos, size_t word_id)
	///@param tr(byte_t c) translate c, such as 'tolower'
	template<class OnMatch, class Translator>
    size_t match_words(fstring str, OnMatch on_match, Translator tr) const {
		return static_cast<const Derived*>(this)->
			match_words<const char*, OnMatch, Translator>(str.begin(), str.end(), on_match, tr);
	}
};

/////////////////////////////////////////////////////////////////////////////

#endif // __febird_automata_dfa_algo_hpp__

