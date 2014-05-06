#ifndef __dawg_penglei__
#define __dawg_penglei__

#include "automata.hpp"
#include <febird/util/autoclose.hpp>

template<class BaseState> 
struct DAWG_State : public BaseState {
	typedef typename BaseState::state_id_t state_id_t;
	struct transition_t {
		typedef state_id_t  num_t; // could bigger than state_id??
		state_id_t  target;
		     num_t  num;
		operator state_id_t() const { return target; }

		transition_t(state_id_t t) : target(t), num(~num_t(0)) {}
		transition_t(state_id_t t, num_t num) : target(t), num(num) {}
	};
	static transition_t first_trans(state_id_t t) { return transition_t(t, 0); }
};

#if FEBIRD_WORD_BITS >= 64
template<> 
struct DAWG_State<State34> : public State34 {
	typedef State34::state_id_t state_id_t;
	struct transition_t {
		typedef uint32_t  num_t;
		state_id_t  target : 34;
		     num_t  num    : 30;
		operator state_id_t() const { return target; }

		transition_t(state_id_t t) : target(t), num(0x3FFFFFFF) {}
		transition_t(state_id_t t, num_t num) : target(t), num(num) {}
	};
	static transition_t first_trans(state_id_t t) { return transition_t(t, 0); }
};
#endif

