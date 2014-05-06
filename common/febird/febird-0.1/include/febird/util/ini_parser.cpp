/* vim: set tabstop=4 : */
//#include "stdafx.h"
#include "ini_parser.hpp"

#include <assert.h>
#include <string.h>

#include <fstream>

#include <map>
#include <set>
#include <vector>
#include <limits>
#include <limits.h>
#include <stdexcept>
#include <algorithm>

//#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)
# if _MSC_VER > 1300
#  define conf_parser_stricmp _stricmp
# else
#  define conf_parser_stricmp stricmp
# endif
#else
# define conf_parser_stricmp strcasecmp
#endif

namespace febird {

class ini_parser_impl
{
public:
	ini_parser_impl(const std::string& filename, bool writable)
		: m_linebuf(4096)
	{
		m_isModified = false;
		m_writable = writable;
		m_filename = filename;
	}

protected:
	static void trim_right(std::string& x)
	{
		while (!x.empty() && isspace(*(x.end()-1)))
			x.resize(x.size() - 1);
	}

	void match(char ch)
	{
		if (ch != *m_pos)
		{
			string_appender<> oss;
			oss << ch << " expected at (line=" << m_lineno << ", col=" << (m_pos-&*m_linebuf.begin()) << ")";
			throw std::logic_error(oss.str().c_str());
		}
	}

	struct item_t
	{
		int lineno;

		explicit item_t(int lineno) : lineno(lineno) {}

		virtual void write_line(std::ostream& file) {}
		virtual ~item_t() {}
	};
	struct val_item : public item_t
	{
		std::string key;
		std::string val;

		val_item(const std::string& key, const std::string& val, int lineno)
			: item_t(lineno), key(key), val(val) {}
		val_item(const std::string& key)
			: item_t(INT_MAX), key(key) {}
		val_item() : item_t(INT_MAX) {}

		virtual void write_line(std::ostream& file)
		{
			file << key << '=' << val;
		}
		bool operator<(const val_item& y) const
		{
			return conf_parser_stricmp(key.c_str(), y.key.c_str()) < 0;
		}
	};
	typedef std::set<val_item> kvmap_t;
	struct section_item : public item_t
	{
		kvmap_t kvmap;
		int lastLine; //!< lineno of last key-value

		//! compare string no case
		bool operator()(const val_item& x, const val_item& y) const
		{
			return conf_parser_stricmp(x.key.c_str(), y.key.c_str()) < 0;
		}
		section_item(int lineno = INT_MAX) : item_t(lineno) {}
	};
	typedef std::map<std::string, section_item> sec_map_t;

	struct comp_sec_ptr
	{
		bool operator()(const sec_map_t::value_type* left, const sec_map_t::value_type* right) const
		{ return left->second.lineno < right->second.lineno; }
	};

	sec_map_t m_sections;

	std::string  m_filename;
	std::vector<char> m_linebuf;
	char* m_pos;
	int m_lineno;
	section_item* m_cursec;

	struct comment_item : public item_t
	{
		std::string comment;

		comment_item(int lineno, const std::string& text) : item_t(lineno), comment(text) {}
	};
	struct compare_lineno
	{
		template<class Val>
		bool operator()(const Val& left, const Val& right) const
		{ return left.lineno < right.lineno; }

		template<class Val>
		bool operator()(const Val* left, const Val* right) const
		{ return left->lineno < right->lineno; }
	};
	typedef std::set<comment_item, compare_lineno> comment_set_t;
	comment_set_t m_comments;

	bool m_writable; //! ��������־Ϊ false, �޸�ֻ���ڴ��н��У�����д�뵽�ļ�
	bool m_isModified;

public:
	size_t section_count() const
	{
		return m_sections.size();
	}

	size_t key_val_count(const std::string& section) const
	{
		sec_map_t::const_iterator sec = m_sections.find(section);
		if (m_sections.end() == sec) return 0;

		return (*sec).second.kvmap.size();
	}

	void parse_file()
	{
		m_lineno = 0;
		m_cursec = 0;

		std::ifstream file(m_filename.c_str());
		if (!file.is_open())
		{
			string_appender<> oss;
			oss << "can not open config file: " << m_filename;
			throw std::runtime_error(oss.str());
		}
		while (!file.eof())
		{
			file.getline(&*m_linebuf.begin(), std::streamsize(m_linebuf.size()));
			char* p = &*m_linebuf.begin();
			while (*p && '\r'!=*p && '\n'!=*p)
				++p;
			*p = 0;
			parse_line();
			++m_lineno;
		}
		file.close();
	}

