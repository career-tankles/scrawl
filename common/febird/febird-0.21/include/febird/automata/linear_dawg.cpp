#include "linear_dawg.hpp"
#include "tmplinst.hpp"
#include "dfa_mmap_header.hpp"

template<int StateBytes>
void LinearDAWG<StateBytes>::finish_load_mmap(const DFA_MmapHeader* base)
{
	assert(sizeof(State) == base->state_size);
	byte_t* bbase = (byte_t*)base;
	states.clear();
	states.risk_set_data((State*)(bbase + base->blocks[0].offset));
	states.risk_set_size(base->total_states);
	states.risk_set_capacity(base->total_states);
	zpath_states = base->zpath_states;
	this->is_compiled = true;
	this->n_words = base->dawg_num_words;
}

template<int StateBytes>
void LinearDAWG<StateBytes>::
prepare_save_mmap(DFA_MmapHeader* base, const void** dataPtrs)
const {
	base->zpath_states = zpath_states ;
	base->state_size   = sizeof(State);
	base->total_states = states.size();
	base->num_blocks   = 1;
	base->blocks[0].offset = sizeof(DFA_MmapHeader);
	base->blocks[0].length = sizeof(State)*states.size();
	dataPtrs[0] = states.data();
	base->dawg_num_words = this->n_words;
}

namespace febird {

#if defined(FEBIRD_INST_ALL_DFA_TYPES)
TMPL_INST_DFA_CLASS(LinearDAWG<2>)
TMPL_INST_DFA_CLASS(LinearDAWG<3>)
#endif
TMPL_INST_DFA_CLASS(LinearDAWG<4>)
TMPL_INST_DFA_CLASS(LinearDAWG<5>)

#if FEBIRD_WORD_BITS >= 64
TMPL_INST_DFA_CLASS(LinearDAWG<6>)
#endif

} // namespace febird