// Indexed Directed Acyclic Word Graph
// Indexed Acyclic DFA, not required to be minimal
template<class BaseState>
class DAWG :
	public Automata<DAWG_State<BaseState> >,
	public DAWG_Interface
{
	typedef Automata<DAWG_State<BaseState> > super;
	typedef DAWG_State<BaseState> State;
    using super::states;
    using super::pool;
    typedef typename super::header_b32 header_b32;
    typedef typename super::header_max header_max;
    typedef typename super::Color Color;
	using DAWG_Interface::n_words;
public:
	typedef DAWG_State<BaseState> state_t;
    typedef typename state_t::transition_t transition_t;
    typedef typename transition_t::num_t      num_t;
    typedef typename State::state_id_t        state_id_t;
    using super::nil_state;
    using DAWG_Interface::null_word;
    const static num_t max_num = ~num_t(0);

	DAWG() { is_compiled = false; }

   bool compile() {
        assert(this->tpl_is_dag());
		valvec<Color> marks(states.size(), Color::White);
		valvec<num_t> paths_from_state;
#if defined(NDEBUG)
        paths_from_state.resize_no_init(states.size());
#else
        paths_from_state.resize(states.size(), (num_t)max_num);
#endif
        bool r = compile_loop(initial_state, &marks[0], &paths_from_state[0]);
		is_compiled = r;
		n_words = paths_from_state[initial_state];
        return r;
    }
private:
    // true if graph from curr is a DAG
    bool compile_loop(state_id_t curr, Color* mark, num_t* paths_from_state) {
        assert(curr < (state_id_t)states.size());
        if (Color::Grey == mark[curr]) {
            assert(0);
			throw std::logic_error("calling DAWG::compile on cyclic automata");
        }
        mark[curr] = Color::Grey;
        const State s = states[curr];
        num_t num = 0;
        if (!s.more_than_one_child()) {
            state_id_t target = super::single_target(s);
            if (nil_state != target) {
                if (!compile_loop(target, mark, paths_from_state))
                    return false;
                num = paths_from_state[target];
                assert(num < INT_MAX);
            }
        } else {
			int bits = s.rlen(), nt;
            transition_t* pt;
            if (super::is_32bitmap(bits)) {
                header_b32& hb = pool.template at<header_b32>(s.getpos());
                nt = fast_popcount32(hb.b);
                pt = hb.s;
            } else {
                header_max& hb = pool.template at<header_max>(s.getpos());
                nt = hb.popcnt_aligned(bits);
                pt = hb.base(bits);
            }
            for (int i = 0; i < nt; ++i) {
                assert(num < INT_MAX);
                pt[i].num = num;
                state_id_t target = pt[i].target;
                if (!compile_loop(target, mark, paths_from_state))
                    return false;
                assert(paths_from_state[target] < INT_MAX);
                num += paths_from_state[target];
            }
        }
        if (s.is_term())
            num++;
        assert(num >= 1);
        assert(num < max_num);
		if (num >= max_num)
			throw std::out_of_range("num for dawg index exceed max_num");
        paths_from_state[curr] = num;
        mark[curr] = Color::Black;
        return true;
    }
	void finish_load_mmap(const DFA_MmapHeader*) override;
	void prepare_save_mmap(DFA_MmapHeader*, const void**) const override;
public:
    size_t index(const fstring str) const {
		assert(is_compiled);
		ASSERT_isNotFree(initial_state);
        state_id_t curr = initial_state;
        size_t idx = 0;
        for (size_t i = 0; ; ++i) {
			const State& s = states[curr];
			if (s.is_pzip()) {
				const byte_t* s2 = this->get_zpath_data(s);
				size_t n = *s2++;
				assert(n > 0);
				if (i + n > str.size())
					return null_word;
				size_t j = 0;
				do { // prefer do .. while for performance
					if ((byte_t)str.p[i++] != s2[j++])
						return null_word;
				} while (j < n);
			}
			assert(i <= str.size());
			if (str.size() == i)
				return s.is_term() ? idx : null_word;
            transition_t t = this->state_move(s, (byte_t)str.p[i]);
            if (nil_state == t.target)
                return null_word;
			assert(t.target < states.size());
			ASSERT_isNotFree(t.target);
			if (s.is_term()) {
				idx++;
				assert(idx < n_words);
			}
            idx += t.num;
			assert(idx < n_words);
            curr = t.target;
        }
    }

	template<class Range, class OnMatch, class Translator>
    size_t match_words(Range& r, OnMatch on_match, Translator tr) const {
		assert(is_compiled);
		ASSERT_isNotFree(initial_state);
        state_id_t curr = initial_state;
        size_t idx = 0;
        for (size_t i = 0; ; ++i, ++r) {
			const State& s = states[curr];
			if (s.is_pzip()) {
				const byte_t* s2 = this->get_zpath_data(s);
				size_t n2 = *s2++;
				for (size_t j = 0; j < n2; ++j, ++r)
					if (r.empty() || (byte_t)tr(*r) != s2[j]) return i+j;
				i += n2;
			}
			assert(idx < n_words);
			if (s.is_term())
			   	on_match(i, idx++);
			if (r.empty())
				return i;
            transition_t t = this->state_move(s, (byte_t)tr(*r));
            if (nil_state == t.target)
                return i;
			assert(t.target < states.size());
			ASSERT_isNotFree(t.target);
            idx += t.num;
            curr = t.target;
        }
		assert(0);
    }

	std::string nth_word(size_t nth) const {
		assert(is_compiled);
		std::string word;
		nth_word(nth, &word);
		return word;
	}
	void nth_word(size_t nth, std::string* word) const {
		assert(is_compiled);
		if (nth >= n_words) {
			string_appender<> msg;
			msg << __FILE__ ":" << __LINE__
			   	<< " , func: " << BOOST_CURRENT_FUNCTION
				<< ", nth=" << nth
				<< ", n_words=" << n_words;
			throw std::out_of_range(msg);
		}
		word->resize(0);
		state_id_t s = initial_state;
		size_t hit = 0;
		for (;;) {
			assert(hit <= nth);
			const State& ss = states[s];
			if (ss.is_pzip()) {
				const byte_t* s2 = this->get_zpath_data(ss);
				size_t n = *s2++;
				assert(n > 0);
				word->append((const char*)s2, n);
			}
			if (ss.is_term()) {
				if (hit == nth)
					break;
				++hit;
			}
			assert(hit <= nth);
			byte_t the_ch = 0;
			size_t inc = 0;
			state_id_t next = nil_state;
		#if 0 // in concise form:
			this->for_each_move_break(s, // sequencial search
				[&](state_id_t, const transition_t& t, byte_t c) {
					if (hit + t.num <= nth) {
						the_ch = c, next = t.target, inc = t.num;
						return true;
					} else
						return false;
			});
		#else // optimize for speed: (copy+paste+modify from for_each_move)
			  // ~20% performance gain
			if (!ss.more_than_one_child()) {
				s = super::single_target(ss); assert(nil_state != s);
				word->push_back(ss.getMinch());
				continue;
			}
			const int bits = ss.rlen();
			if (super::is_32bitmap(bits)) {
				const header_b32& hb = pool.template at<header_b32>(ss.getpos());
				uint32_t bm = hb.b;
				byte_t ch = ss.getMinch();
				for (int pos = 0; bm; ++pos) {
					const transition_t& t = hb.s[pos];
					int ctz = fast_ctz32(bm);
					ch += ctz;
					if (hit + t.num <= nth)
						the_ch = ch, next = t.target, inc = t.num;
					else
						break;
					ch++;
					bm >>= ctz; // must not be bm >>= ctz + 1
					bm >>= 1;   // because ctz + 1 may be bits of bm(32 or 64)
				}
			} else {
				const header_max& hb = pool.template at<header_max>(ss.getpos());
				const transition_t* base = hb.base(bits);
				const int cBlock = hb.align_bits(bits) / hb.BlockBits;
				for (int pos = 0, i = 0; i < cBlock; ++i) {
					typename super::big_bm_b_t bm = hb.block(i);
					byte_t ch = ss.getMinch() + i * hb.BlockBits;
					for (; bm; ++pos) {
						const transition_t& t = base[pos];
						int ctz = fast_ctz(bm);
						ch += ctz;
						if (hit + t.num <= nth)
							the_ch = ch, next = t.target, inc = t.num;
						else
							goto LoopEnd;
						ch++;
						bm >>= ctz; // must not be bm >>= ctz + 1
						bm >>= 1;   // because ctz + 1 may be bits of bm(32 or 64)
						assert(pos < hb.BlockBits * cBlock);
					}
				}
			LoopEnd:;
			}
		#endif
			assert(nil_state != next);
			word->push_back(the_ch);
			s = next, hit += inc;
		}
	}

	void
	dot_write_one_move_ex(FILE* fp, long s, const transition_t& t, byte_t c)
	const {
		assert(is_compiled);
		char buf[16];
		char ch = c;
		febird::dot_escape(&ch, 1, buf, sizeof(buf));
		fprintf(fp, "\tstate%ld -> state%ld [label=\"%s,%ld\"];\n"
				, s, long(t.target), buf, long(t.num));
	}
    void write_dot_file_ex(FILE* fp) const {
		assert(is_compiled);
	//	printf("%s:%d %s\n", __FILE__, __LINE__, BOOST_CURRENT_FUNCTION);
		fprintf(fp, "digraph G {\n");
		valvec<state_id_t> stack;
		valvec<size_t>     dests;
		simplebitvec       color(this->total_states());
		color.set1(initial_state);
		stack.push_back(initial_state);
		while (!stack.empty()) {
			state_id_t s = stack.back(); stack.pop_back();
			this->get_all_dest(s, &dests);
			for(size_t i = 0; i < dests.size(); ++i) {
				size_t child = dests[i];
				if (color.is0(child)) {
					color.set1(child);
					stack.push_back(child);
				}
			}
			this->dot_write_one_state(fp, s);
        }
		color.fill(0);
		color.set1(initial_state);
		stack.push_back(initial_state);
		while (!stack.empty()) {
			size_t parent = stack.back(); stack.pop_back();
			this->for_each_move(parent,
			[&](state_id_t parent, const transition_t& t, byte_t c) {
				if (color.is0(t.target)) {
					color.set1(t.target);
					stack.push_back(t.target);
				}
				this->dot_write_one_move_ex(fp, parent, t, c);
			});
        }
		fprintf(fp, "}\n");
    }
    void write_dot_file_ex(const char* fname) const {
		febird::Auto_fclose fp(fopen(fname, "w"));
		if (fp) {
			write_dot_file_ex(fp);
		} else {
			fprintf(stderr, "failed: %s: fopen(%s, w) = %s\n"
				, BOOST_CURRENT_FUNCTION, fname, strerror(errno));
		}
	}

	void clear() {
		super::clear();
		is_compiled = false;
		n_words = 0;
	}
	void swap(DAWG& y) {
		assert(typeid(*this) == typeid(y));
		std::swap(is_compiled, y.is_compiled);
		std::swap(n_words    , y.n_words);
		super::swap(y);
	}

	template<class DataIO>
	friend void DataIO_loadObject(DataIO& dio, DAWG& da) {
		DataIO_loadObject(dio, static_cast<super&>(da));
		if (!da.m_is_dag) {
			typedef typename DataIO::my_DataFormatException bad_format;
			throw bad_format("DAWG must be a DAG");
		}
		typename DataIO::my_var_uint64_t cnt;
		dio >> cnt;
		da.n_words = cnt.t;
		da.is_compiled = true;
	}
	template<class DataIO>
	friend void DataIO_saveObject(DataIO& dio, const DAWG& da) {
		if (!da.is_compiled) {
			throw std::logic_error(BOOST_CURRENT_FUNCTION);
		}
		DataIO_saveObject(dio, static_cast<const super&>(da));
		dio << typename DataIO::my_var_uint64_t(da.n_words);
	}
private:
	bool is_compiled;
typedef DAWG MyType;
#include "ppi/dawg_const_virtuals.hpp"
#include "ppi/dfa_methods_calling_swap_using_super.hpp"
#include "ppi/dfa_methods_calling_swap.hpp"
};

