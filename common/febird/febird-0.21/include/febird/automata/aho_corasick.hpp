#ifndef __aho_corasick_penglei__
#define __aho_corasick_penglei__

#include "automata.hpp"
#include "double_array_trie.hpp"

template<class State>
struct AC_State;

template<>
struct AC_State<State32> : State32 {
    AC_State() {
        output = 0;
        lfail = 0; // initial_state
    }
    uint32_t output; // start index to total output set
    uint32_t lfail;  // link to fail state
	const static uint32_t max_output = max_state;
};
BOOST_STATIC_ASSERT(sizeof(AC_State<State32>) == 16);
#pragma pack(push,1)
template<>
struct AC_State<State5B> : State5B {
    AC_State() {
        output = 0;
        lfail = 0; // initial_state
    }
    unsigned output : 30; // start index to total output set
    unsigned lfail  : 26; // link to fail state
	const static uint32_t max_output = max_state;
};
BOOST_STATIC_ASSERT(sizeof(AC_State<State5B>) == 12);
template<>
struct AC_State<State4B> : State4B {
    AC_State() {
        output = 0;
        lfail = 0; // initial_state
    }
    unsigned output : 15; // start index to total output set
    unsigned lfail  : 17; // link to fail state
	const static uint32_t max_state = 0x1FFFF; // 128K-1
//	const static uint32_t nil_state = 0x1FFFF; // 128K-1, Don't override nil_state
	const static uint32_t max_output = 0x07FFF; //  32K-1
};
BOOST_STATIC_ASSERT(sizeof(AC_State<State4B>) == 8);

template<>
struct AC_State<DA_State8B> : DA_State8B {
    AC_State() {
        output = 0;
        lfail = 0; // initial_state
    }
    uint32_t output; // start index to total output set
    uint32_t lfail;  // link to fail state
	const static uint32_t max_output = max_state;
};
BOOST_STATIC_ASSERT(sizeof(AC_State<DA_State8B>) == 16);

#pragma pack(pop)

// Aho-Corasick implementation
template<class BaseAutomata>
class Aho_Corasick : public BaseAutomata, public AC_Scan_Interface {
    typedef BaseAutomata super;
public:
    typedef typename BaseAutomata::state_t    state_t;
    typedef typename BaseAutomata::state_id_t state_id_t;
    using super::states;
    using super::nil_state;
private:
//	word_id_t is typedef'ed in AC_Scan_Interface
    typedef valvec<word_id_t> output_t;
	output_t output; // compacted output

    void set_fail(state_id_t s, state_id_t f) { states[s].lfail = f; }
    state_id_t  ffail(state_id_t s) const { return states[s].lfail; }

    class BFS_AC_Builder; friend class BFS_AC_Builder;
    class BFS_AC_Builder2; friend class BFS_AC_Builder2;
public:
	const static word_id_t null_word = ~word_id_t(0);
	class ac_builder; friend class ac_builder;
	class ac_builder2; friend class ac_builder2;

	~Aho_Corasick();
	void finish_load_mmap(const DFA_MmapHeader*);
	void prepare_save_mmap(DFA_MmapHeader*, const void**) const;

    size_t mem_size() const {
        return super::mem_size() + sizeof(output[0]) * output.size();
    }
	void clear() {
		output.clear();
		super::clear();
	}
	void swap(Aho_Corasick& y) {
		assert(typeid(*this) == typeid(y));
		super::swap(y);
		output.swap(y.output);
		AC_Scan_Interface::swap(y);
	}
	using super::total_states;
	using super::total_transitions;
	using super::internal_get_states;
	using super::for_each_move;
//	using super::TreeWalker;

	void v_scan(fstring input, const OnHitWords& on_hit)
	const override final {
		scan(input, on_hit);
	}
	void v_scan(fstring input, const OnHitWords& on_hit, const ByteTR& tr)
	const override final {
		scan(input, on_hit, tr);
	}
	void v_scan(ByteInputRange& r, const OnHitWords& on_hit)
	const override final {
		scan_stream<ByteInputRange&, const OnHitWords&>(r, on_hit);
	}
	void v_scan(ByteInputRange& r, const OnHitWords& on_hit, const ByteTR& tr)
	const override final {
		scan_stream<ByteInputRange&, const OnHitWords&, const ByteTR&>
			(r, on_hit, tr);
	}

	const word_id_t* words(state_id_t state, size_t* num_words) const {
		assert(state < states.size()-1);
		size_t off0 = states[state+0].output;
		size_t off1 = states[state+1].output;
		*num_words = off1 - off0;
		return &output[off0];
	}

