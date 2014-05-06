#include "tmplinst.hpp"
#include "automata.hpp"
#include "dense_dfa.hpp"
#include "mre_match.hpp"
#include "dfa_mmap_header.hpp"
//#include "nfa.hpp"
#include <febird/util/autofree.hpp>
#include <pthread.h>

#define DFA_TMPL_CLASS_PREFIX template<class State> void Automata<State>
#include "ppi/pool_dfa_mmap.hpp"

#define DFA_TMPL_CLASS_PREFIX \
	template<class StateID, int Sigma, class State> \
	void DenseDFA<StateID, Sigma, State>
#include "ppi/pool_dfa_mmap.hpp"

#include <febird/config.hpp>
#include <atomic>

#if defined(FSA_USE_TBB_RW_LOCK)
	#include <tbb/queuing_rw_mutex.h>
	typedef tbb::queuing_rw_mutex rw_mutex_t;
#else
	#include <tbb/null_rw_mutex.h>
	typedef tbb::null_rw_mutex rw_mutex_t;
#endif

struct DynDenseState : public DenseState<uint32_t, 320> {
	uint32_t ps_indx; // index to power_set
	uint32_t ps_link; // hash link
	MyBitMap has_dnext;
	BOOST_STATIC_ASSERT(sizeof(size_t) == sizeof(bm_uint_t));

	DynDenseState() : has_dnext(1) {}

	size_t get_cached_hash() const {
		if (sizeof(size_t) == 4) {
			return has_dnext.block(MyBitMap::BlockN-1);
		}
		assert(sizeof(size_t) == 8);
		size_t low29 = this->reserved_bits;
		size_t hig35 = has_dnext.block(MyBitMap::BlockN-1) & size_t(-1) << 29;
		size_t hash = low29 | hig35;
		return hash;
	}
	void set_cached_hash(size_t hash) {
		bm_uint_t& bl = has_dnext.block(MyBitMap::BlockN-1);
		if (sizeof(size_t) == 4) {
			bl = hash;
		} else {
			size_t low29 = hash & 0x1FFFFFFF;
			size_t hig35 = hash & size_t(-1) << 29;
		   	this->reserved_bits = low29;
			bl = bl & 0x1FFFFFFF | hig35;
		}
	}
};

typedef DenseDFA<uint32_t, 320, DynDenseState> DenseDFA_DynDFA_320_Base;
class DenseDFA_DynDFA_320 : public DenseDFA<uint32_t, 320, DynDenseState> {
	state_id_t* pBucket;
	size_t      nBucket;
	state_id_t  nStateNumComplete;
	state_id_t  nStateNumHotLimit;

	template<class DataIO>
	friend void
	DataIO_loadObject(DataIO& dio, DenseDFA_DynDFA_320& da) {
		dio >> static_cast<DenseDFA_DynDFA_320_Base&>(da);
	}
	template<class DataIO>
	friend void
	DataIO_saveObject(DataIO& dio, const DenseDFA_DynDFA_320& da) {
		dio << static_cast<const DenseDFA_DynDFA_320_Base&>(da);
	}
	size_t subset_hash(const state_id_t* beg, size_t len) const;
	bool subset_equal(const state_id_t* xbeg, size_t xlen, size_t y) const;

	///@param nfa is DynDFA_Context::dfa
	template<class DFA> void dyn_init_aux(const DFA* nfa);
	template<class DFA> void warm_up_aux(const DFA* nfa);

	state_id_t add_next_set_single_thread(size_t oldsize, state_id_t dcurr);
	void hash_resize();

public:
	size_t maxmem;
	valvec<state_id_t>  power_set;
	std::atomic<size_t> refcnt;
	std::atomic<size_t> num_readers;
	rw_mutex_t rw_mutex;

	DenseDFA_DynDFA_320();
	~DenseDFA_DynDFA_320();

	size_t dyn_mem_size() const;
	size_t ps_index(size_t s) const {
		assert(s < states.size());
		return states[s].ps_indx;
	}
	void dyn_discard_deep_cache();
	void dyn_init(const DFA_Interface* nfa);
	void warm_up(const DFA_Interface* nfa);

	template<class DFA>
	state_id_t dyn_state_move_mt(const DFA* nfa, state_id_t dcurr, auchar_t ch, class DynDFA_Context* ctx);
	template<class DFA>
	state_id_t dyn_state_move_st(const DFA* nfa, state_id_t dcurr, auchar_t ch);
};
namespace febird {

// instantiated these 3 classes

TMPL_INST_DFA_CLASS(Automata<State32_512>);
TMPL_INST_DFA_CLASS(DenseDFA<uint32_t>);
TMPL_INST_DFA_CLASS(DenseDFA_uint32_320);
TMPL_INST_DFA_CLASS(DenseDFA_DynDFA_320);

} // namespace febird

class DynDFA_Context : boost::noncopyable {
public:
	struct MatchEntry {
		int curr_pos; // i
		int skip_len; // j
		size_t state_id;
	};
	valvec<MatchEntry> stack;

	// only used by multi thread matching
	valvec<uint32_t>   next_set;
	rw_mutex_t::scoped_lock* lock;

	DynDFA_Context() {
		lock = NULL;
	}
};

