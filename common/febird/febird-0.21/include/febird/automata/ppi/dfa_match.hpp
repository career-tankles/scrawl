public:
/// @param on_suffix(size_t nth, fstring suffix, state_id_t accept_state)
///    calling of on_nth_word is in lexical_graphical order,
///    nth is the ordinal, 0 based
/// @return number of suffixes
template<class OnSuffix>
size_t tpl_for_each_suffix(fstring prefix, OnSuffix on_suffix) const {
	return tpl_for_each_suffix<OnSuffix>(initial_state, prefix, on_suffix);
}
template<class OnSuffix, class TR>
size_t tpl_for_each_suffix(fstring prefix, OnSuffix on_suffix, TR tr) const {
	return tpl_for_each_suffix<OnSuffix, TR>(initial_state, prefix, on_suffix, tr);
}
template<class OnSuffix>
size_t
tpl_for_each_suffix(state_id_t RootState, fstring prefix, OnSuffix on_suffix)
const {
	return tpl_for_each_suffix<OnSuffix>
		(RootState, prefix, on_suffix, IdentityTranslator());
}
template<class OnSuffix, class TR>
size_t
tpl_for_each_suffix(state_id_t RootState, fstring prefix, OnSuffix on_suffix, TR tr)
const {
	assert(RootState < total_states());
	ASSERT_isNotFree(RootState);
	state_id_t curr = RootState;
	size_t prelen = 0;
	for (size_t i = 0; ; ++i) {
		if (is_pzip(curr)) {
			const byte_t* s2 = get_zpath_data(curr);
			size_t n2 = *s2++;
			size_t n  = std::min<size_t>(n2, prefix.n-i);
			for (size_t j = 0; j < n; ++i, ++j)
				if ((byte_t)tr(prefix.p[i]) != s2[j]) return 0;
			prelen = n;
		} else
			prelen = 0;
		assert(i <= prefix.size());
		if (prefix.size() == i)
			break;
		state_id_t next = state_move(curr, (byte_t)tr(prefix.p[i]));
		if (nil_state == next)
			return 0;
		assert(next < total_states());
		ASSERT_isNotFree(next);
		curr = next;
	}
	ForEachWord_DFS<MyType, OnSuffix> vis(this, on_suffix);
	vis.start(curr, prelen);
	return vis.nth;
}

bool accept(const fstring str) const {
	return accept(str, IdentityTranslator());
}
template<class TR>
bool accept(const fstring str, TR tr) const {
	state_id_t curr = initial_state;
	for (size_t i = 0; ; ++i) {
		if (is_pzip(curr)) {
			const byte_t* s2 = get_zpath_data(curr);
			size_t n = *s2++;
			assert(n > 0);
			if (i + n > str.size())
				return false;
			size_t j = 0;
			do { // prefer do .. while for performance
				if ((byte_t)tr(str.p[i++]) != s2[j++])
					return false;
			} while (j < n);
		}
		assert(i <= str.size());
		if (str.size() == i)
			return is_term(curr);
		state_id_t next = state_move(curr, (byte_t)tr(str.p[i]));
		if (nil_state == next)
			return false;
		assert(next < total_states());
		ASSERT_isNotFree(next);
		curr = next;
	}
}

/// @param on_match(size_t endpos)
/// @return max forward scaned chars
template<class OnMatch>
size_t tpl_match_prefix(fstring str, OnMatch on_match) const {
	return tpl_match_prefix<OnMatch>(str, on_match, IdentityTranslator());
}

/// @param on_match(size_t endpos)
/// @param tr(byte_t c) translate c, such as 'tolower'
/// @return max forward scaned chars
template<class OnMatch, class Translator>
size_t tpl_match_prefix(fstring str, OnMatch on_match, Translator tr) const {
	state_id_t curr = initial_state;
	for (size_t i = 0; ; ++i) {
		if (is_pzip(curr)) {
			const byte_t* s2 = get_zpath_data(curr);
			size_t n2 = *s2++;
			size_t n  = std::min<size_t>(n2, str.size()-i);
			for (size_t j = 0; j < n; ++i, ++j)
				if ((byte_t)tr(str.p[i]) != s2[j]) return i;
			if (n < n2)
				return str.size();
		}
		assert(i <= str.size());
		if (is_term(curr))
			on_match(i);
		if (str.size() == i)
			return i;
		state_id_t next = state_move(curr, (byte_t)tr(str.p[i]));
		if (nil_state == next)
			return i;
		assert(next < total_states());
		ASSERT_isNotFree(next);
		curr = next;
	}
	assert(0);
}