	template<class Offset>
	word_id_t find(const fstring str, const Offset* offsets) const {
		state_id_t curr = initial_state;
		for (size_t i = 0; i < str.size(); ++i) {
			state_id_t next = super::state_move(curr, (byte_t)str.p[i]);
			if (nil_state == next)
				return null_word;
			curr = next;
		}
		if (states[curr].is_term()) {
			size_t out0 = states[curr+0].output;
			size_t out1 = states[curr+1].output;
			assert(out1 > out0);
			size_t i = out0;
			do {
				word_id_t word_id = output[i];
				Offset off0 = offsets[word_id+0];
				Offset off1 = offsets[word_id+1];
				if (off1 - off0 == str.size())
					return word_id;
			} while (++i < out1);
		}
		return null_word;
	}

	/// @param on_match(size_t endpos, word_id)
	/// @return max forward scaned chars
	template<class GetOffset, class OnMatch>
	size_t match_prefix(const fstring str
			, GetOffset get_offset, OnMatch on_match) const {
		return match_prefix<GetOffset, OnMatch>(str, get_offset, on_match, IdentityTranslator());
	}

	/// @param on_match(size_t endpos, word_id)
	/// @return max forward scaned chars
	template<class GetOffset, class OnMatch, class Translator>
	size_t match_prefix(const fstring str 
			, GetOffset get_offset, OnMatch on_match, Translator tr) const {
		state_id_t curr = initial_state;
		for (size_t i = 0; ; ++i) {
		//	assert(!s.is_pzip()); // DoubleArrayTrie.State has no is_pzip
			if (states[curr].is_term()) {
				size_t out0 = states[curr+0].output;
				size_t out1 = states[curr+1].output;
				assert(out1 > out0);
				size_t j = out0;
				do {
					word_id_t word_id = output[j];
					size_t off0 = get_offset(word_id+0);
					size_t off1 = get_offset(word_id+1);
					if (off1 - off0 == i) {
						on_match(i, word_id);
						break;
					}
				} while (++j < out1);
			}
			if (str.size() == i)
				return i;
			state_id_t next = this->state_move(curr, (byte_t)tr(str.p[i]));
			if (nil_state == next)
				return i;
			assert(next < states.size());
			ASSERT_isNotFree(next);
			curr = next;
		}
		assert(0);
	}

    // res[i].first : KeyWord id
    // res[i].second: end position of str
	// Aho-Corasick don't save strlen(word_id)
	template<class Pos>
    void scan(const fstring str, valvec<std::pair<word_id_t, Pos> >* res) const {
		assert(!output.empty());
		scan(str, [=](size_t endpos, const word_id_t* hits, size_t n) {
						for (size_t i = 0; i < n; ++i) {
							std::pair<word_id_t, Pos> x;
							x.first = hits[i];
							x.second = Pos(endpos);
							res->push_back(x);
						}
					});
	}

	template<class OnHitWord>
    void scan(const fstring str, OnHitWord on_hit_word) const {
		typedef iterator_to_byte_stream<const char*> input_t;
		input_t input(str.begin(), str.end());
		scan_stream_imp<input_t, OnHitWord, IdentityTranslator>
			(input, on_hit_word, IdentityTranslator());
	}

	///@param on_hit_word(size_t end_pos_of_hit, const word_id_t* first, size_t num)
	///@note @Translator in match_prefix could be supported by custom InputStream
	template<class InputStream, class OnHitWord>
    void scan_stream(InputStream& input, OnHitWord on_hit_word) const {
		scan_stream_imp<InputStream&, OnHitWord, IdentityTranslator>
			(input, on_hit_word, IdentityTranslator());
	}

	template<class OnHitWord, class TR>
    void scan(const fstring str, OnHitWord on_hit_word, TR tr) const {
		typedef iterator_to_byte_stream<const char*> input_t;
		input_t input(str.begin(), str.end());
		scan_stream_imp<input_t, OnHitWord>(input, on_hit_word, tr);
	}