	virtual void parse_line()
	{
		using namespace std;

		m_pos = &*m_linebuf.begin();

		if (read_comment())
		{
			// whole line comment
			;// do nothing...
		}
		else if ('[' == *m_pos)
		{
			std::string section;
			++m_pos;
			while (isspace(*m_pos) && 0 != *m_pos) ++m_pos;
			while (']'  != *m_pos && 0 != *m_pos)
				section.append(1, *m_pos++);
			match(']');
			trim_right(section);
			m_pos++;

			std::pair<sec_map_t::iterator, bool> ib =
				m_sections.insert(make_pair(section, section_item(m_lineno)));
			if (!ib.second)
			{
				string_appender<> oss;
				oss << "duplicate section:" << section << " at line:" << m_lineno;
				throw std::logic_error(oss.str().c_str());
			}
			m_cursec = &(*ib.first).second;

			read_comment(); // comment after section line
		}
		else // key-value
		{
			char* pEnd = m_pos;
			std::string key, val;
			while ('=' != *pEnd && 0 != *pEnd)
				++pEnd;
			key.assign(m_pos, pEnd);
			m_pos = pEnd;
			trim_right(key);
			match('=');
			m_pos++;
			while (isspace(*m_pos) && 0 != *m_pos) ++m_pos;

			if ('"' == *m_pos || '\'' == *m_pos)
			{
				pEnd = m_pos;
				const char quote = *m_pos;
				while (*pEnd && quote != *pEnd)
					++pEnd;
				// ���Ա�����ȱʧ�Ĵ���
				val.assign(m_pos+1, quote == pEnd[-1] ? pEnd-1 : pEnd);
				m_pos = pEnd;
			}
			else
			{
				pEnd = m_pos;
				while (*pEnd && '#' != *pEnd && ';' != *pEnd)
					++pEnd;
				val.assign(m_pos, pEnd);
				m_pos = pEnd;
			}

			trim_right(val);

			if (0 == m_cursec)
			{
				string_appender<> oss;
				oss << "(key=\"" << key << "\", val=\"" << val << "\") is not in a section";
				throw std::logic_error(oss.str().c_str());
			}
			m_cursec->kvmap.insert(val_item(key, val, m_lineno));

			read_comment(); // comment after key-value line
		}
	}

	std::string getstr(const std::string& section, const std::string& key, const std::string& defaultVal) const
	{
		sec_map_t::const_iterator sec = m_sections.find(section);
		if (m_sections.end() == sec) return defaultVal;
		kvmap_t::const_iterator kv = (*sec).second.kvmap.find(key);
		if ((*sec).second.kvmap.end() == kv) return defaultVal;
		return (*kv).val;
	}
	void flush()
	{
		if (m_writable && m_isModified)
		{
			std::ofstream file(m_filename.c_str());
			write_file_with_comment(file);
			m_isModified = false;
		}
	}

	bool read_comment()
	{
		const char* comment_begin = m_pos;

		while (isspace(*m_pos) && 0 != *m_pos) ++m_pos;

		const char commentMark[] = "#;\r\n";

		if (0 == *m_pos)
			return true;
		if (0 == strchr(commentMark, *m_pos))
			// not comment
			return false;

		const bool hasLeadingSpace = m_pos != comment_begin;

		// ����Ҳ��Ϊ��ע�ͣ������������
		// 1. \0
		// 2. SPSP\0
		// 3. #;\0
		// 4. SP#;\0
		if (0 == *m_linebuf.begin() ||  // 1
			hasLeadingSpace         ||  // 2
			'#' == *m_pos || ';' == *m_pos ||  // 3,4
			0)
		{
			const char* p;
			for (p = m_pos; *p; ++p)
				if ('\r' == *p || '\n' == *p) break;
			m_comments.insert(comment_item(m_lineno, std::string(comment_begin, p)));
		}
		return true;
	}

	void setstr(const std::string& section, const std::string& key, const std::string& val)
	{
		m_isModified = true;
		section_item& sec = m_sections[section];
		std::pair<kvmap_t::iterator, bool> ib = sec.kvmap.insert(val_item(key, val, INT_MAX));
		if (!ib.second)
			const_cast<std::string&>(ib.first->val) = val;
	}

	void write_comment(std::ostream& os, int lineno)
	{
		comment_set_t::iterator iter = m_comments.find(comment_item(lineno, ""));
		if (m_comments.end() != iter)
		{
			os << (*iter).comment;
		}
	}

