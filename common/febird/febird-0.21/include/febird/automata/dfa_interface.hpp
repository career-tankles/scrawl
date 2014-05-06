#ifndef __febird_automata_dfa_interface_hpp__
#define __febird_automata_dfa_interface_hpp__

#include <stdio.h>
#include <febird/fstring.hpp>
#include <febird/valvec.hpp>
#include <boost/function.hpp>

typedef unsigned char  byte_t;
typedef unsigned char  uint08_t;
typedef uint16_t       auchar_t;
const size_t initial_state = 0;

inline size_t align_to_64(size_t x) { return (x + 63) & size_t(-64); }

struct DFA_MmapHeader; // forward declaration

#pragma pack(push,1)
template<class StateID>
struct CharTarget {
	auchar_t ch;
	StateID  target;
	CharTarget(auchar_t c, StateID t) : ch(c), target(t) {}
	CharTarget() {}
};
template<>
struct CharTarget<uint64_t> {
	auchar_t  ch     :  9;
	uint64_t  target : 55;
	CharTarget(auchar_t c, uint64_t t) : ch(c), target(t) {}
	CharTarget() {}
};
#pragma pack(pop)
BOOST_STATIC_ASSERT(sizeof(CharTarget<size_t>)==8 || sizeof(size_t)==4);

struct CharTarget_By_ch {
	template<class StateID>
	bool operator()(CharTarget<StateID> x, CharTarget<StateID> y) const {
		return x.ch < y.ch;
	}
	template<class StateID>
	bool operator()(auchar_t x, CharTarget<StateID> y)const{return x < y.ch;}
	template<class StateID>
	bool operator()(CharTarget<StateID> x, auchar_t y)const{return x.ch < y;}
};

template<class StateID>
bool operator<(CharTarget<StateID> x, CharTarget<StateID> y) {
	if (x.ch != y.ch)
		return x.ch < y.ch;
	else
		return x.target < y.target;
}
template<class StateID>
bool operator==(CharTarget<StateID> x, CharTarget<StateID> y) {
	return x.ch == y.ch && x.target == y.target;
}
template<class StateID>
bool operator!=(CharTarget<StateID> x, CharTarget<StateID> y) {
	return x.ch != y.ch || x.target != y.target;
}

class ByteInputRange { // or named ByteInputStream ?
public:
	virtual ~ByteInputRange();
	virtual void operator++() = 0; // strict, don't return *this
	virtual byte_t operator*() = 0; // non-const
	virtual bool empty() const = 0;
};
typedef boost::function<byte_t(byte_t)> ByteTR;

class DFA_Interface { // readonly dfa interface
public:
	static DFA_Interface* load_from(FILE*);
	static DFA_Interface* load_from(fstring fname);
	static DFA_Interface* load_from_NativeInputBuffer(void* nativeInputBufferObject);
	void save_to(FILE*) const;
	void save_to(fstring fname) const;

	static DFA_Interface* load_mmap(int fd);
	static DFA_Interface* load_mmap(const char* fname);
	void save_mmap(int fd);
	void save_mmap(const char* fname);

	DFA_Interface();
	virtual ~DFA_Interface();
	size_t get_sigma() const { return m_dyn_sigma; }

	virtual bool has_freelist() const = 0;
//	virtual bool compute_is_dag() const = 0;

	virtual void dot_write_one_state(FILE* fp, size_t ls) const;
	virtual void dot_write_one_move(FILE* fp, size_t s, size_t target, auchar_t ch) const;
    virtual void write_dot_file(FILE* fp) const;
    virtual void write_dot_file(fstring fname) const;

	void multi_root_write_dot_file(const size_t* pRoots, size_t nRoots, FILE* fp) const;
	void multi_root_write_dot_file(const size_t* pRoots, size_t nRoots, fstring fname) const;
	void multi_root_write_dot_file(const valvec<size_t>& roots, FILE* fp) const;
	void multi_root_write_dot_file(const valvec<size_t>& roots, fstring fname) const;

	// return written bytes
	size_t print_all_words(FILE* fp) const;
    size_t print_output(const char* fname = "-", size_t RootState = initial_state) const;

	typedef boost::function<void(size_t nth, fstring)> OnNthWord;