/// @param r requirements:
///    bool_t b =  r.empty();
///    byte_t c = *r; // read once, so InputRange is OK
///              ++r;
/// @param on_match(size_t endpos)
/// @param tr(byte_t c) translate c, such as 'tolower'
/// @return max forward scaned chars
template<class Range, class OnMatch, class Translator>
size_t 
stream_match_prefix(Range& r, OnMatch on_match, Translator tr)
const {
	state_id_t curr = initial_state;
	for (size_t i = 0; ; ++i, ++r) {
		if (is_pzip(curr)) {
			const byte_t* s2 = get_zpath_data(curr);
			size_t n2 = *s2++;
			for (size_t j = 0; j < n2; ++j, ++r)
				if (r.empty() || (byte_t)tr(*r) != s2[j]) return i+j;
			i += n2;
		}
		if (is_term(curr))
			on_match(i);
		if (r.empty())
			return i;
		state_id_t next = state_move(curr, (byte_t)tr(*r));
		if (nil_state == next)
			return i;
		assert(next < total_states());
		ASSERT_isNotFree(next);
		curr = next;
	}
	assert(0);
}
template<class Range, class OnMatch>
size_t stream_match_prefix(Range& r, OnMatch on_match) const {
	return stream_match_prefix<Range, OnMatch, IdentityTranslator>
		(r, on_match, IdentityTranslator());
}

/// @param on_match(size_t keylen, size_t value_idx, fstring value)
/// @return max forward scaned chars
template<class OnMatch>
size_t tpl_match_key(auchar_t delim, fstring str, OnMatch on_match) const {
	assert(delim < sigma);
	return tpl_match_key<OnMatch, IdentityTranslator>
		(delim, str, on_match, IdentityTranslator());
}

/// @param on_match(size_t keylen, size_t value_idx, fstring value)
/// @param tr(byte_t c) translate c, such as 'tolower'
/// @return max forward scaned chars
template<class OnMatch, class Translator>
size_t
tpl_match_key(auchar_t delim, fstring str, OnMatch on_match, Translator tr)
const {
	assert(delim < sigma);
	typedef iterator_to_byte_stream<const char*> R;
	R r(str.begin(), str.end());
	return stream_match_key<R, OnMatch, Translator>(delim, r, on_match, tr);
}
template<class OnMatch, class Translator>
size_t
tpl_match_key(state_id_t RootState,
	auchar_t delim, fstring str, OnMatch on_match, Translator tr)
const {
	assert(delim < sigma);
	typedef iterator_to_byte_stream<const char*> R;
	R r(str.begin(), str.end());
	return stream_match_key<R, OnMatch, Translator>
			(RootState, delim, r, on_match, tr);
}

/// @param r requirements:
///    bool_t b =  r.empty();
///    byte_t c = *r; // read once, so InputRange is OK
///              ++r;
/// @param on_match(size_t keylen, size_t value_idx, fstring value)
/// @param tr(byte_t c) translate c, such as 'tolower'
/// @return max forward scaned chars
template<class Range, class OnMatch, class Translator>
size_t
stream_match_key(auchar_t delim, Range& r, OnMatch on_match, Translator tr)
const {
	return stream_match_key<Range, OnMatch, Translator>
			(initial_state, delim, r, on_match, tr);
}
template<class Range, class OnMatch, class Translator>
size_t
stream_match_key(state_id_t RootState, 
	auchar_t delim, Range& r, OnMatch on_match, Translator tr)
const {
	assert(delim < sigma);
	state_id_t curr = RootState;
	size_t i;
	auto on_suffix = [&](size_t nth, fstring value, size_t) {
		const size_t const_i = i; // keylen
		on_match(const_i, nth, value);
	};
	ForEachWord_DFS<MyType, decltype(on_suffix)> vis(this, on_suffix);
	for (i = 0; ; ++i, ++r) {
		if (is_pzip(curr)) {
			const byte_t* s2 = get_zpath_data(curr);
			size_t n2 = *s2++;
			for (size_t j = 0; j < n2; ++j, ++r) {
				byte_t c2 = s2[j];
				if (c2 == delim) {
					i += j; // i is referred in `vis`
					vis.start(curr, j+1);
					return i;
				}
				if (r.empty() || (byte_t)tr(*r) != c2)
					return i + j;
			}
			i += n2;
		}
		state_id_t value_start = state_move(curr, delim);
		if (nil_state != value_start) {
			vis.start(value_start, 0);
		}
		if (r.empty())
			return i;
		state_id_t next = state_move(curr, (byte_t)tr(*r));
		if (nil_state == next)
			return i;
		assert(next < total_states());
		ASSERT_isNotFree(next);
		curr = next;
	}
	assert(0);
}
template<class Range, class OnMatch>
size_t stream_match_key(auchar_t delim, Range& r, OnMatch on_match) const {
	assert(delim < sigma);
	return stream_match_key<Range, OnMatch, IdentityTranslator>
		(delim, r, on_match, IdentityTranslator());
}

template<class OnMatch>
size_t tpl_match_key_l(auchar_t delim, fstring str, OnMatch on_match) const {
	assert(delim < sigma);
	return tpl_match_key_l<OnMatch, IdentityTranslator>
		(delim, str, on_match, IdentityTranslator());
}
template<class OnMatch, class Translator>
size_t
tpl_match_key_l(auchar_t delim, fstring str, OnMatch on_match, Translator tr)
const {
	assert(delim < sigma);
	typedef iterator_to_byte_stream<const char*> R;
	R r(str.begin(), str.end());
	return stream_match_key_l<R, OnMatch, Translator>(delim, r, on_match, tr);
}
template<class OnMatch, class Translator>
size_t
tpl_match_key_l(state_id_t RootState,
	auchar_t delim, fstring str, OnMatch on_match, Translator tr)
