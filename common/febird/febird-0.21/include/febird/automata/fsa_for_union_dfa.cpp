#include "fsa_for_union_dfa.hpp"
#include <febird/util/throw.hpp>

FSA_ForUnionDFA::FSA_ForUnionDFA() {
	m_total_states = 0;
	m_is_owner = false;
}

FSA_ForUnionDFA::~FSA_ForUnionDFA() {
	clear();
}

void FSA_ForUnionDFA::clear() {
	if (m_is_owner) {
		for (size_t i = 0; i < m_dfas.size(); ++i)
			delete const_cast<DFA_Interface*>(m_dfas[i]);
	}
	m_index.clear();
	m_total_states = 0;
}

void FSA_ForUnionDFA::add_dfa(const DFA_Interface* dfa) {
	m_dfas.push_back(dfa);
}

void FSA_ForUnionDFA::get_all_root(valvec<state_id_t>* roots) const {
	roots->resize_no_init(m_index.size());
	std::copy(m_index.begin(), m_index.end(), roots->begin());
}

void FSA_ForUnionDFA::compile() {
	m_index.resize_no_init(m_dfas.size());
	size_t root = 1; // 0 is reserved for initial_state
	m_max_sigma = 256;
	m_free_states = 0;
	for (size_t i = 0; i < m_index.size(); ++i) {
		auto dfa = m_dfas[i];
		m_index[i] = root;
		root += dfa->v_total_states();
		if (root >= max_state) {
			THROW_STD(out_of_range, "root=%zd max_state=%zd"
				, root, (size_t)max_state);
		}
		m_max_sigma = std::max(m_max_sigma, dfa->get_sigma());
	//	m_free_states += dfa->v_num_free_states();
	}
	m_total_states = root;
	m_stack_dests.reserve(2 * m_max_sigma);
	m_stack_moves.reserve(2 * m_max_sigma);
}

bool FSA_ForUnionDFA::is_final(size_t s) const {
	assert(s < m_total_states);
	if (initial_state == s) {
		return false;
	}
	auto i = std::upper_bound(m_index.begin(), m_index.end(), s) - 1;
	assert(i >= m_index.begin());
	auto root = *i;
	auto dfa = m_dfas[i - m_index.begin()];
	assert(s - root < dfa->v_total_states());
	return dfa->v_is_term(s - root);
}
void FSA_ForUnionDFA::get_epsilon_targets(size_t s, valvec<state_id_t>* children) const {
	assert(s < m_total_states);
	children->erase_all();
	if (initial_state == s) {
		for (auto root : m_index)
			children->push_back(root);
	}
}
void FSA_ForUnionDFA::get_non_epsilon_targets(size_t s, valvec<CharTarget<size_t> >* children)
const {
	assert(s < m_total_states);
	children->erase_all();
	if (initial_state == s) {
		return;
	}
	auto i = std::upper_bound(m_index.begin(), m_index.end(), s) - 1;
	assert(i >= m_index.begin());
	size_t root = *i;
	auto dfa = m_dfas[i - m_index.begin()];
	assert(s - root < dfa->v_total_states());
	dfa->get_all_move(s - root, children);
	auto beg = children->data();
	size_t len = children->size();
	for (size_t i = 0; i < len; ++i) {
		beg[i].target += root;
	}
}

// DFA operations: the FSA is DFA except initial_state
// the DFA has multiple roots, and can be minimized by Hopcroft/adfa_minimize
//
size_t FSA_ForUnionDFA::get_all_dest(size_t s, size_t* dests) const {
	auto i = std::upper_bound(m_index.begin(), m_index.end(), s) - 1;
	assert(i >= m_index.begin());
	size_t root = *i;
	auto dfa = m_dfas[i - m_index.begin()];
	assert(s - root < dfa->v_total_states());
	size_t cnt = dfa->get_all_dest(s - root, dests);
	for (size_t j = 0; j < cnt; ++j) dests[j] += root;
	return cnt;
}
size_t FSA_ForUnionDFA::get_all_move(size_t s, CharTarget<size_t>* moves) const {
	auto i = std::upper_bound(m_index.begin(), m_index.end(), s) - 1;
	assert(i >= m_index.begin());
	size_t root = *i;
	auto dfa = m_dfas[i - m_index.begin()];
	assert(s - root < dfa->v_total_states());
	size_t cnt = dfa->get_all_move(s - root, moves);
	for (size_t j = 0; j < cnt; ++j) moves[j].target += root;
	return cnt;
}

