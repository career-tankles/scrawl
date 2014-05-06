
public:
typedef DFA_Interface::OnNthWord  OnNthWord;
typedef DFA_Interface::OnMatchKey OnMatchKey;

// No mark bits, optimized for tree shape Automata
template<class Visitor>
void bfs_tree(state_id_t RootState, Visitor vis) const {
	valvec<state_id_t> q1, q2;
	q1.push_back(RootState);
	do {
		for (size_t i = 0; i < q1.size(); ++i) {
			state_id_t source = q1[i];
			for_each_move(source,
				[&](state_id_t parent, state_id_t child, auchar_t ch) {
					vis(parent, child, ch);
					q2.push_back(child);
				});
		}
		q1.swap(q2);
		q2.erase_all();
	} while (!q1.empty());
}

/// @param on_nth_word(size_t nth, fstring word, state_id_t accept_state)
///    calling of on_nth_word is in lexical_graphical order,
///    nth is the ordinal, 0 based
/// @returns  number of all words
template<class OnNthWordT>
size_t tpl_for_each_word(OnNthWordT on_nth_word) const {
	ForEachWord_DFS<MyType, OnNthWordT> vis(this, on_nth_word);
	vis.start(initial_state, 0);
	return vis.nth;
}
template<class OnNthWordT>
size_t tpl_for_each_word(state_id_t RootState, OnNthWordT on_nth_word) const {
	ForEachWord_DFS<MyType, OnNthWordT> vis(this, on_nth_word);
	vis.start(RootState, 0);
	return vis.nth;
}

bool has_bytes(fstring bytes) {
	BitMap<sigma> bits(0);
	for (size_t i = 0; i < bytes.size(); ++i) {
		bits.set1((byte_t)bytes[i]);
	}
	valvec<state_id_t> stack;
	simplebitvec       color(total_states());
	stack.push_back(initial_state);
	color.set1(initial_state);
	while (!stack.empty()) {
		state_id_t parent = stack.back(); stack.pop_back();
		bool found = false;
		for_each_move(parent,
			[&](state_id_t, state_id_t child, auchar_t ch) {
				if (color.is0(child)) {
					color.set1(child);
					stack.push_back(child);
				}
				if (bits.is1(ch))
					found = true;
			});
		if (found)
			return true;
	}
	return false;
}

void compute_indegree(size_t RootState, valvec<state_id_t>& in) const {
	compute_indegree(&RootState, 1, in);
}
void
compute_indegree(const size_t* pRoots, size_t nRoots, valvec<state_id_t>& in)
const {
	assert(nRoots >= 1);
	assert(nRoots < total_states());
	in.resize(total_states(), 0);
	simplebitvec color(total_states(), 0);
	valvec<state_id_t> stack;
	stack.reserve(std::max<size_t>(get_sigma(), nRoots));
	for(size_t i = 0; i < nRoots; ++i) {
		size_t root = pRoots[i];
		assert(root < total_states());
		ASSERT_isNotFree(root);
		stack.unchecked_push_back(root);
		color.set1(root);
	}
	while (!stack.empty()) {
		state_id_t parent = stack.back(); stack.pop_back();
		for_each_dest_rev(parent, [&](state_id_t, state_id_t child) {
			ASSERT_isNotFree(child);
			if (color.is0(child)) {
				color.set1(child);
				stack.push_back(child);
			}
			in[child]++;
		});
	}
}

void get_final_states(valvec<state_id_t>* final_states) const {
	return get_final_states(initial_state, final_states);
}
void get_final_states(state_id_t root, valvec<state_id_t>* final_states)
const {
	PFS_GraphWalker<state_id_t> walker;
	get_final_states(root, final_states, walker);
}
template<class Walker>
void get_final_states(state_id_t root, valvec<state_id_t>* final_states,
					  Walker& walker)
const {
	final_states->erase_all();
	walker.resize(total_states());
	walker.put_root(root);
	while (!walker.is_finished()) {
		state_id_t curr = walker.next();
		if (is_term(curr))
			final_states->push_back(curr);
		walker.putChildren(this, curr);
	}
}

