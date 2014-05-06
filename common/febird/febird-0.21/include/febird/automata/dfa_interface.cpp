#include "dfa_interface.hpp"
#include "dfa_mmap_header.hpp"
#include "tmplinst.hpp"

#include <febird/util/autoclose.hpp>
#include <febird/bitmap.hpp>
#include <febird/util/unicode_iterator.hpp>

#include <febird/io/DataIO.hpp>
#include <febird/io/FileStream.hpp>
#include <febird/io/StreamBuffer.hpp>
#include <febird/util/throw.hpp>

#if !defined(MAP_POPULATE)
	#define  MAP_POPULATE 0
#endif

#if !defined(O_LARGEFILE)
	#define  O_LARGEFILE 0
#endif

ByteInputRange::~ByteInputRange() {}

DFA_Interface::DFA_Interface() {
	mmap_base = NULL;
	mmap_fd = -1;
	m_fake_mmap = 0;
	m_kv_delim = 256; // default use min non-byte as delim
	m_is_dag = 0;
	zpath_states = 0;
}

void DFA_Interface::get_all_dest(size_t s, valvec<size_t>* dests) const {
	dests->resize_no_init(512); // current max possible sigma
	dests->risk_set_size(get_all_dest(s, dests->data()));
	assert(dests->size() <= 512);
}
void DFA_Interface::get_all_move(size_t s, valvec<CharTarget<size_t> >* moves) const {
	moves->resize_no_init(512); // current max possible sigma
	moves->risk_set_size(get_all_move(s, moves->data()));
	assert(moves->size() <= 512);
}

#define CheckBufferOverRun(inclen) \
	if (q + inclen > oend) { \
		std::string msg; \
		msg += __LINE__; \
		msg += ":"; \
		msg += BOOST_STRINGIZE(__LINE__); \
		msg += ": "; \
		msg += BOOST_CURRENT_FUNCTION; \
		throw std::runtime_error(msg); \
	}
namespace febird {
template<class Char>
size_t 
dot_escape_aux(const Char* ibuf, size_t ilen, char* obuf, size_t olen) {
	const Char* iend = ibuf + ilen;
	char* q = obuf;
	char* oend = obuf + olen - 1;
	const char* hex = "0123456789ABCDEF";
	for (const Char* p = ibuf; p < iend && q < oend; ++p) {
		Char ch = *p;
		switch (ch) {
		default:
			if (sizeof(Char) == 1 && (ch && 0xC0) == 0xC0) {
				size_t utf8_len = febird::utf8_byte_count(ch);
				CheckBufferOverRun(utf8_len);
				if (p + utf8_len <= iend) {
					memcpy(q, p, utf8_len); 
					p += utf8_len - 1;
					q += utf8_len;
					break;
				}
			}
			if (isgraph(ch))
				*q++ = ch;
			else {
				CheckBufferOverRun(5);
				*q++ = '\\';
				*q++ = '\\';
				*q++ = 'x';
				if (sizeof(Char) > 1 && ch & 0xFF00) {
					if (ch & 0xF000)
						*q++ = hex[ch >> 12 & 0x0F];
					*q++ = hex[ch >> 8 & 0x0F];
				}
				*q++ = hex[ch >> 4 & 0x0F];
				*q++ = hex[ch >> 0 & 0x0F];
			}
			break;
		case '\\':
			CheckBufferOverRun(4);
			*q++ = '\\'; *q++ = '\\';
			*q++ = '\\'; *q++ = '\\';
			break;
		case '"':
			CheckBufferOverRun(2);
			*q++ = '\\';
			*q++ = '"';
			break;
		case '[': // meta char
		case ']': // meta char
		case '.': // meta char
			CheckBufferOverRun(3);
			*q++ = '\\'; *q++ = '\\'; *q++ = ch;
			break;
		case '\b':
			CheckBufferOverRun(3);
			*q++ = '\\'; *q++ = '\\'; *q++ = 'b';
			break;
		case '\v':
			CheckBufferOverRun(3);
			*q++ = '\\'; *q++ = '\\'; *q++ = 'v';
			break;
		case '\t':
			CheckBufferOverRun(3);
			*q++ = '\\'; *q++ = '\\'; *q++ = 't';
			break;
		case '\r':
			CheckBufferOverRun(3);
			*q++ = '\\'; *q++ = '\\'; *q++ = 'r';
			break;
		case '\n':
			CheckBufferOverRun(3);
			*q++ = '\\'; *q++ = '\\'; *q++ = 'n';
			break;
		}
	}
	*q = '\0';
	return q - obuf;
}

size_t 
dot_escape(const char* ibuf, size_t ilen, char* obuf, size_t olen) {
	return dot_escape_aux(ibuf, ilen, obuf, olen);
}
size_t 
dot_escape(const auchar_t* ibuf, size_t ilen, char* obuf, size_t olen) {
	return dot_escape_aux(ibuf, ilen, obuf, olen);
}

} // namespace febird
using febird::dot_escape;