size_t FSA_ForUnionDFA::risk_get_all_dest(size_t s, size_t* pcnt) const {
	auto i = std::upper_bound(m_index.begin(), m_index.end(), s) - 1;
	assert(i >= m_index.begin());
	size_t root = *i;
	auto dfa = m_dfas[i - m_index.begin()];
	assert(s - root < dfa->v_total_states());
	size_t oldsize = m_stack_dests.size();
	m_stack_dests.resize_no_init(oldsize + m_max_sigma);
	size_t cnt = dfa->get_all_dest(s - root, m_stack_dests.begin() + oldsize);
	for (size_t i = oldsize; i < oldsize + cnt; ++i) {
		m_stack_dests[i] += root;
	}
	m_stack_dests.risk_set_size(oldsize + cnt);
	*pcnt = cnt;
	return oldsize;
}

size_t FSA_ForUnionDFA::risk_get_all_move(size_t s, size_t* pcnt) const {
	auto i = std::upper_bound(m_index.begin(), m_index.end(), s) - 1;
	assert(i >= m_index.begin());
	size_t root = *i;
	auto dfa = m_dfas[i - m_index.begin()];
	assert(s - root < dfa->v_total_states());
	size_t oldsize = m_stack_moves.size();
	m_stack_moves.resize_no_init(oldsize + m_max_sigma);
	size_t cnt = dfa->get_all_move(s - root, m_stack_moves.begin() + oldsize);
	for (size_t i = oldsize; i < oldsize + cnt; ++i) {
		m_stack_moves[i].target += root;
	}
	m_stack_moves.risk_set_size(oldsize + cnt);
	*pcnt = cnt;
	return oldsize;
}

bool FSA_ForUnionDFA::is_dag() const {
	for (auto dfa : m_dfas)
		if (!dfa->is_dag())
			return false;
	return true;
}

bool FSA_ForUnionDFA::is_free(size_t s) const {
#if 0
	auto i = std::upper_bound(m_index.begin(), m_index.end(), s) - 1;
	assert(i >= m_index.begin());
	size_t root = *i;
	auto dfa = m_dfas[i - m_index.begin()];
	assert(s - root < dfa->v_total_states());
	return dfa->v_is_free(s - root);
#else
	return false;
#endif
}

size_t FSA_ForUnionDFA::adfa_hash_hash(const state_id_t* Min, size_t s) const {
	assert(s != initial_state);
	assert(s < m_total_states);
	auto i = std::upper_bound(m_index.begin(), m_index.end(), s) - 1;
	assert(i >= m_index.begin());
	auto root = *i;
	auto dfa = m_dfas[i - m_index.begin()];
	assert(s - root < dfa->v_total_states());
	size_t h = dfa->v_is_term(s - root) ? 1 : 0;
	size_t oldsize = m_stack_moves.size();
	m_stack_moves.resize_no_init(oldsize + m_max_sigma);
	CharTarget<size_t>* moves = m_stack_moves.data() + oldsize;
	size_t cnt = dfa->get_all_move(s - root, moves);
	for (size_t i = oldsize; i < oldsize + cnt; ++i) {
		h = h * 17 + Min[moves[i].target + root];
		h = h * 17 +     moves[i].ch;
	}
	m_stack_moves.risk_set_size(oldsize);
	return h;
}

bool
FSA_ForUnionDFA::adfa_hash_equal(const state_id_t* Min, size_t x, size_t y)
const {
	assert(x != initial_state);
	assert(y != initial_state);
	assert(x < m_total_states);
	assert(y < m_total_states);
	const DFA_Interface *dfa1, *dfa2;
	size_t root1, root2;
  {
   	auto i = std::upper_bound(m_index.begin(), m_index.end(), x) - 1;
	assert(i >= m_index.begin());
	dfa1 = m_dfas[i - m_index.begin()];
	root1 = *i;
	assert(x - root1 < dfa1->v_total_states());
  	i = std::upper_bound(m_index.begin(), m_index.end(), y) - 1;
	assert(i >= m_index.begin());
	dfa2 = m_dfas[i - m_index.begin()];
	root2 = *i;
	assert(y - root2 < dfa2->v_total_states());
  }
	if (dfa1->v_is_term(x - root1) != dfa2->v_is_term(y - root2))
		return false;
	auto oldsize = m_stack_moves.size();
	m_stack_moves.resize_no_init(oldsize + 2*m_max_sigma);
	auto xmoves = m_stack_moves.data() + oldsize;
	auto xcnt = dfa1->get_all_move(x - root1, xmoves);
	auto ymoves = xmoves + xcnt;
	auto ycnt = dfa2->get_all_move(y - root2, ymoves);
	bool ret = false;
	if (xcnt == ycnt) {
		for (size_t i = 0; i < xcnt; ++i) {
			if (xmoves[i].ch != ymoves[i].ch)
				goto Done;
			size_t tx = xmoves[i].target + root1;
			size_t ty = ymoves[i].target + root2;
			if (tx != ty && Min[tx] != Min[ty])
				goto Done;
		}
		ret = true;
	}
Done:
	m_stack_moves.risk_set_size(oldsize);
	return ret;
}