template<class DFA>
class MultiRegexSubmatchTmpl : public MultiRegexSubmatch  {
public:
	template<class TR>
	size_t dmatch_func2(auchar_t delim, fstring text, const TR& tr) {
		const DFA* dfa = static_cast<const DFA*>(this->dfa);
		assert(delim < DFA::sigma);
		this->reset();
		auto on_match = [&](size_t klen, size_t, fstring val) {
			assert(val.size() == sizeof(CaptureBinVal));
			CaptureBinVal ci;
			memcpy(&ci, val.data(), sizeof(ci));
			this->set_capture(ci.regex_id, ci.capture_id, klen);
		};
		(void)dfa->tpl_match_key(delim, text, on_match, tr);
		size_t match_len = 0;
		for (int regex_id : m_fullmatch_regex) {
			match_len = std::max(size_t(this->get(regex_id, 1)), match_len);
		}
		return match_len;
	}
	static size_t
	match_func1(MultiRegexSubmatch* xThis, fstring text) {
		auto self = static_cast<MultiRegexSubmatchTmpl*>(xThis);
		auchar_t delim = self->dfa->kv_delim();
		assert(delim < DFA::sigma);
		return self->dmatch_func2(delim, text, IdentityTranslator());
	}
	static size_t
	match_func2(MultiRegexSubmatch* xThis, fstring text, const ByteTR& tr) {
		auto self = static_cast<MultiRegexSubmatchTmpl*>(xThis);
		auchar_t delim = self->dfa->kv_delim();
		assert(delim < DFA::sigma);
		return self->dmatch_func2(delim, text, tr);
	}
};

#define ON_DFA_CLASS(MyClassName, DFA) \
	else if (dynamic_cast<const DFA*>(dfa)) { \
		pf_match1 = &MyClassName##Tmpl<DFA >::match_func1; \
		pf_match2 = &MyClassName##Tmpl<DFA >::match_func2; \
	}
#define DispatchConcreteDFA(MyClassName, OnDFA) \
	if (0) {} \
	OnDFA(MyClassName, Automata<State32_512>) \
	OnDFA(MyClassName, DenseDFA<uint32_t>) \
	OnDFA(MyClassName, DenseDFA_uint32_320) \
	else { \
		FEBIRD_THROW(std::invalid_argument \
			, "Don't support %s, dfa_class: %s" \
			, "MultiRegexFullMatchDynamicDfa", typeid(*dfa).name() \
			); \
	}
#define DO_SET_func_ptr(MyClassName) \
	DispatchConcreteDFA(MyClassName, ON_DFA_CLASS)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class DFA>
class MultiRegexFullMatchTmpl : public MultiRegexFullMatch {
public:
	template<class TR>
	size_t dmatch_func2(auchar_t delim, fstring text, const TR& tr) {
		DynDFA_Context* ctx = this->m_ctx;
		const DFA* au = static_cast<const DFA*>(m_dfa);
		assert(delim < DFA::sigma);
		this->erase_all();
		ctx->stack.erase_all();
		typedef typename DFA::state_id_t state_id_t;
		typedef iterator_to_byte_stream<const char*> R;
		R r(text.begin(), text.end());
		state_id_t curr = initial_state;
		size_t i;
		for (i = 0; ; ++i, ++r) {
			if (au->is_pzip(curr)) {
				const byte_t* s2 = au->get_zpath_data(curr);
				size_t n2 = *s2++;
				for (size_t j = 0; j < n2; ++j, ++r) {
					byte_t c2 = s2[j];
					if (febird_unlikely(c2 == delim)) {
						i += j;
						DynDFA_Context::MatchEntry e;
						e.curr_pos = i;
						e.skip_len = j + 1;
						e.state_id = curr;
						ctx->stack.push_back(e);
					}
				   	if (febird_unlikely(r.empty() || (byte_t)tr(*r) != c2)) {
						i += j;
						goto Done;
					}
				}
				i += n2;
			}
			state_id_t value_start = au->state_move(curr, delim);
			if (febird_unlikely(DFA::nil_state != value_start)) {
				DynDFA_Context::MatchEntry e;
				e.curr_pos = i;
				e.skip_len = 0;
				e.state_id = value_start;
				ctx->stack.push_back(e);
			}
			if (febird_unlikely(r.empty())) {
				goto Done;
			}
			state_id_t next = au->state_move(curr, (byte_t)tr(*r));
			if (febird_unlikely(DFA::nil_state == next)) {
				goto Done;
			}
			assert(next < au->total_states());
			ASSERT_isNotFree2(au, next);
			curr = next;
		}
		assert(0);
	Done:
		for (size_t j = ctx->stack.size(); j > 0; ) {
			--j;
			const DynDFA_Context::MatchEntry e = ctx->stack[j];
			auto on_suffix = [&](size_t, fstring val, size_t) {
				MultiRegexSubmatch::CaptureBinVal bv;
				assert(val.size() == sizeof(MultiRegexSubmatch::CaptureBinVal));
				memcpy(&bv, val.data(), sizeof(bv));
				if (1 == bv.capture_id) { // full match end point
					this->push_back(bv.regex_id);
				//	printf("delim=%X j=%zd regex_id=%d  text=%.*s\n", delim, j, bv.regex_id, text.ilen(), text.p);
				}
			};
			const int Bufsize = sizeof(MultiRegexSubmatch::CaptureBinVal); // = 8
			ForEachWord_DFS<DFA, decltype(on_suffix), Bufsize> vis(au, on_suffix);
			vis.start(e.state_id, e.skip_len);
			if (!this->empty())
				return e.curr_pos; // fullmatch length
		}
		return 0;
	}

