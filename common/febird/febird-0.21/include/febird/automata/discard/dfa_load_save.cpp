#include "../config.hpp"
#include "dfa_algo.hpp"
#include "automata.hpp"
#include "dawg.hpp"
#include "linear_dfa.hpp"
#include "linear_dfa2.hpp"
#include "linear_dfa3.hpp"
#include "linear_dawg.hpp"
#include "double_array_trie.hpp"
#include "aho_corasick.hpp"
#include "dense_dfa.hpp"
#include "mre_match.hpp"

#include <febird/io/DataIO.hpp>
#include <febird/io/FileStream.hpp>
#include <febird/io/StreamBuffer.hpp>
#include <febird/util/throw.hpp>

/////////////////////////////////////////////////////////////////////////
// Memory Map support, now, only linux
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

struct DFA_BlockDataEntry {
	uint64_t offset;
	uint64_t length;
	uint64_t endpos() const { return offset + length; }
};
inline size_t align_to_64(size_t x) { return (x + 63) & size_t(-64); }

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

	DFA_BlockDataEntry blocks[8];
};
struct DFA_MmapHeader : DFA_MmapHeaderBase {
	char reserved[1024-sizeof(DFA_MmapHeaderBase)];
};
BOOST_STATIC_ASSERT(sizeof(DFA_MmapHeader) == 1024);

DFA_Interface::~DFA_Interface() {
	if (mmap_base) {
		if (m_fake_mmap)
			::free((void*)mmap_base);
		else
			::munmap((void*)mmap_base, mmap_base->file_size);
		if (mmap_fd > 0)
			::close(mmap_fd);
	}
}

#define DFA_TMPL_CLASS_PREFIX template<class State> void Automata<State>
#include "pool_dfa_mmap.hpp"

#define DFA_TMPL_CLASS_PREFIX template<class StateID, int Sigma> void DenseDFA<StateID, Sigma>
#include "pool_dfa_mmap.hpp"

#define DFA_TMPL_CLASS_PREFIX template<int StateBytes, int Sigma> void LinearDFA<StateBytes, Sigma>
#include "flat_dfa_mmap.hpp"

#define DFA_TMPL_CLASS_PREFIX template<class State> void DoubleArrayTrie<State>
#include "flat_dfa_mmap.hpp"

#define DFA_TMPL_CLASS_PREFIX template<class State> void DAWG<State>
#include "dawg_dfa_mmap.hpp"

#if defined(FEBIRD_INST_ALL_DFA_TYPES)
  #define DFA_TMPL_CLASS_PREFIX template<class State> void DAWG0<State>
  #include "dawg_dfa_mmap.hpp"
#endif

#define DFA_TMPL_CLASS_PREFIX template<int StateBytes> void LinearDAWG<StateBytes>
#include "dawg_dfa_mmap.hpp"

template<class BaseDFA>
void Aho_Corasick<BaseDFA>::finish_load_mmap(const DFA_MmapHeader* base)
{
	byte_t* bbase = (byte_t*)base;
	BaseDFA::finish_load_mmap(base);
	assert(base->num_blocks >= 2);
	auto   last_block = base->blocks[base->num_blocks-1];
	size_t num_output = last_block.length / sizeof(word_id_t);
	output.clear();
	output.risk_set_data((word_id_t*)(bbase + last_block.offset));
	output.risk_set_size(num_output);
	output.risk_set_capacity(num_output);
}

template<class BaseDFA>
void 
Aho_Corasick<BaseDFA>::prepare_save_mmap(DFA_MmapHeader* base, const void** dataPtrs)
const {
	BaseDFA::prepare_save_mmap(base, dataPtrs);
	auto blocks_end = &base->blocks[base->num_blocks];
	blocks_end[0].offset = align_to_64(blocks_end[-1].endpos());
	blocks_end[0].length = sizeof(word_id_t) * output.size();
	dataPtrs[base->num_blocks] = output.data();
	base->num_blocks++;
}

