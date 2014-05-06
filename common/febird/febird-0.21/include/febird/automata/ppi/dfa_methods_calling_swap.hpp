
public:
///@param WalkMethod "bfs" or "dfs"
void normalize(const char* WalkMethod) {
	MyType res;
	this->normalize(res, WalkMethod);
	this->swap(res);
}

void path_zip(const char* WalkMethod) {
	MyType zipped;
	this->path_zip(zipped, WalkMethod);
	this->swap(zipped);
}

void remove_dead_states() {
	MyType clean;
	this->remove_dead_states(clean);
	this->swap(clean);
}

void graph_dfa_minimize() {
	MyType minimized;
	this->graph_dfa_minimize(minimized);
	this->swap(minimized);
}
void trie_dfa_minimize() {
	MyType minimized;
	this->trie_dfa_minimize(minimized);
	this->swap(minimized);
}
void adfa_minimize() {
	MyType minimized;
	this->adfa_minimize(minimized);
	this->swap(minimized);
}