void DFA_Interface::dot_write_one_state(FILE* fp, size_t s) const {
	long ls = s;
	if (v_is_pzip(ls)) {
		const byte_t* s2 = v_get_zpath_data(ls);
		int n = *s2++;
		char buf[1040];
		dot_escape((char*)s2, n, buf, sizeof(buf)-1);
		if (v_is_term(ls))
			fprintf(fp, "\tstate%ld[label=\"%ld: %s\" shape=\"doublecircle\"];\n", ls, ls, buf);
		else
			fprintf(fp, "\tstate%ld[label=\"%ld: %s\"];\n", ls, ls, buf);
	}
	else {
		if (v_is_term(ls))
			fprintf(fp, "\tstate%ld[label=\"%ld\" shape=\"doublecircle\"];\n", ls, ls);
		else
			fprintf(fp, "\tstate%ld[label=\"%ld\"];\n", ls, ls);
	}
}

void DFA_Interface::dot_write_one_move(FILE* fp, size_t s, size_t t, auchar_t ch)
const {
	char buf[32];
	dot_escape(&ch, 1, buf, sizeof(buf));
	fprintf(fp, "\tstate%ld -> state%ld [label=\"%s\"];\n", long(s), long(t), buf);
}

void DFA_Interface::write_dot_file(const fstring fname) const {
	febird::Auto_fclose dot(fopen(fname.p, "w"));
	if (dot) {
		write_dot_file(dot);
	} else {
		fprintf(stderr, "can not open %s for write: %m\n", fname.p);
	}
}

