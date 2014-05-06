#ifndef __febird_automata_dfa_algo_hpp__
#define __febird_automata_dfa_algo_hpp__

#if defined(__GNUC__)
	#if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 7
	#elif defined(__clang__)
	#else
		#error "Requires GCC-4.7+"
	#endif
#endif

#include <boost/function.hpp>
#include <string>

#include <febird/fstring.hpp>
#include <febird/valvec.hpp>
#include <febird/mempool.hpp>
#include <febird/num_to_str.hpp>
#include <febird/util/throw.hpp>

#include "dfa_interface.hpp"
#include "nfa.hpp"
#include "graph_walker.hpp"
#include "hopcroft.hpp"
#include "onfly_minimize.hpp"

template<class Au, class OnNthWord, int NestBufsize = 128>
struct ForEachWord_DFS : private boost::noncopyable {
	const Au* au;
	char*     beg;
	char*     cur;
	char*     end;
	size_t	  nth;
	OnNthWord on_nth_word;
	char      nest_buf[NestBufsize];

	typedef typename Au::state_id_t state_id_t;
	ForEachWord_DFS(const Au* au1, OnNthWord op)
	 : au(au1), nth(0), on_nth_word(op) {
		beg = nest_buf;
		cur = nest_buf;
		end = nest_buf + NestBufsize;
	}
	~ForEachWord_DFS() {
		if (nest_buf != beg) {
			assert(NULL != beg);
			free(beg);
		}
	}
	void start(state_id_t RootState, size_t SkipRootPrefixLen) {
		nth = 0;
		cur = beg;
		enter(RootState, SkipRootPrefixLen);
		au->template for_each_move<ForEachWord_DFS&>(RootState, *this);
	}
	size_t enter(state_id_t tt, size_t SkipLen) {
		size_t slen = 0;
		if (au->is_pzip(tt)) {
			const byte_t* s2 = au->get_zpath_data(tt);
			slen = *s2++;
			assert(SkipLen <= slen);
			this->append(s2 + SkipLen, slen - SkipLen);
		} else {
			assert(0 == SkipLen);
		}
		if (au->is_term(tt)) {
			fstring fstr(beg, cur);
			on_nth_word(nth, fstr, tt);
			++nth;
		}
		return slen;
	}
	void operator()(state_id_t, state_id_t tt, auchar_t ch) {
		this->push_back(ch);
		const size_t slen = enter(tt, 0) + 1;
		au->template for_each_move<ForEachWord_DFS&>(tt, *this);
		assert(cur >= beg + slen);
		cur -= slen;
	}

private:
	void enlarge(size_t inc_size) {
		size_t capacity = end-beg;
		size_t cur_size = cur-beg;
		size_t required_total_size = cur_size + inc_size;
		while (capacity < required_total_size) {
			capacity *= 2;
			char* q;
			if (nest_buf == beg) {
				assert(cur_size <= NestBufsize);
				q = (char*)malloc(capacity);
				if (NULL == q) throw std::bad_alloc();
				memcpy(q, nest_buf, cur_size);
			}
			else {
				q = (char*)realloc(beg, capacity);
				if (NULL == q) throw std::bad_alloc();
			}
			beg = q;
		}
		cur = beg + cur_size;
		end = beg + capacity;
	}
	void push_back(char ch) {
		assert(cur <= end);
		if (cur < end)
			*cur++ = ch;
		else {
		   	enlarge(1);
			*cur++ = ch;
		}
	}
	void append(const void* p, size_t n) {
		if (cur+n > end) {
			enlarge(n);
			assert(cur+n <= end);
		}
		memcpy(cur, p, n);
		cur += n;
	}
};

struct IdentityTranslator {
	byte_t operator()(byte_t c) const { return c; }
};
struct TableTranslator {
	const byte_t* tr_tab;
	byte_t operator()(byte_t c) const { return tr_tab[c]; }
	TableTranslator(const byte_t* tr_tab1) : tr_tab(tr_tab1) {}
};

template<class InputIter>
class iterator_to_byte_stream : public std::pair<InputIter, InputIter> {
	iterator_to_byte_stream& operator++(int); // disable suffix operator++

public:
	iterator_to_byte_stream(InputIter beg, InputIter end)
	   	: std::pair<InputIter, InputIter>(beg, end) {}

	bool empty() const { return this->first == this->second; }

	typename std::iterator_traits<InputIter>::value_type
	operator*() const {
		assert(this->first != this->second);
		return *this->first;
	}

	iterator_to_byte_stream& operator++() {
		assert(this->first != this->second);
		++this->first;
		return *this;
	}

	typedef InputIter iterator, const_iterator;
	InputIter begin() const { return this->first ; }
	InputIter   end() const { return this->second; }
};

template<class DstDFA, class SrcDFA = DstDFA>
struct WorkingCacheOfCopyFrom {
	valvec<CharTarget<size_t> > allmove;
	valvec<typename DstDFA::state_id_t> idmap;
	valvec<typename SrcDFA::state_id_t> stack;

	WorkingCacheOfCopyFrom(size_t SrcTotalStates) {
		idmap.resize_no_init(SrcTotalStates);
		idmap.fill(0+DstDFA::nil_state);
		stack.reserve(512);
	}
	typename SrcDFA::state_id_t
	copy_from(DstDFA* au, const SrcDFA& au2, size_t SrcRoot = initial_state) {
		return copy_from(au, au->new_state(), au2, SrcRoot);
	}
	typename SrcDFA::state_id_t
	copy_from(DstDFA* au, size_t DstRoot, const SrcDFA& au2, size_t SrcRoot = initial_state) {
		assert(0 == au->num_zpath_states());
		assert(SrcRoot < au2.total_states());
		assert(idmap.size() == au2.total_states());
		idmap[SrcRoot] = DstRoot; // idmap also serve as "color"
		stack.erase_all();
		stack.push_back((typename SrcDFA::state_id_t)SrcRoot);
		allmove.reserve(au2.get_sigma());
		while (!stack.empty()) {
			auto parent = stack.back(); stack.pop_back();
			size_t old_stack_size = stack.size();
			assert(!au2.is_pzip(parent));
			assert(DstDFA::nil_state != idmap[parent]);
			allmove.erase_all();
			au2.for_each_move(parent,
				[&](typename SrcDFA::state_id_t parent,
					typename SrcDFA::state_id_t child, auchar_t c)
			{
				if (idmap[child] == DstDFA::nil_state) {
					idmap[child] = au->new_state();
					stack.push_back(child);
				}
				allmove.emplace_back(c, idmap[child]);
			});
			typename DstDFA::state_id_t dst_parent = idmap[parent];
			au->add_all_move(dst_parent, allmove);
			if (au2.is_term(parent))
				au->set_term_bit(dst_parent);
			std::reverse(stack.begin() + old_stack_size, stack.end());
		}
		return idmap[SrcRoot];
	}
};

#endif // __febird_automata_dfa_algo_hpp__

