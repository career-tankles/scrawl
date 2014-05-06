#include "linear_dfa.hpp"
#include "tmplinst.hpp"
#include "dfa_mmap_header.hpp"

#define DFA_TMPL_CLASS_PREFIX template<int StateBytes, int Sigma> void LinearDFA<StateBytes, Sigma>
#include "ppi/flat_dfa_mmap.hpp"

namespace febird {

#if defined(FEBIRD_INST_ALL_DFA_TYPES)
TMPL_INST_DFA_CLASS(LinearDFA<2>)
TMPL_INST_DFA_CLASS(LinearDFA<3>)
#endif
TMPL_INST_DFA_CLASS(LinearDFA<4>)
TMPL_INST_DFA_CLASS(LinearDFA<5>)

#if defined(FEBIRD_INST_ALL_DFA_TYPES)
TMPL_INST_DFA_CLASS(LinearDFA_2B_512)
TMPL_INST_DFA_CLASS(LinearDFA_3B_512)
#endif
TMPL_INST_DFA_CLASS(LinearDFA_4B_512)
TMPL_INST_DFA_CLASS(LinearDFA_5B_512)

#if FEBIRD_WORD_BITS >= 64
	TMPL_INST_DFA_CLASS(LinearDFA<6>)
	TMPL_INST_DFA_CLASS(LinearDFA_6B_512)
#endif


#if defined(FEBIRD_INST_ALL_DFA_TYPES)
#include "linear_dfa2.hpp"
#include "linear_dfa3.hpp"

ON_CLASS_IO(LinearDFA2<3>)
ON_CLASS_IO(LinearDFA2<4>)
ON_CLASS_IO(LinearDFA2<5>)

ON_CLASS_IO(LinearDFA3<3>)
ON_CLASS_IO(LinearDFA3<4>)
ON_CLASS_IO(LinearDFA3<5>)
#endif

#if FEBIRD_WORD_BITS >= 64 && defined(FEBIRD_INST_ALL_DFA_TYPES)
ON_CLASS_IO(LinearDFA2<6>)
ON_CLASS_IO(LinearDFA3<6>)
#endif

} // namespace febird