	template<class TR>
	size_t dmatch_func3(auchar_t delim, fstring text, const TR& tr) {
		DynDFA_Context* ctx = this->m_ctx;
		const DFA* au = static_cast<const DFA*>(m_dfa);
		assert(delim < DFA::sigma);
		assert(delim == 257); // now this is true
		this->erase_all();
		ctx->stack.erase_all();
		typedef typename DFA::state_id_t state_id_t;
		typedef iterator_to_byte_stream<const char*> R;
		R r(text.begin(), text.end());
		state_id_t curr = initial_state;
		size_t i;
		for (i = 0; ; ++i, ++r) {
			if (au->is_pzip(curr)) {
				const byte_t* s2 = au->get_zpath_data(curr);
				size_t n2 = *s2++;
				for (size_t j = 0; j < n2; ++j, ++r) {
					byte_t c2 = s2[j];
				   	if (febird_unlikely(r.empty() || (byte_t)tr(*r) != c2)) {
						i += j;
						goto Done;
					}
				}
				i += n2;
			}
			state_id_t value_start = au->state_move(curr, delim);
			if (febird_unlikely(DFA::nil_state != value_start)) {
				DynDFA_Context::MatchEntry e;
				e.curr_pos = i;
				e.skip_len = 0; // unused in this function
				e.state_id = value_start;
				ctx->stack.push_back(e);
			}
			if (febird_unlikely(r.empty())) {
				goto Done;
			}
			state_id_t next = au->state_move(curr, (byte_t)tr(*r));
			if (febird_unlikely(DFA::nil_state == next)) {
				goto Done;
			}
			assert(next < au->total_states());
			ASSERT_isNotFree2(au, next);
			curr = next;
		}
		assert(0);
	Done:
		for (size_t j = ctx->stack.size(); j > 0; ) {
			--j;
			const DynDFA_Context::MatchEntry e = ctx->stack[j];
			auto on_suffix = [&](size_t, fstring val, size_t) {
				uint32_t regex_id;
				assert(val.size() == sizeof(regex_id));
				memcpy(&regex_id, val.data(), sizeof(regex_id));
				this->push_back(regex_id);
			//	printf("delim=%X j=%zd regex_id=%d  text=%.*s\n", delim, j, regex_id, text.ilen(), text.p);
			};
			const int Bufsize = sizeof(MultiRegexSubmatch::CaptureBinVal); // = 8
			ForEachWord_DFS<DFA, decltype(on_suffix), Bufsize> vis(au, on_suffix);
			vis.start(e.state_id, 0);
			if (!this->empty())
				return e.curr_pos; // fullmatch length
		}
		return 0;
	}