	virtual
	size_t for_each_word(size_t RootState, const OnNthWord& on_word) const = 0;
	size_t for_each_word(const OnNthWord& on_word) const;

	virtual size_t for_each_suffix(size_t RootState, fstring prefix
			, const OnNthWord& on_suffix
			, const ByteTR& tr
			) const = 0;
	size_t for_each_suffix(fstring prefix
			, const OnNthWord& on_suffix
			, const ByteTR& tr
			) const;
	virtual size_t for_each_suffix(size_t RootState, fstring prefix
			, const OnNthWord& on_suffix
			) const = 0;
	size_t for_each_suffix(fstring prefix
			, const OnNthWord& on_suffix
			) const;

	typedef boost::function<
		void(size_t keylen, size_t value_idx, fstring value)
	  > OnMatchKey;
	virtual size_t match_key(auchar_t delim, fstring str
			, const OnMatchKey& on_match
			) const = 0;
	virtual size_t match_key(auchar_t delim, fstring str
			, const OnMatchKey& on_match
			, const ByteTR& tr
			) const = 0;
	virtual size_t match_key(auchar_t delim, ByteInputRange& r
			, const OnMatchKey& on_match
			) const = 0;
	virtual size_t match_key(auchar_t delim, ByteInputRange& r
			, const OnMatchKey& on_match
			, const ByteTR& tr
			) const = 0;

	inline size_t match_key(fstring str
			, const OnMatchKey& on_match
			) const { return match_key(m_kv_delim, str, on_match); }
	inline size_t match_key(fstring str
			, const OnMatchKey& on_match
			, const ByteTR& tr
			) const { return match_key(m_kv_delim, str, on_match, tr); }
	inline size_t match_key(ByteInputRange& r
			, const OnMatchKey& on_match
			) const { return match_key(m_kv_delim, r, on_match); };
	inline size_t match_key(ByteInputRange& r
			, const OnMatchKey& on_match
			, const ByteTR& tr
			) const { return match_key(m_kv_delim, r, on_match, tr); }

	virtual size_t match_key_l(auchar_t delim, fstring str
			, const OnMatchKey& on_match
			) const = 0;
	virtual size_t match_key_l(auchar_t delim, fstring str
			, const OnMatchKey& on_match
			, const ByteTR& tr
			) const = 0;
	virtual size_t match_key_l(auchar_t delim, ByteInputRange& r
			, const OnMatchKey& on_match
			) const = 0;
	virtual size_t match_key_l(auchar_t delim, ByteInputRange& r
			, const OnMatchKey& on_match
			, const ByteTR& tr
			) const = 0;

	inline size_t match_key_l(fstring str
			, const OnMatchKey& on_match
			) const { return match_key_l(m_kv_delim, str, on_match); }
	inline size_t match_key_l(fstring str
			, const OnMatchKey& on_match
			, const ByteTR& tr
			) const { return match_key_l(m_kv_delim, str, on_match, tr); }
	inline size_t match_key_l(ByteInputRange& r
			, const OnMatchKey& on_match
			) const { return match_key_l(m_kv_delim, r, on_match); };
	inline size_t match_key_l(ByteInputRange& r
			, const OnMatchKey& on_match
			, const ByteTR& tr
			) const { return match_key_l(m_kv_delim, r, on_match, tr); }

	typedef boost::function<void(size_t keylen, size_t field_id, size_t idx, fstring field_data)>
			OnField;

	virtual size_t max_prefix_match(fstring text, size_t* max_len) const = 0;
	virtual size_t max_prefix_match(fstring text, size_t* max_len, const ByteTR& tr) const = 0;
	virtual size_t max_prefix_match(ByteInputRange& r, size_t* max_len) const = 0;
	virtual size_t max_prefix_match(ByteInputRange& r, size_t* max_len, const ByteTR& tr) const = 0;

	virtual size_t all_prefix_match(fstring text
			, const boost::function<void(size_t match_len)>& on_match
			) const = 0;
	virtual size_t all_prefix_match(fstring text
			, const boost::function<void(size_t match_len)>& on_match
			, const ByteTR& tr
			) const = 0;
	virtual size_t all_prefix_match(ByteInputRange& r
			, const boost::function<void(size_t match_len)>& on_match
			) const = 0;
	virtual size_t all_prefix_match(ByteInputRange& r
			, const boost::function<void(size_t match_len)>& on_match
			, const ByteTR& tr
			) const = 0;

