
DFA_TMPL_CLASS_PREFIX::finish_load_mmap(const DFA_MmapHeader* base) {
	assert(sizeof(State) == base->state_size);
	byte_t* bbase = (byte_t*)base;

	states.clear();
	pool.get_data_byte_vec().clear();
	states.risk_set_data((State*)(bbase + base->blocks[0].offset));
	states.risk_set_size(base->total_states);
	states.risk_set_capacity(base->total_states);

	pool.get_data_byte_vec().risk_set_data(base->blocks[1].offset + bbase);
	pool.get_data_byte_vec().risk_set_size(base->blocks[1].length);
	pool.get_data_byte_vec().risk_set_capacity(base->blocks[1].length);

	firstFreeState = base->firstFreeState;
	numFreeStates  = base->numFreeStates;
	zpath_states   = base->zpath_states;
	transition_num = base->transition_num;
}

DFA_TMPL_CLASS_PREFIX::prepare_save_mmap(DFA_MmapHeader* base, const void** dataPtrs)
const {
	base->firstFreeState = firstFreeState;
	base->numFreeStates  = numFreeStates ;
	base->zpath_states   = zpath_states  ;
	base->transition_num = transition_num;
	base->state_size     = sizeof(State);
	base->total_states   = states.size();
	base->num_blocks     = 2;
	base->blocks[0].offset = sizeof(DFA_MmapHeader);
	base->blocks[0].length = sizeof(State)*states.size();
	base->blocks[1].offset = sizeof(DFA_MmapHeader) + align_to_64(sizeof(State)*states.size());
	base->blocks[1].length = pool.size();
	dataPtrs[0] = states.data();
	dataPtrs[1] = pool.data();
}

#undef  DFA_TMPL_CLASS_PREFIX