// this function take the ownership of fd
DFA_Interface* DFA_Interface::load_mmap(int fd) {
	if (fd < 0) {
		THROW_STD(invalid_argument,	"fd=%d < 0", fd);
	}
	struct stat st;
	if (::fstat(fd, &st) < 0) {
		THROW_STD(runtime_error, "fstat(fd=%d) = %m", fd);
	}
	if (!S_ISREG(st.st_mode)) {
		THROW_STD(invalid_argument, "st_mode=0x%lX is not a regular file",
		  (long)st.st_mode);
	}
	if (st.st_size <= (long)sizeof(DFA_MmapHeader)) {
		THROW_STD(invalid_argument, "FileSize=%ld is less than HeaderSize=%ld",
		  (long)st.st_size, (long)sizeof(DFA_MmapHeader));
	}
	const DFA_MmapHeader* base = (const DFA_MmapHeader*)
		::mmap(NULL, st.st_size, PROT_READ, MAP_SHARED|MAP_POPULATE, fd, 0);
	if (MAP_FAILED == base) {
		THROW_STD(runtime_error, "mmap(PROT_READ, fd=%d) = %m", fd);
	}
	if (strcmp(base->magic, "febird-dfa-mmap") != 0) {
		THROW_STD(invalid_argument, "file is not febird-dfa-mmap");
	}
	DFA_Interface* dfa = load_mmap_fmt(base);
	dfa->mmap_fd = fd;
	dfa->m_fake_mmap = false;
	return dfa;
}

DFA_Interface* DFA_Interface::load_mmap_fmt(const DFA_MmapHeader* base) {
	std::unique_ptr<DFA_Interface> dfa;
#define ON_CLASS_IO(Class) \
	else if (strcmp(base->dfa_class_name, BOOST_STRINGIZE(Class)) == 0) { \
		dfa.reset(new Class); \
	}
	if (0) {}
#include "dfa_class_io.hpp"
	else {
		FEBIRD_THROW(std::invalid_argument, 
			": unknown dfa_class: %s", base->dfa_class_name);
	}
	dfa->finish_load_mmap(base);
	dfa->mmap_base  = base;
	dfa->m_is_dag   = base->is_dag ? 1 : 0;
	dfa->m_kv_delim = base->kv_delim;
	return dfa.release();
}

void DFA_Interface::save_mmap(int fd) {
	DFA_MmapHeader header;
	memset(&header, 0, sizeof(header));
#define ON_CLASS_IO(Class) \
	else if (dynamic_cast<Class*>(this)) { \
		assert(strlen(BOOST_STRINGIZE(Class)) <= sizeof(header.dfa_class_name)); \
		strcpy(header.dfa_class_name, BOOST_STRINGIZE(Class)); \
	}
	if (0) {}
#include "dfa_class_io.hpp"
	else {
		FEBIRD_THROW(std::invalid_argument,
			"dfa_class=%s don't support save_mmap", typeid(*this).name());
	}
	strcpy(header.magic, "febird-dfa-mmap");
	header.header_size = sizeof(header);
	header.magic_len = 15;
	header.version  = DFA_MmapHeader::current_version;
	header.is_dag   = this->m_is_dag;
	header.kv_delim = this->m_kv_delim;
	const void* dataPtrs[8] = { NULL };
	prepare_save_mmap(&header, dataPtrs);
	assert(15 == header.magic_len);
	header.file_size = align_to_64(header.blocks[header.num_blocks-1].endpos());
	if (::ftruncate(fd, 0) < 0) {
		THROW_STD(runtime_error, "ftruncate(fd=%d, 0): %m", fd);
	}
	size_t toWrite = sizeof(header);
	size_t written = ::write(fd, &header, toWrite);
	if (written != toWrite) {
		FEBIRD_THROW(std::length_error,
			"write(fd=%d, toWrite=%zd) = %zd : %m", fd, toWrite, written);
	}
	assert(sizeof(header) == header.blocks[0].offset);
	for(size_t i = 0, n = header.num_blocks; i < n; ++i) {
		assert(header.blocks[i].offset % 64 == 0);
		toWrite = header.blocks[i].length;
		written = ::write(fd, dataPtrs[i], toWrite);
		if (written != toWrite) {
			FEBIRD_THROW(std::length_error,
				"write(fd=%d, toWrite=%zd) = %zd : %m", fd, toWrite, written);
		}
		if (header.blocks[i].length % 64 != 0) {
			char zeros[64] = { 0 };
			toWrite = 64 - header.blocks[i].length % 64;
			written = ::write(fd, zeros, toWrite);
			if (written != toWrite) {
				FEBIRD_THROW(std::length_error,
					"write(fd=%d, toWrite=%zd) = %zd : %m", fd, toWrite, written);
			}
		}
	}
	::fsync(fd);
#ifndef NDEBUG
	struct stat st;
	if (::fstat(fd, &st) < 0) {
		FEBIRD_THROW(std::runtime_error, "fstat(fd=%d) = %m", fd);
	}
	assert(!S_ISREG(st.st_mode) || uint64_t(st.st_size) == header.file_size);
#endif
}

