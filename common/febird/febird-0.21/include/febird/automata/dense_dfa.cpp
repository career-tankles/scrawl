#include "dense_dfa.hpp"
#include "tmplinst.hpp"
#include "dfa_mmap_header.hpp"

#define DFA_TMPL_CLASS_PREFIX \
	template<class StateID, int Sigma, class State> \
	void DenseDFA<StateID, Sigma, State>
#include "ppi/pool_dfa_mmap.hpp"

namespace febird {

// inst in mre_tmplinst.cpp
//TMPL_INST_DFA_CLASS(DenseDFA<uint32_t>)
//TMPL_INST_DFA_CLASS(DenseDFA_uint32_320)

#if FEBIRD_WORD_BITS >= 64 && defined(FEBIRD_INST_ALL_DFA_TYPES)
	TMPL_INST_DFA_CLASS(DenseDFA<uint64_t>)
	TMPL_INST_DFA_CLASS(DenseDFA_uint64_320)
#endif

} // namespace febird
