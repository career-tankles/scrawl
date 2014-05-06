public:
bool add_word(fstring word) {
	return add_word_aux(initial_state, word);
}
bool add_word(size_t RootState, fstring word) {
	return add_word_aux(RootState, word);
}
bool add_word16(fstring16 word) {
	return add_word_aux(initial_state, word);
}
bool add_word16(size_t RootState, fstring16 word) {
	return add_word_aux(RootState, word);
}

template<class Char>
bool add_word_aux(size_t RootState, basic_fstring<Char> word) {
	assert(0 == num_zpath_states());
	assert(RootState < total_states());
	size_t curr = RootState;
	size_t next;
	size_t j = 0;
	while (j < word.size()) {
		next = state_move(curr, word.uch(j));
		if (nil_state == next)
			goto AddNewStates;
		assert(next < total_states());
		curr = next;
		++j;
	}
	// word could be the prefix of some existed word word
	if (is_term(curr)) {
		return false; // existed
	} else {
		set_term_bit(curr);
		return true;
	}
AddNewStates:
	do {
		next = new_state();
		add_move_checked(curr, next, word.uch(j));
		curr = next;
		++j;
	} while (j < word.size());
	// now curr == next, and it is a Terminal State
	set_term_bit(curr);
	return true;
}

void append_delim_value(auchar_t delim, fstring value) {
	valvec<state_id_t> finals;
	get_final_states(&finals);
	for (size_t i = 0; i < finals.size(); ++i) {
		state_id_t root = finals[i];
		if (state_move(root, delim) != nil_state) {
			char msg[128];
			sprintf(msg, "dfa_mutate.append_delim_value: has delim=0x%02X(%c)", delim, delim);
			throw std::logic_error(msg);
		}
	}
	state_id_t next = new_state();
	for (size_t i = 0; i < finals.size(); ++i) {
		state_id_t root = finals[i];
		add_move(root, next, delim);
		clear_term_bit(root);
	}
	add_word(next, value);
}

///@param out path
///@return
///  - nil_state   : the str from 'from' to 'to' is newly created
///  - same as 'to' : the str from 'from' to 'to' has existed
///  - others       : the str from 'from' to 'XX' has existed, XX is not 'to'
///                   in this case, the Automata is unchanged
///@note
///  - This function don't change the Automata if str from 'from' to 'to'
///    has existed. The caller should check the return value if needed.
template<class Char>
state_id_t make_path( state_id_t from, state_id_t to
					, const basic_fstring<Char> str
					, valvec<state_id_t>* path) {
	assert(num_zpath_states() == 0);
	assert(nil_state != from);
	assert(nil_state != to);
	assert(NULL != path);
	assert(NULL != str.p);
#ifdef NDEBUG
	if (0 == str.n) return false;
#else
	assert(str.n > 0); // don't allow empty str
#endif
	auto& p = *path;
	p.resize(str.n+1);
	p[0] = from;
	for (ptrdiff_t j = 0; j < str.n-1; ++j) {
		if (nil_state == (p[j+1] = state_move(p[j], str.uch(j)))) {
			do add_move(p[j], p[j+1] = new_state(), str.uch(j));
			while (++j < str.n-1);
			break;
		}
		assert(p[j+1] < total_states());
	}
	state_id_t old_to = add_move(p[str.n-1], to, str.uch(str.n-1));
	p[str.n] = (nil_state == old_to) ? to : old_to;
	return old_to;
}

// in most cases this function is fast enough
// in extreme cases, assuming you need to copy many small DFA
// preallocate a WorkingCacheOfCopyFrom object could
// reduce malloc/realloc/free and a little faster
template<class Au2>
state_id_t
copy_from(const Au2& au2, size_t SrcRoot = initial_state) {
	WorkingCacheOfCopyFrom<MyType, Au2> ctx(au2.total_states());
	return ctx.copy_from(this, au2, SrcRoot);
}
template<class Au2>
state_id_t
copy_from(state_id_t DstRoot, const Au2& au2, size_t SrcRoot = initial_state) {
	WorkingCacheOfCopyFrom<MyType, Au2> ctx(au2.total_states());
	return ctx.copy_from(this, DstRoot, au2, SrcRoot);
}