	template<class TR>
	size_t dmatch_func(auchar_t delim, fstring text, const TR& tr) {
		assert(delim <= 256); // now this is true
		if (delim == 256)
			return this->dmatch_func3(257, text, tr);
		else
			return this->dmatch_func2(delim, text, tr);
	}
	static size_t
	match_func1(MultiRegexFullMatch* xThis, fstring text) {
		auto self = static_cast<MultiRegexFullMatchTmpl*>(xThis);
		auchar_t delim = self->m_dfa->kv_delim();
		assert(delim < DFA::sigma);
		return self->dmatch_func(delim, text, IdentityTranslator());
	}
	static size_t
	match_func2(MultiRegexFullMatch* xThis, fstring text, const ByteTR& tr) {
		auto self = static_cast<MultiRegexFullMatchTmpl*>(xThis);
		auchar_t delim = self->m_dfa->kv_delim();
		assert(delim < DFA::sigma);
		return self->dmatch_func(delim, text, tr);
	}
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//

DenseDFA_DynDFA_320::DenseDFA_DynDFA_320() {
	refcnt = 0;
	maxmem = size_t(-1); // unlimited in initialize
	num_readers = 0;
	nStateNumComplete = 0;
	nStateNumHotLimit = 0;
	pBucket = NULL;
	nBucket = 0;
	this->m_dyn_sigma = 256 + 29; // 285
}
DenseDFA_DynDFA_320::~DenseDFA_DynDFA_320() {
	if (NULL != pBucket)
		::free(pBucket);
}

size_t DenseDFA_DynDFA_320::dyn_mem_size() const {
	return 0
		+ this->pool.size()
		+ sizeof(state_t) * states.size()
		+ sizeof(state_id_t) * power_set.size()
		+ sizeof(state_id_t) * nBucket
		;
}

inline
size_t
DenseDFA_DynDFA_320::subset_hash(const state_id_t* beg, size_t len)
const {
	assert(len > 0);
	size_t h = 0;
	size_t i = 0;
	do {
		h += beg[i];
		h *= 31;
		h += h << 3 | h >> (sizeof(h)*8-3);
	} while (++i < len);
	return h;
}
inline
bool
DenseDFA_DynDFA_320::
subset_equal(const state_id_t* px0, size_t xlen, size_t y)
const {
	size_t iy0 = states[y+0].ps_indx;
	size_t iy1 = states[y+1].ps_indx;
	size_t ylen = iy1 - iy0;
	if (xlen != ylen) return false;
	assert(xlen > 0);
	const state_id_t* py0 = &power_set[iy0];
	size_t i = 0;
	do {
		if (px0[i] != py0[i])
			return false;
	} while (++i < xlen);
	return true;
}

template<class StateID>
struct OneCharSubSet : valvec<StateID> {
	void resize0() {
		this->ch = -1;
		valvec<StateID>::resize0();
	}
	OneCharSubSet() { ch = -1; }
	ptrdiff_t  ch; // use ptrdiff_t for better align
};

void DenseDFA_DynDFA_320::dyn_discard_deep_cache() {
#if 1
	size_t mem_cap = 0
		+ this->pool.capacity()
		+ sizeof(state_t) * states.capacity()
		+ sizeof(state_id_t) * power_set.capacity()
		+ sizeof(state_id_t) * nBucket
		;
	fprintf(stderr
		, "dyn_discard_deep_cache: power_set=%zd states=%zd"
		  " powerlen/states=%f mem_size=%zd mem_cap=%zd\n"
		, power_set.size(), states.size()
		, power_set.size()/(states.size()+0.0)
		, dyn_mem_size(), mem_cap
		);
#endif
	assert(states.size() > nStateNumHotLimit+1);
	size_t my_sigma = this->get_sigma();
	febird::AutoFree<CharTarget<size_t> > allmove(
		(CharTarget<size_t>*)malloc(sizeof(CharTarget<size_t>)*my_sigma)
	  );
	if (NULL == allmove.p) {
		THROW_STD(runtime_error, "malloc allmove");
	}
	for(size_t dcurr = nStateNumComplete; dcurr < nStateNumHotLimit; ++dcurr) {
		size_t nmove = get_all_move(dcurr, allmove.p);
		assert(nmove <= my_sigma);
		size_t i = 0;
		for(size_t j = 0; j < nmove; ++j) {
			if (allmove.p[j].target < nStateNumHotLimit)
				allmove.p[i++] = allmove.p[j];
		}
		if (i < nmove) {
			del_all_move(dcurr);
			add_all_move(dcurr, allmove.p, i);
		}
	}
	size_t nPowerSetHotLimit = states[nStateNumHotLimit].ps_indx;
	states.resize(nStateNumHotLimit);
	states.push_back(); // the guard
//	states.shrink_to_fit(); // don't shrink_to_fit?
	states.back().ps_indx = nPowerSetHotLimit;
	power_set.resize(nPowerSetHotLimit);
//	power_set.shrink_to_fit(); // don't shrink_to_fit?
	this->compact(); // required
	hash_resize();
}

void DenseDFA_DynDFA_320::dyn_init(const DFA_Interface* dfa) {
	assert(power_set.size() == 0);
#define On_dyn_init(Class, DFA) \
	else if (const DFA* d = dynamic_cast<const DFA*>(dfa)) { \
		dyn_init_aux(d); }
	DispatchConcreteDFA(DenseDFA_DynDFA_320, On_dyn_init);
#undef On_dyn_init
}

void DenseDFA_DynDFA_320::warm_up(const DFA_Interface* dfa) {
	if (states.size() > 2) {
		fprintf(stderr
			, "DenseDFA_DynDFA_320 has been warmed up, power_set.size=%zd\n"
			, power_set.size());
		return;
	}
#define OnDFA(Class, DFA) \
	else if (const DFA* d = dynamic_cast<const DFA*>(dfa)) { \
		warm_up_aux(d); }
//	size_t oldsize = dyn->power_set.size();
	DispatchConcreteDFA(DenseDFA_DynDFA_320, OnDFA);
//	fprintf(stderr, "warm_up: power_set=%zd:%zd\n", oldsize, dyn->power_set.size());
#undef OnDFA
}

template<class DFA>
void DenseDFA_DynDFA_320::dyn_init_aux(const DFA* nfa) {
	power_set.reserve(nfa->total_states() * 2);
	states.reserve(nfa->total_states() * 1);
	states.resize(1);
	states[0].ps_indx = 0;
	auchar_t FakeEpsilonChar = 258;
	state_id_t curr = nfa->state_move(initial_state, FakeEpsilonChar);
	if (DFA::nil_state == curr) {
		THROW_STD(invalid_argument, "no transition on FakeEpsilonChar=258");
	}
	while (DFA::nil_state != curr) {
		power_set.unchecked_push_back(curr);
		curr = nfa->state_move(curr, FakeEpsilonChar);
	}
	nBucket = __hsm_stl_next_prime(states.capacity() * 3/2);
	pBucket = (state_id_t*)malloc(sizeof(state_id_t) * nBucket);
	if (NULL == pBucket) {
		THROW_STD(runtime_error, "malloc(%zd) hash bucket failed",
				sizeof(state_id_t) * nBucket);
	}
	std::fill_n(pBucket, nBucket, nil_state+0);
	state_id_t dnext = add_next_set_single_thread(0, nil_state);
	assert(initial_state == dnext); (void)dnext;
	assert(states.size() == 2);
}

template<class DFA>
void DenseDFA_DynDFA_320::warm_up_aux(const DFA* nfa) {
#if 0
	fprintf(stderr, "nBucket=%u states.capacity=%zd power_set.size=\n"
		, nBucket, states.capacity(), power_set.size());
#endif

// discover DynDFA states near initial_state...
//
	size_t my_sigma = this->get_sigma();
	smallmap<OneCharSubSet<state_id_t> > chmap(my_sigma);
	febird::AutoFree<CharTarget<size_t> > allmove(
		(CharTarget<size_t>*)malloc(sizeof(CharTarget<size_t>)*my_sigma)
	  );
	if (NULL == allmove.p) {
		THROW_STD(runtime_error, "malloc allmove");
	}
	size_t nPowerSetRoughLimit = nfa->total_states();
	// this loop is an implicit BFS search
	// states.size() is increasing during the loop execution
	for(size_t dcurr = 0; dcurr < states.size()-1; ++dcurr) {
		size_t sub_beg = states[dcurr+0].ps_indx;
		size_t sub_end = states[dcurr+1].ps_indx;
		chmap.resize0();
		for(size_t j = sub_beg; j < sub_end; ++j) {
			size_t snfa = power_set[j]; 
			size_t nmove = nfa->get_all_move(snfa, allmove.p);
			assert(nmove <= my_sigma);
			for(size_t k = 0; k < nmove; ++k) {
				auto  ct = allmove.p[k];
				chmap.bykey(ct.ch).push_back(ct.target);
			}
		}
		states[dcurr].has_dnext.fill(0);
		size_t i = 0;
		for(size_t j = 0; j < chmap.size(); ++j) {
			auto& subset = chmap.byidx(j);
			if (subset.ch >= 256 && subset.ch != 257)
			   	continue;
			size_t oldsize = power_set.size();
			power_set.append(subset);
			state_id_t dnext = add_next_set_single_thread(oldsize, nil_state);
			if (nil_state == dnext) {
				THROW_STD(runtime_error, "out of memory on warming up");
			}
			states[dcurr].has_dnext.set1(subset.ch);
			allmove.p[i].ch = subset.ch;
			allmove.p[i].target = dnext;
			i++;
		}
		std::sort(allmove.p, allmove.p + i, CharTarget_By_ch());
		this->add_all_move(dcurr, allmove.p, i);
		nStateNumComplete = dcurr+1;
		if (power_set.size() >= nPowerSetRoughLimit)
			break;
	}
	nStateNumHotLimit = states.size()-1; // exact limit
	size_t minmem = 8*1024 // plus all capacity sum
		+ this->pool.capacity()
		+ sizeof(state_t) * states.capacity()
		+ sizeof(state_id_t) * power_set.capacity()
		+ sizeof(state_id_t) * nBucket
		;
	if (size_t(-1) == maxmem)
		maxmem = 2 * minmem;
	else if (maxmem < minmem) {
		fprintf(stderr
			, "WARN:%s:%d: func=%s : maxmem=%zd < minmem=%zd, use default\n"
			, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION
			, maxmem, minmem);
		maxmem = minmem;
	}
	fprintf(stderr
		, "warmed: Complete=%u HotLimit=%u"
		 " Bucket=%u Power=%zd"
		 " Power/Complete=%f"
		 " Power/HotLimit=%f"
		 "\n"
		, nStateNumComplete, nStateNumHotLimit
		, nBucket, power_set.size()
		, power_set.size()/(nStateNumComplete+0.0)
		, power_set.size()/(nStateNumHotLimit+0.0)
		);
}

// multi thread, the whole speed is still slower slower than
// single thread, even running many threads
template<class DFA> // typename
DenseDFA_DynDFA_320::state_id_t
DenseDFA_DynDFA_320::
dyn_state_move_mt(const DFA* nfa, state_id_t dcurr, auchar_t ch, DynDFA_Context* ctx)
{
	if (states[dcurr].has_dnext.is0(ch)) {
		return nil_state;
	}
	state_id_t dnext = this->state_move(dcurr, ch);
	if (nil_state != dnext)
		return dnext;

	// cache missed, discovering dnext...

	size_t sub_beg = this->states[dcurr + 0].ps_indx;
	size_t sub_end = this->states[dcurr + 1].ps_indx;
	assert(sub_beg < sub_end);
	ctx->next_set.resize_no_init(sub_end - sub_beg);
	state_id_t* next_beg = ctx->next_set.data();
	size_t next_len = 0;
	for (size_t j = sub_beg; j < sub_end; ++j) {
		state_id_t curr = power_set[j];
		state_id_t next = nfa->state_move(curr, ch);
		if (DFA::nil_state != next)
			next_beg[next_len++] = next;
	}
	if (0 == next_len) { // ctx->next_set is empty
		// just in reader lock, not in writer lock
		// if data race occurred, it is harmless
		states[dcurr].has_dnext.set0(ch);
		return nil_state;
	}
	std::sort(next_beg, next_beg + next_len);
	next_len = std::unique(next_beg, next_beg + next_len) - next_beg;
//	ctx->next_set.risk_set_size(next_len);
	size_t hash = subset_hash(next_beg, next_len);
	size_t ihit = hash % nBucket;
	dnext = pBucket[ihit];
	while (nil_state != dnext) {
		if (subset_equal(next_beg, next_len, dnext)) {
			// the subset dnext has existed, return it
			return dnext;
		}
		dnext = states[dnext].ps_link;
	}
	if (!ctx->lock->upgrade_to_writer()) {
	   	// didn't immediately get writer lock
	   	// It did temporarily unlock and retry, so need retry find
	//	fprintf(stderr, "upgrade_to_writer: temporarily unlocked\n");
		ihit = hash % nBucket; // nBucket may be enlarged
		dnext = pBucket[ihit];
		while (nil_state != dnext) {
			if (subset_equal(next_beg, next_len, dnext)) {
				// the subset dnext has existed, return it
				return dnext;
			}
			dnext = states[dnext].ps_link;
		}
	}
	if (dcurr < nStateNumHotLimit && dyn_mem_size() > maxmem) {
		if (num_readers.load(std::memory_order_relaxed) == 1)
			dyn_discard_deep_cache();
	}
	power_set.append(next_beg, next_len);
	dnext = states.size()-1;
	bool should_realloc = states.size() == states.capacity();
	states.push_back();
	states[dnext+0].set_cached_hash(hash);
	states[dnext+1].ps_indx = power_set.size();
	if (!should_realloc) {
	   	// insert to hash table
		states[dnext].ps_link = pBucket[ihit];
		pBucket[ihit] = state_id_t(dnext);
	}
   	else {
		hash_resize();
	//	fprintf(stderr, "hash_resize in dyn_state_move_mt, states=%zd nBucket=%u\n", states.size(), nBucket);
	}
	add_move_checked(dcurr, dnext, ch);
	return dnext;
}

// single thread
template<class DFA> // typename
DenseDFA_DynDFA_320::state_id_t
DenseDFA_DynDFA_320::
dyn_state_move_st(const DFA* nfa, state_id_t dcurr, auchar_t ch) {
	if (states[dcurr].has_dnext.is0(ch)) {
		return nil_state;
	}
	state_id_t dnext = this->state_move(dcurr, ch);
	if (nil_state != dnext)
		return dnext;

	size_t oldsize = this->power_set.size();
	size_t sub_beg = this->states[dcurr + 0].ps_indx;
	size_t sub_end = this->states[dcurr + 1].ps_indx;
	assert(sub_beg < sub_end);
	this->power_set.resize_no_init(power_set.size() + sub_end - sub_beg);
	state_id_t* power_data = power_set.begin();
	size_t newsize = oldsize;
	for (size_t j = sub_beg; j < sub_end; ++j) {
		state_id_t curr = power_set[j];
		state_id_t next = nfa->state_move(curr, ch);
		if (DFA::nil_state != next)
			power_data[newsize++] = next;
	}
	power_set.risk_set_size(newsize);
	if (oldsize == newsize) {
		states[dcurr].has_dnext.set0(ch);
		return nil_state;
	}
	else {
		dnext = add_next_set_single_thread(oldsize, dcurr);
		add_move_checked(dcurr, dnext, ch);
		return dnext;
	}
}

// This function should always success
// cache discarding will be delayed
//typename
DenseDFA_DynDFA_320::state_id_t
DenseDFA_DynDFA_320::add_next_set_single_thread(size_t oldsize, state_id_t dcurr) {
	// this is a specialized hash table find or insert
	assert(states.back().ps_indx == oldsize);
	state_id_t* next_beg = power_set.begin() + oldsize;
	  std::sort(next_beg , power_set.end());
	state_id_t* next_end = std::unique(next_beg, power_set.end());
	size_t next_len = next_end - next_beg;
	size_t hash = subset_hash(next_beg, next_len);
	size_t ihit = hash % nBucket;
	state_id_t dstate = pBucket[ihit];
	while (nil_state != dstate) {
		if (subset_equal(next_beg, next_len, dstate)) {
			// the subset dstate has existed, return it
			power_set.resize(oldsize);
			return dstate;
		}
		dstate = states[dstate].ps_link;
	}
	// the subset is not existed, add it
	power_set.trim(next_end);
	if (dcurr < nStateNumHotLimit && dyn_mem_size() > maxmem) {
		valvec<state_id_t> tmp(next_beg, next_end);
		dyn_discard_deep_cache();
		power_set.append(tmp);
	}
	size_t dnext = states.size()-1;
	bool should_realloc = states.size() == states.capacity();
	states.push_back();
	states[dnext+0].set_cached_hash(hash);
	states[dnext+1].ps_indx = power_set.size();
	if (!should_realloc) {
	   	// insert to hash table
		states[dnext].ps_link = pBucket[ihit];
		pBucket[ihit] = state_id_t(dnext);
	}
   	else {
		hash_resize();
	//	fprintf(stderr, "hash_resize in add_next_set_single_thread, states=%zd nBucket=%u\n", states.size(), nBucket);
	}
	return state_id_t(dnext);
}

void DenseDFA_DynDFA_320::hash_resize() {
	assert(NULL != pBucket);
	size_t uBucket = __hsm_stl_next_prime(states.capacity() * 3/2);
	if (nBucket != uBucket) {
		size_t bucket_mem_size = sizeof(state_id_t) * uBucket;
		::free(pBucket);
		nBucket = uBucket;
		pBucket = (state_id_t*)malloc(bucket_mem_size);
		if (NULL == pBucket) {
			THROW_STD(runtime_error, "malloc(%zd)", bucket_mem_size);
		}
	}
	std::fill_n(pBucket, nBucket, nil_state+0);
#ifndef NDEBUG
	const state_id_t* power_data = power_set.data();
#endif
	auto pstates = &states[0];
	auto qBucket = pBucket; // compiler will optimize local var
	size_t n = states.size() - 1;
	for(size_t i = 0; i < n; ++i) {
		size_t hash = pstates[i].get_cached_hash();
		size_t ihit = hash % uBucket;
	#ifndef NDEBUG
		size_t ibeg = pstates[i+0].ps_indx;
		size_t iend = pstates[i+1].ps_indx;
		size_t ilen = iend - ibeg;
		assert(subset_hash(power_data + ibeg, iend - ibeg) == hash);
	#endif
		pstates[i].ps_link = qBucket[ihit];
		qBucket[ihit] = state_id_t(i);
	}
}

template<class DFA>
class MultiRegexFullMatchDynamicDfaTmpl : public MultiRegexFullMatch {
public:
	typedef typename DFA::state_id_t state_id_t;