	///@param on_hit_word(size_t end_pos_of_hit, const word_id_t* first, size_t num)
	///@note @Translator in match_prefix could be supported by custom InputStream
	template<class InputStream, class OnHitWord, class TR>
    void scan_stream(InputStream& input, OnHitWord on_hit_word, TR tr) const {
		scan_stream_imp<InputStream&, OnHitWord>(input, on_hit_word, tr);
	}

private:
	// pass input by value
	template<class InputStream, class OnHitWord, class TR>
    void scan_stream_imp(InputStream input, OnHitWord on_hit_word, TR tr) const {
		assert(!output.empty());
        state_id_t curr = initial_state;
		for (size_t pos = 0; !input.empty(); ++pos) {
            byte_t c = tr(*input); ++input;
            state_id_t next;
            while ((next = this->state_move(curr, c)) == nil_state) {
                if (initial_state == curr)
                    goto ThisCharEnd;
                curr = ffail(curr);
                assert(curr < (state_id_t)states.size());
            }
            assert(next != initial_state);
            assert(next < (state_id_t)states.size());
            curr = next;
            if (states[curr].is_term()) {
                size_t oBeg = states[curr+0].output;
                size_t oEnd = states[curr+1].output;
				assert(oBeg < oEnd);
				assert(oEnd <= output.size());
				size_t end_pos_of_hit = pos + 1;
				on_hit_word(end_pos_of_hit, &output[oBeg], oEnd-oBeg);
            } else {
                // here: output of curr state must be empty
                assert(states[curr].output == states[curr+1].output);
            }
        ThisCharEnd:;
        }
    }

	template<class DoubleArrayAC, class Y_AC>
	friend void double_array_ac_build_from_au(DoubleArrayAC& x_ac, const Y_AC& y_ac, const char* BFS_or_DFS);

	template<class DataIO>
	friend void DataIO_loadObject(DataIO& dio, Aho_Corasick& x) {
		typename DataIO::my_var_uint64_t n_words;
		dio >> static_cast<super&>(x);
		dio >> x.output;
		dio >> n_words; x.n_words = n_words;
		byte_t word_ext;
		dio >> word_ext;
		if (word_ext >= 1) dio >> x.offsets;
		if (word_ext >= 2) dio >> x.strpool;
	}
	template<class DataIO>
	friend void DataIO_saveObject(DataIO& dio, const Aho_Corasick& x) {
		dio << static_cast<const super&>(x);
		dio << x.output;
		dio << typename DataIO::my_var_uint64_t(x.n_words);
		byte_t word_ext = 0;
		if (x.has_word_length()) word_ext = 1;
		if (x.has_word_content()) word_ext = 2;
		dio << word_ext;
		if (word_ext >= 1) dio << x.offsets;
		if (word_ext >= 2) dio << x.strpool;
	}
};

/// @param x_ac double array ac
template<class DoubleArrayAC, class Y_AC>
void double_array_ac_build_from_au(DoubleArrayAC& x_ac, const Y_AC& y_ac, const char* BFS_or_DFS) {
	typedef DoubleArrayAC X_AC;
	valvec<typename X_AC::state_id_t> map_y2x;
	valvec<typename Y_AC::state_id_t> map_x2y;
	const size_t extra = 1;
	x_ac.build_from(y_ac, map_x2y, map_y2x, BFS_or_DFS, extra);
	assert(x_ac.states.back().is_free());
	assert(x_ac.output.size() == 0);
	x_ac.output.reserve(y_ac.output.size());
	x_ac.states[0].output = 0;
	for (size_t x = 0; x < map_x2y.size()-extra; ++x) {
		typename Y_AC::state_id_t y = map_x2y[x];
		if (Y_AC::nil_state != y) {
			typename Y_AC::state_id_t f = y_ac.states[y].lfail;
			x_ac.states[x].lfail = map_y2x[f];
			size_t out0 = y_ac.states[y+0].output;
			size_t out1 = y_ac.states[y+1].output;
			x_ac.output.append(&y_ac.output[0]+out0, &y_ac.output[0]+out1);
		}
		x_ac.states[x+1].output = x_ac.output.size();
	}
	assert(x_ac.output.size() == y_ac.output.size());
	x_ac.n_words = y_ac.n_words;
	x_ac.offsets = y_ac.offsets;
	x_ac.strpool = y_ac.strpool;
}

template<class BaseAutomata>
class Aho_Corasick<BaseAutomata>::BFS_AC_Builder {
	Aho_Corasick* ac;
	valvec<output_t>* tmpOut;
public:
	BFS_AC_Builder(Aho_Corasick* ac, valvec<output_t>* tmpOut) {
		this->ac = ac;
		this->tmpOut = tmpOut;
	}
	void operator()(state_id_t parent, state_id_t curr, auchar_t c) const {
		assert(c < 256);
		state_id_t back = ac->ffail(parent);
		state_id_t next;
		while ((next = ac->state_move(back, c)) == nil_state) {
			if (initial_state == back) {
				ac->set_fail(curr, initial_state);
				return;
			}
			back = ac->ffail(back);
		}
		if (curr != next) {
			if (ac->states[next].is_term()) {
				ac->states[curr].set_term_bit();
				(*tmpOut)[curr] += (*tmpOut)[next];
			}
			ac->set_fail(curr, next);
		} else
			ac->set_fail(curr, initial_state);
	}
};