// dead states: states which are unreachable to any final states
template<class CleanDFA>
void remove_dead_states(CleanDFA& clean) {
	assert(has_freelist());
	CompactReverseNFA<state_id_t> inv(graph_reverse_nfa());
	valvec<state_id_t> stack;
	simplebitvec mark(total_states(), false);
	if (num_free_states()) {
		for (size_t root = 0, n = total_states(); root < n; ++root) {
			if (!is_free(root) && is_term(root))
				stack.push_back(root), mark.set1(root);
		}
	} else { // has no any free state
		for (size_t root = 0, n = total_states(); root < n; ++root) {
			if (is_term(root))
				stack.push_back(root), mark.set1(root);
		}
	}
	// DFS search from multiple final states
	while (!stack.empty()) {
		state_id_t parent = stack.back(); stack.pop_back();
		size_t beg = inv.index[parent+0];
		size_t end = inv.index[parent+1];
		assert(beg <= end);
		for (size_t i = beg; i < end; ++i) {
			state_id_t child = inv.data[i].target;
			assert(child < total_states());
			if (mark.is0(child))
				stack.push_back(child), mark.set1(child);
		}
	}
	size_t new_au_states = 0;
	typedef typename CleanDFA::state_id_t state_id_t2;
	valvec<state_id_t2> map(total_states(), (state_id_t2)clean.nil_state);
	for (size_t s = 0, n = total_states(); s < n; ++s) {
		if (mark.is1(s))
			map[s] = new_au_states++;
	}
	clean.resize_states(new_au_states);
	for (size_t s = 0, n = total_states(); s < n; ++s) {
		if (map[s] == nil_state)
			continue;
		for_each_move(s, [&](state_id_t src, state_id_t dst, auchar_t ch) {
			if (map[dst] != clean.nil_state) {
				state_id_t2 src2 = map[src];
				state_id_t2 dst2 = map[dst];
				clean.add_move(src2, dst2, ch);
			}
		});
		if (is_term(s))
			clean.set_term_bit(map[s]);
	}
}

// sub graph rooted at RootState must has no intersection nodes
// with other sub graphs
void remove_sub(size_t RootState) {
	assert(RootState < total_states());
	auto on_finish = [this](state_id_t s) { del_state(s); };
	auto on_back_edge = [&](state_id_t, state_id_t child){
		if (child == RootState) {
			string_appender<> msg;
			msg << BOOST_CURRENT_FUNCTION
				<< ": found cycle to RootState=" << RootState;
			throw std::logic_error(msg);
		}
	};
	dfs_post_order_walk(RootState, on_finish, on_back_edge);
}