	template<class TR>
	size_t tpl_match(auchar_t delim, fstring text, const TR& tr) {
		if (1)
			return tpl_match_st(delim, text, tr);
		else
			return tpl_match_mt(delim, text, tr);
	}

	// multi-thread matching
	template<class TR>
	size_t tpl_match_mt(auchar_t delim, fstring text, const TR& tr) {
		if (text.empty())
			return 0;
		if (delim == 256)
			delim =  257;
	//	printf("delim=0x%X text=%.*s\n", delim, text.ilen(), text.p);
		DynDFA_Context* ctx = this->m_ctx;
		const DFA* nfa = static_cast<const DFA*>(this->m_dfa);
		ctx->stack.erase_all();
		this->erase_all();
		DenseDFA_DynDFA_320* ddfa = m_dyn;
		assert(ddfa != NULL);
		assert(ddfa->power_set.size() >= 2);
		ddfa->num_readers.fetch_add(1, std::memory_order_relaxed);
	  {
		rw_mutex_t::scoped_lock lock(ddfa->rw_mutex, false); // reader lock
		ctx->lock = &lock;
		state_id_t dcurr = initial_state;
		for (size_t i = 0; ; ++i) {
			state_id_t valroot = ddfa->dyn_state_move_mt(nfa, dcurr, delim, ctx);
			if (ddfa->nil_state != valroot) {
				size_t sub_beg = ddfa->ps_index(valroot + 0);
				size_t sub_end = ddfa->ps_index(valroot + 1);
				for (size_t k = sub_end; k > sub_beg; ) {
					--k;
					state_id_t subroot = ddfa->power_set[k];
					DynDFA_Context::MatchEntry e;
					e.curr_pos = i;
					e.skip_len = 0;
					e.state_id = subroot;
					ctx->stack.push_back(e);
				}
			}
			if (text.size() == i)
				break;
			byte_t ch = tr(text[i]);
			state_id_t dnext = ddfa->dyn_state_move_mt(nfa, dcurr, ch, ctx);
			if (ddfa->nil_state == dnext)
				break;
			dcurr = dnext;
		}
		ddfa->num_readers.fetch_sub(1, std::memory_order_relaxed);
	  }
	  return finish_match(nfa, ctx);
	}