template<class BaseAutomata>
class Aho_Corasick<BaseAutomata>::ac_builder {
	Aho_Corasick* ac;
    valvec<output_t> tmpOut; // needs more memory than ac->output

public:
	static const state_id_t max_output = state_t::max_output;
	size_t all_words_size; // for easy access

	explicit ac_builder(Aho_Corasick* ac) { reset(ac); }

	void reset(Aho_Corasick* ac) {
		assert(ac->total_states() == 1);
		this->ac = ac;
		all_words_size = 0;
		ac->n_words = 0;
        tmpOut.resize(1); // keep parallel with states
		tmpOut[0].resize0();
	}

	///@returns null_word : key is a new word
	///         others    : the existing word_id of key
	word_id_t& add_word(const fstring key) {
		assert(key.n > 0); // don't allow empty key
		state_id_t curr = initial_state;
		size_t j = 0;
		while (j < key.size()) {
			state_id_t next = ac->state_move(curr, (byte_t)key.p[j]);
			if (nil_state == next)
				goto AddNewStates;
			assert(next < (state_id_t)ac->states.size());
			curr = next;
			++j;
		}
		// key could be the prefix of some existed key word
		if (ac->states[curr].is_term()) {
			assert(tmpOut[curr].size() == 1);
			return tmpOut[curr][0]; // return the existing kid ref
		} else {
			assert(tmpOut[curr].size() == 0);
			goto DoAdd;
		}
	AddNewStates:
		do {
			state_id_t next = ac->new_state();
			ac->add_move_checked(curr, next, (byte_t)key.p[j]);
			curr = next;
			++j;
		} while (j < key.size());
		// now curr == next, and it is a Terminal State
		assert(tmpOut.size() < ac->states.size());
		tmpOut.reserve(ac->states.capacity());
		tmpOut.resize(ac->states.size());
	DoAdd:
		if (ac->n_words == max_output) {
			string_appender<> msg;
			msg << "num_words exceeding max_output: " << BOOST_CURRENT_FUNCTION;
			throw std::out_of_range(msg);
		}
		tmpOut[curr].push_back((word_id_t)null_word);
		ac->states[curr].set_term_bit();
		all_words_size += key.size();
		ac->n_words += 1;
		return tmpOut[curr][0];
	}

    void compile() {
    //    printf("size[tmpOut=%ld, ac->states=%ld]\n",
    //            (long)tmpOut.size(), (long)ac->states.size());
        assert(tmpOut.size() == ac->states.size());

        // build failure function
        ac->set_fail(initial_state, initial_state);
        ac->bfs_tree(initial_state, BFS_AC_Builder(ac, &tmpOut));

        // compress output set
        size_t total = 0;
        for (size_t i = 0, n = tmpOut.size(); i < n; ++i)
            total += tmpOut[i].size();
		if (total > max_output) {
			string_appender<> msg;
			msg << "num_words exceed max_output: " << BOOST_CURRENT_FUNCTION;
			throw std::out_of_range(msg);
		}
        ac->output.reserve(total);
        total = 0;
        for (size_t i = 0, n = tmpOut.size(); i < n; ++i) {
            ac->output += tmpOut[i];
            ac->states[i].output = total;
            total += tmpOut[i].size();
        }
        ac->states.push_back(); // for guard 'output' =>
        ac->states.back().output = total; // this state is not reachable

        ac->pool.shrink_to_fit();
        ac->states.shrink_to_fit();
        tmpOut.clear();
    }

	void fill_word_offsets() {
		set_word_id_as_lex_ordinal(&ac->offsets, NULL);
	}
	void fill_word_contents() {
		set_word_id_as_lex_ordinal(&ac->offsets, &ac->strpool);
	}

	void set_word_id_as_lex_ordinal() {
		ac->for_each_word(
			[&](size_t nth, fstring, state_id_t s) {
				assert(tmpOut[s].size() == 1);
				tmpOut[s][0] = nth; // overwrite old_word_id
			});
	}

