#ifndef __febird_fsa_dfa_mmap_header_hpp__
#define __febird_fsa_dfa_mmap_header_hpp__

#include "dfa_interface.hpp"

struct DFA_BlockDataEntry {
	uint64_t offset;
	uint64_t length;
	uint64_t endpos() const { return offset + length; }
};

struct DFA_MmapHeaderBase {
	enum { current_version = 1 };
	uint8_t   magic_len; // 15
	char      magic[19]; // febird-dfa-mmap
	char      dfa_class_name[60];

	byte_t    is_dag;
	byte_t    num_blocks;
	uint16_t  kv_delim;
	uint32_t  header_size; // == sizeof(DFA_MmapHeader)
	uint32_t  version;
	uint32_t  state_size;

	uint64_t  file_size;
	uint64_t  total_states;
	uint64_t  zpath_states;
	uint64_t  numFreeStates;
	uint64_t  firstFreeState;
	uint64_t  transition_num;
	uint64_t  dawg_num_words;

	byte_t    ac_word_ext; // 0: no length; 1: length; 2: length+content
	byte_t    padding1[7];
	uint64_t  reserve1[16];

	DFA_BlockDataEntry blocks[8];
};
struct DFA_MmapHeader : DFA_MmapHeaderBase {
	char reserved[1024-sizeof(DFA_MmapHeaderBase)];
};
BOOST_STATIC_ASSERT(sizeof(DFA_MmapHeader) == 1024);


#endif // __febird_fsa_dfa_mmap_header_hpp__