#pragma pack(push, 4)
template<class State> struct DAWG0_State;
template<> struct DAWG0_State<State32> : State32 {
    typedef uint32_t  num_t;
	const static num_t max_num = 0xFFFFFFFF;
	uint32_t num;
	DAWG0_State() : num(0) {}
};
#pragma pack(pop)
#pragma pack(push, 1)
template<> struct DAWG0_State<State4B> : State4B {
    typedef uint32_t  num_t;
	const static num_t max_num = 0x00FFFFFF;
	uint32_t num : 24;
	DAWG0_State() : num(0) {}
};
template<> struct DAWG0_State<State5B> : State5B {
    typedef uint32_t  num_t;
	const static num_t max_num = 0x00FFFFFF;
	uint32_t num : 24;
	DAWG0_State() : num(0) {}
};
template<> struct DAWG0_State<State6B> : State6B {
    typedef uint32_t  num_t;
	const static num_t max_num = 0xFFFFFFFF;
	uint32_t num;
	DAWG0_State() : num(0) {}
};
#if FEBIRD_WORD_BITS >= 64 && defined(FEBIRD_INST_ALL_DFA_TYPES)
template<> struct DAWG0_State<State7B> : State7B {
    typedef uint64_t  num_t;
	const static num_t max_num = 0xFFFFFFFFFF; // 2**40
	uint64_t num : 40;
	DAWG0_State() : num(0) {}
};
typedef DAWG0_State<State7B> DAWG0_State12B;
BOOST_STATIC_ASSERT(sizeof(DAWG0_State12B) == 12);
#endif
#pragma pack(pop)
typedef DAWG0_State<State32> DAWG0_State32; // 12 Bytes, aligned
typedef DAWG0_State<State4B> DAWG0_State7B;
typedef DAWG0_State<State5B> DAWG0_State8B;
typedef DAWG0_State<State6B> DAWG0_State10B;
BOOST_STATIC_ASSERT(sizeof(DAWG0_State32 ) == 12);
BOOST_STATIC_ASSERT(sizeof(DAWG0_State7B ) ==  7);
BOOST_STATIC_ASSERT(sizeof(DAWG0_State8B ) ==  8);
BOOST_STATIC_ASSERT(sizeof(DAWG0_State10B) == 10);

