#ifndef __febird_automata_re2_nfa_adapter_hpp__
#define __febird_automata_re2_nfa_adapter_hpp__

#include <febird/automata/dfa_interface.hpp>
#include <febird/util/fstrvec.hpp>
#include <boost/noncopyable.hpp>

namespace re2 {

	class Prog; // forward declaration

	class RE2_VM_NFA : boost::noncopyable {
	private:
	//	const Prog* m_prog;
		Prog* m_prog;
		basic_fstrvec<auchar_t> m_captures;
		valvec<int> m_cap_ptrs;

	public:
		enum { sigma = 258 };
		typedef uint32_t state_id_t;

		explicit RE2_VM_NFA(Prog* prog);

		std::string m_val_prefix;
		bool  m_add_dot_star;
		bool  m_use_bin_val;
		bool  m_do_capture;
		int   m_exclude_char;

		size_t get_sigma() const { return sigma; }

		void compile();

		size_t captures() const { return m_captures.size(); }
		size_t total_states() const;
		bool is_final(size_t) const;
		void get_epsilon_targets(state_id_t s, valvec<state_id_t>* children) const;
		void get_non_epsilon_targets(state_id_t s, valvec<CharTarget<size_t> >* children) const;
	};

} // namespace re2

#endif // __febird_automata_re2_nfa_adapter_hpp__

