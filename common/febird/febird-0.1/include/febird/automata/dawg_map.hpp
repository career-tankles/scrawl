#ifndef __febird_dawg_map_hpp__
#define __febird_dawg_map_hpp__

#include "dfa_interface.hpp"
#include <febird/bitmap.hpp>
#include <febird/valvec.hpp>
#include <febird/util/linebuf.hpp>
#include <febird/io/DataIO.hpp>
#include <febird/io/FileStream.hpp>
#include <febird/io/StreamBuffer.hpp>
#include <febird/num_to_str.hpp>

template<class Value>
class DAWG_Map : boost::noncopyable {
	DAWG_Interface* dawg;
	valvec<Value> values;
	bool is_patched;

public:
	explicit DAWG_Map(DAWG_Interface* dawg) : dawg(dawg) {
		assert(NULL != dawg);
		is_patched = false;
	}
	DAWG_Map() {
		dawg = NULL;
		is_patched = false;
	}
	~DAWG_Map() {
		delete dawg;
	}
	const DAWG_Interface* get_dawg() const { return dawg; }

	size_t size() const {
		assert(NULL != dawg);
		assert(values.size() == dawg->num_words());
	   	return values.size();
   	}
	
	std::string key(size_t nth) const {
		assert(NULL != dawg);
		return dawg->nth_word(nth);
   	}
	const Value& val(size_t nth) const {
		assert(NULL != dawg);
		assert(values.size() == dawg->num_words());
		assert(nth < values.size());
		return values[nth];
   	}
	Value& val(size_t nth) {
		assert(NULL != dawg);
		assert(values.size() == dawg->num_words());
		assert(nth < values.size());
		return values[nth];
   	}

	const Value* find(fstring key) const {
		assert(NULL != dawg);
		assert(values.size() == dawg->num_words());
		size_t nth = dawg->index(key);
		if (dawg->null_word == nth)
			return NULL;
		else
			return &values[nth];
	}
	Value* find(fstring key) {
		assert(NULL != dawg);
		assert(values.size() == dawg->num_words());
		size_t nth = dawg->index(key);
		if (dawg->null_word == nth)
			return NULL;
		else
			return &values[nth];
	}

	template<class OnKeyValue, class ValueVec>
	struct OnKeyValueOP {
		OnKeyValue onKeyVal;
		ValueVec values;
		void operator()(size_t nth, fstring key) const {
			assert(nth < values.size());
			onKeyVal(nth, key, values[nth]);
		}
		OnKeyValueOP(OnKeyValue onKV, ValueVec vv)
		   	: onKeyVal(onKV), values(vv) {}
	};

	// OnKeyValue(nth, key, value)
	template<class OnKeyValue>
	void for_each(OnKeyValue onKeyVal) const {
		const DFA_Interface* dfa = dynamic_cast<const DFA_Interface*>(dawg);
		if (NULL == dfa) {
			throw std::logic_error("Unexpected: DAWG is not a DFA");
		}
		OnKeyValueOP<OnKeyValue, const valvec<Value>&> on_word0(onKeyVal, values);
		boost::function<void(size_t nth, fstring)> on_word(boost::ref(on_word0));
		dfa->for_each_word(on_word);
	}
	template<class OnKeyValue>
	void for_each(OnKeyValue onKeyVal) {
		const DFA_Interface* dfa = dynamic_cast<const DFA_Interface*>(dawg);
		if (NULL == dfa) {
			throw std::logic_error("Unexpected: DAWG is not a DFA");
		}
		OnKeyValueOP<OnKeyValue, valvec<Value>&> on_word0(onKeyVal, values);
		boost::function<void(size_t nth, fstring)> on_word(boost::ref(on_word0));
		dfa->for_each_word(on_word);
	}