// Indexed Directed Acyclic Word Graph
// Indexed Acyclic DFA, not required to be minimal
template<class State>
class DAWG0 : public Automata<State>, public DAWG_Interface
{
	typedef Automata<State> super;
    using super::states;
    typedef typename super::header_b32 header_b32;
    typedef typename super::header_max header_max;
    typedef typename super::Color Color;
	using DAWG_Interface::n_words;
public:
    typedef typename State::num_t      num_t;
    typedef typename State::state_id_t state_id_t;
    using super::nil_state;
    using DAWG_Interface::null_word;
    const static num_t max_num = State::max_num;

	DAWG0() { is_compiled = false; }

    void compile() {
		valvec<Color> marks(states.size(), Color::White);
        compile_loop(initial_state, &marks[0]);
		is_compiled = true;
		n_words = states[initial_state].num;
    }
private:
    // true if graph from curr is a DAG
    void compile_loop(state_id_t curr, Color* mark) {
        assert(curr < (state_id_t)states.size());
        if (Color::Grey == mark[curr]) {
            assert(0); // is not a dag
			throw std::logic_error("calling DAWG0::compile on cyclic automata");
        }
        mark[curr] = Color::Grey;
		size_t num = 0; // may > num_t
		this->for_each_dest(curr, [&](state_id_t, state_id_t target) {
				this->compile_loop(target, mark);
				num += this->states[target].num;
			});
        if (states[curr].is_term())
            num++;
        assert(num >= 1);
		if (initial_state == curr) {
			n_words = num;
		}
		if (num >= max_num) {
			if (initial_state == curr)
				states[curr].num = max_num;
			else
				throw std::out_of_range("num for dawg index exceed max_num");
		} else
			states[curr].num = num;
        mark[curr] = Color::Black;
    }
	void finish_load_mmap(const DFA_MmapHeader*) override;
	void prepare_save_mmap(DFA_MmapHeader*, const void**) const override;
public:
    size_t index(const fstring str) const override {
		assert(is_compiled);
		ASSERT_isNotFree(initial_state);
        state_id_t curr = initial_state;
        size_t idx = 0;
        for (size_t i = 0; ; ++i) {
			const State& s = states[curr];
			if (s.is_pzip()) {
				const byte_t* s2 = this->get_zpath_data(s);
				size_t n = *s2++;
				assert(n > 0);
				if (i + n > str.size())
					return null_word;
				size_t j = 0;
				do { // prefer do .. while for performance
					if ((byte_t)str.p[i++] != s2[j++])
						return null_word;
				} while (j < n);
			}
			if (str.size() == i)
				return s.is_term() ? idx : null_word;
			const byte_t ch = str.p[i];
			if (s.is_term()) {
				idx++;
				assert(idx < n_words);
			}
            state_id_t target = nil_state;
			this->for_each_move_break(curr,
			  [&,ch](state_id_t, state_id_t child, byte_t ch0)->bool {
				assert(child < this->states.size());
				if (ch0 < ch) {
					idx += this->states[child].num;
					// idx == n_words when str is larger than all words
					assert(idx <= this->n_words);
					return true;
				} else if (ch0 == ch) {
					target = child;
				}
				return false;
			});
			if (nil_state == target)
				return null_word;
			assert(target < states.size());
			ASSERT_isNotFree(target);
			curr = target;
        }
    }