DFA_Interface* DFA_Interface::load_mmap(const char* fname) {
	int fd = ::open(fname, O_RDONLY);
	if (fd < 0) {
		THROW_STD(runtime_error, "error: open(%s, O_RDONLY) = %m", fname);
	}
	return load_mmap(fd);
}

void DFA_Interface::save_mmap(const char* fname) {
	febird::Auto_close_fd fd;
	fd.f = ::open(fname, O_CREAT|O_WRONLY|O_LARGEFILE, 0644);
	if (fd.f < 0) {
		FEBIRD_THROW(std::runtime_error,
			"error: open(%s, O_CREAT|O_LARGEFILE, 0644) = %m", fname);
	}
	save_mmap(fd);
}

/////////////////////////////////////////////////////////////////////////

DFA_Interface* DFA_Interface::load_from(fstring fname) {
	assert('\0' == *fname.end());
	febird::FileStream file(fname.p, "rb");
	return load_from(file);
}

DFA_Interface* DFA_Interface::load_from_NativeInputBuffer(void* nativeInputBufferObject) {
	using namespace febird;
	NativeDataInput<InputBuffer>& dio = *(NativeDataInput<InputBuffer>*)nativeInputBufferObject;
	std::string className;
	dio >> className;
#define ON_CLASS_IO(Class) \
		if (BOOST_STRINGIZE(Class) == className) { \
			std::unique_ptr<Class> au(new Class); \
			dio >> *au; \
			return au.release(); \
		}
#include "dfa_class_io.hpp"
	if (strcmp(className.c_str(), "febird-dfa-mmap") == 0) {
		DFA_Interface* dfa = NULL;
		IInputStream* stream = dio.get_stream();
		if (stream) {
			if (FileStream* fp = dynamic_cast<FileStream*>(stream)) {
				try {
					dfa = load_mmap(::fileno(*fp));
				}
				catch (const std::exception&) {
					fprintf(stderr, "mmap failed: trying to load by stream read\n");
				}
			}
		} else {
			fprintf(stderr, "Warning: in %s: IInputStream is not a FileStream\n"
					, BOOST_CURRENT_FUNCTION);
		}
		if (NULL == dfa) {
			febird::AutoFree<DFA_MmapHeader> base;
			base.p = (DFA_MmapHeader*)malloc(sizeof(DFA_MmapHeader));
			if (NULL == base) {
				THROW_STD(runtime_error, "malloc DFA_MmapHeader");
			}
			base->magic_len = 15;
			memcpy(base->magic, "febird-dfa-mmap", base->magic_len);
			int remain_header_len = sizeof(DFA_MmapHeader) - base->magic_len - 1;
			dio.ensureRead(base->magic + base->magic_len, remain_header_len);
			DFA_MmapHeader* q = (DFA_MmapHeader*)realloc(base, base->file_size);
			if (NULL == q) {
				THROW_STD(runtime_error, "malloc FileSize=%lld includes DFA_MmapHeader"
						, (long long)base->file_size);
			}
			base.p = q;
			dio.ensureRead(q+1, q->file_size - sizeof(DFA_MmapHeader));
			dfa = load_mmap_fmt(q);
			dfa->m_fake_mmap = true;
			base.p = NULL; // release ownership
		}
		return dfa;
	}
	FEBIRD_THROW(std::invalid_argument, ": unknown dfa_class: %s", className.c_str());
}

