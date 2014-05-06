#ifndef __febird_automata_dfa_edit_distance_hpp__
#define __febird_automata_dfa_edit_distance_hpp__

#if defined(__GNUC__)
	#if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 7
	#elif defined(__clang__)
	#else
		#error "Requires GCC-4.7+"
	#endif
#endif

//#include <boost/current_function.hpp> 
#include <febird/hash_strmap.hpp>
#include <febird/util/fstrvec.hpp>

template<class Au>
class DFA_EditDistance {
	const Au* const au;

public:
	DFA_EditDistance(const Au* au1) : au(au1) {}

	typedef typename Au::state_id_t state_id_t;
	typedef typename Au::transition_t transition_t;

    static const state_id_t max_state = Au::max_state;
    static const state_id_t nil_state = Au::nil_state;

	static size_t // s1 is input string
	expand_edit_distance(size_t beg1, size_t len1, const byte_t* s1
					   , size_t beg2, size_t len2, const byte_t* s2
					   , valvec<size_t>& work, size_t colsize)
   	{
		assert((beg2 + len2) <= colsize);
		assert((beg1 + len1)  * colsize <= work.size());
		assert(beg1 >= 1);
		assert(beg2 >= 1);
		for(size_t i = beg1; i < beg1 + len1; ++i) {
			for(size_t j = beg2; j < beg2 + len2; ++j) {
				size_t d0 = work.get_2d(colsize, i-1, j-1);
				if (s1[i-beg1] == s2[j-beg2]) {
					work.get_2d(colsize, i, j) = d0;
				}/*
				else if (beg1 > 1 && // twiddle
						 s1[i-beg1-1] == s2[j-beg2-0] && 
						 s1[i-beg1-0] == s2[j-beg2-1]) {
					work.get_2d(colsize, i, j) = d0;
				}*/
			   	else {
					size_t d1 = work.get_2d(colsize, i-1, j-0);
					size_t d2 = work.get_2d(colsize, i-0, j-1);
					size_t d3 = std::min(d0, std::min(d1, d2)) + 1;
					work.get_2d(colsize, i, j) = d3;
				}
			}
		}
		return work.get_2d(colsize, beg1+len1-1, beg1+len2-1);
	}

	size_t match_edit_distance(state_id_t RootState,
						       fstring str, valvec<hash_strmap<unsigned> >& matched) const {
		assert(0 == au->num_zpath_states()); // now, must not path_zip
		assert(matched.size() >= 1);
		const size_t max_edit_distance = matched.size() - 1;
		for (size_t i = 0; i < matched.size(); ++i) {
			matched[i].erase_all();
		}
		struct Elem {
			state_id_t state;
			size_t     edit_distance;
			size_t     input_pos;
		};
		size_t const size_t_max = size_t(~size_t(0));
		valvec<CharTarget<state_id_t> > children;
		valvec<size_t> work;
		valvec<Elem> q1, q2;
		fstrvec      s1, s2;
		size_t n_try = 0;
		q2.push_back({RootState, 0, 0});
		s2.emplace_back("", 0);
		while (!q2.empty()) { // This is a FAST DFS search
			size_t old_top = q2.size()-1;
			const Elem old = q2[old_top]; // must by value
			if (size_t_max == old.edit_distance) {
				q2.pop_back();
				s2.pop_back();
				continue;
			}
			assert(old.edit_distance <= max_edit_distance);
			assert(old.input_pos <= str.size());
			if (str.size() == old.input_pos && au->is_term(old.state)) {
				matched[old.edit_distance][s2[old_top]]++;
			}
			children.resize_no_init(0);
			au->for_each_move(old.state,
			  [&](state_id_t, state_id_t next, byte_t c2) {
				children.emplace_back(c2, next);
			});
			if (au->is_pzip(old.state)) {
			#if 0
				const byte_t* p1 = str.p + old.input_pos;
					  size_t  n1 = str.n - old.input_pos;
				const byte_t* p2 = au->get_zpath_data(old.state);
				const size_t  n2 = *p2++;
				const size_t  colsize = n2 + 2;
				n1 = std::min(n1, n2+1 + max_edit_distance - old.edit_distance);
				work.resize_no_init((n1 + 1) * colsize);
				for (size_t j = 0; j < n1 + 1; ++j)	work.get_2d(colsize, j, 0) = j;
				for (size_t j = 1; j < n2 + 2; ++j) work.get_2d(colsize, 0, j) = j;
				expand_edit_distance(1, n1, p1, 1, n2, p2, work, colsize);
				for (auto ct : children) {
					byte_t c2 = (byte_t)ct.ch;
					expand_edit_distance(1, n1, p1, n2+1, 1, &c2, work, colsize);
					printf("str1=[0x%zX]:%-30.*s str2=[0x%zX]: %.*s%c : edit_distance:"
							, n1, int(n1), p1, n2+1, int(n2), p2, c2);
					for(size_t i = n1; i > 0; --i) {
						size_t d = work.get_2d(colsize, i, n2+1);
						printf(" %zX", d);
						if (d + old.edit_distance <= max_edit_distance) {
							assert(old.input_pos + i <= str.size());
							q2.push_back({ct.target, old.edit_distance + d, old.input_pos + i-1});
							s2.push_back(s2[old_top]);
							s2.back_append((const char*)p2, n2); // the zpath
							s2.back_append(c2);
						}
					}
					printf("\n");
					for(size_t i = 0; i <= n1; ++i) {
						for(size_t j = 0; j <= n2+1; ++j)
							printf(" %zX", work.get_2d(colsize, i, j));
						printf("\n");
					}
				}
				n_try += (n1 + 1) * (n2 + 1);
			#else
				assert(0);
			#endif
			}
			else {
				size_t ed = old.edit_distance;
				size_t ip = old.input_pos;
				if (ed < max_edit_distance) {
					for (auto ct : children) {
						byte_t c2 = ct.ch;
						s2.push_back(s2[old_top]);
						s2.back_append(c2); // insert c2
						q2.push_back({ct.target, ed+1, ip});
					}
					n_try += children.size();
				}
				if (ip < str.size() && ed <= max_edit_distance) {
					byte_t c1 = str.p[ip];
					for (auto ct : children) {
						byte_t c2 = ct.ch;
						if (c1 == c2) { // match
							s2.push_back(s2[old_top]);
							s2.back_append(c2);
							q2.push_back({ct.target, ed+0, ip+1});
						}
						else if (ed < max_edit_distance) {
							s2.push_back(s2[old_top]);
							s2.back_append(c2); // replace
							q2.push_back({ct.target, ed+1, ip+1});
						}
					}
					n_try += children.size();
				}
				while (ed < max_edit_distance && ip < str.size()) {
					// delete str[ip]
					++ip, ++ed;
					s2.push_back(s2[old_top]);
					q2.push_back({old.state, ed, ip});
					n_try += children.size();
				}
			}
			q2[old_top].edit_distance = size_t_max; // mark as visted
		}
		return n_try;
	}

public:

	/// @returns pathes tried
	/// allow:
	///   1 insertion
	///   1 deletion
	///   1 replace
	size_t match_edit_distance(const fstring str, valvec<hash_strmap<unsigned> >& matched) const {
		return match_edit_distance(initial_state, str, matched);
	}
	size_t match_edit_distance1(state_id_t RootState,
						        fstring str, valvec<hash_strmap<unsigned> >& matched) const {
		assert(0 == au->num_zpath_states()); // now, must not path_zip
		assert(matched.size() >= 1);
		const unsigned max_edit_distance = matched.size() - 1;
		for (size_t i = 0; i < matched.size(); ++i) {
			matched[i].erase_all();
		}
		struct Elem {
			state_id_t   state;
			unsigned     edit_distance;
		};
		valvec<Elem> q1, q2;
		fstrvec      s1, s2;
		size_t n_try = 0;
		q1.push_back({RootState, 0});
		s1.emplace_back("", 0);
		if (max_edit_distance > 0) { // insert at beginning:
			au->for_each_move(RootState,
				[&](state_id_t, state_id_t next3, byte_t c3) {
					n_try++;
					q1.push_back({next3, 1});
					s1.emplace_back((char*)&c3, 1); // insert c3
				});
		}
		for(size_t i = 0; i < str.size(); ++i) {
			byte_t c1 = (byte_t)str.p[i];
			q2.resize(0);
			s2.resize(0);
			for(size_t j = 0; j < q1.size(); ++j) {
				const Elem old = q1[j];
				au->for_each_move(old.state,
					[&](state_id_t, state_id_t next, byte_t c2) {
						n_try++;
						if (old.edit_distance < max_edit_distance) {
							q2.push_back({old.state, old.edit_distance + 1});
							s2.push_back(s1[j]); // match delete c1 from input
							if (c1 == c2) { // insert a char(c3)
								au->for_each_move(next,
									[&](state_id_t, state_id_t next3, byte_t c3) {
										Elem e;
										e.state = next3;
										e.edit_distance = old.edit_distance + 1;
										q2.push_back(e);
										s2.push_back(s1[j]);
										s2.back_append(c2);
										s2.back_append(c3); // insert c3
									});
							}
						}
						if (c2 == c1 || old.edit_distance < max_edit_distance) {
							Elem e;
							e.state = next;
							e.edit_distance = old.edit_distance + (c2 == c1 ? 0 : 1);
							q2.push_back(e);
							s2.push_back(s1[j]);
							s2.back_append(c2);
						}
					});
			}
			if (q2.empty()) {
				return n_try;
			}
			q1.swap(q2);
			s1.swap(s2);
		}
		assert(q1.size() == s1.size());
		for (size_t j = 0; j < q1.size(); ++j) {
			const Elem& e = q1[j];
			assert(e.edit_distance <= max_edit_distance);
			if (au->is_term(e.state)) {
				fstring word = s1[j];
				matched[e.edit_distance][word]++;
			}
		}
		return n_try;
	}
};

template<class Au> DFA_EditDistance<Au> dfa_edit_distance(      Au* au) { return  au; }
template<class Au> DFA_EditDistance<Au> dfa_edit_distance(const Au* au) { return  au; }
template<class Au> DFA_EditDistance<Au> dfa_edit_distance(const Au& au) { return &au; }

#endif // __febird_automata_dfa_edit_distance_hpp__