	template<class Range, class OnMatch, class Translator>
    size_t match_words(Range& r, OnMatch on_match, Translator tr) const {
		assert(is_compiled);
		ASSERT_isNotFree(initial_state);
        state_id_t curr = initial_state;
        size_t idx = 0;
        for (size_t i = 0; ; ++i, ++r) {
			const State& s = states[curr];
			if (s.is_pzip()) {
				const byte_t* s2 = this->get_zpath_data(s);
				size_t n2 = *s2++;
				for (size_t j = 0; j < n2; ++j, ++r)
					if (r.empty() || (byte_t)tr(*r) != s2[j]) return i+j;
				i += n2;
			}
			assert(idx < n_words);
			if (s.is_term())
			   on_match(i, idx++);
			if (r.empty())
				return i;
			const byte_t ch = tr(*r);
            state_id_t target = nil_state;
			ASSERT_isNotFree(target);
			this->for_each_move_break(curr,
			  [&,ch](state_id_t, state_id_t child, byte_t ch0)->bool {
				assert(child < this->states.size());
				if (ch0 < ch) {
					idx += this->states[child].num;
					return true;
				} else if (ch0 == ch) {
					target = child;
					return false;
				}
				assert(0); // shouldn't go here!
				abort();
				return false;
			});
			if (nil_state == target)
				return i;
			curr = target;
        }
    }

