#include "dfa_interface.hpp"
#include <febird/util/autoclose.hpp>
#include <febird/bitmap.hpp>

DFA_Interface::DFA_Interface() {}
DFA_Interface::~DFA_Interface() {}

void DFA_Interface::dot_write_one_state(FILE* fp, size_t s) const {
	long ls = s;
	if (v_is_pzip(ls)) {
		const byte_t* s2 = v_get_zpath_data(ls);
		int n = *s2++;
		if (v_is_term(ls))
			fprintf(fp, "\tstate%ld[label=\"%ld:%.*s\" shape=\"doublecircle\"];\n", ls, ls, n, s2);
		else
			fprintf(fp, "\tstate%ld[label=\"%ld:%.*s\"];\n", ls, ls, n, s2);
	}
	else {
		if (v_is_term(ls))
			fprintf(fp, "\tstate%ld[label=\"%ld\" shape=\"doublecircle\"];\n", ls, ls);
		else
			fprintf(fp, "\tstate%ld[label=\"%ld\"];\n", ls, ls);
	}
}

void DFA_Interface::dot_write_one_transition(FILE* fp, size_t s, size_t t, char_t ch) const {
	fprintf(fp, "\tstate%ld -> state%ld [label=\"%c\"];\n",	(long)s, (long)t, (int)ch);
}

void DFA_Interface::write_dot_file(const fstring fname) const {
	febird::Auto_fclose dot(fopen(fname.p, "w"));
	if (dot) {
		write_dot_file(dot);
	} else {
		fprintf(stderr, "can not open %s for write\n", fname.p);
	}
}

void DFA_Interface::write_dot_file(FILE* fp) const {
//	printf("%s:%d %s\n", __FILE__, __LINE__, BOOST_CURRENT_FUNCTION);
	fprintf(fp, "digraph G {\n");
	simplebitvec   color(v_total_states(), 0);
	valvec<size_t> stack;
	boost::function<void(size_t,size_t)> on_dest = 
		[&](size_t, size_t child) {
			if (color.is0(child)) {
				color.set1(child);
				stack.push_back(child);
			}
		};
	size_t RootState = 0;
	color.set1(RootState);
	stack.push_back(RootState);
	while (!stack.empty()) {
		size_t curr = stack.back(); stack.pop_back();
		dot_write_one_state(fp, curr);
		v_for_each_dest(curr, on_dest);
	}
	boost::function<void(size_t,size_t,byte_t)> on_move = 
		[&](size_t parent, size_t child, char_t ch) {
			if (color.is0(child)) {
				color.set1(child);
				stack.push_back(child);
			}
			dot_write_one_transition(fp, parent, child, ch);
		};
	color.fill(0);
	color.set1(RootState);
	stack.push_back(RootState);
	while (!stack.empty()) {
		size_t curr = stack.back(); stack.pop_back();
		v_for_each_move(curr, on_move);
	}
	fprintf(fp, "}\n");
}

size_t DFA_Interface::print_all_words(FILE* fp) const {
	size_t bytes = 0;
	boost::function<void(size_t nth, fstring)> on_word =
	  [&](size_t, fstring word) {
		bytes += fprintf(fp, "%s\n", word.c_str());
	  };
	this->for_each_word(on_word);
	return bytes;
}

size_t DFA_Interface::for_each_word(const boost::function<void(size_t nth, fstring)>& on_word) const {
	return for_each_word(initial_state, on_word);
}

size_t DFA_Interface::for_each_suffix(fstring prefix
		, const boost::function<void(size_t nth, fstring)>& on_suffix
		, const boost::function<byte_t(byte_t)>& tr
		) const {
	return for_each_suffix(initial_state, prefix, on_suffix, tr);
}

size_t DFA_Interface::for_each_suffix(fstring prefix
			, const boost::function<void(size_t nth, fstring)>& on_suffix
			) const {
	return for_each_suffix(initial_state, prefix, on_suffix);
}

//#include <cxxabi.h>
// __cxa_demangle(const char* __mangled_name, char* __output_buffer, size_t* __length, int* __status);

/////////////////////////////////////////////////////////
//
DFA_MutationInterface::DFA_MutationInterface() {}
DFA_MutationInterface::~DFA_MutationInterface() {}

/////////////////////////////////////////////////////////
//

DAWG_Interface::DAWG_Interface() {
	n_words = 0;
}
DAWG_Interface::~DAWG_Interface() {}

std::string DAWG_Interface::nth_word(size_t nth) const {
	std::string word;
	nth_word(nth, &word);
	return word;
}