	void write_comment_multi_line(std::ostream& os, int beg_line, int end_line)
	{
		comment_set_t::iterator lower = m_comments.lower_bound(comment_item(beg_line,""));
		comment_set_t::iterator upper = m_comments.upper_bound(comment_item(end_line,""));

		for (comment_set_t::iterator iter = lower; iter != upper; ++iter)
		{
			os << (*iter).comment << "\n";
		}
	}

	/**
	 @brief д�������ļ�

	  д�ļ�ʱ��������ע��
	 */
	void write_file_with_comment(std::ostream& os)
	{
		if (m_sections.empty())
		{
			write_comment_multi_line(os, 0, INT_MAX);
			return;
		}
		typedef std::vector<sec_map_t::value_type*> vsec_t;
		vsec_t vsec;
		vsec.reserve(m_sections.size());
		for (sec_map_t::iterator i = m_sections.begin(); i != m_sections.end(); ++i)
		{
			vsec.push_back(&*i);
		}
		std::sort(vsec.begin(), vsec.end(), comp_sec_ptr());

		assert(!vsec.empty());
		write_comment_multi_line(os, 0, vsec[0]->second.lineno);

		for (vsec_t::iterator i = vsec.begin(); i != vsec.end(); ++i)
		{
			// write section
			os << "[" << (**i).first << "]";
			write_comment(os, (**i).second.lineno);
			os << "\n";

			if ((**i).second.kvmap.empty())
			{
				write_comment_multi_line(os, (**i).second.lineno, i == vsec.end()-1 ? INT_MAX : i[1]->second.lineno);
				continue;
			}

			// write key-value pairs...
			kvmap_t& kvm = (**i).second.kvmap;
			typedef std::vector<const kvmap_t::value_type*> vkvm_t;
			vkvm_t vkvm(kvm.size());
			vkvm_t::iterator k = vkvm.begin();
			for (kvmap_t::iterator j = kvm.begin(); j != kvm.end(); ++j, ++k)
			{
				*k = &*j;
			}
			std::sort(vkvm.begin(), vkvm.end(), compare_lineno());
			k = vkvm.begin();
			if (INT_MAX != (**i).second.lineno)
				write_comment_multi_line(os, (**i).second.lineno+1, (**k).lineno);

			for (k = vkvm.begin(); k != vkvm.end(); ++k)
			{
				os << (**k).key << "=" << (**k).val;
				write_comment(os, (**k).lineno);
				os << "\n";

				if (k != vkvm.end()-1 && INT_MAX != (**k).lineno)
					write_comment_multi_line(os, k[0]->lineno+1, k[1]->lineno);
			}
		}
	}

	void close()
	{
		flush();
		m_sections.clear();
		m_comments.clear();
		m_cursec = 0;
	}
};

//////////////////////////////////////////////////////////////////////////

ini_parser::ini_parser(const std::string& filename, bool writable)
{
	open(filename, writable);
}

ini_parser::~ini_parser()
{
	close();
}

void ini_parser::open(const std::string& filename, bool writable)
{
	m_parser.reset(new ini_parser_impl(filename, writable));
	m_parser->parse_file();
}

//! note: this function is no thread-safe
const char* ini_parser::getcstr(const std::string& section, const std::string& key, const std::string& defaultVal) const
{
	m_tmpcstr = m_parser->getstr(section, key, defaultVal);
	return m_tmpcstr.c_str();
}

std::string ini_parser::getstr(const std::string& section, const std::string& key, const std::string& defaultVal) const
{
	return m_parser->getstr(section, key, defaultVal);
}

void ini_parser::setstr(const std::string& section, const std::string& key, const std::string& val)
{
	m_parser->setstr(section, key, val);
}

int ini_parser::getint(const std::string& section, const std::string& key, int defaultVal) const
{
	const char* szInt = getcstr(section, key);
	if (0 == *szInt)
		return defaultVal;
	else
		return atoi(szInt);
}
void ini_parser::setint(const std::string& section, const std::string& key, int val)
{
	char szVal[32] = {0};
	sprintf(szVal, "%d", val);
	m_parser->setstr(section, key, std::string(szVal));
}

size_t ini_parser::section_count() const
{
	return m_parser->section_count();
}

size_t ini_parser::key_val_count(const std::string& section) const
{
	return m_parser->key_val_count(section);
}

void ini_parser::close()
{
	return m_parser->close();
}

void ini_parser::flush()
{
	return m_parser->flush();
}

} // namespace febird