/////////////////////////////////////////////////////////////////
// virtuals:

public:
bool v_is_pzip(size_t s) const override final { return this->is_pzip(s); }
bool v_is_term(size_t s) const override final { return this->is_term(s); }
size_t  v_total_states() const override final { return this->total_states(); }

const byte_t* v_get_zpath_data(size_t s) const override final {
	return this->get_zpath_data(s);
}
size_t v_nil_state() const override final { return nil_state; }
size_t v_max_state() const override final { return max_state; }

size_t v_state_move(size_t curr, auchar_t ch) const override final
{ return state_move(curr, ch); }

using DFA_Interface::get_all_move;
size_t get_all_move(size_t s, CharTarget<size_t>* moves)
const override final {
	assert(NULL != moves);
	size_t idx = 0;
	this->for_each_move(s,
		[moves,&idx](size_t, size_t t, auchar_t c) {
			moves[idx++] = {c, t};
		});
	return idx;
}
using DFA_Interface::get_all_dest;
size_t get_all_dest(size_t s, size_t* dests)
const override final {
	assert(NULL != dests);
	size_t idx = 0;
	this->for_each_dest(s,
		[dests,&idx](size_t, size_t t) {
			dests[idx++] = t;
		});
	return idx;
}

private:
struct OnWord3 {
	void operator()(size_t nth, fstring w, size_t) const {
		on_word(nth, w);
	}
	const OnNthWord& on_word;
	OnWord3(const OnNthWord& on) : on_word(on) {}
};
public:
size_t for_each_word(size_t RootState, const OnNthWord& on_word)
const override final {
	assert(m_is_dag);
	check_is_dag(BOOST_CURRENT_FUNCTION);
	return tpl_for_each_word(RootState, OnWord3(on_word));
}
size_t for_each_suffix(size_t RootState, fstring prefix
		, const OnNthWord& on_word, const ByteTR& tr
		) const override final {
	assert(m_is_dag);
	check_is_dag(BOOST_CURRENT_FUNCTION);
	return tpl_for_each_suffix<OnWord3, const ByteTR&>
			(RootState, prefix, OnWord3(on_word), tr);
}
size_t for_each_suffix(size_t RootState, fstring prefix
		, const OnNthWord& on_word
		) const override final {
	assert(m_is_dag);
	check_is_dag(BOOST_CURRENT_FUNCTION);
	return tpl_for_each_suffix(RootState, prefix, OnWord3(on_word));
}

size_t match_key(auchar_t delim, fstring str
		, const OnMatchKey& on_match
		) const override final {
	if (delim < 256 && delim != m_kv_delim) {
		assert(m_is_dag);
		check_is_dag(BOOST_CURRENT_FUNCTION);
	}
	return tpl_match_key<const OnMatchKey&>(delim, str, on_match);
}
size_t match_key(auchar_t delim, fstring str
		, const OnMatchKey& on_match, const ByteTR& tr
		) const override final {
	if (delim < 256 && delim != m_kv_delim) {
		assert(m_is_dag);
		check_is_dag(BOOST_CURRENT_FUNCTION);
	}
	return tpl_match_key<const OnMatchKey&, const ByteTR&>
			(delim, str, on_match, tr);
}

size_t match_key(auchar_t delim, ByteInputRange& r
		, const OnMatchKey& on_match
		) const override final {
	if (delim < 256 && delim != m_kv_delim) {
		assert(m_is_dag);
		check_is_dag(BOOST_CURRENT_FUNCTION);
	}
	return stream_match_key<ByteInputRange, const OnMatchKey&>
			(delim, r, on_match);
}
size_t match_key(auchar_t delim, ByteInputRange& r
		, const OnMatchKey& on_match, const ByteTR& tr
		) const override final {
	if (delim < 256 && delim != m_kv_delim) {
		assert(m_is_dag);
		check_is_dag(BOOST_CURRENT_FUNCTION);
	}
	return stream_match_key<ByteInputRange, const OnMatchKey&, const ByteTR&>
			(delim, r, on_match, tr);
}