namespace {
struct By_ch {
template<class CT> bool operator()(const CT& x, auchar_t y) const { return x.ch < y; }
template<class CT> bool operator()(auchar_t x, const CT& y) const { return x < y.ch; }
};
static 
void cclabel0(const CharTarget<size_t>* p
			, const CharTarget<size_t>* q
			, std::string& buf)
{
	for(auto i = p; i < q; ) {
		auto c = i->ch;
		auto j = i + 1;
		while (j < q && j-i == j->ch-c) ++j;
		if (j - i == 1) {
			buf.push_back(c);
		}
	   	else if (j - i <= 3) {
			for (auto k = i; k < j; ++k) buf.push_back(k->ch);
		}
		else {
			buf.push_back(i->ch);
			buf.push_back('-');
			buf.push_back(j[-1].ch);
		}
		i = j;
	}
}
static 
void cclabel2(const CharTarget<size_t>* p
			, const CharTarget<size_t>* q
			, std::string& sbuf)
{
	for(auto i = p; i < q; ) {
		auto j = i + 1;
		auchar_t minch = i->ch;
		while (j < q && j-i == j->ch-minch) ++j;
		auchar_t maxch = j[-1].ch;
	//	fprintf(stderr, "cclabel2: j-i=%ld minch=%02X maxch=%02X\n", j-i, minch, maxch);
		if (j-i >= 4
			//	&& ((minch > '^' && maxch & 0x80) || maxch < 0x30)
				&& (0 == minch || !strchr("-^[]", minch))
				&& (maxch > 255 || !strchr("-^[]", maxch))
		 ) {
	//		fprintf(stderr, "cclabel2: 鬼鬼\n");
			char buf[32];
			auchar_t ch = minch;
			sbuf.append(buf, dot_escape(&ch, 1, buf, sizeof(buf)));
			sbuf.push_back('-');
			ch = maxch;
			sbuf.append(buf, dot_escape(&ch, 1, buf, sizeof(buf)));
		}
	   	else {
			for (auto k = i; k < j; ++k) {
				char buf[32];
				if (k->ch < 256) {
					auchar_t ch = k->ch;
					if ('-' == ch || '^' == ch)
						sbuf.append("\\\\"), sbuf.push_back(ch);
					else
						sbuf.append(buf, dot_escape(&ch, 1, buf, sizeof(buf)));
				} else {
					sprintf(buf, "\\\\%03X", k->ch);
					sbuf.append(buf);
				}
			}
		}
		i = j;
	}
}
static void
print_labels(FILE* fp, size_t parent, const valvec<CharTarget<size_t> >& children) {
	std::string sbuf;
	for(auto i = children.begin(); i < children.end(); ) {
		auto j = i;
		auto t = i->target;
		while (j < children.end() && j->target == t) ++j;
		sbuf.resize(0);
		if (j - i == 1) { // same as DFA_Interface::dot_write_one_move
			char buf[32];
			auchar_t ch = i->ch;
			sbuf.append(buf, dot_escape(&ch, 1, buf, sizeof(buf)));
		}
		else if (j - i == 256) {
			sbuf.append(".");
		}
		else if (j - i >= 236) {
			sbuf.append("[^");
			for (size_t c = 0; c < 512; ++c) {
				if (c != '-' && !std::binary_search(i, j, c, By_ch())) {
					char buf[32];
					auchar_t ch = c;
					sbuf.append(buf, dot_escape(&ch, 1, buf, sizeof(buf)));
				}
			}
			if (!std::binary_search(i, j, '-', By_ch()))
			 	sbuf.push_back('-');
			sbuf.append("]");
		}
		else {
			sbuf.append("[");
			auto dig0 = std::lower_bound(i, j, '0', By_ch());
			auto dig1 = std::upper_bound(i, j, '9', By_ch());
			auto upp0 = std::lower_bound(i, j, 'A', By_ch());
			auto upp1 = std::upper_bound(i, j, 'Z', By_ch());
			auto low0 = std::lower_bound(i, j, 'a', By_ch());
			auto low1 = std::upper_bound(i, j, 'z', By_ch());
			if (dig0 < dig1) cclabel0(dig0, dig1, sbuf);
			if (upp0 < upp1) cclabel0(upp0, upp1, sbuf);
			if (low0 < low1) cclabel0(low0, low1, sbuf);
			if (i    < dig0) cclabel2(i   , dig0, sbuf);
			if (dig1 < upp0) cclabel2(dig1, upp0, sbuf);
			if (upp1 < low0) cclabel2(upp1, low0, sbuf);
			if (low1 < j   ) cclabel2(low1, j   , sbuf);
			sbuf.append("]");
		}
		fprintf(fp, "\tstate%ld -> state%ld [label=\"%s\"];\n"
			, long(parent), long(t), sbuf.c_str());
		i = j;
	}
}
} // namespace

void DFA_Interface::write_dot_file(FILE* fp) const {
	size_t RootState = 0;
	multi_root_write_dot_file(&RootState, 1, fp);
}

void DFA_Interface::
multi_root_write_dot_file(const size_t* pRoots, size_t nRoots, FILE* fp)
const {
//	printf("%s:%d %s\n", __FILE__, __LINE__, BOOST_CURRENT_FUNCTION);
	fprintf(fp, "digraph G {\n");
	simplebitvec   color(v_total_states(), 0);
	valvec<size_t> stack;
	valvec<size_t> dests;
	valvec<CharTarget<size_t> > children;
	for(size_t i = nRoots; i > 0; ) {
		size_t RootState = pRoots[--i];
		color.set1(RootState);
		stack.push_back(RootState);
	}
	while (!stack.empty()) {
		size_t curr = stack.back(); stack.pop_back();
		get_all_dest(curr, &dests);
		for(size_t i = 0; i < dests.size(); ++i) {
			size_t child = dests[i];
			if (color.is0(child)) {
				color.set1(child);
				stack.push_back(child);
			}
		}
		dot_write_one_state(fp, curr);
	}
	color.fill(0);
	for(size_t i = nRoots; i > 0; ) {
		size_t RootState = pRoots[--i];
		color.set1(RootState);
		stack.push_back(RootState);
	}
	while (!stack.empty()) {
		size_t curr = stack.back(); stack.pop_back();
		get_all_move(curr, &children);
		if (children.size() > 1) {
			auto By_target_ch =
			[](const CharTarget<size_t>& x, const CharTarget<size_t>& y) {
				 if (x.target != y.target)
					 return x.target < y.target;
				 else
					 return x.ch < y.ch;
			};
			std::sort(children.begin(), children.end(), By_target_ch);
			print_labels(fp, curr, children);
		}
		for(size_t i = 0; i < children.size(); ++i) {
			size_t child = children[i].target;
			auchar_t ch  = children[i].ch;
			if (color.is0(child)) {
				color.set1(child);
				stack.push_back(child);
			}
			if (children.size() == 1)
				dot_write_one_move(fp, curr, child, ch);
		}
	}
	fprintf(fp, "}\n");
}

