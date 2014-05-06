DFA_TMPL_CLASS_PREFIX::finish_load_mmap(const DFA_MmapHeader* base) {
	super::finish_load_mmap(base);
	this->is_compiled = true;
	this->n_words = base->dawg_num_words;
}

DFA_TMPL_CLASS_PREFIX::prepare_save_mmap(DFA_MmapHeader* base, const void** dataPtrs)
const {
	super::prepare_save_mmap(base, dataPtrs);
	base->dawg_num_words = this->n_words;
}

#undef DFA_TMPL_CLASS_PREFIX

