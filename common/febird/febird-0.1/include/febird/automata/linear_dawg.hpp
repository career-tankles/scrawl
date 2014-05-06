#ifndef __febird_automata_linear_dawg_hpp__
#define __febird_automata_linear_dawg_hpp__

#include <febird/num_to_str.hpp>
#include <febird/valvec.hpp>
#include <boost/mpl/if.hpp>
#include "dfa_algo.hpp"

template<int StateBytes>
class LinearDAWG :
   	public  DFA_VirtualAdapter<LinearDAWG<StateBytes> >,
   	public DAWG_VirtualAdapter<LinearDAWG<StateBytes> >
{
	typedef DAWG_VirtualAdapter<LinearDAWG<StateBytes> > base_dawg;
	typedef typename boost::mpl::if_c<(sizeof(long) >= StateBytes-1)
							, unsigned long
							, unsigned long long
							>
							::type link_uint_t;
	using DAWG_Interface::n_words;
public:
	typedef typename boost::mpl::if_c<(sizeof(int) >= StateBytes-1)
							, unsigned int
							, uint64_t
							>
							::type state_id_t;
	#pragma pack(push,1)
	struct State {
		union u_ {
			struct state_ {
				link_uint_t n_cnt : StateBytes*8 - 10; // size of right language
				unsigned pzip_bit : 1;
				unsigned term_bit : 1;
				unsigned n_trans  : 8; // range: [1, 256] inclusive
			} s;
			struct edge_ {
				link_uint_t dest : StateBytes*8 - 8;
				unsigned    ch   : 8;
			} e;
			struct zpath_ {
				unsigned char data[StateBytes];
			} z;
		} u;
		State() {
			memset(this, 0, sizeof(*this));
		}
	};
	#pragma pack(pop)
	BOOST_STATIC_ASSERT(sizeof(State) == StateBytes);

	typedef State state_t;
	typedef state_id_t transition_t;
	static const state_id_t  max_state = state_id_t((uint64_t(1) << (StateBytes*8 - 8)) - 1);
	static const state_id_t null_state = max_state;
	static const size_t max_cnt_in_state = state_id_t((uint64_t(1) << (StateBytes*8 - 10)) - 1);
	using DAWG_Interface::null_word;

	LinearDAWG() {
	   	zpath_states = 0;
		states.resize(2);
		states[1].u.e.dest = null_state;
   	}

	void clear() {
		states.clear();
		zpath_states = 0;
	}
	void erase_all() {
		states.erase_all();
		zpath_states = 0;
	}
	void swap(LinearDAWG& y) {
		states.swap(y.states);
		std::swap(zpath_states, y.zpath_states);
	}

	bool has_freelist() const override final { return false; }

	size_t total_states() const { return states.size(); }
	size_t mem_size() const override final { return sizeof(State) * states.size(); }

	template<class Au>
	void build_from(const char* WalkerName, const Au& au, typename Au::state_id_t root = initial_state) {
		if (strcasecmp(WalkerName, "BFS") == 0) {
			build_from_aux<BFS_GraphWalker<typename Au::state_id_t> >(au, root);
			return;
		}
		if (strcasecmp(WalkerName, "DFS") == 0) {
			build_from_aux<DFS_GraphWalker<typename Au::state_id_t> >(au, root);
			return;
		}
		if (strcasecmp(WalkerName, "PFS") == 0) {
			build_from_aux<PFS_GraphWalker<typename Au::state_id_t> >(au, root);
			return;
		}
		throw std::invalid_argument("LinearDAWG::build_from: unknown WalkerName");
	}
	template<class OP>
	void for_each_dest(state_id_t curr, OP op) const {
		assert(curr + 1 < states.size());
		size_t n = states[curr].u.s.n_trans + 1;
		if (1 == n) {
			auto dest = states[curr+1].u.e.dest;
			if (null_state != dest)
				op(curr, dest);
			return;
		}
		size_t i = 0;
		do { ++i;
			assert(curr+i < states.size());
			state_id_t dest = states[curr+i].u.e.dest;
			assert(null_state != dest);
			op(curr, dest);
		} while (i < n);
	}
	template<class OP>
	void for_each_dest_rev(state_id_t curr, OP op) const {
		assert(curr + 1 < states.size());
		size_t n = states[curr].u.s.n_trans + 1;
		if (1 == n) {
			auto dest = states[curr+1].u.e.dest;
			if (null_state != dest)
				op(curr, dest);
			return;
		}
		size_t i = n+1;
		do { --i;
			assert(curr+i < states.size());
			state_id_t dest = states[curr+i].u.e.dest;
			assert(null_state != dest);
			op(curr, dest);
		} while (i > 1);
	}
	template<class OP>
	void for_each_move(state_id_t curr, OP op) const {
		assert(curr + 1 < states.size());
		size_t n = states[curr].u.s.n_trans + 1;
		if (1 == n) {
			auto e = states[curr+1].u.e;
			if (null_state != e.dest)
				op(curr, e.dest, e.ch);
			return;
		}
		size_t i = 0;
		do { ++i;
			assert(curr+i < states.size());
			state_id_t dest = states[curr+i].u.e.dest;
			assert(null_state != dest);
			op(curr, dest, states[curr+i].u.e.ch);
		} while (i < n);
	}
	state_id_t state_move(state_id_t curr, unsigned char ch) const {
		assert(curr + 1 < states.size());
		size_t n = states[curr].u.s.n_trans + 1;
		assert(curr + n < states.size());
	#if 1 // add linear search will increase code size
		if (n < 8) { // linear search for small array
			for (size_t i = 1; i < n+1; ++i) { 
				if (states[curr+i].u.e.ch == ch)
					return states[curr+i].u.e.dest;
			}
			return null_state;
		}
	#endif
		// binary search
		size_t lo = curr + 1, hi = curr + n+1;
		while (lo < hi) {
			size_t mid = (lo + hi) / 2;
			if (states[mid].u.e.ch < ch)
				lo = mid + 1;
			else
				hi = mid;
		}
		if (states[lo].u.e.ch == ch)
			return states[lo].u.e.dest;
		else
			return null_state;
	}
	bool is_pzip(size_t s) const {
		assert(s < states.size());
		return states[s].u.s.pzip_bit;
	}
	bool is_term(size_t s) const {
		assert(s < states.size());
		return states[s].u.s.term_bit;
	}
	bool is_free(size_t s) const {
		assert(s < states.size());
		return false;
	}
	const unsigned char* get_zpath_data(size_t s) const {
		assert(s+1 < states.size());
		assert(states[s].u.s.pzip_bit);
		size_t n_trans = states[s].u.s.n_trans + 1;
		const unsigned char* p = states[s+1+n_trans].u.z.data;
#ifndef NDEBUG
		size_t zlen = *p;
		size_t zp_slots = slots_of_bytes(zlen+1);
		assert(s + zp_slots < states.size());
#endif
	//	printf("LinearDAWG::get_zpath_data(%zd) = %02X: %.*s\n", s, *p, *p, p+1);
		return p;
	}
    size_t index(const fstring str) const override final {
	//	assert(n_words > 0);
        state_id_t curr = initial_state;
        size_t idx = 0;
        for (size_t i = 0; ; ++i) {
			if (this->is_pzip(curr)) {
				const byte_t* s2 = this->get_zpath_data(curr);
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
				return this->is_term(curr) ? idx : null_word;
			if (this->is_term(curr)) {
				idx++;
				assert(idx < n_words);
			}
			size_t n_trans = states[curr].u.s.n_trans + 1;
			if (1 == n_trans && null_state == states[curr+1].u.e.dest) {
			//	printf("null_word on a final state which has no any child\n");
				return null_word;
			}
			const char_t ch = str.p[i];
			for (size_t j = 0; j < n_trans; ++j) {
				const auto e = states[curr+1+j].u.e;
			//	printf("i=%zd n_trans=%zd j=%zd e.ch=%02X e.dest.n_cnt=%zd\n"
			//		, i, n_trans, j, e.ch, states[e.dest].u.s.n_cnt);
				if (e.ch < ch) {
					idx += states[e.dest].u.s.n_cnt;
					// idx == n_words when str is larger than all words
					assert(idx <= n_words);
				} else if (e.ch == ch) {
					curr = e.dest;
					goto Found; // needn't to check idx==n_words
				} else
					return null_word;
			}
			if (idx == n_words) {
				// if str is larger than all words, it will go here!
				return null_word;
			}
		Found:;
		}
		assert(0);
    }

	using base_dawg::match_words;
	template<class ForwardIter, class OnMatch, class Translator>
    size_t match_words(ForwardIter first, ForwardIter last, OnMatch on_match, Translator tr) const {
		size_t len = std::distance(first, last);
        state_id_t curr = initial_state;
        size_t idx = 0;
        for (size_t i = 0; ; ++i) {
			if (this->is_pzip(curr)) {
				const byte_t* s2 = this->get_zpath_data(curr);
				size_t n2 = *s2++;
				size_t n  = std::min<size_t>(n2, len-i);
				for (size_t j = 0; j < n; ++i, ++j, ++first)
					if ((byte_t)tr(*first) != s2[j]) return i;
				if (n < n2)
					return len; // matched prefix of the zipped path
			}
			if (this->is_term(curr)) {
				on_match(i, idx);
				idx++;
				assert(idx <= n_words);
			}
			if (i == len)
				return i;
			const char_t ch = tr(*first);
			size_t n_trans = states[curr].u.s.n_trans + 1;
			if (1 == n_trans && null_state == states[curr+1].u.e.dest) {
				return i;
			}
			for (size_t j = 0; j < n_trans; ++j) {
				const auto e = states[curr+1+j].u.e;
				if (e.ch < ch) {
					idx += states[e.dest].u.s.n_cnt;
					assert(idx <= n_words);
				}
				else if (e.ch == ch) {
					curr = e.dest;
					goto Found;
				} else
					break;
			}
			return i;
		Found:
			++first;
        }
    }

	void nth_word(size_t nth, std::string* word) const override final {
		if (nth >= n_words) {
			string_appender<> msg;
			msg << __FILE__ ":" << __LINE__
			   	<< " , func: " << BOOST_CURRENT_FUNCTION
				<< ", nth=" << nth
				<< ", n_words=" << n_words;
			throw std::out_of_range(msg);
		}
		word->resize(0);
		state_id_t curr = initial_state;
		size_t hit = 0;
		for (;;) {
			assert(hit <= nth);
			if (this->is_pzip(curr)) {
				const byte_t* s2 = this->get_zpath_data(curr);
				size_t n = *s2++;
				assert(n > 0);
				word->append((const char*)s2, n);
			}
			if (this->is_term(curr)) {
				if (hit == nth)
					break;
				++hit;
			}
			assert(hit <= nth);
			size_t n_trans = states[curr].u.s.n_trans + 1;
			if (1 == n_trans && null_state == states[curr+1].u.e.dest) {
				abort(); // Unexpected
			}
			for (size_t j = 0; j < n_trans; ++j) {
				auto e = states[curr+1+j].u.e;
				assert(null_state != e.dest);
				size_t hit2 = hit + states[e.dest].u.s.n_cnt;
				if (hit2 <= nth) {
					hit = hit2;
				} else {
					word->push_back(e.ch);
					curr = e.dest;
					break;
				}
			}
		}
	}

	void dot_write_one_state(FILE* fp, size_t s) const override final {
		long ls = s;
		long num = states[ls].u.s.n_cnt;
		if (this->is_pzip(ls)) {
			const byte_t* s2 = this->get_zpath_data(ls);
			int n = *s2++;
			if (this->is_term(ls))
				fprintf(fp, "\tstate%ld[label=\"%ld:%.*s=%ld\" shape=\"doublecircle\"];\n", ls, ls, n, s2, num);
			else
				fprintf(fp, "\tstate%ld[label=\"%ld:%.*s=%ld\"];\n", ls, ls, n, s2, num);
		}
		else {
			if (this->is_term(ls))
				fprintf(fp, "\tstate%ld[label=\"%ld=%ld\" shape=\"doublecircle\"];\n", ls, ls, num);
			else
				fprintf(fp, "\tstate%ld[label=\"%ld=%ld\"];\n", ls, ls, num);
		}
    }

private:
	void compile() {
		assert(!states.empty());
		n_words = 0;
		dfa_algo(this).topological_sort(initial_state, [&](size_t parent) {
			assert(parent+1 < states.size());
			size_t pathes = states[parent].u.s.term_bit ? 1 : 0;
			this->for_each_dest(parent, [&](size_t parent, size_t child) {
				assert(child+1 < this->states.size());
				pathes += this->states[child].u.s.n_cnt;
			});
			if (initial_state == parent) {
				this->n_words = pathes;
			}
			if (pathes >= max_cnt_in_state) {
				if (initial_state == parent)
					states[parent].u.s.n_cnt = max_cnt_in_state;
				else
					throw std::out_of_range("LinearDAWG::compile, pathes is too large");
			} else
				states[parent].u.s.n_cnt = pathes;
		});
		assert(0 != n_words);
	}
	size_t right_language_size(size_t s) const {
		assert(s+1 < states.size());
		return states[s].u.s.n_cnt;
	}
	size_t zpath_slots(size_t s) const {
		size_t zlen = *this->get_zpath_data(s);
		return slots_of_bytes(zlen+1);
	}
	static size_t slots_of_bytes(size_t bytes) {
		return (bytes + sizeof(State)-1) / sizeof(State);
	}
	template<class Walker, class Au>
	void build_from_aux(const Au& au, typename Au::state_id_t root) {
		typedef typename Au::state_id_t au_state_id_t;
		valvec<au_state_id_t> map;
		map.resize_no_init(au.total_states());
		size_t idx = 0;
		Walker walker;
		walker.resize(au.total_states());
		walker.put_root(root);
		while (!walker.is_finished()) {
			au_state_id_t curr = walker.next();
			map[curr] = idx;
			size_t n_child = au.trans_num(curr);
			if (au.is_pzip(curr)) {
				size_t zlen = *au.get_zpath_data(curr);
				idx += slots_of_bytes(zlen+1);
			}
			idx += 1;
			idx += (0 == n_child) ? 1 : n_child;
			walker.putChildren(&au, curr);
		}
		if (idx >= max_state) {
			string_appender<> msg;
			msg << "LinearDAWG::build_from_aux: idx=" << idx
			   	<< " exceeds max_state=" << max_state;
			throw std::out_of_range(msg);
		}
		states.resize_no_init(idx);
		walker.resize(au.total_states());
		walker.put_root(root);
		idx = 0;
		while (!walker.is_finished()) {
			au_state_id_t curr = walker.next();
			size_t j = 0;
			assert(idx == map[curr]);
			states[idx].u.s.pzip_bit = au.is_pzip(curr);
			states[idx].u.s.term_bit = au.is_term(curr);
			au.for_each_move(curr,
				[&](au_state_id_t, au_state_id_t dst, unsigned char ch) {
					assert(dst < map.size());
					assert(idx+1+j < states.size());
					states[idx+1+j].u.e.ch = ch;
					states[idx+1+j].u.e.dest = map[dst];
					j++;
				});
			if (0 == j) {
				states[idx].u.s.n_trans = 0;
				states[idx].u.s.n_cnt = 0;
				states[idx+1].u.e.ch = 0;
				states[idx+1].u.e.dest = null_state;
				idx += 2;
			} else {
				states[idx].u.s.n_trans = j-1;
				states[idx].u.s.n_cnt = 0;
				idx += j+1;
			}
			if (au.is_pzip(curr)) {
				const unsigned char* zd = au.get_zpath_data(curr);
				int zlen = *zd;
				size_t zp_slots = slots_of_bytes(zlen+1);
				memcpy(states[idx].u.z.data, zd, zlen+1);
				idx += zp_slots;
			}
			walker.putChildren(&au, curr);
		}
		zpath_states = au.num_zpath_states();
		assert(states.size() == idx);
		compile();
	}
public:
	enum { SERIALIZATION_VERSION = 1 };
	template<class DataIO> void load_au(DataIO& dio, size_t version) {
		typename DataIO::my_var_uint64_t stateBytes, n_states, n_zpath_states;
		typename DataIO::my_var_uint64_t n_words2;
		dio >> stateBytes;
		dio >> n_zpath_states;
		dio >> n_states;
		dio >> n_words2;
		if (sizeof(State) != stateBytes.t) {
			typedef typename DataIO::my_DataFormatException bad_format;
			string_appender<> msg;
			msg << "LinearDAWG::load_au: StateBytes[";
			msg << "data=" << stateBytes << " ";
			msg << "code=" << sizeof(State) << "]";
			throw bad_format(msg);
		}
		states.resize_no_init(n_states.t);
		dio.ensureRead(&states[0], sizeof(State) * n_states.t);
		zpath_states = size_t(n_zpath_states.t);
		n_words = n_words2;
	}
	template<class DataIO> void save_au(DataIO& dio) const {
		dio << typename DataIO::my_var_uint64_t(sizeof(State));
		dio << typename DataIO::my_var_uint64_t(zpath_states);
		dio << typename DataIO::my_var_uint64_t(states.size());
		dio << typename DataIO::my_var_uint64_t(n_words);
		dio.ensureWrite(&states[0], sizeof(State) * states.size());
	}
	template<class DataIO>
	friend void DataIO_loadObject(DataIO& dio, LinearDAWG& au) {
		typename DataIO::my_var_uint64_t version;
		dio >> version;
		if (version > SERIALIZATION_VERSION) {
			typedef typename DataIO::my_BadVersionException bad_ver;
			throw bad_ver(version.t, SERIALIZATION_VERSION, "LinearDAWG");
		}
		au.load_au(dio, version.t);
	}
	template<class DataIO>
	friend void DataIO_saveObject(DataIO& dio, const LinearDAWG& au) {
		dio << typename DataIO::my_var_uint64_t(SERIALIZATION_VERSION);
		au.save_au(dio);
	}
protected:
	valvec<State> states;
	size_t zpath_states;
};

#endif // __febird_automata_linear_dawg_hpp__