void DFA_Interface::
multi_root_write_dot_file(const size_t* pRoots, size_t nRoots, fstring fname)
const {
	febird::Auto_fclose dot(fopen(fname.p, "w"));
	if (dot) {
		multi_root_write_dot_file(pRoots, nRoots, dot);
	} else {
		fprintf(stderr, "can not open %s for write: %m\n", fname.p);
	}
}

void DFA_Interface::
multi_root_write_dot_file(const valvec<size_t>& roots, FILE* fp)
const {
	multi_root_write_dot_file(roots.data(), roots.size(), fp);
}

void DFA_Interface::
multi_root_write_dot_file(const valvec<size_t>& roots, fstring fname)
const {
	multi_root_write_dot_file(roots.data(), roots.size(), fname);
}

size_t DFA_Interface::print_all_words(FILE* fp) const {
	assert(this->is_dag());
	size_t bytes = 0;
	auto on_word = [&](size_t, fstring word) {
		bytes += fprintf(fp, "%.*s\n", word.ilen(), word.data());
	};
	this->for_each_word(boost::ref(on_word));
	return bytes;
}

static
FILE* fopen_output(const char* fname, const char* cpp_fname, int lineno) {
	FILE* fp;
	if (strcmp(fname, "-") == 0 ||
		strcmp(fname, "/dev/fd/1") == 0 ||
		strcmp(fname, "/dev/stdout") == 0) {
			fp = stdout;
	} else {
		fp = fopen(fname, "w");
		if (NULL == fp) {
			THROW_STD(runtime_error
				, "%s:%d can not open \"%s\", err=%m\n"
				, cpp_fname, lineno, fname);
		}
	}
	return fp;
}

size_t
DFA_Interface::print_output(const char* fname, size_t RootState) const {
	printf("print_output(\"%s\", RootState=%ld)\n", fname, (long)RootState);
	FILE* fp = fopen_output(fname, __FILE__, __LINE__);
	auto on_word = [fp](size_t, fstring word) {
		fprintf(fp, "%.*s\n", word.ilen(), word.data());
	};
	size_t cnt = for_each_word(RootState, boost::ref(on_word));
	if (fp != stdout) fclose(fp);
	return cnt;
}

size_t
DFA_Interface::for_each_word(const boost::function<void(size_t,fstring)>& on_word)
const {
	assert(this->is_dag());
	return for_each_word(initial_state, on_word);
}

size_t DFA_Interface::for_each_suffix(fstring prefix
		, const boost::function<void(size_t nth, fstring)>& on_suffix
		, const boost::function<byte_t(byte_t)>& tr
		) const {
	assert(this->is_dag());
	return for_each_suffix(initial_state, prefix, on_suffix, tr);
}

size_t DFA_Interface::for_each_suffix(fstring prefix
			, const boost::function<void(size_t nth, fstring)>& on_suffix
			) const {
	assert(this->is_dag());
	return for_each_suffix(initial_state, prefix, on_suffix);
}

bool DFA_Interface::full_match(fstring text) const {
	size_t max_len = 0;
	max_prefix_match(text, &max_len);
	return text.size() == max_len;
}
bool DFA_Interface::full_match(fstring text, const ByteTR& tr) const {
	size_t max_len = 0;
	max_prefix_match(text, &max_len, tr);
	return text.size() == max_len;
}
bool DFA_Interface::full_match(ByteInputRange& r) const {
	size_t max_len = 0;
	size_t plen = max_prefix_match(r, &max_len);
	return plen == max_len && r.empty();
}
bool DFA_Interface::full_match(ByteInputRange& r, const ByteTR& tr) const {
	size_t max_len = 0;
	size_t plen = max_prefix_match(r, &max_len, tr);
	return plen == max_len && r.empty();
}

void DFA_Interface::finish_load_mmap(const DFA_MmapHeader*) {
	// do nothing
}
void DFA_Interface::prepare_save_mmap(DFA_MmapHeader*, const void**) const {
	// do nothing
}

