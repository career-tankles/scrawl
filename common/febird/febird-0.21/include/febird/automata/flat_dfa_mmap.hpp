
DFA_TMPL_CLASS_PREFIX::finish_load_mmap(const DFA_MmapHeader* base)
{
	assert(sizeof(State) == base->state_size);
	byte_t* bbase = (byte_t*)base;
	states.clear();
	states.risk_set_data((State*)(bbase + base->blocks[0].offset));
	states.risk_set_size(base->total_states);
	states.risk_set_capacity(base->total_states);
	zpath_states = base->zpath_states;
}

DFA_TMPL_CLASS_PREFIX::prepare_save_mmap(DFA_MmapHeader* base, const void** dataPtrs)
const {
	base->zpath_states = zpath_states ;
	base->state_size   = sizeof(State);
	base->total_states = states.size();
	base->num_blocks   = 1;
	base->blocks[0].offset = sizeof(DFA_MmapHeader);
	base->blocks[0].length = sizeof(State)*states.size();
	dataPtrs[0] = states.data();
}

#undef DFA_TMPL_CLASS_PREFIX