DFA_Interface* DFA_Interface::load_from(FILE* fp) {
	using namespace febird;
	NonOwnerFileStream file(fp);
	file.disbuf();
	InputBuffer dio(&file);
	return load_from_NativeInputBuffer(&dio);
}

void DFA_Interface::save_to(fstring fname) const {
	assert('\0' == *fname.end());
	febird::FileStream file(fname.p, "wb");
	save_to(file);
}
void DFA_Interface::save_to(FILE* fp) const {
	using namespace febird;
	NonOwnerFileStream file(fp);
	file.disbuf();
	NativeDataOutput<OutputBuffer> dio; dio.attach(&file);
#define ON_CLASS_IO(Class) \
		if (const Class* au = dynamic_cast<const Class*>(this)) { \
			dio << fstring(BOOST_STRINGIZE(Class)); \
			dio << *au; \
			dio.flush(); \
			file.detach(); \
			return;\
		}
#include "dfa_class_io.hpp"
	FEBIRD_THROW(std::invalid_argument,
		": unknown dfa_class: %s", typeid(*this).name());
}

/////////////////////////////////////////////////////////////////////////


template<class DFA>
class MultiRegexSubmatchTmpl : public MultiRegexSubmatch  {
public:
	template<class TR>
	static size_t
	dmatch_func2(auchar_t delim, MultiRegexSubmatch* xThis, fstring text
				, const TR& tr) {
		auto self = static_cast<MultiRegexSubmatchTmpl*>(xThis);
		const DFA* dfa = static_cast<const DFA*>(self->dfa);
		assert(delim < DFA::sigma);
		self->reset();
		auto on_match = [&](size_t klen, size_t, fstring val) {
			assert(val.size() == sizeof(CaptureBinVal));
			CaptureBinVal ci;
			memcpy(&ci, val.data(), sizeof(ci));
			self->set_capture(ci.regex_id, ci.capture_id, klen);
		};
		return dfa_algo(dfa).match_key(delim, text, on_match, tr);
	}
	static size_t
	match_func1(MultiRegexSubmatch* xThis, fstring text) {
		auto self = static_cast<MultiRegexSubmatchTmpl*>(xThis);
		const DFA* dfa = static_cast<const DFA*>(self->dfa);
		auchar_t delim = dfa->kv_delim();
		assert(delim < DFA::sigma);
		return dmatch_func2(delim, self, text, IdentityTranslator());
	}
	static size_t
	match_func2(MultiRegexSubmatch* xThis, fstring text, const ByteTR& tr) {
		auto self = static_cast<MultiRegexSubmatchTmpl*>(xThis);
		const DFA* dfa = static_cast<const DFA*>(self->dfa);
		auchar_t delim = dfa->kv_delim();
		assert(delim < DFA::sigma);
		return dmatch_func2(delim, self, text, tr);
	}
};

void MultiRegexSubmatch::set_func(const DFA_Interface* dfa) {
#define ON_DFA_CLASS(DFA) \
	else if (dynamic_cast<const DFA*>(dfa)) { \
		pf_match1 = &MultiRegexSubmatchTmpl<DFA >::match_func1; \
		pf_match2 = &MultiRegexSubmatchTmpl<DFA >::match_func2; \
	}
	if (0) {}
	ON_DFA_CLASS(Automata<State32_512>)
	ON_DFA_CLASS(DenseDFA<uint32_t>)
	ON_DFA_CLASS(DenseDFA_uint32_320)
	else {
		FEBIRD_THROW(std::invalid_argument, 
	  "Don't support MultiRegexSubmatch, dfa_class: %s", typeid(*dfa).name());
	}
#undef ON_DFA_CLASS
}

