#ifndef __febird_automata_dfa_interface_hpp__
#define __febird_automata_dfa_interface_hpp__

#include <stdio.h>
#include <string>
#include <febird/fstring.hpp>
#include <boost/function.hpp>

typedef unsigned char  char_t;
typedef unsigned char  byte_t;
const size_t initial_state = 0;

class DFA_Interface { // readonly dfa interface
public:

	static DFA_Interface* load_from(FILE*);
	static DFA_Interface* load_from(fstring fname);
	static DFA_Interface* load_from_NativeInputBuffer(void* nativeInputBufferObject);
	void save_to(FILE*) const;
	void save_to(fstring fname) const;

	DFA_Interface();
	virtual ~DFA_Interface();

	virtual bool has_freelist() const = 0;

	virtual void dot_write_one_state(FILE* fp, size_t ls) const;
	virtual void dot_write_one_transition(FILE* fp, size_t s, size_t target, char_t ch) const;
    virtual void write_dot_file(FILE* fp) const;
    virtual void write_dot_file(const fstring fname) const;

	// return written bytes
	size_t print_all_words(FILE* fp) const;

	virtual size_t for_each_word(size_t RootState, const boost::function<void(size_t nth, fstring)>& on_word) const = 0;
	size_t for_each_word(const boost::function<void(size_t nth, fstring)>& on_word) const;

	virtual size_t for_each_suffix(size_t RootState, fstring prefix
			, const boost::function<void(size_t nth, fstring)>& on_suffix
			, const boost::function<byte_t(byte_t)>& tr
			) const = 0;
	size_t for_each_suffix(fstring prefix
			, const boost::function<void(size_t nth, fstring)>& on_suffix
			, const boost::function<byte_t(byte_t)>& tr
			) const;
	virtual size_t for_each_suffix(size_t RootState, fstring prefix
			, const boost::function<void(size_t nth, fstring)>& on_suffix
			) const = 0;
	size_t for_each_suffix(fstring prefix
			, const boost::function<void(size_t nth, fstring)>& on_suffix
			) const;

	virtual size_t match_key(char delim, fstring str
			, const boost::function<void(size_t keylen, size_t value_idx, fstring value)>& on_match
			) const = 0;
	virtual size_t match_key(char delim, fstring str
			, const boost::function<void(size_t keylen, size_t value_idx, fstring value)>& on_match
			, const boost::function<byte_t(byte_t)>& tr
			) const = 0;

	virtual size_t  v_total_states() const = 0;
	virtual bool v_is_pzip(size_t s) const = 0;
	virtual bool v_is_term(size_t s) const = 0;
	virtual void v_for_each_move(size_t s, const boost::function<void(size_t parent, size_t child, byte_t ch)>& op) const = 0;
	virtual void v_for_each_dest(size_t s, const boost::function<void(size_t parent, size_t child)>& op) const = 0;
	virtual void v_for_each_dest_rev(size_t s, const boost::function<void(size_t parent, size_t child)>& op) const = 0;
	virtual const byte_t* v_get_zpath_data(size_t s) const = 0;
	virtual size_t v_null_state() const = 0;
	virtual size_t v_max_state() const = 0;
    virtual size_t mem_size() const = 0;
};

class DFA_MutationInterface {
public:
	DFA_MutationInterface();
	virtual ~DFA_MutationInterface();

	virtual void resize_states(size_t new_states_num) = 0;
	virtual size_t new_state() = 0;
    virtual size_t clone_state(size_t source) = 0;
	virtual void del_state(size_t state_id) = 0;
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
    virtual size_t add_move_imp(size_t source, size_t target, char_t ch, bool OverwriteExisted) = 0;
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

    virtual size_t v_match_words(fstring str, const boost::function<void(size_t len, size_t nth)>& on_match) const = 0;
    virtual size_t v_match_words(fstring str, const boost::function<void(size_t len, size_t nth)>& on_match, const boost::function<byte_t(byte_t)>& tr) const = 0;
    virtual size_t v_match_words(fstring str, const boost::function<void(size_t len, size_t nth)>& on_match, const byte_t* tr) const = 0;

	virtual bool longest_prefix(fstring str, size_t* len, size_t* nth, const boost::function<byte_t(byte_t)>& tr) const = 0;
	virtual bool longest_prefix(fstring str, size_t* len, size_t* nth, const byte_t* tr) const = 0;
	virtual bool longest_prefix(fstring str, size_t* len, size_t* nth) const = 0;
};

#endif // __febird_automata_dfa_interface_hpp__