/////////////////////////////////////////////////////////////////////////
// Memory Map support, now, only linux
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

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
	using namespace febird;
	const DFA_ClassMetaInfo* meta = DFA_ClassMetaInfo::find(base->dfa_class_name);
	if (NULL == meta) {
		FEBIRD_THROW(std::invalid_argument,
			": unknown dfa_class: %s", base->dfa_class_name);
	}
	std::unique_ptr<DFA_Interface> dfa(meta->create());
	dfa->finish_load_mmap(base);
	dfa->mmap_base  = base;
	dfa->m_is_dag   = base->is_dag ? 1 : 0;
	dfa->m_kv_delim = base->kv_delim;
	return dfa.release();
}

void DFA_Interface::save_mmap(int fd) {
	using namespace febird;
	DFA_MmapHeader header;
	memset(&header, 0, sizeof(header));
	const DFA_ClassMetaInfo* meta = DFA_ClassMetaInfo::find(this);
	if (NULL == meta) {
		FEBIRD_THROW(std::invalid_argument,
			"dfa_class=%s don't support save_mmap", typeid(*this).name());
	}
	strcpy(header.dfa_class_name, meta->class_name);
	strcpy(header.magic, "febird-dfa-mmap");
	header.header_size = sizeof(header);
	header.magic_len = 15;
	header.version  = DFA_MmapHeader::current_version;
	header.is_dag   = this->m_is_dag;
	header.kv_delim = this->m_kv_delim;
	const void* dataPtrs[8] = { NULL };
	prepare_save_mmap(&header, dataPtrs);
	assert(15 == header.magic_len);
	assert(sizeof(header) == header.blocks[0].offset);
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
	for(size_t i = 0, n = header.num_blocks; i < n; ++i) {
		assert(header.blocks[i].offset % 64 == 0);
		const byte_t* data = reinterpret_cast<const byte_t*>(dataPtrs[i]);
		toWrite = header.blocks[i].length;
		written = 0;
		while (written < toWrite) {
			size_t len1 = toWrite - written;
			size_t len2 = ::write(fd, data + written, len1);
			if (len2 != len1 && 0 != errno) {
				THROW_STD(length_error,
					"write(fd=%d, len=%zd) = %zd : %m", fd, len1, len2);
			}
			written += len2;
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
	if (::fsync(fd) < 0) {
		THROW_STD(length_error, "fsync(fd=%d) = %m", fd);
	}
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
			"error: open(%s, O_CREAT|O_WRONLY|O_LARGEFILE, 0644) = %m", fname);
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
	const DFA_ClassMetaInfo* meta = DFA_ClassMetaInfo::find(className);
	if (meta) {
		std::unique_ptr<DFA_Interface> dfa(meta->create());
		meta->load(dio, &*dfa);
		return dfa.release();
	}
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
	const DFA_ClassMetaInfo* meta = DFA_ClassMetaInfo::find(this);
	if (meta) {
		dio << fstring(meta->class_name);
		meta->save(dio, this);
	} else
	FEBIRD_THROW(std::invalid_argument,
		": unknown dfa_class: %s", typeid(*this).name());
}

void DFA_Interface::swap(DFA_Interface& y) {
	assert(typeid(*this) == typeid(y));
#define DO_SWAP(f) { auto t = f; f = y.f; y.f = t; }
   	DO_SWAP(mmap_base);
   	DO_SWAP(mmap_fd);
   	DO_SWAP(m_kv_delim);
   	DO_SWAP(m_is_dag);
   	DO_SWAP(m_fake_mmap);
#undef DO_SWAP
}

//#include <cxxabi.h>
// __cxa_demangle(const char* __mangled_name, char* __output_buffer, size_t* __length, int* __status);

/////////////////////////////////////////////////////////
//
DFA_MutationInterface::DFA_MutationInterface() {}
DFA_MutationInterface::~DFA_MutationInterface() {}

// this all-O(n) maybe a slow implement
// a faster override maybe best-O(1) and worst-O(n)
void DFA_MutationInterface::del_move(size_t s, auchar_t ch) {
	CharTarget<size_t> moves[512]; // max possible sigma
//	printf("del_move------------: c=%c:%02X\n", ch, ch);
	size_t n = get_DFA_Interface()->get_all_move(s, moves);
	size_t i = 0;
	for (size_t j = 0; j < n; ++j) {
		if (moves[j].ch != ch)
			moves[i++] = moves[j];
	}
	del_all_move(s);
	add_all_move(s, moves, i);
}

AC_Scan_Interface::AC_Scan_Interface() {
	n_words = 0;
}
AC_Scan_Interface::~AC_Scan_Interface() {
}

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