size_t match_key_l(auchar_t delim, fstring str
		, const OnMatchKey& on_match
		) const override final {
	if (delim < 256 && delim != m_kv_delim) {
		assert(m_is_dag);
		check_is_dag(BOOST_CURRENT_FUNCTION);
	}
	return tpl_match_key_l<const OnMatchKey&>(delim, str, on_match);
}
size_t match_key_l(auchar_t delim, fstring str
		, const OnMatchKey& on_match, const ByteTR& tr
		) const override final {
	if (delim < 256 && delim != m_kv_delim) {
		assert(m_is_dag);
		check_is_dag(BOOST_CURRENT_FUNCTION);
	}
	return tpl_match_key_l<const OnMatchKey&, const ByteTR&>
			(delim, str, on_match, tr);
}

size_t match_key_l(auchar_t delim, ByteInputRange& r
		, const OnMatchKey& on_match
		) const override final {
	if (delim < 256 && delim != m_kv_delim) {
		assert(m_is_dag);
		check_is_dag(BOOST_CURRENT_FUNCTION);
	}
	return stream_match_key_l<ByteInputRange, const OnMatchKey&>
			(delim, r, on_match);
}
size_t match_key_l(auchar_t delim, ByteInputRange& r
		, const OnMatchKey& on_match, const ByteTR& tr
		) const override final {
	if (delim < 256 && delim != m_kv_delim) {
		assert(m_is_dag);
		check_is_dag(BOOST_CURRENT_FUNCTION);
	}
	return stream_match_key_l<ByteInputRange, const OnMatchKey&, const ByteTR&>
			(delim, r, on_match, tr);
}

private:
template<class T>
struct GetArg {
	T t;
	explicit GetArg(const T& x = T()) : t(x) {}
	void operator()(const T& x) { t = x; }
};
typedef GetArg<size_t> GetLen;
public:
size_t max_prefix_match(fstring text, size_t* max_len)
const override final {
	GetLen full_len(0);
	size_t plen = tpl_match_prefix<GetLen&>(text, full_len);
	*max_len = full_len.t;
	return plen;
}
size_t max_prefix_match(fstring text, size_t* max_len
		, const ByteTR& tr
		) const override final {
	typedef decltype(tr) TR;
	GetLen full_len(0);
	size_t plen = tpl_match_prefix<GetLen&, TR>(text, full_len, tr);
	*max_len = full_len.t;
	return plen;
}
size_t max_prefix_match(ByteInputRange& r, size_t* max_len)
const override final {
	GetLen full_len(0);
	size_t plen = stream_match_prefix<ByteInputRange, GetLen&>(r, full_len);
	*max_len = full_len.t;
	return plen;
}
size_t max_prefix_match(ByteInputRange& r, size_t* max_len
		, const ByteTR& tr
		) const override final {
	GetLen full_len(0);
	size_t plen = stream_match_prefix
		<ByteInputRange, GetLen&, const ByteTR&>(r, full_len, tr);
	*max_len = full_len.t;
	return plen;
}

size_t all_prefix_match(fstring text
		, const boost::function<void(size_t match_len)>& on_match
		) const override final {
	typedef decltype(on_match) OnMatch;
	return tpl_match_prefix<OnMatch>(text, on_match);
}
size_t all_prefix_match(fstring text
		, const boost::function<void(size_t match_len)>& on_match
		, const ByteTR& tr
		) const override final {
	typedef decltype(on_match) OnMatch;
	typedef decltype(tr) TR;
	return tpl_match_prefix<OnMatch,TR>(text,on_match,tr);
}
size_t all_prefix_match(ByteInputRange& r
		, const boost::function<void(size_t match_len)>& on_match
		) const override final {
	typedef decltype(on_match) OnMatch;
	return stream_match_prefix<ByteInputRange, OnMatch>(r, on_match);
}
size_t all_prefix_match(ByteInputRange& r
		, const boost::function<void(size_t match_len)>& on_match
		, const ByteTR& tr
		) const override final {
	typedef decltype(on_match) OnMatch;
	typedef decltype(tr) TR;
	return stream_match_prefix<ByteInputRange, OnMatch, TR>(r, on_match, tr);
}