	// single-thread matching is faster in all tested cases
	template<class TR>
	size_t tpl_match_st(auchar_t delim, fstring text, const TR& tr) {
		if (text.empty())
			return 0;
		if (delim == 256)
			delim =  257;
	//	printf("delim=0x%X text=%.*s\n", delim, text.ilen(), text.p);
		DynDFA_Context* ctx = this->m_ctx;
		const DFA* nfa = static_cast<const DFA*>(m_dfa);
		ctx->stack.erase_all();
		this->erase_all();
		DenseDFA_DynDFA_320* ddfa = m_dyn;
		assert(ddfa != NULL);
		assert(ddfa->power_set.size() >= 2);
		state_id_t dcurr = initial_state;
		for (size_t i = 0; ; ++i) {
			state_id_t valroot = ddfa->dyn_state_move_st(nfa, dcurr, delim);
			if (ddfa->nil_state != valroot) {
				size_t sub_beg = ddfa->ps_index(valroot + 0);
				size_t sub_end = ddfa->ps_index(valroot + 1);
				for (size_t k = sub_end; k > sub_beg; ) {
					--k;
					state_id_t subroot = ddfa->power_set[k];
					DynDFA_Context::MatchEntry e;
					e.curr_pos = i;
					e.skip_len = 0;
					e.state_id = subroot;
					ctx->stack.push_back(e);
				}
			}
			if (text.size() == i)
				break;
			byte_t ch = tr(text[i]);
			state_id_t dnext = ddfa->dyn_state_move_st(nfa, dcurr, ch);
			if (ddfa->nil_state == dnext)
				break;
			dcurr = dnext;
		}
		return finish_match(nfa, ctx);
	}

