#ifndef __febird_automata_linear_dfa2_hpp__
#define __febird_automata_linear_dfa2_hpp__

#include "linear_dfa.hpp"

template<int StateBytes>
class LinearDFA2 : public DFA_VirtualAdapter<LinearDFA2<StateBytes> > {
	typedef typename boost::mpl::if_c<(sizeof(long) >= StateBytes-1)
							, unsigned long
							, unsigned long long
							>
							::type link_uint_t;
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
				link_uint_t  dest : StateBytes * 8 - 8;
				unsigned       ch : 8;
			} s;
			struct zpath_ {
				unsigned char data[StateBytes];
			} z;
		} u;
		State() {
			memset(this, 0, sizeof(*this));
			u.s.dest = null_state;
		}
	};
	#pragma pack(pop)
	BOOST_STATIC_ASSERT(sizeof(State) == StateBytes);

	typedef State state_t;
	typedef state_id_t transition_t;
	static const state_id_t  max_state = state_id_t((uint64_t(1) << (StateBytes*8 - 8)) - 1);
	static const state_id_t null_state = max_state;

	LinearDFA2() {
		states.resize(1);
		zpath_states = 0;
	}

	void clear() {
		states.clear();
		zpath_states = 0;
	}
	void erase_all() {
		states.erase_all();
		zpath_states = 0;
	}
	void swap(LinearDFA2& y) {
		states.swap(y.states);
		std::swap(zpath_states, y.zpath_states);
	}

	bool has_freelist() const override final { return false; }

	size_t total_states() const { return states.size(); }
	size_t mem_size() const override final { return sizeof(State) * states.size() + m_bits.size()/8; }

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
		throw std::invalid_argument("LinearDFA2::build_from: unknown WalkerName");
	}
	template<class OP>
	void for_each_dest(state_id_t curr, OP op) const {
		assert(curr < states.size());
		size_t i;
		if (get_pzip_bit(curr)) {
			state_id_t dest = states[curr].u.s.dest;
			if (null_state != dest)
				op(curr, dest);
			if (get_last_bit(curr))
				return;
			i = curr + zpath_slots(curr);
		} else {
			i = size_t(curr)-1;
		}
		do { ++i;
			assert(i < states.size());
			state_id_t dest = states[i].u.s.dest;
			if (null_state != dest)
				op(curr, dest);
		} while (!get_last_bit(i));
	}
	template<class OP>
	void for_each_dest_rev(state_id_t curr, OP op) const {
		assert(curr < states.size());
		size_t i, n;
		if (get_pzip_bit(curr)) {
			state_id_t dest = states[curr].u.s.dest;
			if (get_last_bit(curr))
				i = curr;
			else {
				i = curr + zpath_slots(curr);
			}
			n = 1;
		} else {
			i = size_t(curr)-1;
			n = 0;
		}
		do { ++i; ++n;
			assert(i < states.size());
			state_id_t dest = states[i].u.s.dest;
		} while (!get_last_bit(i));
		if (1 == n) {
			state_id_t dest = states[curr].u.s.dest;
			if (null_state != dest)
				op(curr, dest);
		}
		else if (get_pzip_bit(curr)) {
			for(size_t j = 0; j < n-1; ++j) {
				size_t k = i - j;
				state_id_t dest = states[k].u.s.dest;
				assert(null_state != dest);
				op(curr, dest);
			}
			state_id_t dest = states[curr].u.s.dest;
			assert(null_state != dest);
			op(curr, dest);
		}
		else {
			for(size_t j = 0; j < n; ++j) {
				size_t k = i - j;
				state_id_t dest = states[k].u.s.dest;
				assert(null_state != dest);
				op(curr, dest);
			}
		}
	}
	template<class OP>
	void for_each_move(state_id_t curr, OP op) const {
		assert(curr < states.size());
		size_t i;
		if (get_pzip_bit(curr)) {
			state_id_t dest = states[curr].u.s.dest;
			if (null_state != dest)
				op(curr, dest, states[curr].u.s.ch);
			if (get_last_bit(curr))
				return;
			i = curr + zpath_slots(curr);
		} else {
			i = size_t(curr)-1;
		}
		do { ++i;
			assert(i < states.size());
			state_id_t dest = states[i].u.s.dest;
			if (null_state != dest)
				op(curr, dest, states[i].u.s.ch);
		} while (!get_last_bit(i));
	}
	state_id_t state_move(state_id_t curr, unsigned char ch) const {
		assert(curr < states.size());
		size_t i;
		if (get_pzip_bit(curr)) {
			if (ch == states[curr].u.s.ch)
				return states[curr].u.s.dest;
			if (get_last_bit(curr))
				return null_state;
			i = curr + zpath_slots(curr);
		} else {
			i = size_t(curr)-1;
		}
		do { ++i;
			assert(i < states.size());
			if (ch == states[i].u.s.ch)
				return states[i].u.s.dest;
		} while (!get_last_bit(i));
		return null_state;
	}
	bool is_pzip(size_t s) const {
		assert(s < states.size());
		return get_pzip_bit(s);
	}
	bool is_term(size_t s) const {
		assert(s < states.size());
		return get_term_bit(s);
	}
	bool is_free(size_t s) const {
		assert(s < states.size());
		return false;
	}
	const unsigned char* get_zpath_data(size_t s) const {
		assert(s+1 < states.size());
		assert(get_pzip_bit(s));
		const unsigned char* p = states[s+1].u.z.data;
#ifndef NDEBUG
		size_t zlen = *p;
		size_t zp_slots = slots_of_bytes(zlen+1);
		assert(s + zp_slots < states.size());
#endif
	//	printf("LinearDFA2::get_zpath_data(%zd) = %02X: %.*s\n", s, *p, *p, p+1);
		return p;
	}