	bool full_match(fstring text) const;
	bool full_match(fstring text, const ByteTR& tr) const;
	bool full_match(ByteInputRange& r) const;
	bool full_match(ByteInputRange& r, const ByteTR& tr) const;

	virtual bool v_is_pzip(size_t s) const = 0;
	virtual bool v_is_term(size_t s) const = 0;
	virtual byte_t const* v_get_zpath_data(size_t s) const = 0;
	virtual size_t v_total_states() const = 0;
	virtual size_t v_nil_state() const = 0;
	virtual size_t v_max_state() const = 0;
    virtual size_t mem_size() const = 0;
	virtual size_t v_state_move(size_t curr, auchar_t ch) const = 0;

	// length of dest/moves must be at least sigma
	virtual size_t get_all_dest(size_t s, size_t* dests) const = 0;
	virtual size_t get_all_move(size_t s, CharTarget<size_t>* moves) const = 0;

	void get_all_dest(size_t s, valvec<size_t>* dests) const;
	void get_all_move(size_t s, valvec<CharTarget<size_t> >* moves) const;

	void swap(DFA_Interface& y);

	inline size_t num_zpath_states() const { return zpath_states; }

	inline bool is_dag() const { return m_is_dag; }

// low level operations
	inline void set_is_dag(bool val) { m_is_dag = val; }

	inline void set_kv_delim(auchar_t delim) {
	   	assert(delim < 512);
	   	m_kv_delim = delim;
   	}
	inline auchar_t kv_delim() const { return m_kv_delim; }

	template<class DataIO>
	void load_kv_delim_and_is_dag(DataIO& dio) {
		uint16_t kv_delim;
		uint08_t b_is_dag;
		dio >> kv_delim;
		dio >> b_is_dag;
		if (kv_delim >= 512) {
			typedef typename DataIO::my_DataFormatException bad_format;
			throw bad_format("kv_delim must < 512");
		}
		this->m_kv_delim = kv_delim;
		this->m_is_dag = b_is_dag ? 1 : 0;
	}

	inline bool is_mmap() const { return NULL != mmap_base; }

protected:
	virtual void finish_load_mmap(const DFA_MmapHeader*);
	virtual void prepare_save_mmap(DFA_MmapHeader*, const void**) const;
	static DFA_Interface* load_mmap_fmt(const DFA_MmapHeader*);

	inline void check_is_dag(const char* func) const {
		if (!m_is_dag) {
			std::string msg;
			msg += func;
			msg += " : DFA is not a DAG";
			throw std::logic_error(msg);
		}
	}

	const DFA_MmapHeader* mmap_base;
	int      mmap_fd;
	unsigned m_kv_delim : 9;
	unsigned m_is_dag   : 1;
	unsigned m_fake_mmap: 1; // is mmap format but loaded by read, not by mmap
	unsigned m_dyn_sigma:10;
	size_t   zpath_states;
};
//BOOST_STATIC_ASSERT(sizeof(DFA_Interface) == sizeof(size_t)*3);

class DFA_MutationInterface {
public:
	DFA_MutationInterface();
	virtual ~DFA_MutationInterface();
	virtual const DFA_Interface* get_DFA_Interface() const = 0;

	virtual void resize_states(size_t new_states_num) = 0;
	virtual size_t new_state() = 0;
    virtual size_t clone_state(size_t source) = 0;
	virtual void del_state(size_t state_id) = 0;
	virtual void add_all_move(size_t state_id, const CharTarget<size_t>* moves, size_t n_moves) = 0;
	inline  void add_all_move(size_t state_id, const valvec<CharTarget<size_t> >& moves) {
				 add_all_move(state_id, moves.data(), moves.size()); }

	virtual void del_all_move(size_t state_id) = 0;
	virtual void del_move(size_t s, auchar_t ch);

#ifdef FEBIRD_AUTOMATA_ENABLE_MUTATION_VIRTUALS
    virtual bool add_word(size_t RootState, fstring key) = 0;
    bool add_word(fstring key) { return add_word(initial_state, key); }