	size_t finish_match(const DFA* nfa, DynDFA_Context* ctx) {
		if (ctx->stack.empty())
			return 0;
		int max_match = ctx->stack.back().curr_pos;
		for (size_t j = ctx->stack.size(); j > 0; ) {
			--j;
			DynDFA_Context::MatchEntry e = ctx->stack[j];
			assert(e.curr_pos <= max_match);
			if (e.curr_pos != max_match) {
				assert(!this->empty());
				break;
			}
			auto on_suffix = [&](size_t, fstring val, size_t) {
				uint32_t regex_id;
				assert(val.size() == sizeof(regex_id));
				memcpy(&regex_id, val.data(), sizeof(regex_id));
				this->push_back(regex_id);
			};
			const int Bufsize = 8; // >= sizeof(int)
			ForEachWord_DFS<DFA, decltype(on_suffix), Bufsize> vis(nfa, on_suffix);
			vis.start(e.state_id, 0);
			assert(!this->empty());
		}
		return size_t(max_match);
	}

	static size_t
	match_func1(MultiRegexFullMatch* xThis, fstring text) {
		auto self = static_cast<MultiRegexFullMatchDynamicDfaTmpl*>(xThis);
		auchar_t delim = self->m_dfa->kv_delim();
		assert(delim < DFA::sigma);
		return self->tpl_match(delim, text, IdentityTranslator());
	}
	static size_t
	match_func2(MultiRegexFullMatch* xThis, fstring text, const ByteTR& tr) {
		auto self = static_cast<MultiRegexFullMatchDynamicDfaTmpl*>(xThis);
		auchar_t delim = self->m_dfa->kv_delim();
		assert(delim < DFA::sigma);
		return self->tpl_match(delim, text, tr);
	}
};

MultiRegexFullMatch::MultiRegexFullMatch(const DFA_Interface* dfa) {
	if (NULL == dfa) {
		THROW_STD(invalid_argument, "dfa is NULL");
	}
	if (dfa->v_state_move(initial_state, 258) != dfa->v_nil_state()) {
		DO_SET_func_ptr(MultiRegexFullMatchDynamicDfa);
		m_dyn = new DenseDFA_DynDFA_320;
		m_dyn->dyn_init(dfa);
	} else {
		m_dyn = NULL;
		DO_SET_func_ptr(MultiRegexFullMatch);
	}
	m_dfa = dfa;
	m_ctx = new DynDFA_Context;
}

MultiRegexFullMatch::MultiRegexFullMatch(const MultiRegexFullMatch& y)
  : valvec<int>(y)
  , m_dfa(y.m_dfa)
  , m_dyn(y.m_dyn)
  , pf_match1(y.pf_match1)
  , pf_match2(y.pf_match2)
{
	if (m_dyn)
		m_dyn->refcnt++;
	m_ctx = new DynDFA_Context;
}

MultiRegexFullMatch::~MultiRegexFullMatch() {
	if (m_dyn && 0 == --m_dyn->refcnt)
		delete m_dyn;
	delete m_ctx;
}

void MultiRegexFullMatch::set_maxmem(size_t maxmem) {
	if (m_dyn)
		m_dyn->maxmem = maxmem;
}

size_t MultiRegexFullMatch::get_maxmem() const {
	if (m_dyn)
		return m_dyn->maxmem;
	return 0;
}

void MultiRegexFullMatch::warm_up() {
	if (m_dyn)
		m_dyn->warm_up(m_dfa);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
template<class DFA>
class MultiRegexSubmatchTwoPassTmpl : public MultiRegexSubmatch {
public:
	// RootState(regex_id) = regex_id + 1
	template<class TR>
	size_t dmatch_func2(auchar_t delim, fstring text, const TR& tr) {
		const DFA* dfa = static_cast<const DFA*>(this->dfa);
		assert(delim < DFA::sigma);
		assert(NULL != this->m_first_pass);
		size_t match_len;
	//	printf("twopass: delim=%d dyn_dfa=%p\n", delim, m_first_pass->get_dyn_dfa());
		if (m_first_pass->get_dyn_dfa())
			match_len = static_cast<MultiRegexFullMatchDynamicDfaTmpl<DFA>*>
				(m_first_pass)->tpl_match(delim, text, tr);
		else
			match_len = static_cast<MultiRegexFullMatchTmpl<DFA>*>
				(m_first_pass)->dmatch_func(delim, text, tr);
		this->m_fullmatch_regex.swap(*this->m_first_pass);
		for (int regex_id : this->m_fullmatch_regex) {
			int* pcap = cap_pos_ptr[regex_id];
			auto on_match = [&](size_t klen, size_t, fstring val) {
				assert(val.size() == sizeof(CaptureBinVal));
				CaptureBinVal ci;
				memcpy(&ci, val.data(), sizeof(ci));
				pcap[ci.capture_id] = klen;
				assert(ci.regex_id == regex_id);
				assert(pcap + ci.capture_id < this->cap_pos_ptr[regex_id+1]);
			//	printf("set_capture: regex_id=%d cap_id=%d pos=%d\n", ci.regex_id, ci.capture_id, klen);
			};
		//	printf("twopass: regex_id=%d\n", regex_id);
			this->reset(regex_id);
			int root;
		   	if (this->cap_pos_ptr.size() == 2) // only 1 regex
				root = initial_state; // regex[0] == regex[all]
			else
				root = regex_id + 1;
			dfa->tpl_match_key(root, delim, text, on_match, tr);
		}
		return match_len;
	}
	static size_t
	match_func1(MultiRegexSubmatch* xThis, fstring text) {
		auto self = static_cast<MultiRegexSubmatchTwoPassTmpl*>(xThis);
		auchar_t delim = self->dfa->kv_delim();
		assert(delim < DFA::sigma);
		assert(NULL != self->m_first_pass);
		return self->dmatch_func2(delim, text, IdentityTranslator());
	}
	static size_t
	match_func2(MultiRegexSubmatch* xThis, fstring text, const ByteTR& tr) {
		auto self = static_cast<MultiRegexSubmatchTwoPassTmpl*>(xThis);
		auchar_t delim = self->dfa->kv_delim();
		assert(delim < DFA::sigma);
		assert(NULL != self->m_first_pass);
		return self->dmatch_func2(delim, text, tr);
	}
};

void MultiRegexSubmatch::set_func(const DFA_Interface* dfa, int scanpass) {
	assert(NULL != dfa);
	if (NULL == this->dfa) {
		THROW_STD(invalid_argument, "dfa is NULL");
	}
	switch (scanpass) {
	  default:
		THROW_STD(invalid_argument, "scanpass=%d", scanpass);
		break;
	  case 1:
		m_first_pass = NULL;
		DO_SET_func_ptr(MultiRegexSubmatch);
		break;
	  case 2:
		m_first_pass = new MultiRegexFullMatch(dfa);
		DO_SET_func_ptr(MultiRegexSubmatchTwoPass);
		break;
	}
}

void MultiRegexSubmatch::warm_up() {
	if (m_first_pass && m_first_pass->get_dyn_dfa())
		m_first_pass->warm_up();
}