template<class DFA>
class MultiRegexFullMatchTmpl : public MultiRegexFullMatch {
public:
	static size_t
	dmatch_func1(auchar_t delim, MultiRegexFullMatch* xThis, fstring text) {
		return dmatch_func2(delim, xThis, text, IdentityTranslator());
	}
	template<class TR>
	static size_t
	dmatch_func2(auchar_t delim, MultiRegexFullMatch* xThis, fstring text
				, const TR& tr) {
		auto self = static_cast<MultiRegexFullMatchTmpl*>(xThis);
		const DFA* au = static_cast<const DFA*>(self->dfa);
		assert(delim < DFA::sigma);
		self->erase_all();
		self->stack.erase_all();
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
					if (c2 == delim) {
						i += j;
						MultiRegexFullMatch::MatchEntry e;
						e.curr_pos = i;
						e.skip_len = j + 1;
						e.state_id = curr;
						self->stack.push_back(e);
					}
				   	if (r.empty() || (byte_t)tr(*r) != c2) {
						i += j;
						goto Done;
					}
				}
				i += n2;
			}
			state_id_t value_start = au->state_move(curr, delim);
			if (DFA::nil_state != value_start) {
				MatchEntry e;
				e.curr_pos = i;
				e.skip_len = 0;
				e.state_id = value_start;
				self->stack.push_back(e);
			}
			if (r.empty()) {
				goto Done;
			}
			state_id_t next = au->state_move(curr, (byte_t)tr(*r));
			if (DFA::nil_state == next) {
				goto Done;
			}
			assert(next < au->total_states());
			ASSERT_isNotFree2(au, next);
			curr = next;
		}
		assert(0);
	Done:
		for (size_t j = self->stack.size(); j > 0 && self->empty(); ) {
			--j;
			const MatchEntry e = self->stack[j];
			auto on_suffix = [&](size_t nth, fstring val, size_t) {
				MultiRegexSubmatch::CaptureBinVal bv;
				assert(val.size() == sizeof(MultiRegexSubmatch::CaptureBinVal));
				memcpy(&bv, val.data(), sizeof(bv));
				if (1 == bv.capture_id) { // full match end point
					RegexMatchPoint x;
					x.pos = e.curr_pos;
					x.regex_id = bv.regex_id;
					self->push_back(x);
				}
			};
			const int Bufsize = sizeof(MultiRegexSubmatch::CaptureBinVal); // = 8
			ForEachWord_DFS<DFA, decltype(on_suffix), Bufsize> vis(au, on_suffix);
			vis.start(e.state_id, e.skip_len);
		}
		return i;
	}

	static size_t
	match_func1(MultiRegexFullMatch* xThis, fstring text) {
		auto self = static_cast<MultiRegexFullMatchTmpl*>(xThis);
		const DFA* dfa = static_cast<const DFA*>(self->dfa);
		return dmatch_func2(dfa->kv_delim(), xThis, text, IdentityTranslator());
	}
	static size_t
	match_func2(MultiRegexFullMatch* xThis, fstring text, const ByteTR& tr) {
		auto self = static_cast<MultiRegexFullMatchTmpl*>(xThis);
		const DFA* dfa = static_cast<const DFA*>(self->dfa);
		return dmatch_func2(dfa->kv_delim(), xThis, text, tr);
	}
};

void MultiRegexFullMatch::set_func(const DFA_Interface* dfa) {
#define ON_DFA_CLASS(DFA) \
	else if (dynamic_cast<const DFA*>(dfa)) { \
		pf_match1 = &MultiRegexFullMatchTmpl<DFA >::match_func1; \
		pf_match2 = &MultiRegexFullMatchTmpl<DFA >::match_func2; \
	}
	if (0) {}
	ON_DFA_CLASS(Automata<State32_512>)
	ON_DFA_CLASS(DenseDFA<uint32_t>)
	ON_DFA_CLASS(DenseDFA_uint32_320)
	else {
		FEBIRD_THROW(std::invalid_argument, 
	  "Don't support MultiRegexFullMatch, dfa_class: %s", typeid(*dfa).name());
	}
#undef ON_DFA_CLASS
}