	/// When calling add_word, pass word_id as any thing, then call this function
	/// then call compile:
	///@code
	///   AC_T::ac_builder builder(&ac);
	///   builder.add_word("abc");
	///   ...
	///   builder.set_word_id_as_lex_ordinal(&offsets, &strpool);
	///   builder.compile();
	///   ac.scan(...);
	///@endcode
	template<class Offset>
	void set_word_id_as_lex_ordinal(valvec<Offset>* offsets, valvec<char>* strpool) {
		if (strpool)
			strpool->resize(0);
		offsets->resize(0);
		offsets->reserve(ac->n_words+1);
		if (strpool)
			strpool->reserve(all_words_size);
		Offset curr_offset = 0;
		ac->tpl_for_each_word(
			[&](size_t nth, fstring word, state_id_t s) {
				assert(tmpOut[s].size() == 1);
				tmpOut[s][0] = nth; // overwrite old_word_id
				offsets->unchecked_push_back(curr_offset);
				if (strpool)
					strpool->append(word);
				curr_offset += word.size();
			});
		offsets->unchecked_push_back(curr_offset);
		assert(offsets->size() == ac->n_words+1);
		assert(curr_offset == all_words_size);
	}

	template<class StrVec>
	void set_word_id_as_lex_ordinal(StrVec* vec) {
		vec->resize(0);
		vec->reserve(ac->n_words);
		ac->tpl_for_each_word(
			[&](size_t nth, fstring word, state_id_t s) {
				assert(tmpOut[s].size() == 1);
				tmpOut[s][0] = nth; // overwrite old_word_id
				vec->push_back(word);
			});
		vec->shrink_to_fit();
		assert(vec->size() == ac->n_words);
	}

	template<class WordCollector>
	void set_word_id_as_lex_ordinal(WordCollector word_collector) {
		ac->tpl_for_each_word(
			[=](size_t nth, fstring word, state_id_t s) {
				assert(tmpOut[s].size() == 1);
				size_t old_word_id = tmpOut[s][0];
				tmpOut[s][0] = nth; // nth is new word_id
				word_collector(nth, old_word_id, word);
			});
	}

	void sort_by_word_len() {
		sort_by_word_len(ac->offsets);
	}

	/// @note: before calling this function:
	///   -# word_id must be lex_ordinal
	///   -# WordAttribute array contains the offsets as set_word_id_as_lex_ordinal
	template<class Offset>
	void sort_by_word_len(const valvec<Offset>& offsets) {
		sort_by_word_len(offsets.begin(), offsets.end());
	}
	template<class Offset>
	void sort_by_word_len(const Offset* beg, const Offset* end) {
		sort_by_word_len(beg, end, [](Offset off){return off;});
	}
	// Offset could be a member of WordAttributs, it is extracted by get_offset
	template<class WordAttributs, class GetOffset>
	void sort_by_word_len(const valvec<WordAttributs>& word_attrib, GetOffset get_offset) {
		sort_by_word_len(word_attrib.begin(), word_attrib.end(), get_offset);
	}
	template<class WordAttributs, class GetOffset>
	void sort_by_word_len(const WordAttributs* beg, const WordAttributs* end, GetOffset get_offset) {
		assert(ac->n_words + 1  == size_t(end-beg));
		assert(all_words_size == size_t(get_offset(end[-1])));
		const auto* states = &ac->states[0];
		word_id_t * output = &ac->output[0];
		for (size_t i = 0, n = ac->states.size()-1; i < n; ++i) {
			size_t out0 = states[i+0].output;
			size_t out1 = states[i+1].output;
			assert(out0 <= out1);
			if (out1 - out0 > 1) {
				std::sort(output+out0, output+out1,
				[=](word_id_t x, word_id_t y) {
					size_t xoff0 = get_offset(beg[x+0]);
					size_t xoff1 = get_offset(beg[x+1]);
					size_t yoff0 = get_offset(beg[y+0]);
					size_t yoff1 = get_offset(beg[y+1]);
					assert(xoff0 < xoff1); // assert not be empty
					assert(yoff0 < yoff1); // assert not be empty
					return xoff1 - xoff0 > yoff1 - yoff0; // greater
				});
			}
		}
	}
};

typedef Aho_Corasick<DoubleArrayTrie<AC_State<DA_State8B> > > Aho_Corasick_DoubleArray_8B;

typedef Aho_Corasick<Automata<AC_State<State32> > > Aho_Corasick_SmartDFA_AC_State32;
typedef Aho_Corasick<Automata<AC_State<State5B> > > Aho_Corasick_SmartDFA_AC_State5B;
typedef Aho_Corasick<Automata<AC_State<State4B> > > Aho_Corasick_SmartDFA_AC_State4B;
//typedef Aho_Corasick<Automata<AC_State<State6B> > > Aho_Corasick_SmartDFA_AC_State6B;


#endif // __aho_corasick_penglei__