	virtual size_t copy_from(size_t SrcRoot) = 0;
	virtual void remove_dead_states() = 0;
	virtual void remove_sub(size_t RootState) = 0;

    virtual void graph_dfa_minimize() = 0;
    virtual void trie_dfa_minimize() = 0;
    virtual void adfa_minimize() = 0;
#endif
protected:
    virtual size_t add_move_imp(size_t source, size_t target, auchar_t ch, bool OverwriteExisted) = 0;
};

class AC_Scan_Interface {
public:
	typedef uint32_t word_id_t;
	typedef boost::function<void(size_t endpos, const word_id_t* a, size_t n)>
			OnHitWords; // handle multiple words in one callback

	AC_Scan_Interface();
	virtual ~AC_Scan_Interface();
	virtual void v_scan(fstring input, const OnHitWords&) const = 0;
	virtual void v_scan(fstring input, const OnHitWords&, const ByteTR&) const = 0;
	virtual void v_scan(ByteInputRange&, const OnHitWords&) const = 0;
	virtual void v_scan(ByteInputRange&, const OnHitWords&, const ByteTR&) const = 0;

	size_t num_words() const { return n_words; }

	bool has_word_length() const {
		assert(offsets.size() == 0 || offsets.size() == n_words + 1);
	   	return offsets.size() >= 1;
   	}

	bool has_word_content() const {
#ifndef NDEBUG
		if (offsets.empty()) {
			assert(strpool.empty());
		} else {
		   	assert(offsets.size() == n_words + 1);
			assert(strpool.size() == 0 || strpool.size() == offsets.back());
		}
#endif
	   	return offsets.size() >= 1 && !strpool.empty();
   	}

	size_t wlen(size_t wid) const {
		assert(offsets.size() == n_words + 1);
		assert(wid < n_words);
		size_t off0 = offsets[wid+0];
		size_t off1 = offsets[wid+1];
		return off1 - off0;
	}
	
	fstring word(size_t wid) const {
		// offsets and strpool are optional
		// if this function is called, offsets and strpool must be used
		assert(offsets.size() == n_words + 1);
		assert(offsets.back() == strpool.size());
		assert(wid < n_words);
		size_t off0 = offsets[wid+0];
		size_t off1 = offsets[wid+1];
		return fstring(strpool.data() + off0, off1 - off0);
	}

	void swap(AC_Scan_Interface& y) {
		assert(typeid(*this) == typeid(y));
		std::swap(n_words, y.n_words);
		offsets.swap(y.offsets);
		strpool.swap(y.strpool);
	}

protected:
	typedef uint32_t offset_t;
	size_t n_words;
	valvec<offset_t> offsets; // optional
	valvec<char>     strpool; // optional
};

class DAWG_Interface {
protected:
	size_t n_words;
public:
	static const size_t null_word = size_t(-1);

	DAWG_Interface();
	virtual ~DAWG_Interface();

	size_t num_words() const { return n_words; }

	virtual size_t index(fstring word) const = 0;
	virtual void nth_word(size_t nth, std::string* word) const = 0;
	std::string  nth_word(size_t nth) const; // template method

	typedef boost::function<void(size_t len, size_t nth)> OnMatchDAWG;
    virtual size_t v_match_words(fstring str, const OnMatchDAWG& on_match) const = 0;
    virtual size_t v_match_words(fstring str, const OnMatchDAWG& on_match, const ByteTR& tr) const = 0;
    virtual size_t v_match_words(fstring str, const OnMatchDAWG& on_match, const byte_t* tr) const = 0;

	virtual bool longest_prefix(fstring str, size_t* len, size_t* nth, const ByteTR& tr) const = 0;
	virtual bool longest_prefix(fstring str, size_t* len, size_t* nth, const byte_t* tr) const = 0;
	virtual bool longest_prefix(fstring str, size_t* len, size_t* nth) const = 0;
};

namespace febird {
size_t 
dot_escape(const char* ibuf, size_t ilen, char* obuf, size_t olen);
size_t 
dot_escape(const auchar_t* ibuf, size_t ilen, char* obuf, size_t olen);
}

#endif // __febird_automata_dfa_interface_hpp__