	// dawg_file will be appended the values
	template<class ParseLine>
	void complete_dawg_map(fstring text_key_value_file, fstring bin_dawg_file, ParseLine parse_line, bool overwriteValues = true) {
		using namespace febird;
		assert(NULL == dawg);
		assert('\0' == *text_key_value_file.end());
		assert('\0' == *bin_dawg_file.end());
		FileStream dawg_fp(bin_dawg_file.c_str(), "rb+");
		DFA_Interface* dfa;
		{
			NativeDataInput<SeekableInputBuffer> dio; dio.attach(&dawg_fp);
			dfa = DFA_Interface::load_from_NativeInputBuffer(&dio);
			dawg = dynamic_cast<DAWG_Interface*>(dfa);
			if (NULL == dawg) {
				delete dfa;
				throw std::logic_error("DAWG_Map::load_from(), not a dawg");
			}
			long long dawg_dfa_bytes = dio.tell();
			long long dawg_file_size = dawg_fp.size();
			if (dawg_file_size != dawg_dfa_bytes && !overwriteValues) {
				string_appender<> msg;
				msg << "DAWG_Map::complete_dawg_map("
					<< text_key_value_file.c_str()
					<< ", "
					<< bin_dawg_file.c_str()
					<< ", parse_line, overwriteValues=false): "
					<< "dawg_file_size=" << dawg_file_size
					<< "dawg_dfa_bytes=" << dawg_dfa_bytes
					<< "\n";
				fputs(msg.c_str(), stderr);
				throw std::logic_error(msg);
			}
			dawg_fp.seek(dawg_dfa_bytes);
		}
		FileStream text_fp(text_key_value_file.c_str(), "r");
		patch_values<ParseLine>(text_fp, parse_line);
		NativeDataOutput<OutputBuffer> dio; dio.attach(&dawg_fp);
		dio << values;
	}

//	void patch_values(FILE* text_fp, const boost::function<void(fstring line, fstring* pKey, Value* pVal)>& parse_line)
	template<class ParseLine>
	void patch_values(FILE* text_fp, ParseLine parse_line) {
		assert(NULL != dawg);
		assert(values.empty());
		values.resize(dawg->num_words());
		febird::LineBuf line;
		fstring theKey;
		Value theVal;
		simplebitvec bits(values.size(), 0, 1);
		while (line.getline(text_fp) > 0) {
			parse_line(fstring(line), &theKey, &theVal);
			size_t idx = dawg->index(theKey);
			if (dawg->null_word == idx) {
				throw std::logic_error("");
			}
			assert(idx < values.size());
			bits.set1(idx);
			values[idx] = theVal;
		}
		if (!bits.isall1()) {
			throw std::logic_error("some values were not padded");
		}
		is_patched = true;
	}
	void append_values_to_file(fstring fname) {
		assert(*fname.end() == '\0');
		assert(NULL != dawg);
		assert(values.size() == dawg->num_words());
		using namespace febird;
		FileStream fp(fname.c_str(), "ab");
		NativeDataOutput<OutputBuffer> dio; dio.attach(&fp);
		dio << values;
	}
	template<class File> // fname or FILE*
	void load_dawg(File file) {
		assert(NULL == dawg);
		DFA_Interface* dfa = DFA_Interface::load_from(file);
		dawg = dynamic_cast<DAWG_Interface*>(dfa);
		if (NULL == dawg) {
			delete dfa;
			throw std::logic_error("DAWG_Map::load_dawg(), not a dawg");
		}
	}
	void load_map(fstring fname) {
		using namespace febird;
		assert(*fname.end() == '\0');
		FileStream fp(fname.c_str(), "rb");
		NativeDataInput<InputBuffer> dio; dio.attach(&fp);
		DFA_Interface* dfa = DFA_Interface::load_from_NativeInputBuffer(&dio);
		dawg = dynamic_cast<DAWG_Interface*>(dfa);
		if (NULL == dawg) {
			delete dfa;
			throw std::logic_error("DAWG_Map::load_from(), not a dawg");
		}
		dio >> values;
		assert(values.size() == dawg->num_words());
		is_patched = true;
	}
	void save_map(fstring fname) {
		assert(*fname.end() == '\0');
		assert(NULL != dawg);
		assert(values.size() == dawg->num_words());
		using namespace febird;
		FileStream fp(fname.c_str(), "wb");
		DFA_Interface* dfa = dynamic_cast<DFA_Interface*>(dawg);
		assert(NULL != dfa);
		dfa->save_to(fp);
		NativeDataOutput<OutputBuffer> dio; dio.attach(&fp);
		dio << values;
	}
};

/*
struct fstring;

// abstract interface
// could handle and sized value, but all value in a dawg_map are the same size
struct dawg_map_i {
	typedef boost::function<void(      void* value)> OnExisted;
	typedef boost::function<void(const void* found)> OnFound;
	typedef boost::function<void(const fstring& key, void* val)> OnEachKV;

	virtual ~dawg_map_i();

	virtual bool insert(const fstring& word, const void* val, const OnExisted& on_existed) = 0;
	virtual bool find(const fstring& word, const OnFound& on_found);
	virtual void compact() = 0;

	virtual bool   empty() const = 0;
	virtual size_t size () const = 0;

	virtual bool for_each(const OnEachKV& on_each) = 0;

	static dawg_map_i* create(const char* state_class, size_t value_size, const char* tmpDataFile);
};

// thin template wrapper on dawg_map_i
// @note Value must be simple value, it must be memmovable
// if value has some allocated resource, must use for_each before destruct
template<class Value>
class dawg_map {
	std::auto_ptr<dawg_map_i> handle;

public:
	typedef boost::function<void(      Value& value)> OnExisted;
	typedef boost::function<void(const Value& value)> OnFound;
	typedef boost::function<void(const fstring& key, Value& val)> OnEachKV;

	dawg_map(const char* state_class, const char* tmpDataFile) {
		handle.reset(dawg_map_i::create(state_class, sizeof(Value), tmpDataFile));
	}
	bool insert(const fstring& word, const Value& val, const OnExisted& on_existed) {
		return handle->insert(word, &val,
				reinterpret_cast<dawg_map_i::OnExisted>(on_existed));
	}
	bool find(const fstring& word, const OnFound& on_found) const {
		return handle->find(word, reinterpret_cast<dawg_map_i::OnFound>(on_found));
	}
	void compact() {
		handle->compact();
	}
	bool   empty() const { return handle->empty(); }
	size_t size () const { return handle->size (); }

	void for_each(const OnEachKV& on_each) {
		handle->for_each(reinterpret_cast<dawg_map_i::OnEachKV>(on_each));
	}
};
*/

#endif // __febird_dawg_map_hpp__