	void nth_word(size_t nth, std::string* word) const override {
		assert(is_compiled);
		if (nth >= n_words) {
			string_appender<> msg;
			msg << __FILE__ ":" << __LINE__
			   	<< " , func: " << BOOST_CURRENT_FUNCTION
				<< ", nth=" << nth
				<< ", n_words=" << n_words;
			throw std::out_of_range(msg);
		}
		word->resize(0);
		state_id_t s = initial_state;
		size_t hit = 0;
		for (;;) {
			assert(hit <= nth);
			const State& ss = states[s];
			if (ss.is_pzip()) {
				const byte_t* s2 = this->get_zpath_data(ss);
				size_t n = *s2++;
				assert(n > 0);
				word->append((const char*)s2, n);
			}
			if (ss.is_term()) {
				if (hit == nth)
					break;
				++hit;
			}
			assert(hit <= nth);
			byte_t the_ch = 0;
			state_id_t next = nil_state;
			this->for_each_move_break(s, // sequencial search
				[&,nth](state_id_t, state_id_t child, byte_t ch)->bool {
					size_t hit2 = hit + this->states[child].num;
					the_ch = ch, next = child;
					if (hit2 <= nth) {
						hit = hit2;
						return true;
					} else
						return false;
				});
			assert(nil_state != next);
			word->push_back(the_ch);
			s = next;
		}
	}

	void dot_write_one_state(FILE* fp, size_t s) const override {
		if (!is_compiled) {
			super::dot_write_one_state(fp, s);
			return;
		}
		long ls = s;
		long num = states[ls].num;
		if (this->states[ls].is_pzip()) {
			const byte_t* s2 = this->get_zpath_data(this->states[ls]);
			int n = *s2++;
			if (states[ls].is_term())
				fprintf(fp, "\tstate%ld[label=\"%ld:%.*s=%ld\" shape=\"doublecircle\"];\n", ls, ls, n, s2, num);
			else
				fprintf(fp, "\tstate%ld[label=\"%ld:%.*s=%ld\"];\n", ls, ls, n, s2, num);
		}
		else {
			if (states[ls].is_term())
				fprintf(fp, "\tstate%ld[label=\"%ld=%ld\" shape=\"doublecircle\"];\n", ls, ls, num);
			else
				fprintf(fp, "\tstate%ld[label=\"%ld=%ld\"];\n", ls, ls, num);
		}
    }
	void clear() {
		super::clear();
		is_compiled = false;
		n_words = 0;
	}
	void swap(DAWG0& y) {
		assert(typeid(*this) == typeid(y));
		std::swap(is_compiled, y.is_compiled);
		std::swap(n_words    , y.n_words);
		super::swap(y);
	}

	template<class DataIO>
	friend void DataIO_loadObject(DataIO& dio, DAWG0& da) {
		DataIO_loadObject(dio, static_cast<super&>(da));
		if (!da.m_is_dag) {
			typedef typename DataIO::my_DataFormatException bad_format;
			throw bad_format("DAWG must be a DAG");
		}
		typename DataIO::my_var_uint64_t cnt;
		dio >> cnt;
		da.n_words = cnt.t;
		da.is_compiled = true;
	}
	template<class DataIO>
	friend void DataIO_saveObject(DataIO& dio, const DAWG0& da) {
		if (!da.is_compiled) {
			throw std::logic_error(BOOST_CURRENT_FUNCTION);
		}
		DataIO_saveObject(dio, static_cast<const super&>(da));
		dio << typename DataIO::my_var_uint64_t(da.n_words);
	}
private:
    bool is_compiled;
typedef DAWG0 MyType;
#include "ppi/dawg_const_virtuals.hpp"
#include "ppi/dfa_methods_calling_swap_using_super.hpp"
#include "ppi/dfa_methods_calling_swap.hpp"
};

#endif

