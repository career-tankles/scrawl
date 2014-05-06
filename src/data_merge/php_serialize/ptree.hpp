#ifndef __febird_ptree_hpp__
#define __febird_ptree_hpp__

#include "objbox.hpp"
#include <febird/hash_strmap.hpp>
#include <febird/gold_hash_map.hpp>
//#include <tr1/unordered_map>
#include <boost/function.hpp>

namespace febird {
	namespace objbox {

	class ptree : public obj {
	public:
		hash_strmap<obj_ptr> children;
		hash_strmap<obj_ptr> attributes;

		~ptree();

		const ptree& get_boxed() const { return *this; }
		      ptree& get_boxed()       { return *this; }

		obj* get(fstring path) const;

		      fstring get(fstring path,       fstring Default) const;
		  signed  int get(fstring path,   signed  int Default) const;
		unsigned  int get(fstring path, unsigned  int Default) const;
		  signed long get(fstring path,   signed long Default) const;
		unsigned long get(fstring path, unsigned long Default) const;
		  signed long long get(fstring path,   signed long long Default) const;
		unsigned long long get(fstring path, unsigned long long Default) const;
		        float get(fstring path,         float Default) const;
		       double get(fstring path,        double Default) const;
		  long double get(fstring path,   long double Default) const;

		obj_ptr* add(fstring path);
		obj_ptr* set(fstring path, const boost::function<bool(fstring, obj**)>& on_node);
		void set(fstring path, const std::string& val);
		void set(fstring path,       fstring val);
		void set(fstring path,   signed  int val);
		void set(fstring path, unsigned  int val);
		void set(fstring path,   signed long val);
		void set(fstring path, unsigned long val);
		void set(fstring path,   signed long long val);
		void set(fstring path, unsigned long long val);
		void set(fstring path,         float val);
		void set(fstring path,        double val);
		void set(fstring path,   long double val);

		void for_each(fstring base_dir, const boost::function<void(fstring name, obj&)>& op);
	private:
		void for_each_loop(std::string& path, const boost::function<void(fstring name, obj&)>& op);
	};

	class php_object : public obj {
	public:
		hash_strmap<obj_ptr> fields;
		std::string cls_name;
		php_object();
		~php_object();
		const php_object& get_boxed() const { return *this; }
			  php_object& get_boxed()       { return *this; }
	};
	
	struct php_hash {
		size_t operator()(const obj_ptr&) const;
	};
	struct php_equal {
		bool operator()(const obj_ptr&, const obj_ptr&) const;
	};
	// php_array is a general associated array
	typedef gold_hash_map<obj_ptr, obj_ptr, php_hash, php_equal> php_array_base;
	FEBIRD_BOXING_OBJECT_DERIVE(php_array, php_array_base)

	template<> struct obj_val<     ptree> { typedef      ptree type; };
	template<> struct obj_val<   fstring> { typedef obj_string type; };
	template<> struct obj_val<php_object> { typedef php_object type; };

	obj* php_load(const char** beg, const char* end);
	void php_save(const obj* self, std::string* out);
	void php_save_as_json(const obj* self, std::string* out);
	void php_append(const obj* self, std::string* out);
	void php_append_json(const obj* self, std::string* out);
    std::string json_escape_string(const char *str);

} // namespace objbox

} // namespace febird

#endif // __febird_ptree_hpp__

