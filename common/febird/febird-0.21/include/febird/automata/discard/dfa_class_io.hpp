#ifndef FEBIRD_WORD_BITS
#error FEBIRD_WORD_BITS is undefined
#endif

#if FEBIRD_WORD_BITS != 32 && FEBIRD_WORD_BITS != 64
#error "FEBIRD_WORD_BITS"=FEBIRD_WORD_BITS is invalid
#endif

ON_CLASS_IO(Automata<State32>)
ON_CLASS_IO(Automata<State4B>)
ON_CLASS_IO(Automata<State5B>)
ON_CLASS_IO(Automata<State6B>)

ON_CLASS_IO(Automata<State32_512>)
ON_CLASS_IO(Automata<State4B_448>)
ON_CLASS_IO(Automata<State5B_448>)
ON_CLASS_IO(Automata<State6B_512>)

ON_CLASS_IO(DAWG<State32>)
ON_CLASS_IO(DAWG<State4B>)
ON_CLASS_IO(DAWG<State5B>)
ON_CLASS_IO(DAWG<State6B>)

#if FEBIRD_WORD_BITS >= 64 && defined(FEBIRD_INST_ALL_DFA_TYPES)
ON_CLASS_IO(Automata<State34>)
ON_CLASS_IO(Automata<State7B>)
ON_CLASS_IO(Automata<State7B_448>)
ON_CLASS_IO(DAWG<State34>)
ON_CLASS_IO(DAWG<State7B>)
#endif

#if defined(FEBIRD_INST_ALL_DFA_TYPES)
ON_CLASS_IO(DAWG0<DAWG0_State<State32> >)
ON_CLASS_IO(DAWG0<DAWG0_State<State4B> >)
ON_CLASS_IO(DAWG0<DAWG0_State<State5B> >)
ON_CLASS_IO(DAWG0<DAWG0_State<State6B> >)
#endif

#if FEBIRD_WORD_BITS >= 64 && defined(FEBIRD_INST_ALL_DFA_TYPES)
ON_CLASS_IO(DAWG0<DAWG0_State<State34> >)
ON_CLASS_IO(DAWG0<DAWG0_State<State7B> >)
#endif

#if defined(FEBIRD_INST_ALL_DFA_TYPES)
ON_CLASS_IO(LinearDFA<2>)
ON_CLASS_IO(LinearDFA<3>)
#endif
ON_CLASS_IO(LinearDFA<4>)
ON_CLASS_IO(LinearDFA<5>)

#if defined(FEBIRD_INST_ALL_DFA_TYPES)
ON_CLASS_IO(LinearDFA_2B_512)
ON_CLASS_IO(LinearDFA_3B_512)
#endif
ON_CLASS_IO(LinearDFA_4B_512)
ON_CLASS_IO(LinearDFA_5B_512)

#if defined(FEBIRD_INST_ALL_DFA_TYPES)
ON_CLASS_IO(LinearDFA2<3>)
ON_CLASS_IO(LinearDFA2<4>)
ON_CLASS_IO(LinearDFA2<5>)

ON_CLASS_IO(LinearDFA3<3>)
ON_CLASS_IO(LinearDFA3<4>)
ON_CLASS_IO(LinearDFA3<5>)
#endif

#if FEBIRD_WORD_BITS >= 64 && defined(FEBIRD_INST_ALL_DFA_TYPES)
ON_CLASS_IO(LinearDFA2<6>)
ON_CLASS_IO(LinearDFA3<6>)
#endif

#if defined(FEBIRD_INST_ALL_DFA_TYPES)
ON_CLASS_IO(LinearDAWG<2>)
ON_CLASS_IO(LinearDAWG<3>)
#endif
ON_CLASS_IO(LinearDAWG<4>)
ON_CLASS_IO(LinearDAWG<5>)

ON_CLASS_IO(DenseDFA<uint32_t>)
ON_CLASS_IO(DenseDFA_uint32_320)

#if FEBIRD_WORD_BITS >= 64
	ON_CLASS_IO(LinearDFA<6>)
	ON_CLASS_IO(LinearDFA_6B_512)
	ON_CLASS_IO(LinearDAWG<6>)
#endif

#if FEBIRD_WORD_BITS >= 64 && defined(FEBIRD_INST_ALL_DFA_TYPES)
	ON_CLASS_IO(DenseDFA<uint64_t>)
	ON_CLASS_IO(DenseDFA_uint64_320)
#endif

ON_CLASS_IO(DoubleArrayTrie_8B)
ON_CLASS_IO(Aho_Corasick_DoubleArray_8B)
ON_CLASS_IO(Aho_Corasick_SmartDFA_AC_State32)
ON_CLASS_IO(Aho_Corasick_SmartDFA_AC_State5B)
//ON_CLASS_IO(Aho_Corasick_SmartDFA_AC_State6B)

#undef ON_CLASS_IO