const {
	assert(delim < sigma);
	typedef iterator_to_byte_stream<const char*> R;
	R r(str.begin(), str.end());
	return stream_match_key_l<R, OnMatch, Translator>
			(RootState, delim, r, on_match, tr);
}
template<class Range, class OnMatch, class Translator>
size_t
stream_match_key_l(auchar_t delim, Range& r, OnMatch on_match, Translator tr)
const {
	return stream_match_key_l<Range, OnMatch, Translator>
			(initial_state, delim, r, on_match, tr);
}
template<class Range, class OnMatch, class Translator>
size_t
stream_match_key_l(state_id_t RootState,
	auchar_t delim, Range& r, OnMatch on_match, Translator tr)
const {
	assert(delim < sigma);
	state_id_t curr = RootState;
	state_id_t last_state = nil_state;
	size_t last_i = 0, last_j = 0, i;
	for (i = 0; ; ++i, ++r) {
		if (is_pzip(curr)) {
			const byte_t* s2 = get_zpath_data(curr);
			size_t n2 = *s2++;
			for (size_t j = 0; j < n2; ++j, ++r) {
				byte_t c2 = s2[j];
				if (c2 == delim) {
					i += j;
					last_i = i;
					last_j = j + 1;
					last_state = curr;
					goto Done;
				}
				if (r.empty() || (byte_t)tr(*r) != c2) {
					i += j;
					goto Done;
				}
			}
			i += n2;
		}
		state_id_t value_start = state_move(curr, delim);
		if (nil_state != value_start) {
			last_i = i;
			last_j = 0;
			last_state = value_start;
		}
		if (r.empty()) {
			goto Done;
		}
		state_id_t next = state_move(curr, (byte_t)tr(*r));
		if (nil_state == next) {
			goto Done;
		}
		assert(next < total_states());
		ASSERT_isNotFree(next);
		curr = next;
	}
	assert(0);
Done:
	if (nil_state != last_state) {
		auto on_suffix = [&](size_t nth, fstring val, size_t) {
			const size_t const_i = last_i; // keylen
			on_match(const_i, nth, val);
		};
		ForEachWord_DFS<MyType, decltype(on_suffix)> vis(this, on_suffix);
		vis.start(last_state, last_j);
	}
	return i;
}
template<class Range, class OnMatch>
size_t stream_match_key_l(auchar_t delim, Range& r, OnMatch on_match) const {
	assert(delim < sigma);
	return stream_match_key_l<Range, OnMatch, IdentityTranslator>
		(delim, r, on_match, IdentityTranslator());
}

/// @param tr(byte_t c) translate c, such as 'tolower'
template<class Translator>
size_t
match_path(fstring str, valvec<state_id_t>* path, Translator tr) const {
	return match_path<Translator>(initial_state, str, path, tr);
}
template<class Translator>
size_t match_path(state_id_t RootState, fstring str
				, valvec<state_id_t>* path, Translator tr) const {
	state_id_t curr = RootState;
	path->resize(0);
	for (size_t i = 0; ; ++i) {
		if (is_pzip(curr)) {
			const byte_t* s2 = get_zpath_data(curr);
			size_t n = std::min<size_t>(*s2++, str.size()-i);
			for (size_t j = 0; j < n; ++i, ++j) {
				path->push_back(curr);
				if ((byte_t)tr(str.p[i]) != s2[j]) return i;
			}
		}
		assert(i <= str.size());
		path->push_back(curr);
		if (str.size() == i)
			return i;
		state_id_t next = state_move(curr, (byte_t)tr(str.p[i]));
		if (nil_state == next)
			return i;
		assert(next < total_states());
		ASSERT_isNotFree(next);
		curr = next;
	}
	assert(0);
}

/// @return pos <  len     : the mismatch pos
///         pos == len     : matched all
///         pos == len + 1 : matched state is not a final state
size_t first_mismatch_pos(const fstring str) const {
	state_id_t curr = initial_state;
	for (size_t i = 0; i < str.size(); ++i) {
		if (is_pzip(curr)) {
			const byte_t* s2 = get_zpath_data(curr);
			size_t n = *s2++;
			assert(n > 0);
			size_t m = i + n > str.size() ? str.size() - i : n;
			size_t j = 0;
			do { // prefer do .. while for performance
				if ((byte_t)str.p[i] != s2[j])
					return i;
				++i; ++j;
			} while (j < m);
			if (str.size() == i)
				return is_term(curr) ? str.n : str.n + 1;
		}
		state_id_t next = state_move(curr, (byte_t)str.p[i]);
		if (nil_state == next)
			return i;
		assert(next < total_states());
		ASSERT_isNotFree(next);
		curr = next;
	}
	return !is_pzip(curr) && is_term(curr) ? str.n : str.n + 1;
}


