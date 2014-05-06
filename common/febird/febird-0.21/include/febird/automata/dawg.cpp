#include "dawg.hpp"
#include "tmplinst.hpp"
#include "dfa_mmap_header.hpp"

#define DFA_TMPL_CLASS_PREFIX template<class State> void Automata<State>
#include "ppi/pool_dfa_mmap.hpp"

#define DFA_TMPL_CLASS_PREFIX template<class State> void DAWG<State>
#include "ppi/dawg_dfa_mmap.hpp"

#define DFA_TMPL_CLASS_PREFIX template<class State> void DAWG0<State>
#include "ppi/dawg_dfa_mmap.hpp"

namespace febird {

TMPL_INST_DFA_CLASS(DAWG<State32>)
TMPL_INST_DFA_CLASS(DAWG<State4B>)
TMPL_INST_DFA_CLASS(DAWG<State5B>)
TMPL_INST_DFA_CLASS(DAWG<State6B>)

#if FEBIRD_WORD_BITS >= 64
TMPL_INST_DFA_CLASS(DAWG<State34>)
#endif
#if FEBIRD_WORD_BITS >= 64 && defined(FEBIRD_INST_ALL_DFA_TYPES)
TMPL_INST_DFA_CLASS(DAWG<State7B>)
#endif

} // namespace febird
