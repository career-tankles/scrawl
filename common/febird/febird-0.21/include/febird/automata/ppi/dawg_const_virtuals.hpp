public:

size_t v_match_words(fstring str, const OnMatchDAWG& on_match)
const override final {
	return this->match_words(str, on_match);
}
size_t v_match_words(fstring str, const OnMatchDAWG& on_match, const ByteTR& tr)
const override final {
	return this->match_words(str, on_match, tr);
}
size_t v_match_words(fstring str, const OnMatchDAWG& on_match, const byte_t* tr)
const override final {
	assert(NULL != tr);
	auto tr_by_tab = [tr](byte_t ch){ return tr[ch]; };
	return this->match_words(str, on_match, tr_by_tab);
}

struct LongestOnMatch {
	size_t cnt = 0;
	size_t len = 0;
	size_t nth = 0;
	void operator()(size_t len2, size_t nth2) {
		cnt++;
		len = len2;
		nth = nth2;
	}
	bool get_result(size_t* p_len, size_t* p_nth) const {
		if (cnt) {
			*p_len = len;
			*p_nth = nth;
			return true;
		}
		return false;
	}
};
bool longest_prefix(fstring str, size_t* len, size_t* nth, const ByteTR& tr)
const override final {
	assert(NULL != len);
	assert(NULL != nth);
	LongestOnMatch on_match;
	match_words<LongestOnMatch&>(str, on_match, tr);
	return on_match.get_result(len, nth);
}
bool longest_prefix(fstring str, size_t* len, size_t* nth, const byte_t* tr)
const override final {
	assert(NULL != len);
	assert(NULL != nth);
	assert(NULL != tr);
	LongestOnMatch on_match;
	match_words<LongestOnMatch&>(str, on_match, TableTranslator(tr));
	return on_match.get_result(len, nth);
}
bool longest_prefix(fstring str, size_t* len, size_t* nth)
const override final {
	assert(NULL != len);
	assert(NULL != nth);
	LongestOnMatch on_match;
	this->match_words<LongestOnMatch&>(str, on_match);
	return on_match.get_result(len, nth);
}

//-----------------------------------------------------------------------
///@param on_hit(size_t match_end_pos, size_t word_id)
template<class OnMatch>
size_t match_words(fstring str, OnMatch on_match)
const {
	typedef iterator_to_byte_stream<const char*> R;
	R r(str.begin(), str.end());
	return this->match_words<R, OnMatch>(r, on_match, IdentityTranslator());
}

///@param on_hit(size_t match_end_pos, size_t word_id)
///@param tr(byte_t c) translate c, such as 'tolower'
template<class OnMatch, class Translator>
size_t match_words(fstring str, OnMatch on_match, Translator tr)
const {
	typedef iterator_to_byte_stream<const char*> R;
	R r(str.begin(), str.end());
	return this->match_words<R, OnMatch, Translator>(r, on_match, tr);
}

