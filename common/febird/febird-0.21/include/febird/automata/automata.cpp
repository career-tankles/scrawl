#include "automata.hpp"
#include "tmplinst.hpp"
#include "dfa_mmap_header.hpp"

#define DFA_TMPL_CLASS_PREFIX template<class State> void Automata<State>
#include "ppi/pool_dfa_mmap.hpp"

namespace febird {

TMPL_INST_DFA_CLASS(Automata<State32>)
TMPL_INST_DFA_CLASS(Automata<State4B>)
TMPL_INST_DFA_CLASS(Automata<State5B>)
TMPL_INST_DFA_CLASS(Automata<State6B>)

//TMPL_INST_DFA_CLASS(Automata<State32_512>) // inst in mre_tmplinst.cpp
TMPL_INST_DFA_CLASS(Automata<State4B_448>)
TMPL_INST_DFA_CLASS(Automata<State5B_448>)
TMPL_INST_DFA_CLASS(Automata<State6B_512>)

#if FEBIRD_WORD_BITS >= 64
TMPL_INST_DFA_CLASS(Automata<State34>)
#endif

#if FEBIRD_WORD_BITS >= 64 && defined(FEBIRD_INST_ALL_DFA_TYPES)
TMPL_INST_DFA_CLASS(Automata<State7B>)
TMPL_INST_DFA_CLASS(Automata<State7B_448>)
#endif

} // namespace febird