///@param on_fold:
//        void on_fold.start(state_id_t RootState);
///       void on_fold.on_value(size_t nth, fstring value);
///       bool on_fold.finish(size_t num_values, std::string* folded_value);
/// values are suffixes of prefixes after first delim
///@effect
///      if on_fold.finish returns true, values will be
///      folded into a single value (such as sum of numbers)
///@note this Automata must has no cycles in values
template<class OnFold>
void fold_values(auchar_t delim, OnFold* on_fold) {
	fold_values<OnFold&>(delim, *on_fold);
}
template<class OnFold>
void fold_values(auchar_t delim, OnFold on_fold) {
	assert(num_zpath_states() == 0);
	valvec<state_id_t> folds; folds.reserve(128);
	{
	//	BFS_MultiPassGraphWalker<state_id_t> walker; // BFS use more memory
	//	DFS_MultiPassGraphWalker<state_id_t> walker;
		DFS_GraphWalker<state_id_t> walker;
		simplebitvec in_folds(total_states(), 0);
		walker.resize(total_states());
		walker.put_root(initial_state);
		while (!walker.is_finished()) {
			state_id_t parent = walker.next();
			state_id_t vchild = state_move(parent, delim);
			if (nil_state != vchild && in_folds.is0(vchild)) {
				in_folds.set1(vchild);
				folds.push_back(vchild);
			}
			walker.putChildren2(this, parent,
				[=](state_id_t, state_id_t child, bool) {
					// just fold at first delim bound
					return (child != vchild);
				});
		}
	#if !defined(NDEBUG)
		size_t n_reachable = 0;
		walker.resize(total_states());
		walker.put_root(initial_state);
		while (!walker.is_finished()) {
			state_id_t parent = walker.next();
			walker.putChildren(this, parent);
			n_reachable++;
		}
		assert(num_used_states() == n_reachable);
	#endif
	}
	valvec<state_id_t> indegree(total_states(), 0);
	compute_indegree(initial_state, indegree);
	auto on_suffix = [&](size_t nth, fstring value, size_t) {
		on_fold.on_value(nth, value);
	};
	ForEachWord_DFS<MyType, decltype(on_suffix)> vis(this, on_suffix);
	std::string folded_value;
	valvec<state_id_t> del_list;
	valvec<state_id_t> topological, stack, color;
	for (size_t j = 0; j < folds.size(); ++j) {
		state_id_t s = folds[j];
	#if !defined(NDEBUG)
		for (size_t i = 1; i < indegree.size(); ++i) {
			if (is_free(i)) continue;
			// if algorithm is correct, indegree[i] must > 0
			assert(indegree[i] > 0);
		}
	#endif
		on_fold.start(s);
		vis.start(s, 0); // will call on_fold.on_value(...)
		if (on_fold.finish(vis.nth, &folded_value)) {
			del_list.erase_all();
			topological.erase_all();
			assert(stack.empty());
			color.resize(total_states()+1);
			stack.push_back(s);
			topological_sort(stack, color,
					[&](state_id_t x) { topological.push_back(x); });
			assert(topological.back() == s);
			topological.pop_back(); // pop s
			for_each_dest(s, [&](state_id_t, state_id_t child) {
					if (0 == --indegree[child])
						del_list.push_back(child);
				});
			for (ptrdiff_t i = topological.size()-1; i >= 0; --i) {
				state_id_t parent = topological[i];
				for_each_dest(parent,
					[&](state_id_t parent, state_id_t child) {
						if (0 == indegree[parent]) {
							if (0 == --indegree[child])
								del_list.push_back(child);
						}
					});
			}
			for (state_id_t x : del_list) del_state(x);
			del_all_move(s);
			fstring folded(folded_value);
			state_id_t curr = s;
			indegree.resize(std::max(total_states(), num_used_states()+folded.n));
			for (ptrdiff_t i = 0; i < folded.n; ++i) {
				state_id_t next = new_state();
				add_move(curr, next, folded.p[i]);
				indegree[next] = 1;
				curr = next;
			}
			set_term_bit(curr);
			assert(indegree.size() == total_states());
		}
	}
#if !defined(NDEBUG)
	for (size_t i = 1; i < indegree.size(); ++i) {
		if (is_free(i)) continue;
		// if algorithm is correct, indegree[i] must > 0
		assert(indegree[i] > 0);
	}
	// check new indegrees
	indegree.resize_no_init(total_states());
	indegree.fill(0);
	for (size_t i = 0; i < indegree.size(); ++i) {
		if (is_free(i)) continue;
		for_each_dest(i, [&](state_id_t, state_id_t child) {
				indegree[child]++;
			});
	}
	for (size_t i = 1; i < indegree.size(); ++i) {
		if (is_free(i)) continue;
		// if algorithm is correct, indegree[i] must > 0
		assert(indegree[i] > 0);
	}
#endif
}

////////////////////////////////////////////////////////////
// Virtuals

const DFA_Interface* get_DFA_Interface()
const override final {	return this; }

// non-virtual
void add_move_checked(size_t source, size_t target, auchar_t ch) {
	assert(ch < sigma);
	size_t old = this->add_move_imp(source, target, ch, false);
	assert(nil_state == old);
	if (nil_state != old) {
		throw std::logic_error(BOOST_CURRENT_FUNCTION);
	}
}
size_t add_move(size_t source, size_t target, auchar_t ch) {
	assert(ch < sigma);
	return this->add_move_imp(source, target, ch, false);
}
// overwrite existed
/// @return nil_state : if not existed
///         otherwise  : the old target which was overwritten
size_t set_move(size_t source, size_t target, auchar_t ch) {
	assert(ch < sigma);
	return this->add_move_imp(source, target, ch, true);
}

