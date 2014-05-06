
private:
template<class BaseWalker>
class ZipWalker : public BaseWalker {
public:
	valvec<state_id_t> path; // state_move(path[j], strp[j]) == path[j+1]
	valvec<byte_t>     strp;

	void getOnePath(const MyType* au, const bit_access is_confluence) {
		state_id_t s = this->next();
		assert(!au->is_pzip(s));
		strp.resize0();
		path.resize0();
		path.push_back(s);
		if (au->is_term(s) || au->more_than_one_child(s))
			return;
		auchar_t prev_ch = au->get_single_child_char(s);
		if (prev_ch >= 256)
			return;
		for (state_id_t i = au->get_single_child(s);
			 path.size() < 255 && !is_confluence[i]; ) {
			ASSERT_isNotFree2(au, i);
			assert(!au->is_pzip(i));
			assert(this->color.is0(i));
		// 	this->color.set1(i); // not needed
			strp.push_back(prev_ch);
			path.push_back(i);
			if (au->is_term(i) || au->more_than_one_child(i)) break;
			prev_ch = au->get_single_child_char(i);
			if (prev_ch >= 256) break; // not a byte
			i = au->get_single_child(i);
		// only term state could have null target:
			assert(i < au->total_states());
			ASSERT_isNotFree2(au, i);
		}
		strp.push_back('\0');
		strp.unchecked_pop_back();
	 // state_move(path[j], strp[j]) == path[j+1]:
		assert(path.size() == strp.size() + 1);
	}
};
public:
template<class Zipped>
void path_zip(Zipped& zipped, const char* WalkMethod) const {
	size_t Root = initial_state;
	path_zip(&Root, 1, zipped, WalkMethod);
}
template<class Zipped>
void path_zip(const valvec<size_t>& roots
			, Zipped& zipped, const char* WalkMethod)
const {
	path_zip(roots.data(), roots.size(), zipped, WalkMethod);
}
template<class Zipped>
void path_zip(const size_t* pRoots, size_t nRoots
			, Zipped& zipped, const char* WalkMethod)
const {
	assert(nRoots >= 1);
	simplebitvec is_confluence(total_states(), false);
	{
		valvec<state_id_t> indg;
		compute_indegree(pRoots, nRoots, indg);
		for (size_t i = 0, n = indg.size(); i < n; ++i) {
			if (indg[i] > 1)
				is_confluence.set1(i);
		}
	}
	typedef BFS_GraphWalker<state_id_t>  BFS;
	typedef DFS_GraphWalker<state_id_t>  DFS;
	typedef PFS_GraphWalker<state_id_t>  PFS;
	if (strcasecmp(WalkMethod, "BFS") == 0)
		path_zip_imp<Zipped, BFS>(pRoots, nRoots, is_confluence.bldata(), zipped);
	else if (strcasecmp(WalkMethod, "DFS") == 0)
		path_zip_imp<Zipped, DFS>(pRoots, nRoots, is_confluence.bldata(), zipped);
	else if (strcasecmp(WalkMethod, "PFS") == 0)
		path_zip_imp<Zipped, PFS>(pRoots, nRoots, is_confluence.bldata(), zipped);
	else {
		std::string msg = BOOST_CURRENT_FUNCTION;
		msg += ": must be one of: BFS, DFS, PFS";
		throw std::invalid_argument(msg);
	}
}
private:
template<class Zipped, class BaseWalker>
void path_zip_imp(const size_t* pRoots, size_t nRoots
				, bit_access is_confluence, Zipped& zipped)