private:
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
			idx += (0 == n_child) ? 1 : n_child;
			walker.putChildren(&au, curr);
		}
		if (idx >= max_state) {
			throw std::out_of_range("LinearDFA2::build_from_aux: idx exceeds max_state");
		}
		states.resize_no_init(idx);
		m_bits.resize_no_init(idx*3);
		walker.resize(au.total_states());
		walker.put_root(root);
		idx = 0;
		while (!walker.is_finished()) {
			au_state_id_t curr = walker.next();
			size_t j = 0;
			bool b_is_term = au.is_term(curr);
			assert(idx == map[curr]);
			if (au.is_pzip(curr)) {
				const unsigned char* zd = au.get_zpath_data(curr);
				int zlen = *zd;
				size_t zp_slots = slots_of_bytes(zlen+1);
				au.for_each_move(curr,
					[&](au_state_id_t, au_state_id_t dst, unsigned char ch) {
						assert(dst < map.size());
						assert(idx+j < states.size());
						this->set_last_bit(idx+j, 0);
						this->set_pzip_bit(idx+j, 1);
						this->set_term_bit(idx+j, b_is_term);
						states[idx+j].u.s.ch = ch;
						states[idx+j].u.s.dest = map[dst];
						if (0 == j) {
						//	printf("1: j=%zd zlen+1=%d zp_slots=%zd : %.*s\n", j, zlen+1, zp_slots, zlen, zd+1);
							memcpy(states[idx+1].u.z.data, zd, zlen+1);
							j = 1 + zp_slots;
						} else
							j++;
					});
				if (0 == j) {
				//	printf("2: j=%zd zlen+1=%d zp_slots=%zd : %.*s\n", j, zlen+1, zp_slots, zlen, zd+1);
					this->set_last_bit(idx, 1);
					this->set_pzip_bit(idx, 1);
					this->set_term_bit(idx, b_is_term);
					states[idx].u.s.ch = 0;
					states[idx].u.s.dest = null_state;
					memcpy(states[idx+1].u.z.data, zd, zlen+1);
					j = 1 + zp_slots;
				}
				else if (1 + zp_slots == j)
					this->set_last_bit(idx, 1); // last edge
				else
					this->set_last_bit(idx+j-1, 1); // last edge
			}
			else {
				au.for_each_move(curr,
					[&](au_state_id_t, au_state_id_t dst, unsigned char ch) {
						assert(dst < map.size());
						assert(idx+j < states.size());
						this->set_last_bit(idx+j, 0);
						this->set_pzip_bit(idx+j, 0);
						this->set_term_bit(idx+j, b_is_term);
						states[idx+j].u.s.ch = ch;
						states[idx+j].u.s.dest = map[dst];
						j++;
					});
				if (0 == j) {
					this->set_last_bit(idx, 1);
					this->set_pzip_bit(idx, 0);
					this->set_term_bit(idx, b_is_term);
					states[idx].u.s.ch = 0;
					states[idx].u.s.dest = null_state;
					j = 1;
				} else
					this->set_last_bit(idx+j-1, 1); // last edge
			}
			idx += j;
			walker.putChildren(&au, curr);
		}
		zpath_states = au.num_zpath_states();
		assert(states.size() == idx);
	}
