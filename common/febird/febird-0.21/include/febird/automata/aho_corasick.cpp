#include "aho_corasick.hpp"
#include "tmplinst.hpp"
#include "dfa_mmap_header.hpp"

#define DFA_TMPL_CLASS_PREFIX template<class State> void Automata<State>
#include "ppi/pool_dfa_mmap.hpp"

#define DFA_TMPL_CLASS_PREFIX template<class State> void DoubleArrayTrie<State>
#include "ppi/flat_dfa_mmap.hpp"


template<class BaseDFA>
Aho_Corasick<BaseDFA>::~Aho_Corasick()
{
	if (this->mmap_base) {
		output.risk_release_ownership();
		offsets.risk_release_ownership();
		strpool.risk_release_ownership();
	}
}

template<class BaseDFA>
void Aho_Corasick<BaseDFA>::finish_load_mmap(const DFA_MmapHeader* base)
{
	byte_t* bbase = (byte_t*)base;
	BaseDFA::finish_load_mmap(base);
	assert(base->num_blocks >= 2 + base->ac_word_ext);
	assert(base->ac_word_ext <= 2);
	auto   ac_blocks = &base->blocks[base->num_blocks-1 - base->ac_word_ext];
	size_t num_output = ac_blocks[0].length / sizeof(word_id_t);
	output.clear();
	output.risk_set_data((word_id_t*)(bbase + ac_blocks[0].offset));
	output.risk_set_size(num_output);
	output.risk_set_capacity(num_output);
	n_words = base->dawg_num_words;
	offsets.clear();
	strpool.clear();
	if (base->ac_word_ext >= 1) {
		size_t offsize = ac_blocks[1].length / sizeof(offset_t);
		assert(offsize == this->n_words + 1);
		offsets.risk_set_data((offset_t*)(bbase + ac_blocks[1].offset));
		offsets.risk_set_size(offsize);
		offsets.risk_set_capacity(offsize);
	}
	if (base->ac_word_ext >= 2) {
		size_t poolsize = ac_blocks[2].length;
		assert(poolsize == offsets.back());
		strpool.risk_set_data((char*)(bbase + ac_blocks[1].offset));
		strpool.risk_set_size(poolsize);
		strpool.risk_set_capacity(poolsize);
	}
}

template<class BaseDFA>
void 
Aho_Corasick<BaseDFA>::prepare_save_mmap(DFA_MmapHeader* base, const void** dataPtrs)
const {
	BaseDFA::prepare_save_mmap(base, dataPtrs);
	base->dawg_num_words = n_words;
	auto blocks_end = &base->blocks[base->num_blocks];
	blocks_end[0].offset = align_to_64(blocks_end[-1].endpos());
	blocks_end[0].length = sizeof(word_id_t) * output.size();
	dataPtrs[base->num_blocks++] = output.data();
	base->ac_word_ext = 0;
	if (this->has_word_length()) {
		blocks_end[1].offset = align_to_64(blocks_end[0].endpos());
		blocks_end[1].length = sizeof(word_id_t) * offsets.size();
		dataPtrs[base->num_blocks++] = offsets.data();
		base->ac_word_ext = 1;
	}
	if (this->has_word_content()) {
		blocks_end[2].offset = align_to_64(blocks_end[1].endpos());
		blocks_end[2].length = sizeof(char) * strpool.size();
		dataPtrs[base->num_blocks++] = strpool.data();
		base->ac_word_ext = 2;
	}
}

namespace febird {
	
TMPL_INST_DFA_CLASS(DoubleArrayTrie_8B)
TMPL_INST_DFA_CLASS(Aho_Corasick_DoubleArray_8B)
TMPL_INST_DFA_CLASS(Aho_Corasick_SmartDFA_AC_State32)
TMPL_INST_DFA_CLASS(Aho_Corasick_SmartDFA_AC_State5B)
TMPL_INST_DFA_CLASS(Aho_Corasick_SmartDFA_AC_State4B)
//TMPL_INST_DFA_CLASS(Aho_Corasick_SmartDFA_AC_State6B)


} // namespace febird