const {
	assert(0 == num_zpath_states());
	assert(nRoots < total_states());
	ASSERT_isNotFree(initial_state);
	zipped.erase_all();
	const size_t min_compress_path_len = 3;
	const auto nil = typename Zipped::state_id_t(-1);
	valvec<typename Zipped::state_id_t> s2ds(total_states(), nil);
	ZipWalker<BaseWalker> walker;
	walker.resize(total_states());
	for(size_t i = nRoots;  i > 0; ) {
		size_t r = pRoots[--i];
		s2ds[r] = i;
		walker.put_root(r);
	}
	size_t ds = nRoots;
	while (!walker.is_finished()) {
		walker.getOnePath(this, is_confluence);
		auto plen = walker.path.size();
		auto head = walker.path[0];
		auto tail = walker.path.back();
		if (plen >= min_compress_path_len) {
			if (nil == s2ds[head])
				s2ds[tail] = s2ds[head] = ds++;
			else
				s2ds[tail] = s2ds[head];
		} else {
			size_t j = nil == s2ds[head] ? 0 : 1;
			for (; j < plen; ++j) s2ds[walker.path[j]] = ds++;
		}
		walker.putChildren(this, tail);
	}
	zipped.resize_states(ds);
	walker.resize(total_states());
	for (size_t i = nRoots; i > 0; )
		walker.put_root(pRoots[--i]);
	size_t n_zpath_states = 0;
	valvec<CharTarget<size_t> > allmove;
	while (!walker.is_finished()) {
		walker.getOnePath(this, is_confluence);
		allmove.erase_all();
		state_id_t xparent = walker.path.back();
		for_each_move(xparent,
		  [&](state_id_t, state_id_t t, auchar_t c) {
		    assert(s2ds[t] < ds);
			allmove.emplace_back(c, s2ds[t]);
		});
		auto zs = s2ds[xparent];
		zipped.add_all_move(zs, allmove);
		if (walker.path.size() >= min_compress_path_len) {
			n_zpath_states++;
			zipped.add_zpath(zs, walker.strp.data(), walker.strp.size());
		//	printf("add_zpath: %s\n", walker.strp.data());
		} else {
			const state_id_t* path = &walker.path[0];
			for (size_t j = 0; j < walker.strp.size(); ++j) {
				auto yparent = s2ds[path[j+0]];
				auto ychild  = s2ds[path[j+1]];
				assert(yparent < ds);
				assert(ychild  < ds);
				zipped.add_move_checked(yparent, ychild, walker.strp[j]);
				assert(!zipped.more_than_one_child(yparent));
			}
		}
		if (is_term(xparent))
			zipped.set_term_bit(zs);
		walker.putChildren(this, xparent);
	}
	zipped.path_zip_freeze(n_zpath_states);
}

public:
template<class ResDFA>
void normalize(ResDFA& res, const char* WalkMethod) const {
	size_t RootState = initial_state;
	normalize(&RootState, 1, res, WalkMethod);
}
template<class ResDFA>
void normalize(const valvec<size_t>& roots, ResDFA& res, const char* WalkMethod)
const {
	normalize(roots.data(), roots.size(), res, WalkMethod);
}
template<class ResDFA>
void normalize(const size_t* pRoots, size_t nRoots
			 , ResDFA& res, const char* WalkMethod)
const {
	typedef BFS_GraphWalker<state_id_t>  BFS;
	typedef DFS_GraphWalker<state_id_t>  DFS;
	typedef PFS_GraphWalker<state_id_t>  PFS;
	if (strcasecmp(WalkMethod, "BFS") == 0)
		normalize_imp<ResDFA, BFS>(pRoots, nRoots, res);
	else if (strcasecmp(WalkMethod, "DFS") == 0)
		normalize_imp<ResDFA, DFS>(pRoots, nRoots, res);
	else if (strcasecmp(WalkMethod, "PFS") == 0)
		normalize_imp<ResDFA, PFS>(pRoots, nRoots, res);
	else {
		std::string msg = BOOST_CURRENT_FUNCTION;
		msg += ": WalkMethod must be one of: BFS, DFS, PFS";
		throw std::invalid_argument(msg);
	}
}

private:
template<class ResDFA, class WalkerType>
void normalize_imp(const size_t* pRoots, size_t nRoots, ResDFA& res)
const {
	assert(0 == num_zpath_states());
	assert(nRoots < total_states());
	const auto nil = typename ResDFA::state_id_t(-1);
	valvec<typename ResDFA::state_id_t> smap(total_states(), nil);
	WalkerType walker;
	walker.resize(total_states());
	for(size_t i = nRoots; i > 0; ) {
		size_t root = pRoots[--i];
		walker.put_root(root);
		smap[root] = i;
	}
	size_t id = nRoots;
	while (!walker.is_finished()) {
		state_id_t curr = walker.next();
		if (smap[curr] == nil)
			smap[curr] = id++;
		walker.putChildren(this, curr);
	}
	res.erase_all();
	res.resize_states(total_states());
	walker.resize(total_states());
	for(size_t i = nRoots; i > 0; ) {
		size_t root = pRoots[--i];
		walker.put_root(root);
	}
	valvec<CharTarget<size_t> > allmove(get_sigma(), valvec_no_init());
	while (!walker.is_finished()) {
		state_id_t curr = walker.next();
		size_t n = get_all_move(curr, allmove.data());
		for (size_t i = 0; i < n; ++i) {
			CharTarget<size_t>& x = allmove[i];
			x.target = smap[x.target];
		}
		res.add_all_move(smap[curr], allmove.data(), n);
		if (is_term(curr))
			res.set_term_bit(smap[curr]);
		walker.putChildren(this, curr);
	}
	res.shrink_to_fit();
}