public:
	enum { SERIALIZATION_VERSION = 1 };
	template<class DataIO> void load_au(DataIO& dio, size_t version) {
		typename DataIO::my_var_uint64_t stateBytes, n_states, n_zpath_states;
		dio >> stateBytes;
		dio >> n_zpath_states;
		dio >> n_states;
		if (sizeof(State) != stateBytes.t) {
			typedef typename DataIO::my_DataFormatException bad_format;
			string_appender<> msg;
			msg << "LinearDFA2::load_au: StateBytes[";
			msg << "data=" << stateBytes << " ";
			msg << "code=" << sizeof(State) << "]";
			throw bad_format(msg);
		}
		states.resize_no_init(n_states.t);
		m_bits.resize_no_init(n_states.t*3);
		dio.ensureRead(&states[0], sizeof(State) * n_states.t);
		dio.ensureRead(m_bits.risk_data(), m_bits.size()/8);
		zpath_states = size_t(n_zpath_states.t);
	}
	template<class DataIO> void save_au(DataIO& dio) const {
		dio << typename DataIO::my_var_uint64_t(sizeof(State));
		dio << typename DataIO::my_var_uint64_t(zpath_states);
		dio << typename DataIO::my_var_uint64_t(states.size());
		dio.ensureWrite(&states[0], sizeof(State) * states.size());
		dio.ensureWrite(m_bits.risk_data(), m_bits.size()/8);
	}
	template<class DataIO>
	friend void DataIO_loadObject(DataIO& dio, LinearDFA2& au) {
		typename DataIO::my_var_uint64_t version;
		dio >> version;
		if (version > SERIALIZATION_VERSION) {
			typedef typename DataIO::my_BadVersionException bad_ver;
			throw bad_ver(version.t, SERIALIZATION_VERSION, "LinearDFA2");
		}
		au.load_au(dio, version.t);
	}
	template<class DataIO>
	friend void DataIO_saveObject(DataIO& dio, const LinearDFA2& au) {
		dio << typename DataIO::my_var_uint64_t(SERIALIZATION_VERSION);
		au.save_au(dio);
	}

protected:
	void set_last_bit(size_t i, bool val) { m_bits.set(3*i+0, val); }
	void set_pzip_bit(size_t i, bool val) { m_bits.set(3*i+1, val); }
	void set_term_bit(size_t i, bool val) { m_bits.set(3*i+2, val); }
	bool get_last_bit(size_t i) const { return m_bits.is1(3*i+0); }
	bool get_pzip_bit(size_t i) const { return m_bits.is1(3*i+1); }
	bool get_term_bit(size_t i) const { return m_bits.is1(3*i+2); }

	valvec<State> states;
	simplebitvec  m_bits;
	size_t zpath_states;
};

#endif // __febird_automata_linear_dfa2_hpp__

