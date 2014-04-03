/**
 * pugixml parser - version 1.4
 * --------------------------------------------------------
 * Copyright (C) 2006-2014, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
 * Report bugs and download new versions at http://pugixml.org/
 *
 * This library is distributed under the MIT License. See notice at the end
 * of this file.
 *
 * This work is based on the pugxml parser, which is:
 * Copyright (C) 2003, by Kristen Wegner (kristen@tima.net)
 */

#ifndef SOURCE_PUGIXML_CPP
#define SOURCE_PUGIXML_CPP

#include "pugixml.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef PUGIXML_WCHAR_MODE
#	include <wchar.h>
#endif

#ifndef PUGIXML_NO_XPATH
#	include <math.h>
#	include <float.h>
#	ifdef PUGIXML_NO_EXCEPTIONS
#		include <setjmp.h>
#	endif
#endif

#ifndef PUGIXML_NO_STL
#	include <istream>
#	include <ostream>
#	include <string>
#endif

// For placement new
#include <new>

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable: 4127) // conditional expression is constant
#	pragma warning(disable: 4324) // structure was padded due to __declspec(align())
#	pragma warning(disable: 4611) // interaction between '_setjmp' and C++ object destruction is non-portable
#	pragma warning(disable: 4702) // unreachable code
#	pragma warning(disable: 4996) // this function or variable may be unsafe
#	pragma warning(disable: 4793) // function compiled as native: presence of '_setjmp' makes a function unmanaged
#endif

#ifdef __INTEL_COMPILER
#	pragma warning(disable: 177) // function was declared but never referenced 
#	pragma warning(disable: 279) // controlling expression is constant
#	pragma warning(disable: 1478 1786) // function was declared "deprecated"
#	pragma warning(disable: 1684) // conversion from pointer to same-sized integral type
#endif

#if defined(__BORLANDC__) && defined(PUGIXML_HEADER_ONLY)
#	pragma warn -8080 // symbol is declared but never used; disabling this inside push/pop bracket does not make the warning go away
#endif

#ifdef __BORLANDC__
#	pragma option push
#	pragma warn -8008 // condition is always false
#	pragma warn -8066 // unreachable code
#endif

#ifdef __SNC__
// Using diag_push/diag_pop does not disable the warnings inside templates due to a compiler bug
#	pragma diag_suppress=178 // function was declared but never referenced
#	pragma diag_suppress=237 // controlling expression is constant
#endif

// Inlining controls
#if defined(_MSC_VER) && _MSC_VER >= 1300
#	define PUGI__NO_INLINE __declspec(noinline)
#elif defined(__GNUC__)
#	define PUGI__NO_INLINE __attribute__((noinline))
#else
#	define PUGI__NO_INLINE 
#endif

// Simple static assertion
#define PUGI__STATIC_ASSERT(cond) { static const char condition_failed[(cond) ? 1 : -1] = {0}; (void)condition_failed[0]; }

// Digital Mars C++ bug workaround for passing char loaded from memory via stack
#ifdef __DMC__
#	define PUGI__DMC_VOLATILE volatile
#else
#	define PUGI__DMC_VOLATILE
#endif

// Borland C++ bug workaround for not defining ::memcpy depending on header include order (can't always use std::memcpy because some compilers don't have it at all)
#if defined(__BORLANDC__) && !defined(__MEM_H_USING_LIST)
using std::memcpy;
using std::memmove;
#endif

// In some environments MSVC is a compiler but the CRT lacks certain MSVC-specific features
#if defined(_MSC_VER) && !defined(__S3E__)
#	define PUGI__MSVC_CRT_VERSION _MSC_VER
#endif

#ifdef PUGIXML_HEADER_ONLY
#	define PUGI__NS_BEGIN namespace pugi { namespace impl {
#	define PUGI__NS_END } }
#	define PUGI__FN inline
#	define PUGI__FN_NO_INLINE inline
#else
#	if defined(_MSC_VER) && _MSC_VER < 1300 // MSVC6 seems to have an amusing bug with anonymous namespaces inside namespaces
#		define PUGI__NS_BEGIN namespace pugi { namespace impl {
#		define PUGI__NS_END } }
#	else
#		define PUGI__NS_BEGIN namespace pugi { namespace impl { namespace {
#		define PUGI__NS_END } } }
#	endif
#	define PUGI__FN
#	define PUGI__FN_NO_INLINE PUGI__NO_INLINE
#endif

// uintptr_t
#if !defined(_MSC_VER) || _MSC_VER >= 1600
#	include <stdint.h>
#else
#	ifndef _UINTPTR_T_DEFINED
// No native uintptr_t in MSVC6 and in some WinCE versions
typedef size_t uintptr_t;
#define _UINTPTR_T_DEFINED
#	endif
PUGI__NS_BEGIN
	typedef unsigned __int8 uint8_t;
	typedef unsigned __int16 uint16_t;
	typedef unsigned __int32 uint32_t;
PUGI__NS_END
#endif

// Memory allocation
PUGI__NS_BEGIN
	PUGI__FN void* default_allocate(size_t size)
	{
		return malloc(size);
	}

	PUGI__FN void default_deallocate(void* ptr)
	{
		free(ptr);
	}

	template <typename T>
	struct xml_memory_management_function_storage
	{
		static allocation_function allocate;
		static deallocation_function deallocate;
	};

	template <typename T> allocation_function xml_memory_management_function_storage<T>::allocate = default_allocate;
	template <typename T> deallocation_function xml_memory_management_function_storage<T>::deallocate = default_deallocate;

	typedef xml_memory_management_function_storage<int> xml_memory;
PUGI__NS_END

// String utilities
PUGI__NS_BEGIN
	// Get string length
	PUGI__FN size_t strlength(const char_t* s)
	{
		assert(s);

	#ifdef PUGIXML_WCHAR_MODE
		return wcslen(s);
	#else
		return strlen(s);
	#endif
	}

	// Compare two strings
	PUGI__FN bool strequal(const char_t* src, const char_t* dst)
	{
		assert(src && dst);

	#ifdef PUGIXML_WCHAR_MODE
		return wcscmp(src, dst) == 0;
	#else
		return strcmp(src, dst) == 0;
	#endif
	}

	// Compare lhs with [rhs_begin, rhs_end)
	PUGI__FN bool strequalrange(const char_t* lhs, const char_t* rhs, size_t count)
	{
		for (size_t i = 0; i < count; ++i)
			if (lhs[i] != rhs[i])
				return false;
	
		return lhs[count] == 0;
	}

	// Get length of wide string, even if CRT lacks wide character support
	PUGI__FN size_t strlength_wide(const wchar_t* s)
	{
		assert(s);

	#ifdef PUGIXML_WCHAR_MODE
		return wcslen(s);
	#else
		const wchar_t* end = s;
		while (*end) end++;
		return static_cast<size_t>(end - s);
	#endif
	}

#ifdef PUGIXML_WCHAR_MODE
	// Convert string to wide string, assuming all symbols are ASCII
	PUGI__FN void widen_ascii(wchar_t* dest, const char* source)
	{
		for (const char* i = source; *i; ++i) *dest++ = *i;
		*dest = 0;
	}
#endif
PUGI__NS_END

#if !defined(PUGIXML_NO_STL) || !defined(PUGIXML_NO_XPATH)
// auto_ptr-like buffer holder for exception recovery
PUGI__NS_BEGIN
	struct buffer_holder
	{
		void* data;
		void (*deleter)(void*);

		buffer_holder(void* data_, void (*deleter_)(void*)): data(data_), deleter(deleter_)
		{
		}

		~buffer_holder()
		{
			if (data) deleter(data);
		}

		void* release()
		{
			void* result = data;
			data = 0;
			return result;
		}
	};
PUGI__NS_END
#endif

PUGI__NS_BEGIN
	static const size_t xml_memory_page_size =
	#ifdef PUGIXML_MEMORY_PAGE_SIZE
		PUGIXML_MEMORY_PAGE_SIZE
	#else
		32768
	#endif
		;

	static const uintptr_t xml_memory_page_alignment = 32;
	static const uintptr_t xml_memory_page_pointer_mask = ~(xml_memory_page_alignment - 1);
	static const uintptr_t xml_memory_page_name_allocated_mask = 16;
	static const uintptr_t xml_memory_page_value_allocated_mask = 8;
	static const uintptr_t xml_memory_page_type_mask = 7;

	struct xml_allocator;

	struct xml_memory_page
	{
		static xml_memory_page* construct(void* memory)
		{
			if (!memory) return 0; //$ redundant, left for performance

			xml_memory_page* result = static_cast<xml_memory_page*>(memory);

			result->allocator = 0;
			result->memory = 0;
			result->prev = 0;
			result->next = 0;
			result->busy_size = 0;
			result->freed_size = 0;

			return result;
		}

		xml_allocator* allocator;

		void* memory;

		xml_memory_page* prev;
		xml_memory_page* next;

		size_t busy_size;
		size_t freed_size;

		char data[1];
	};

	struct xml_memory_string_header
	{
		uint16_t page_offset; // offset from page->data
		uint16_t full_size; // 0 if string occupies whole page
	};

	struct xml_allocator
	{
		xml_allocator(xml_memory_page* root): _root(root), _busy_size(root->busy_size)
		{
		}

		xml_memory_page* allocate_page(size_t data_size)
		{
			size_t size = offsetof(xml_memory_page, data) + data_size;

			// allocate block with some alignment, leaving memory for worst-case padding
			void* memory = xml_memory::allocate(size + xml_memory_page_alignment);
			if (!memory) return 0;

			// align upwards to page boundary
			void* page_memory = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(memory) + (xml_memory_page_alignment - 1)) & ~(xml_memory_page_alignment - 1));

			// prepare page structure
			xml_memory_page* page = xml_memory_page::construct(page_memory);
			assert(page);

			page->memory = memory;
			page->allocator = _root->allocator;

			return page;
		}

		static void deallocate_page(xml_memory_page* page)
		{
			xml_memory::deallocate(page->memory);
		}

		void* allocate_memory_oob(size_t size, xml_memory_page*& out_page);

		void* allocate_memory(size_t size, xml_memory_page*& out_page)
		{
			if (_busy_size + size > xml_memory_page_size) return allocate_memory_oob(size, out_page);

			void* buf = _root->data + _busy_size;

			_busy_size += size;

			out_page = _root;

			return buf;
		}

		void deallocate_memory(void* ptr, size_t size, xml_memory_page* page)
		{
			if (page == _root) page->busy_size = _busy_size;

			assert(ptr >= page->data && ptr < page->data + page->busy_size);
			(void)!ptr;

			page->freed_size += size;
			assert(page->freed_size <= page->busy_size);

			if (page->freed_size == page->busy_size)
			{
				if (page->next == 0)
				{
					assert(_root == page);

					// top page freed, just reset sizes
					page->busy_size = page->freed_size = 0;
					_busy_size = 0;
				}
				else
				{
					assert(_root != page);
					assert(page->prev);

					// remove from the list
					page->prev->next = page->next;
					page->next->prev = page->prev;

					// deallocate
					deallocate_page(page);
				}
			}
		}

		char_t* allocate_string(size_t length)
		{
			// allocate memory for string and header block
			size_t size = sizeof(xml_memory_string_header) + length * sizeof(char_t);
			
			// round size up to pointer alignment boundary
			size_t full_size = (size + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1);

			xml_memory_page* page;
			xml_memory_string_header* header = static_cast<xml_memory_string_header*>(allocate_memory(full_size, page));

			if (!header) return 0;

			// setup header
			ptrdiff_t page_offset = reinterpret_cast<char*>(header) - page->data;

			assert(page_offset >= 0 && page_offset < (1 << 16));
			header->page_offset = static_cast<uint16_t>(page_offset);

			// full_size == 0 for large strings that occupy the whole page
			assert(full_size < (1 << 16) || (page->busy_size == full_size && page_offset == 0));
			header->full_size = static_cast<uint16_t>(full_size < (1 << 16) ? full_size : 0);

			// round-trip through void* to avoid 'cast increases required alignment of target type' warning
			// header is guaranteed a pointer-sized alignment, which should be enough for char_t
			return static_cast<char_t*>(static_cast<void*>(header + 1));
		}

		void deallocate_string(char_t* string)
		{
			// this function casts pointers through void* to avoid 'cast increases required alignment of target type' warnings
			// we're guaranteed the proper (pointer-sized) alignment on the input string if it was allocated via allocate_string

			// get header
			xml_memory_string_header* header = static_cast<xml_memory_string_header*>(static_cast<void*>(string)) - 1;

			// deallocate
			size_t page_offset = offsetof(xml_memory_page, data) + header->page_offset;
			xml_memory_page* page = reinterpret_cast<xml_memory_page*>(static_cast<void*>(reinterpret_cast<char*>(header) - page_offset));

			// if full_size == 0 then this string occupies the whole page
			size_t full_size = header->full_size == 0 ? page->busy_size : header->full_size;

			deallocate_memory(header, full_size, page);
		}

		xml_memory_page* _root;
		size_t _busy_size;
	};

	PUGI__FN_NO_INLINE void* xml_allocator::allocate_memory_oob(size_t size, xml_memory_page*& out_page)
	{
		const size_t large_allocation_threshold = xml_memory_page_size / 4;

		xml_memory_page* page = allocate_page(size <= large_allocation_threshold ? xml_memory_page_size : size);
		out_page = page;

		if (!page) return 0;

		if (size <= large_allocation_threshold)
		{
			_root->busy_size = _busy_size;

			// insert page at the end of linked list
			page->prev = _root;
			_root->next = page;
			_root = page;

			_busy_size = size;
		}
		else
		{
			// insert page before the end of linked list, so that it is deleted as soon as possible
			// the last page is not deleted even if it's empty (see deallocate_memory)
			assert(_root->prev);

			page->prev = _root->prev;
			page->next = _root;

			_root->prev->next = page;
			_root->prev = page;
		}

		// allocate inside page
		page->busy_size = size;

		return page->data;
	}
PUGI__NS_END

namespace pugi
{
	/// A 'name=value' XML attribute structure.
	struct xml_attribute_struct
	{
		/// Default ctor
		xml_attribute_struct(impl::xml_memory_page* page): header(reinterpret_cast<uintptr_t>(page)), name(0), value(0), prev_attribute_c(0), next_attribute(0)
		{
		}

		uintptr_t header;

		char_t* name;	///< Pointer to attribute name.
		char_t*	value;	///< Pointer to attribute value.

		xml_attribute_struct* prev_attribute_c;	///< Previous attribute (cyclic list)
		xml_attribute_struct* next_attribute;	///< Next attribute
	};

	/// An XML document tree node.
	struct xml_node_struct
	{
		/// Default ctor
		/// \param type - node type
		xml_node_struct(impl::xml_memory_page* page, xml_node_type type): header(reinterpret_cast<uintptr_t>(page) | (type - 1)), parent(0), name(0), value(0), first_child(0), prev_sibling_c(0), next_sibling(0), first_attribute(0)
		{
		}

		uintptr_t header;

		xml_node_struct*		parent;					///< Pointer to parent

		char_t*					name;					///< Pointer to element name.
		char_t*					value;					///< Pointer to any associated string data.

		xml_node_struct*		first_child;			///< First child
		
		xml_node_struct*		prev_sibling_c;			///< Left brother (cyclic list)
		xml_node_struct*		next_sibling;			///< Right brother
		
		xml_attribute_struct*	first_attribute;		///< First attribute
	};
}

PUGI__NS_BEGIN
	struct xml_extra_buffer
	{
		char_t* buffer;
		xml_extra_buffer* next;
	};

	struct xml_document_struct: public xml_node_struct, public xml_allocator
	{
		xml_document_struct(xml_memory_page* page): xml_node_struct(page, node_document), xml_allocator(page), buffer(0), extra_buffers(0)
		{
		}

		const char_t* buffer;

		xml_extra_buffer* extra_buffers;
	};

	inline xml_allocator& get_allocator(const xml_node_struct* node)
	{
		assert(node);

		return *reinterpret_cast<xml_memory_page*>(node->header & xml_memory_page_pointer_mask)->allocator;
	}
PUGI__NS_END

// Low-level DOM operations
PUGI__NS_BEGIN
	inline xml_attribute_struct* allocate_attribute(xml_allocator& alloc)
	{
		xml_memory_page* page;
		void* memory = alloc.allocate_memory(sizeof(xml_attribute_struct), page);

		return new (memory) xml_attribute_struct(page);
	}

	inline xml_node_struct* allocate_node(xml_allocator& alloc, xml_node_type type)
	{
		xml_memory_page* page;
		void* memory = alloc.allocate_memory(sizeof(xml_node_struct), page);

		return new (memory) xml_node_struct(page, type);
	}

	inline void destroy_attribute(xml_attribute_struct* a, xml_allocator& alloc)
	{
		uintptr_t header = a->header;

		if (header & impl::xml_memory_page_name_allocated_mask) alloc.deallocate_string(a->name);
		if (header & impl::xml_memory_page_value_allocated_mask) alloc.deallocate_string(a->value);

		alloc.deallocate_memory(a, sizeof(xml_attribute_struct), reinterpret_cast<xml_memory_page*>(header & xml_memory_page_pointer_mask));
	}

	inline void destroy_node(xml_node_struct* n, xml_allocator& alloc)
	{
		uintptr_t header = n->header;

		if (header & impl::xml_memory_page_name_allocated_mask) alloc.deallocate_string(n->name);
		if (header & impl::xml_memory_page_value_allocated_mask) alloc.deallocate_string(n->value);

		for (xml_attribute_struct* attr = n->first_attribute; attr; )
		{
			xml_attribute_struct* next = attr->next_attribute;

			destroy_attribute(attr, alloc);

			attr = next;
		}

		for (xml_node_struct* child = n->first_child; child; )
		{
			xml_node_struct* next = child->next_sibling;

			destroy_node(child, alloc);

			child = next;
		}

		alloc.deallocate_memory(n, sizeof(xml_node_struct), reinterpret_cast<xml_memory_page*>(header & xml_memory_page_pointer_mask));
	}

	PUGI__FN_NO_INLINE xml_node_struct* append_node(xml_node_struct* node, xml_allocator& alloc, xml_node_type type = node_element)
	{
		xml_node_struct* child = allocate_node(alloc, type);
		if (!child) return 0;

		child->parent = node;

		xml_node_struct* first_child = node->first_child;
			
		if (first_child)
		{
			xml_node_struct* last_child = first_child->prev_sibling_c;

			last_child->next_sibling = child;
			child->prev_sibling_c = last_child;
			first_child->prev_sibling_c = child;
		}
		else
		{
			node->first_child = child;
			child->prev_sibling_c = child;
		}
			
		return child;
	}

	PUGI__FN_NO_INLINE xml_attribute_struct* append_attribute_ll(xml_node_struct* node, xml_allocator& alloc)
	{
		xml_attribute_struct* a = allocate_attribute(alloc);
		if (!a) return 0;

		xml_attribute_struct* first_attribute = node->first_attribute;

		if (first_attribute)
		{
			xml_attribute_struct* last_attribute = first_attribute->prev_attribute_c;

			last_attribute->next_attribute = a;
			a->prev_attribute_c = last_attribute;
			first_attribute->prev_attribute_c = a;
		}
		else
		{
			node->first_attribute = a;
			a->prev_attribute_c = a;
		}
			
		return a;
	}
PUGI__NS_END

// Helper classes for code generation
PUGI__NS_BEGIN
	struct opt_false
	{
		enum { value = 0 };
	};

	struct opt_true
	{
		enum { value = 1 };
	};
PUGI__NS_END

// Unicode utilities
PUGI__NS_BEGIN
	inline uint16_t endian_swap(uint16_t value)
	{
		return static_cast<uint16_t>(((value & 0xff) << 8) | (value >> 8));
	}

	inline uint32_t endian_swap(uint32_t value)
	{
		return ((value & 0xff) << 24) | ((value & 0xff00) << 8) | ((value & 0xff0000) >> 8) | (value >> 24);
	}

	struct utf8_counter
	{
		typedef size_t value_type;

		static value_type low(value_type result, uint32_t ch)
		{
			// U+0000..U+007F
			if (ch < 0x80) return result + 1;
			// U+0080..U+07FF
			else if (ch < 0x800) return result + 2;
			// U+0800..U+FFFF
			else return result + 3;
		}

		static value_type high(value_type result, uint32_t)
		{
			// U+10000..U+10FFFF
			return result + 4;
		}
	};

	struct utf8_writer
	{
		typedef uint8_t* value_type;

		static value_type low(value_type result, uint32_t ch)
		{
			// U+0000..U+007F
			if (ch < 0x80)
			{
				*result = static_cast<uint8_t>(ch);
				return result + 1;
			}
			// U+0080..U+07FF
			else if (ch < 0x800)
			{
				result[0] = static_cast<uint8_t>(0xC0 | (ch >> 6));
				result[1] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
				return result + 2;
			}
			// U+0800..U+FFFF
			else
			{
				result[0] = static_cast<uint8_t>(0xE0 | (ch >> 12));
				result[1] = static_cast<uint8_t>(0x80 | ((ch >> 6) & 0x3F));
				result[2] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
				return result + 3;
			}
		}

		static value_type high(value_type result, uint32_t ch)
		{
			// U+10000..U+10FFFF
			result[0] = static_cast<uint8_t>(0xF0 | (ch >> 18));
			result[1] = static_cast<uint8_t>(0x80 | ((ch >> 12) & 0x3F));
			result[2] = static_cast<uint8_t>(0x80 | ((ch >> 6) & 0x3F));
			result[3] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
			return result + 4;
		}

		static value_type any(value_type result, uint32_t ch)
		{
			return (ch < 0x10000) ? low(result, ch) : high(result, ch);
		}
	};

	struct utf16_counter
	{
		typedef size_t value_type;

		static value_type low(value_type result, uint32_t)
		{
			return result + 1;
		}

		static value_type high(value_type result, uint32_t)
		{
			return result + 2;
		}
	};

	struct utf16_writer
	{
		typedef uint16_t* value_type;

		static value_type low(value_type result, uint32_t ch)
		{
			*result = static_cast<uint16_t>(ch);

			return result + 1;
		}

		static value_type high(value_type result, uint32_t ch)
		{
			uint32_t msh = static_cast<uint32_t>(ch - 0x10000) >> 10;
			uint32_t lsh = static_cast<uint32_t>(ch - 0x10000) & 0x3ff;

			result[0] = static_cast<uint16_t>(0xD800 + msh);
			result[1] = static_cast<uint16_t>(0xDC00 + lsh);

			return result + 2;
		}

		static value_type any(value_type result, uint32_t ch)
		{
			return (ch < 0x10000) ? low(result, ch) : high(result, ch);
		}
	};

	struct utf32_counter
	{
		typedef size_t value_type;

		static value_type low(value_type result, uint32_t)
		{
			return result + 1;
		}

		static value_type high(value_type result, uint32_t)
		{
			return result + 1;
		}
	};

	struct utf32_writer
	{
		typedef uint32_t* value_type;

		static value_type low(value_type result, uint32_t ch)
		{
			*result = ch;

			return result + 1;
		}

		static value_type high(value_type result, uint32_t ch)
		{
			*result = ch;

			return result + 1;
		}

		static value_type any(value_type result, uint32_t ch)
		{
			*result = ch;

			return result + 1;
		}
	};

	struct latin1_writer
	{
		typedef uint8_t* value_type;

		static value_type low(value_type result, uint32_t ch)
		{
			*result = static_cast<uint8_t>(ch > 255 ? '?' : ch);

			return result + 1;
		}

		static value_type high(value_type result, uint32_t ch)
		{
			(void)ch;

			*result = '?';

			return result + 1;
		}
	};

	template <size_t size> struct wchar_selector;

	template <> struct wchar_selector<2>
	{
		typedef uint16_t type;
		typedef utf16_counter counter;
		typedef utf16_writer writer;
	};

	template <> struct wchar_selector<4>
	{
		typedef uint32_t type;
		typedef utf32_counter counter;
		typedef utf32_writer writer;
	};

	typedef wchar_selector<sizeof(wchar_t)>::counter wchar_counter;
	typedef wchar_selector<sizeof(wchar_t)>::writer wchar_writer;

	template <typename Traits, typename opt_swap = opt_false> struct utf_decoder
	{
		static inline typename Traits::value_type decode_utf8_block(const uint8_t* data, size_t size, typename Traits::value_type result)
		{
			const uint8_t utf8_byte_mask = 0x3f;

			while (size)
			{
				uint8_t lead = *data;

				// 0xxxxxxx -> U+0000..U+007F
				if (lead < 0x80)
				{
					result = Traits::low(result, lead);
					data += 1;
					size -= 1;

					// process aligned single-byte (ascii) blocks
					if ((reinterpret_cast<uintptr_t>(data) & 3) == 0)
					{
						// round-trip through void* to silence 'cast increases required alignment of target type' warnings
						while (size >= 4 && (*static_cast<const uint32_t*>(static_cast<const void*>(data)) & 0x80808080) == 0)
						{
							result = Traits::low(result, data[0]);
							result = Traits::low(result, data[1]);
							result = Traits::low(result, data[2]);
							result = Traits::low(result, data[3]);
							data += 4;
							size -= 4;
						}
					}
				}
				// 110xxxxx -> U+0080..U+07FF
				else if (static_cast<unsigned int>(lead - 0xC0) < 0x20 && size >= 2 && (data[1] & 0xc0) == 0x80)
				{
					result = Traits::low(result, ((lead & ~0xC0) << 6) | (data[1] & utf8_byte_mask));
					data += 2;
					size -= 2;
				}
				// 1110xxxx -> U+0800-U+FFFF
				else if (static_cast<unsigned int>(lead - 0xE0) < 0x10 && size >= 3 && (data[1] & 0xc0) == 0x80 && (data[2] & 0xc0) == 0x80)
				{
					result = Traits::low(result, ((lead & ~0xE0) << 12) | ((data[1] & utf8_byte_mask) << 6) | (data[2] & utf8_byte_mask));
					data += 3;
					size -= 3;
				}
				// 11110xxx -> U+10000..U+10FFFF
				else if (static_cast<unsigned int>(lead - 0xF0) < 0x08 && size >= 4 && (data[1] & 0xc0) == 0x80 && (data[2] & 0xc0) == 0x80 && (data[3] & 0xc0) == 0x80)
				{
					result = Traits::high(result, ((lead & ~0xF0) << 18) | ((data[1] & utf8_byte_mask) << 12) | ((data[2] & utf8_byte_mask) << 6) | (data[3] & utf8_byte_mask));
					data += 4;
					size -= 4;
				}
				// 10xxxxxx or 11111xxx -> invalid
				else
				{
					data += 1;
					size -= 1;
				}
			}

			return result;
		}

		static inline typename Traits::value_type decode_utf16_block(const uint16_t* data, size_t size, typename Traits::value_type result)
		{
			const uint16_t* end = data + size;

			while (data < end)
			{
				unsigned int lead = opt_swap::value ? endian_swap(*data) : *data;

				// U+0000..U+D7FF
				if (lead < 0xD800)
				{
					result = Traits::low(result, lead);
					data += 1;
				}
				// U+E000..U+FFFF
				else if (static_cast<unsigned int>(lead - 0xE000) < 0x2000)
				{
					result = Traits::low(result, lead);
					data += 1;
				}
				// surrogate pair lead
				else if (static_cast<unsigned int>(lead - 0xD800) < 0x400 && data + 1 < end)
				{
					uint16_t next = opt_swap::value ? endian_swap(data[1]) : data[1];

					if (static_cast<unsigned int>(next - 0xDC00) < 0x400)
					{
						result = Traits::high(result, 0x10000 + ((lead & 0x3ff) << 10) + (next & 0x3ff));
						data += 2;
					}
					else
					{
						data += 1;
					}
				}
				else
				{
					data += 1;
				}
			}

			return result;
		}

		static inline typename Traits::value_type decode_utf32_block(const uint32_t* data, size_t size, typename Traits::value_type result)
		{
			const uint32_t* end = data + size;

			while (data < end)
			{
				uint32_t lead = opt_swap::value ? endian_swap(*data) : *data;

				// U+0000..U+FFFF
				if (lead < 0x10000)
				{
					result = Traits::low(result, lead);
					data += 1;
				}
				// U+10000..U+10FFFF
				else
				{
					result = Traits::high(result, lead);
					data += 1;
				}
			}

			return result;
		}

		static inline typename Traits::value_type decode_latin1_block(const uint8_t* data, size_t size, typename Traits::value_type result)
		{
			for (size_t i = 0; i < size; ++i)
			{
				result = Traits::low(result, data[i]);
			}

			return result;
		}

		static inline typename Traits::value_type decode_wchar_block_impl(const uint16_t* data, size_t size, typename Traits::value_type result)
		{
			return decode_utf16_block(data, size, result);
		}

		static inline typename Traits::value_type decode_wchar_block_impl(const uint32_t* data, size_t size, typename Traits::value_type result)
		{
			return decode_utf32_block(data, size, result);
		}

		static inline typename Traits::value_type decode_wchar_block(const wchar_t* data, size_t size, typename Traits::value_type result)
		{
			return decode_wchar_block_impl(reinterpret_cast<const wchar_selector<sizeof(wchar_t)>::type*>(data), size, result);
		}
	};

	template <typename T> PUGI__FN void convert_utf_endian_swap(T* result, const T* data, size_t length)
	{
		for (size_t i = 0; i < length; ++i) result[i] = endian_swap(data[i]);
	}

#ifdef PUGIXML_WCHAR_MODE
	PUGI__FN void convert_wchar_endian_swap(wchar_t* result, const wchar_t* data, size_t length)
	{
		for (size_t i = 0; i < length; ++i) result[i] = static_cast<wchar_t>(endian_swap(static_cast<wchar_selector<sizeof(wchar_t)>::type>(data[i])));
	}
#endif
PUGI__NS_END

PUGI__NS_BEGIN
	enum chartype_t
	{
		ct_parse_pcdata = 1,	// \0, &, \r, <
		ct_parse_attr = 2,		// \0, &, \r, ', "
		ct_parse_attr_ws = 4,	// \0, &, \r, ', ", \n, tab
		ct_space = 8,			// \r, \n, space, tab
		ct_parse_cdata = 16,	// \0, ], >, \r
		ct_parse_comment = 32,	// \0, -, >, \r
		ct_symbol = 64,			// Any symbol > 127, a-z, A-Z, 0-9, _, :, -, .
		ct_start_symbol = 128	// Any symbol > 127, a-z, A-Z, _, :
	};

	static const unsigned char chartype_table[256] =
	{
		55,  0,   0,   0,   0,   0,   0,   0,      0,   12,  12,  0,   0,   63,  0,   0,   // 0-15
		0,   0,   0,   0,   0,   0,   0,   0,      0,   0,   0,   0,   0,   0,   0,   0,   // 16-31
		8,   0,   6,   0,   0,   0,   7,   6,      0,   0,   0,   0,   0,   96,  64,  0,   // 32-47
		64,  64,  64,  64,  64,  64,  64,  64,     64,  64,  192, 0,   1,   0,   48,  0,   // 48-63
		0,   192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 64-79
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 0,   0,   16,  0,   192, // 80-95
		0,   192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 96-111
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 0, 0, 0, 0, 0,           // 112-127

		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 128+
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192
	};

	enum chartypex_t
	{
		ctx_special_pcdata = 1,   // Any symbol >= 0 and < 32 (except \t, \r, \n), &, <, >
		ctx_special_attr = 2,     // Any symbol >= 0 and < 32 (except \t), &, <, >, "
		ctx_start_symbol = 4,	  // Any symbol > 127, a-z, A-Z, _
		ctx_digit = 8,			  // 0-9
		ctx_symbol = 16			  // Any symbol > 127, a-z, A-Z, 0-9, _, -, .
	};
	
	static const unsigned char chartypex_table[256] =
	{
		3,  3,  3,  3,  3,  3,  3,  3,     3,  0,  2,  3,  3,  2,  3,  3,     // 0-15
		3,  3,  3,  3,  3,  3,  3,  3,     3,  3,  3,  3,  3,  3,  3,  3,     // 16-31
		0,  0,  2,  0,  0,  0,  3,  0,     0,  0,  0,  0,  0, 16, 16,  0,     // 32-47
		24, 24, 24, 24, 24, 24, 24, 24,    24, 24, 0,  0,  3,  0,  3,  0,     // 48-63

		0,  20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,    // 64-79
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 0,  0,  0,  0,  20,    // 80-95
		0,  20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,    // 96-111
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 0,  0,  0,  0,  0,     // 112-127

		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,    // 128+
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20
	};
	
#ifdef PUGIXML_WCHAR_MODE
	#define PUGI__IS_CHARTYPE_IMPL(c, ct, table) ((static_cast<unsigned int>(c) < 128 ? table[static_cast<unsigned int>(c)] : table[128]) & (ct))
#else
	#define PUGI__IS_CHARTYPE_IMPL(c, ct, table) (table[static_cast<unsigned char>(c)] & (ct))
#endif

	#define PUGI__IS_CHARTYPE(c, ct) PUGI__IS_CHARTYPE_IMPL(c, ct, chartype_table)
	#define PUGI__IS_CHARTYPEX(c, ct) PUGI__IS_CHARTYPE_IMPL(c, ct, chartypex_table)

	PUGI__FN bool is_little_endian()
	{
		unsigned int ui = 1;

		return *reinterpret_cast<unsigned char*>(&ui) == 1;
	}

	PUGI__FN xml_encoding get_wchar_encoding()
	{
		PUGI__STATIC_ASSERT(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4);

		if (sizeof(wchar_t) == 2)
			return is_little_endian() ? encoding_utf16_le : encoding_utf16_be;
		else 
			return is_little_endian() ? encoding_utf32_le : encoding_utf32_be;
	}

	PUGI__FN xml_encoding guess_buffer_encoding(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
	{
		// look for BOM in first few bytes
		if (d0 == 0 && d1 == 0 && d2 == 0xfe && d3 == 0xff) return encoding_utf32_be;
		if (d0 == 0xff && d1 == 0xfe && d2 == 0 && d3 == 0) return encoding_utf32_le;
		if (d0 == 0xfe && d1 == 0xff) return encoding_utf16_be;
		if (d0 == 0xff && d1 == 0xfe) return encoding_utf16_le;
		if (d0 == 0xef && d1 == 0xbb && d2 == 0xbf) return encoding_utf8;

		// look for <, <? or <?xm in various encodings
		if (d0 == 0 && d1 == 0 && d2 == 0 && d3 == 0x3c) return encoding_utf32_be;
		if (d0 == 0x3c && d1 == 0 && d2 == 0 && d3 == 0) return encoding_utf32_le;
		if (d0 == 0 && d1 == 0x3c && d2 == 0 && d3 == 0x3f) return encoding_utf16_be;
		if (d0 == 0x3c && d1 == 0 && d2 == 0x3f && d3 == 0) return encoding_utf16_le;
		if (d0 == 0x3c && d1 == 0x3f && d2 == 0x78 && d3 == 0x6d) return encoding_utf8;

		// look for utf16 < followed by node name (this may fail, but is better than utf8 since it's zero terminated so early)
		if (d0 == 0 && d1 == 0x3c) return encoding_utf16_be;
		if (d0 == 0x3c && d1 == 0) return encoding_utf16_le;

		// no known BOM detected, assume utf8
		return encoding_utf8;
	}

	PUGI__FN xml_encoding get_buffer_encoding(xml_encoding encoding, const void* contents, size_t size)
	{
		// replace wchar encoding with utf implementation
		if (encoding == encoding_wchar) return get_wchar_encoding();

		// replace utf16 encoding with utf16 with specific endianness
		if (encoding == encoding_utf16) return is_little_endian() ? encoding_utf16_le : encoding_utf16_be;

		// replace utf32 encoding with utf32 with specific endianness
		if (encoding == encoding_utf32) return is_little_endian() ? encoding_utf32_le : encoding_utf32_be;

		// only do autodetection if no explicit encoding is requested
		if (encoding != encoding_auto) return encoding;

		// skip encoding autodetection if input buffer is too small
		if (size < 4) return encoding_utf8;

		// try to guess encoding (based on XML specification, Appendix F.1)
		const uint8_t* data = static_cast<const uint8_t*>(contents);

		PUGI__DMC_VOLATILE uint8_t d0 = data[0], d1 = data[1], d2 = data[2], d3 = data[3];

		return guess_buffer_encoding(d0, d1, d2, d3);
	}

	PUGI__FN bool get_mutable_buffer(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, bool is_mutable)
	{
		size_t length = size / sizeof(char_t);

		if (is_mutable)
		{
			out_buffer = static_cast<char_t*>(const_cast<void*>(contents));
			out_length = length;
		}
		else
		{
			char_t* buffer = static_cast<char_t*>(xml_memory::allocate((length + 1) * sizeof(char_t)));
			if (!buffer) return false;

			memcpy(buffer, contents, length * sizeof(char_t));
			buffer[length] = 0;

			out_buffer = buffer;
			out_length = length + 1;
		}

		return true;
	}

#ifdef PUGIXML_WCHAR_MODE
	PUGI__FN bool need_endian_swap_utf(xml_encoding le, xml_encoding re)
	{
		return (le == encoding_utf16_be && re == encoding_utf16_le) || (le == encoding_utf16_le && re == encoding_utf16_be) ||
			   (le == encoding_utf32_be && re == encoding_utf32_le) || (le == encoding_utf32_le && re == encoding_utf32_be);
	}

	PUGI__FN bool convert_buffer_endian_swap(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, bool is_mutable)
	{
		const char_t* data = static_cast<const char_t*>(contents);
		size_t length = size / sizeof(char_t);

		if (is_mutable)
		{
			char_t* buffer = const_cast<char_t*>(data);

			convert_wchar_endian_swap(buffer, data, length);

			out_buffer = buffer;
			out_length = length;
		}
		else
		{
			char_t* buffer = static_cast<char_t*>(xml_memory::allocate((length + 1) * sizeof(char_t)));
			if (!buffer) return false;

			convert_wchar_endian_swap(buffer, data, length);
			buffer[length] = 0;

			out_buffer = buffer;
			out_length = length + 1;
		}

		return true;
	}

	PUGI__FN bool convert_buffer_utf8(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size)
	{
		const uint8_t* data = static_cast<const uint8_t*>(contents);
		size_t data_length = size;

		// first pass: get length in wchar_t units
		size_t length = utf_decoder<wchar_counter>::decode_utf8_block(data, data_length, 0);

		// allocate buffer of suitable length
		char_t* buffer = static_cast<char_t*>(xml_memory::allocate((length + 1) * sizeof(char_t)));
		if (!buffer) return false;

		// second pass: convert utf8 input to wchar_t
		wchar_writer::value_type obegin = reinterpret_cast<wchar_writer::value_type>(buffer);
		wchar_writer::value_type oend = utf_decoder<wchar_writer>::decode_utf8_block(data, data_length, obegin);

		assert(oend == obegin + length);
		*oend = 0;

		out_buffer = buffer;
		out_length = length + 1;

		return true;
	}

	template <typename opt_swap> PUGI__FN bool convert_buffer_utf16(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, opt_swap)
	{
		const uint16_t* data = static_cast<const uint16_t*>(contents);
		size_t data_length = size / sizeof(uint16_t);

		// first pass: get length in wchar_t units
		size_t length = utf_decoder<wchar_counter, opt_swap>::decode_utf16_block(data, data_length, 0);

		// allocate buffer of suitable length
		char_t* buffer = static_cast<char_t*>(xml_memory::allocate((length + 1) * sizeof(char_t)));
		if (!buffer) return false;

		// second pass: convert utf16 input to wchar_t
		wchar_writer::value_type obegin = reinterpret_cast<wchar_writer::value_type>(buffer);
		wchar_writer::value_type oend = utf_decoder<wchar_writer, opt_swap>::decode_utf16_block(data, data_length, obegin);

		assert(oend == obegin + length);
		*oend = 0;

		out_buffer = buffer;
		out_length = length + 1;

		return true;
	}

	template <typename opt_swap> PUGI__FN bool convert_buffer_utf32(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, opt_swap)
	{
		const uint32_t* data = static_cast<const uint32_t*>(contents);
		size_t data_length = size / sizeof(uint32_t);

		// first pass: get length in wchar_t units
		size_t length = utf_decoder<wchar_counter, opt_swap>::decode_utf32_block(data, data_length, 0);

		// allocate buffer of suitable length
		char_t* buffer = static_cast<char_t*>(xml_memory::allocate((length + 1) * sizeof(char_t)));
		if (!buffer) return false;

		// second pass: convert utf32 input to wchar_t
		wchar_writer::value_type obegin = reinterpret_cast<wchar_writer::value_type>(buffer);
		wchar_writer::value_type oend = utf_decoder<wchar_writer, opt_swap>::decode_utf32_block(data, data_length, obegin);

		assert(oend == obegin + length);
		*oend = 0;

		out_buffer = buffer;
		out_length = length + 1;

		return true;
	}

	PUGI__FN bool convert_buffer_latin1(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size)
	{
		const uint8_t* data = static_cast<const uint8_t*>(contents);
		size_t data_length = size;

		// get length in wchar_t units
		size_t length = data_length;

		// allocate buffer of suitable length
		char_t* buffer = static_cast<char_t*>(xml_memory::allocate((length + 1) * sizeof(char_t)));
		if (!buffer) return false;

		// convert latin1 input to wchar_t
		wchar_writer::value_type obegin = reinterpret_cast<wchar_writer::value_type>(buffer);
		wchar_writer::value_type oend = utf_decoder<wchar_writer>::decode_latin1_block(data, data_length, obegin);

		assert(oend == obegin + length);
		*oend = 0;

		out_buffer = buffer;
		out_length = length + 1;

		return true;
	}

	PUGI__FN bool convert_buffer(char_t*& out_buffer, size_t& out_length, xml_encoding encoding, const void* contents, size_t size, bool is_mutable)
	{
		// get native encoding
		xml_encoding wchar_encoding = get_wchar_encoding();

		// fast path: no conversion required
		if (encoding == wchar_encoding) return get_mutable_buffer(out_buffer, out_length, contents, size, is_mutable);

		// only endian-swapping is required
		if (need_endian_swap_utf(encoding, wchar_encoding)) return convert_buffer_endian_swap(out_buffer, out_length, contents, size, is_mutable);

		// source encoding is utf8
		if (encoding == encoding_utf8) return convert_buffer_utf8(out_buffer, out_length, contents, size);

		// source encoding is utf16
		if (encoding == encoding_utf16_be || encoding == encoding_utf16_le)
		{
			xml_encoding native_encoding = is_little_endian() ? encoding_utf16_le : encoding_utf16_be;

			return (native_encoding == encoding) ?
				convert_buffer_utf16(out_buffer, out_length, contents, size, opt_false()) :
				convert_buffer_utf16(out_buffer, out_length, contents, size, opt_true());
		}

		// source encoding is utf32
		if (encoding == encoding_utf32_be || encoding == encoding_utf32_le)
		{
			xml_encoding native_encoding = is_little_endian() ? encoding_utf32_le : encoding_utf32_be;

			return (native_encoding == encoding) ?
				convert_buffer_utf32(out_buffer, out_length, contents, size, opt_false()) :
				convert_buffer_utf32(out_buffer, out_length, contents, size, opt_true());
		}

		// source encoding is latin1
		if (encoding == encoding_latin1) return convert_buffer_latin1(out_buffer, out_length, contents, size);

		assert(!"Invalid encoding");
		return false;
	}
#else
	template <typename opt_swap> PUGI__FN bool convert_buffer_utf16(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, opt_swap)
	{
		const uint16_t* data = static_cast<const uint16_t*>(contents);
		size_t data_length = size / sizeof(uint16_t);

		// first pass: get length in utf8 units
		size_t length = utf_decoder<utf8_counter, opt_swap>::decode_utf16_block(data, data_length, 0);

		// allocate buffer of suitable length
		char_t* buffer = static_cast<char_t*>(xml_memory::allocate((length + 1) * sizeof(char_t)));
		if (!buffer) return false;

		// second pass: convert utf16 input to utf8
		uint8_t* obegin = reinterpret_cast<uint8_t*>(buffer);
		uint8_t* oend = utf_decoder<utf8_writer, opt_swap>::decode_utf16_block(data, data_length, obegin);

		assert(oend == obegin + length);
		*oend = 0;

		out_buffer = buffer;
		out_length = length + 1;

		return true;
	}

	template <typename opt_swap> PUGI__FN bool convert_buffer_utf32(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, opt_swap)
	{
		const uint32_t* data = static_cast<const uint32_t*>(contents);
		size_t data_length = size / sizeof(uint32_t);

		// first pass: get length in utf8 units
		size_t length = utf_decoder<utf8_counter, opt_swap>::decode_utf32_block(data, data_length, 0);

		// allocate buffer of suitable length
		char_t* buffer = static_cast<char_t*>(xml_memory::allocate((length + 1) * sizeof(char_t)));
		if (!buffer) return false;

		// second pass: convert utf32 input to utf8
		uint8_t* obegin = reinterpret_cast<uint8_t*>(buffer);
		uint8_t* oend = utf_decoder<utf8_writer, opt_swap>::decode_utf32_block(data, data_length, obegin);

		assert(oend == obegin + length);
		*oend = 0;

		out_buffer = buffer;
		out_length = length + 1;

		return true;
	}

	PUGI__FN size_t get_latin1_7bit_prefix_length(const uint8_t* data, size_t size)
	{
		for (size_t i = 0; i < size; ++i)
			if (data[i] > 127)
				return i;

		return size;
	}

	PUGI__FN bool convert_buffer_latin1(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, bool is_mutable)
	{
		const uint8_t* data = static_cast<const uint8_t*>(contents);
		size_t data_length = size;

		// get size of prefix that does not need utf8 conversion
		size_t prefix_length = get_latin1_7bit_prefix_length(data, data_length);
		assert(prefix_length <= data_length);

		const uint8_t* postfix = data + prefix_length;
		size_t postfix_length = data_length - prefix_length;

		// if no conversion is needed, just return the original buffer
		if (postfix_length == 0) return get_mutable_buffer(out_buffer, out_length, contents, size, is_mutable);

		// first pass: get length in utf8 units
		size_t length = prefix_length + utf_decoder<utf8_counter>::decode_latin1_block(postfix, postfix_length, 0);

		// allocate buffer of suitable length
		char_t* buffer = static_cast<char_t*>(xml_memory::allocate((length + 1) * sizeof(char_t)));
		if (!buffer) return false;

		// second pass: convert latin1 input to utf8
		memcpy(buffer, data, prefix_length);

		uint8_t* obegin = reinterpret_cast<uint8_t*>(buffer);
		uint8_t* oend = utf_decoder<utf8_writer>::decode_latin1_block(postfix, postfix_length, obegin + prefix_length);

		assert(oend == obegin + length);
		*oend = 0;

		out_buffer = buffer;
		out_length = length + 1;

		return true;
	}

	PUGI__FN bool convert_buffer(char_t*& out_buffer, size_t& out_length, xml_encoding encoding, const void* contents, size_t size, bool is_mutable)
	{
		// fast path: no conversion required
		if (encoding == encoding_utf8) return get_mutable_buffer(out_buffer, out_length, contents, size, is_mutable);

		// source encoding is utf16
		if (encoding == encoding_utf16_be || encoding == encoding_utf16_le)
		{
			xml_encoding native_encoding = is_little_endian() ? encoding_utf16_le : encoding_utf16_be;

			return (native_encoding == encoding) ?
				convert_buffer_utf16(out_buffer, out_length, contents, size, opt_false()) :
				convert_buffer_utf16(out_buffer, out_length, contents, size, opt_true());
		}

		// source encoding is utf32
		if (encoding == encoding_utf32_be || encoding == encoding_utf32_le)
		{
			xml_encoding native_encoding = is_little_endian() ? encoding_utf32_le : encoding_utf32_be;

			return (native_encoding == encoding) ?
				convert_buffer_utf32(out_buffer, out_length, contents, size, opt_false()) :
				convert_buffer_utf32(out_buffer, out_length, contents, size, opt_true());
		}

		// source encoding is latin1
		if (encoding == encoding_latin1) return convert_buffer_latin1(out_buffer, out_length, contents, size, is_mutable);

		assert(!"Invalid encoding");
		return false;
	}
#endif

	PUGI__FN size_t as_utf8_begin(const wchar_t* str, size_t length)
	{
		// get length in utf8 characters
		return utf_decoder<utf8_counter>::decode_wchar_block(str, length, 0);
	}

	PUGI__FN void as_utf8_end(char* buffer, size_t size, const wchar_t* str, size_t length)
	{
		// convert to utf8
		uint8_t* begin = reinterpret_cast<uint8_t*>(buffer);
		uint8_t* end = utf_decoder<utf8_writer>::decode_wchar_block(str, length, begin);
	
		assert(begin + size == end);
		(void)!end;

		// zero-terminate
		buffer[size] = 0;
	}
	
#ifndef PUGIXML_NO_STL
	PUGI__FN std::string as_utf8_impl(const wchar_t* str, size_t length)
	{
		// first pass: get length in utf8 characters
		size_t size = as_utf8_begin(str, length);

		// allocate resulting string
		std::string result;
		result.resize(size);

		// second pass: convert to utf8
		if (size > 0) as_utf8_end(&result[0], size, str, length);

		return result;
	}

	PUGI__FN std::basic_string<wchar_t> as_wide_impl(const char* str, size_t size)
	{
		const uint8_t* data = reinterpret_cast<const uint8_t*>(str);

		// first pass: get length in wchar_t units
		size_t length = utf_decoder<wchar_counter>::decode_utf8_block(data, size, 0);

		// allocate resulting string
		std::basic_string<wchar_t> result;
		result.resize(length);

		// second pass: convert to wchar_t
		if (length > 0)
		{
			wchar_writer::value_type begin = reinterpret_cast<wchar_writer::value_type>(&result[0]);
			wchar_writer::value_type end = utf_decoder<wchar_writer>::decode_utf8_block(data, size, begin);

			assert(begin + length == end);
			(void)!end;
		}

		return result;
	}
#endif

	inline bool strcpy_insitu_allow(size_t length, uintptr_t allocated, char_t* target)
	{
		assert(target);
		size_t target_length = strlength(target);

		// always reuse document buffer memory if possible
		if (!allocated) return target_length >= length;

		// reuse heap memory if waste is not too great
		const size_t reuse_threshold = 32;

		return target_length >= length && (target_length < reuse_threshold || target_length - length < target_length / 2);
	}

	PUGI__FN bool strcpy_insitu(char_t*& dest, uintptr_t& header, uintptr_t header_mask, const char_t* source)
	{
		assert(header);

		size_t source_length = strlength(source);

		if (source_length == 0)
		{
			// empty string and null pointer are equivalent, so just deallocate old memory
			xml_allocator* alloc = reinterpret_cast<xml_memory_page*>(header & xml_memory_page_pointer_mask)->allocator;

			if (header & header_mask) alloc->deallocate_string(dest);
			
			// mark the string as not allocated
			dest = 0;
			header &= ~header_mask;

			return true;
		}
		else if (dest && strcpy_insitu_allow(source_length, header & header_mask, dest))
		{
			// we can reuse old buffer, so just copy the new data (including zero terminator)
			memcpy(dest, source, (source_length + 1) * sizeof(char_t));
			
			return true;
		}
		else
		{
			xml_allocator* alloc = reinterpret_cast<xml_memory_page*>(header & xml_memory_page_pointer_mask)->allocator;

			// allocate new buffer
			char_t* buf = alloc->allocate_string(source_length + 1);
			if (!buf) return false;

			// copy the string (including zero terminator)
			memcpy(buf, source, (source_length + 1) * sizeof(char_t));

			// deallocate old buffer (*after* the above to protect against overlapping memory and/or allocation failures)
			if (header & header_mask) alloc->deallocate_string(dest);
			
			// the string is now allocated, so set the flag
			dest = buf;
			header |= header_mask;

			return true;
		}
	}

	struct gap
	{
		char_t* end;
		size_t size;
			
		gap(): end(0), size(0)
		{
		}
			
		// Push new gap, move s count bytes further (skipping the gap).
		// Collapse previous gap.
		void push(char_t*& s, size_t count)
		{
			if (end) // there was a gap already; collapse it
			{
				// Move [old_gap_end, new_gap_start) to [old_gap_start, ...)
				assert(s >= end);
				memmove(end - size, end, reinterpret_cast<char*>(s) - reinterpret_cast<char*>(end));
			}
				
			s += count; // end of current gap
				
			// "merge" two gaps
			end = s;
			size += count;
		}
			
		// Collapse all gaps, return past-the-end pointer
		char_t* flush(char_t* s)
		{
			if (end)
			{
				// Move [old_gap_end, current_pos) to [old_gap_start, ...)
				assert(s >= end);
				memmove(end - size, end, reinterpret_cast<char*>(s) - reinterpret_cast<char*>(end));

				return s - size;
			}
			else return s;
		}
	};
	
	PUGI__FN char_t* strconv_escape(char_t* s, gap& g)
	{
		char_t* stre = s + 1;

		switch (*stre)
		{
			case '#':	// &#...
			{
				unsigned int ucsc = 0;

				if (stre[1] == 'x') // &#x... (hex code)
				{
					stre += 2;

					char_t ch = *stre;

					if (ch == ';') return stre;

					for (;;)
					{
						if (static_cast<unsigned int>(ch - '0') <= 9)
							ucsc = 16 * ucsc + (ch - '0');
						else if (static_cast<unsigned int>((ch | ' ') - 'a') <= 5)
							ucsc = 16 * ucsc + ((ch | ' ') - 'a' + 10);
						else if (ch == ';')
							break;
						else // cancel
							return stre;

						ch = *++stre;
					}
					
					++stre;
				}
				else	// &#... (dec code)
				{
					char_t ch = *++stre;

					if (ch == ';') return stre;

					for (;;)
					{
						if (static_cast<unsigned int>(static_cast<unsigned int>(ch) - '0') <= 9)
							ucsc = 10 * ucsc + (ch - '0');
						else if (ch == ';')
							break;
						else // cancel
							return stre;

						ch = *++stre;
					}
					
					++stre;
				}

			#ifdef PUGIXML_WCHAR_MODE
				s = reinterpret_cast<char_t*>(wchar_writer::any(reinterpret_cast<wchar_writer::value_type>(s), ucsc));
			#else
				s = reinterpret_cast<char_t*>(utf8_writer::any(reinterpret_cast<uint8_t*>(s), ucsc));
			#endif
					
				g.push(s, stre - s);
				return stre;
			}

			case 'a':	// &a
			{
				++stre;

				if (*stre == 'm') // &am
				{
					if (*++stre == 'p' && *++stre == ';') // &amp;
					{
						*s++ = '&';
						++stre;
							
						g.push(s, stre - s);
						return stre;
					}
				}
				else if (*stre == 'p') // &ap
				{
					if (*++stre == 'o' && *++stre == 's' && *++stre == ';') // &apos;
					{
						*s++ = '\'';
						++stre;

						g.push(s, stre - s);
						return stre;
					}
				}
				break;
			}

			case 'g': // &g
			{
				if (*++stre == 't' && *++stre == ';') // &gt;
				{
					*s++ = '>';
					++stre;
					
					g.push(s, stre - s);
					return stre;
				}
				break;
			}

			case 'l': // &l
			{
				if (*++stre == 't' && *++stre == ';') // &lt;
				{
					*s++ = '<';
					++stre;
						
					g.push(s, stre - s);
					return stre;
				}
				break;
			}

			case 'q': // &q
			{
				if (*++stre == 'u' && *++stre == 'o' && *++stre == 't' && *++stre == ';') // &quot;
				{
					*s++ = '"';
					++stre;
					
					g.push(s, stre - s);
					return stre;
				}
				break;
			}

			default:
				break;
		}
		
		return stre;
	}

	// Utility macro for last character handling
	#define ENDSWITH(c, e) ((c) == (e) || ((c) == 0 && endch == (e)))

	PUGI__FN char_t* strconv_comment(char_t* s, char_t endch)
	{
		gap g;
		
		while (true)
		{
			while (!PUGI__IS_CHARTYPE(*s, ct_parse_comment)) ++s;
		
			if (*s == '\r') // Either a single 0x0d or 0x0d 0x0a pair
			{
				*s++ = '\n'; // replace first one with 0x0a
				
				if (*s == '\n') g.push(s, 1);
			}
			else if (s[0] == '-' && s[1] == '-' && ENDSWITH(s[2], '>')) // comment ends here
			{
				*g.flush(s) = 0;
				
				return s + (s[2] == '>' ? 3 : 2);
			}
			else if (*s == 0)
			{
				return 0;
			}
			else ++s;
		}
	}

	PUGI__FN char_t* strconv_cdata(char_t* s, char_t endch)
	{
		gap g;
			
		while (true)
		{
			while (!PUGI__IS_CHARTYPE(*s, ct_parse_cdata)) ++s;
			
			if (*s == '\r') // Either a single 0x0d or 0x0d 0x0a pair
			{
				*s++ = '\n'; // replace first one with 0x0a
				
				if (*s == '\n') g.push(s, 1);
			}
			else if (s[0] == ']' && s[1] == ']' && ENDSWITH(s[2], '>')) // CDATA ends here
			{
				*g.flush(s) = 0;
				
				return s + 1;
			}
			else if (*s == 0)
			{
				return 0;
			}
			else ++s;
		}
	}
	
	typedef char_t* (*strconv_pcdata_t)(char_t*);
		
	template <typename opt_trim, typename opt_eol, typename opt_escape> struct strconv_pcdata_impl
	{
		static char_t* parse(char_t* s)
		{
			gap g;

			char_t* begin = s;

			while (true)
			{
				while (!PUGI__IS_CHARTYPE(*s, ct_parse_pcdata)) ++s;
					
				if (*s == '<') // PCDATA ends here
				{
					char_t* end = g.flush(s);

					if (opt_trim::value)
						while (end > begin && PUGI__IS_CHARTYPE(end[-1], ct_space))
							--end;

					*end = 0;
					
					return s + 1;
				}
				else if (opt_eol::value && *s == '\r') // Either a single 0x0d or 0x0d 0x0a pair
				{
					*s++ = '\n'; // replace first one with 0x0a
					
					if (*s == '\n') g.push(s, 1);
				}
				else if (opt_escape::value && *s == '&')
				{
					s = strconv_escape(s, g);
				}
				else if (*s == 0)
				{
					char_t* end = g.flush(s);

					if (opt_trim::value)
						while (end > begin && PUGI__IS_CHARTYPE(end[-1], ct_space))
							--end;

					*end = 0;

					return s;
				}
				else ++s;
			}
		}
	};
	
	PUGI__FN strconv_pcdata_t get_strconv_pcdata(unsigned int optmask)
	{
		PUGI__STATIC_ASSERT(parse_escapes == 0x10 && parse_eol == 0x20 && parse_trim_pcdata == 0x0800);

		switch (((optmask >> 4) & 3) | ((optmask >> 9) & 4)) // get bitmask for flags (eol escapes trim)
		{
		case 0: return strconv_pcdata_impl<opt_false, opt_false, opt_false>::parse;
		case 1: return strconv_pcdata_impl<opt_false, opt_false, opt_true>::parse;
		case 2: return strconv_pcdata_impl<opt_false, opt_true, opt_false>::parse;
		case 3: return strconv_pcdata_impl<opt_false, opt_true, opt_true>::parse;
		case 4: return strconv_pcdata_impl<opt_true, opt_false, opt_false>::parse;
		case 5: return strconv_pcdata_impl<opt_true, opt_false, opt_true>::parse;
		case 6: return strconv_pcdata_impl<opt_true, opt_true, opt_false>::parse;
		case 7: return strconv_pcdata_impl<opt_true, opt_true, opt_true>::parse;
		default: assert(false); return 0; // should not get here
		}
	}

	typedef char_t* (*strconv_attribute_t)(char_t*, char_t);
	
	template <typename opt_escape> struct strconv_attribute_impl
	{
		static char_t* parse_wnorm(char_t* s, char_t end_quote)
		{
			gap g;

			// trim leading whitespaces
			if (PUGI__IS_CHARTYPE(*s, ct_space))
			{
				char_t* str = s;
				
				do ++str;
				while (PUGI__IS_CHARTYPE(*str, ct_space));
				
				g.push(s, str - s);
			}

			while (true)
			{
				while (!PUGI__IS_CHARTYPE(*s, ct_parse_attr_ws | ct_space)) ++s;
				
				if (*s == end_quote)
				{
					char_t* str = g.flush(s);
					
					do *str-- = 0;
					while (PUGI__IS_CHARTYPE(*str, ct_space));
				
					return s + 1;
				}
				else if (PUGI__IS_CHARTYPE(*s, ct_space))
				{
					*s++ = ' ';
		
					if (PUGI__IS_CHARTYPE(*s, ct_space))
					{
						char_t* str = s + 1;
						while (PUGI__IS_CHARTYPE(*str, ct_space)) ++str;
						
						g.push(s, str - s);
					}
				}
				else if (opt_escape::value && *s == '&')
				{
					s = strconv_escape(s, g);
				}
				else if (!*s)
				{
					return 0;
				}
				else ++s;
			}
		}

		static char_t* parse_wconv(char_t* s, char_t end_quote)
		{
			gap g;

			while (true)
			{
				while (!PUGI__IS_CHARTYPE(*s, ct_parse_attr_ws)) ++s;
				
				if (*s == end_quote)
				{
					*g.flush(s) = 0;
				
					return s + 1;
				}
				else if (PUGI__IS_CHARTYPE(*s, ct_space))
				{
					if (*s == '\r')
					{
						*s++ = ' ';
				
						if (*s == '\n') g.push(s, 1);
					}
					else *s++ = ' ';
				}
				else if (opt_escape::value && *s == '&')
				{
					s = strconv_escape(s, g);
				}
				else if (!*s)
				{
					return 0;
				}
				else ++s;
			}
		}

		static char_t* parse_eol(char_t* s, char_t end_quote)
		{
			gap g;

			while (true)
			{
				while (!PUGI__IS_CHARTYPE(*s, ct_parse_attr)) ++s;
				
				if (*s == end_quote)
				{
					*g.flush(s) = 0;
				
					return s + 1;
				}
				else if (*s == '\r')
				{
					*s++ = '\n';
					
					if (*s == '\n') g.push(s, 1);
				}
				else if (opt_escape::value && *s == '&')
				{
					s = strconv_escape(s, g);
				}
				else if (!*s)
				{
					return 0;
				}
				else ++s;
			}
		}

		static char_t* parse_simple(char_t* s, char_t end_quote)
		{
			gap g;

			while (true)
			{
				while (!PUGI__IS_CHARTYPE(*s, ct_parse_attr)) ++s;
				
				if (*s == end_quote)
				{
					*g.flush(s) = 0;
				
					return s + 1;
				}
				else if (opt_escape::value && *s == '&')
				{
					s = strconv_escape(s, g);
				}
				else if (!*s)
				{
					return 0;
				}
				else ++s;
			}
		}
	};

	PUGI__FN strconv_attribute_t get_strconv_attribute(unsigned int optmask)
	{
		PUGI__STATIC_ASSERT(parse_escapes == 0x10 && parse_eol == 0x20 && parse_wconv_attribute == 0x40 && parse_wnorm_attribute == 0x80);
		
		switch ((optmask >> 4) & 15) // get bitmask for flags (wconv wnorm eol escapes)
		{
		case 0:  return strconv_attribute_impl<opt_false>::parse_simple;
		case 1:  return strconv_attribute_impl<opt_true>::parse_simple;
		case 2:  return strconv_attribute_impl<opt_false>::parse_eol;
		case 3:  return strconv_attribute_impl<opt_true>::parse_eol;
		case 4:  return strconv_attribute_impl<opt_false>::parse_wconv;
		case 5:  return strconv_attribute_impl<opt_true>::parse_wconv;
		case 6:  return strconv_attribute_impl<opt_false>::parse_wconv;
		case 7:  return strconv_attribute_impl<opt_true>::parse_wconv;
		case 8:  return strconv_attribute_impl<opt_false>::parse_wnorm;
		case 9:  return strconv_attribute_impl<opt_true>::parse_wnorm;
		case 10: return strconv_attribute_impl<opt_false>::parse_wnorm;
		case 11: return strconv_attribute_impl<opt_true>::parse_wnorm;
		case 12: return strconv_attribute_impl<opt_false>::parse_wnorm;
		case 13: return strconv_attribute_impl<opt_true>::parse_wnorm;
		case 14: return strconv_attribute_impl<opt_false>::parse_wnorm;
		case 15: return strconv_attribute_impl<opt_true>::parse_wnorm;
		default: assert(false); return 0; // should not get here
		}
	}

	inline xml_parse_result make_parse_result(xml_parse_status status, ptrdiff_t offset = 0)
	{
		xml_parse_result result;
		result.status = status;
		result.offset = offset;

		return result;
	}

	struct xml_parser
	{
		xml_allocator alloc;
		char_t* error_offset;
		xml_parse_status error_status;
		
		// Parser utilities.
		#define PUGI__SKIPWS()			{ while (PUGI__IS_CHARTYPE(*s, ct_space)) ++s; }
		#define PUGI__OPTSET(OPT)			( optmsk & (OPT) )
		#define PUGI__PUSHNODE(TYPE)		{ cursor = append_node(cursor, alloc, TYPE); if (!cursor) PUGI__THROW_ERROR(status_out_of_memory, s); }
		#define PUGI__POPNODE()			{ cursor = cursor->parent; }
		#define PUGI__SCANFOR(X)			{ while (*s != 0 && !(X)) ++s; }
		#define PUGI__SCANWHILE(X)		{ while ((X)) ++s; }
		#define PUGI__ENDSEG()			{ ch = *s; *s = 0; ++s; }
		#define PUGI__THROW_ERROR(err, m)	return error_offset = m, error_status = err, static_cast<char_t*>(0)
		#define PUGI__CHECK_ERROR(err, m)	{ if (*s == 0) PUGI__THROW_ERROR(err, m); }
		
		xml_parser(const xml_allocator& alloc_): alloc(alloc_), error_offset(0), error_status(status_ok)
		{
		}

		// DOCTYPE consists of nested sections of the following possible types:
		// <!-- ... -->, <? ... ?>, "...", '...'
		// <![...]]>
		// <!...>
		// First group can not contain nested groups
		// Second group can contain nested groups of the same type
		// Third group can contain all other groups
		char_t* parse_doctype_primitive(char_t* s)
		{
			if (*s == '"' || *s == '\'')
			{
				// quoted string
				char_t ch = *s++;
				PUGI__SCANFOR(*s == ch);
				if (!*s) PUGI__THROW_ERROR(status_bad_doctype, s);

				s++;
			}
			else if (s[0] == '<' && s[1] == '?')
			{
				// <? ... ?>
				s += 2;
				PUGI__SCANFOR(s[0] == '?' && s[1] == '>'); // no need for ENDSWITH because ?> can't terminate proper doctype
				if (!*s) PUGI__THROW_ERROR(status_bad_doctype, s);

				s += 2;
			}
			else if (s[0] == '<' && s[1] == '!' && s[2] == '-' && s[3] == '-')
			{
				s += 4;
				PUGI__SCANFOR(s[0] == '-' && s[1] == '-' && s[2] == '>'); // no need for ENDSWITH because --> can't terminate proper doctype
				if (!*s) PUGI__THROW_ERROR(status_bad_doctype, s);

				s += 4;
			}
			else PUGI__THROW_ERROR(status_bad_doctype, s);

			return s;
		}

		char_t* parse_doctype_ignore(char_t* s)
		{
			assert(s[0] == '<' && s[1] == '!' && s[2] == '[');
			s++;

			while (*s)
			{
				if (s[0] == '<' && s[1] == '!' && s[2] == '[')
				{
					// nested ignore section
					s = parse_doctype_ignore(s);
					if (!s) return s;
				}
				else if (s[0] == ']' && s[1] == ']' && s[2] == '>')
				{
					// ignore section end
					s += 3;

					return s;
				}
				else s++;
			}

			PUGI__THROW_ERROR(status_bad_doctype, s);
		}

		char_t* parse_doctype_group(char_t* s, char_t endch, bool toplevel)
		{
			assert((s[0] == '<' || s[0] == 0) && s[1] == '!');
			s++;

			while (*s)
			{
				if (s[0] == '<' && s[1] == '!' && s[2] != '-')
				{
					if (s[2] == '[')
					{
						// ignore
						s = parse_doctype_ignore(s);
						if (!s) return s;
					}
					else
					{
						// some control group
						s = parse_doctype_group(s, endch, false);
						if (!s) return s;

						// skip >
						assert(*s == '>');
						s++;
					}
				}
				else if (s[0] == '<' || s[0] == '"' || s[0] == '\'')
				{
					// unknown tag (forbidden), or some primitive group
					s = parse_doctype_primitive(s);
					if (!s) return s;
				}
				else if (*s == '>')
				{
					return s;
				}
				else s++;
			}

			if (!toplevel || endch != '>') PUGI__THROW_ERROR(status_bad_doctype, s);

			return s;
		}

		char_t* parse_exclamation(char_t* s, xml_node_struct* cursor, unsigned int optmsk, char_t endch)
		{
			// parse node contents, starting with exclamation mark
			++s;

			if (*s == '-') // '<!-...'
			{
				++s;

				if (*s == '-') // '<!--...'
				{
					++s;

					if (PUGI__OPTSET(parse_comments))
					{
						PUGI__PUSHNODE(node_comment); // Append a new node on the tree.
						cursor->value = s; // Save the offset.
					}

					if (PUGI__OPTSET(parse_eol) && PUGI__OPTSET(parse_comments))
					{
						s = strconv_comment(s, endch);

						if (!s) PUGI__THROW_ERROR(status_bad_comment, cursor->value);
					}
					else
					{
						// Scan for terminating '-->'.
						PUGI__SCANFOR(s[0] == '-' && s[1] == '-' && ENDSWITH(s[2], '>'));
						PUGI__CHECK_ERROR(status_bad_comment, s);

						if (PUGI__OPTSET(parse_comments))
							*s = 0; // Zero-terminate this segment at the first terminating '-'.

						s += (s[2] == '>' ? 3 : 2); // Step over the '\0->'.
					}
				}
				else PUGI__THROW_ERROR(status_bad_comment, s);
			}
			else if (*s == '[')
			{
				// '<![CDATA[...'
				if (*++s=='C' && *++s=='D' && *++s=='A' && *++s=='T' && *++s=='A' && *++s == '[')
				{
					++s;

					if (PUGI__OPTSET(parse_cdata))
					{
						PUGI__PUSHNODE(node_cdata); // Append a new node on the tree.
						cursor->value = s; // Save the offset.

						if (PUGI__OPTSET(parse_eol))
						{
							s = strconv_cdata(s, endch);

							if (!s) PUGI__THROW_ERROR(status_bad_cdata, cursor->value);
						}
						else
						{
							// Scan for terminating ']]>'.
							PUGI__SCANFOR(s[0] == ']' && s[1] == ']' && ENDSWITH(s[2], '>'));
							PUGI__CHECK_ERROR(status_bad_cdata, s);

							*s++ = 0; // Zero-terminate this segment.
						}
					}
					else // Flagged for discard, but we still have to scan for the terminator.
					{
						// Scan for terminating ']]>'.
						PUGI__SCANFOR(s[0] == ']' && s[1] == ']' && ENDSWITH(s[2], '>'));
						PUGI__CHECK_ERROR(status_bad_cdata, s);

						++s;
					}

					s += (s[1] == '>' ? 2 : 1); // Step over the last ']>'.
				}
				else PUGI__THROW_ERROR(status_bad_cdata, s);
			}
			else if (s[0] == 'D' && s[1] == 'O' && s[2] == 'C' && s[3] == 'T' && s[4] == 'Y' && s[5] == 'P' && ENDSWITH(s[6], 'E'))
			{
				s -= 2;

				if (cursor->parent) PUGI__THROW_ERROR(status_bad_doctype, s);

				char_t* mark = s + 9;

				s = parse_doctype_group(s, endch, true);
				if (!s) return s;

				assert((*s == 0 && endch == '>') || *s == '>');
				if (*s) *s++ = 0;

				if (PUGI__OPTSET(parse_doctype))
				{
					while (PUGI__IS_CHARTYPE(*mark, ct_space)) ++mark;

					PUGI__PUSHNODE(node_doctype);

					cursor->value = mark;

					PUGI__POPNODE();
				}
			}
			else if (*s == 0 && endch == '-') PUGI__THROW_ERROR(status_bad_comment, s);
			else if (*s == 0 && endch == '[') PUGI__THROW_ERROR(status_bad_cdata, s);
			else PUGI__THROW_ERROR(status_unrecognized_tag, s);

			return s;
		}

		char_t* parse_question(char_t* s, xml_node_struct*& ref_cursor, unsigned int optmsk, char_t endch)
		{
			// load into registers
			xml_node_struct* cursor = ref_cursor;
			char_t ch = 0;

			// parse node contents, starting with question mark
			++s;

			// read PI target
			char_t* target = s;

			if (!PUGI__IS_CHARTYPE(*s, ct_start_symbol)) PUGI__THROW_ERROR(status_bad_pi, s);

			PUGI__SCANWHILE(PUGI__IS_CHARTYPE(*s, ct_symbol));
			PUGI__CHECK_ERROR(status_bad_pi, s);

			// determine node type; stricmp / strcasecmp is not portable
			bool declaration = (target[0] | ' ') == 'x' && (target[1] | ' ') == 'm' && (target[2] | ' ') == 'l' && target + 3 == s;

			if (declaration ? PUGI__OPTSET(parse_declaration) : PUGI__OPTSET(parse_pi))
			{
				if (declaration)
				{
					// disallow non top-level declarations
					if (cursor->parent) PUGI__THROW_ERROR(status_bad_pi, s);

					PUGI__PUSHNODE(node_declaration);
				}
				else
				{
					PUGI__PUSHNODE(node_pi);
				}

				cursor->name = target;

				PUGI__ENDSEG();

				// parse value/attributes
				if (ch == '?')
				{
					// empty node
					if (!ENDSWITH(*s, '>')) PUGI__THROW_ERROR(status_bad_pi, s);
					s += (*s == '>');

					PUGI__POPNODE();
				}
				else if (PUGI__IS_CHARTYPE(ch, ct_space))
				{
					PUGI__SKIPWS();

					// scan for tag end
					char_t* value = s;

					PUGI__SCANFOR(s[0] == '?' && ENDSWITH(s[1], '>'));
					PUGI__CHECK_ERROR(status_bad_pi, s);

					if (declaration)
					{
						// replace ending ? with / so that 'element' terminates properly
						*s = '/';

						// we exit from this function with cursor at node_declaration, which is a signal to parse() to go to LOC_ATTRIBUTES
						s = value;
					}
					else
					{
						// store value and step over >
						cursor->value = value;
						PUGI__POPNODE();

						PUGI__ENDSEG();

						s += (*s == '>');
					}
				}
				else PUGI__THROW_ERROR(status_bad_pi, s);
			}
			else
			{
				// scan for tag end
				PUGI__SCANFOR(s[0] == '?' && ENDSWITH(s[1], '>'));
				PUGI__CHECK_ERROR(status_bad_pi, s);

				s += (s[1] == '>' ? 2 : 1);
			}

			// store from registers
			ref_cursor = cursor;

			return s;
		}

		char_t* parse_tree(char_t* s, xml_node_struct* root, unsigned int optmsk, char_t endch)
		{
			strconv_attribute_t strconv_attribute = get_strconv_attribute(optmsk);
			strconv_pcdata_t strconv_pcdata = get_strconv_pcdata(optmsk);
			
			char_t ch = 0;
			xml_node_struct* cursor = root;
			char_t* mark = s;

			while (*s != 0)
			{
				if (*s == '<')
				{
					++s;

				LOC_TAG:
					if (PUGI__IS_CHARTYPE(*s, ct_start_symbol)) // '<#...'
					{
						PUGI__PUSHNODE(node_element); // Append a new node to the tree.

						cursor->name = s;

						PUGI__SCANWHILE(PUGI__IS_CHARTYPE(*s, ct_symbol)); // Scan for a terminator.
						PUGI__ENDSEG(); // Save char in 'ch', terminate & step over.

						if (ch == '>')
						{
							// end of tag
						}
						else if (PUGI__IS_CHARTYPE(ch, ct_space))
						{
						LOC_ATTRIBUTES:
							while (true)
							{
								PUGI__SKIPWS(); // Eat any whitespace.
						
								if (PUGI__IS_CHARTYPE(*s, ct_start_symbol)) // <... #...
								{
									xml_attribute_struct* a = append_attribute_ll(cursor, alloc); // Make space for this attribute.
									if (!a) PUGI__THROW_ERROR(status_out_of_memory, s);

									a->name = s; // Save the offset.

									PUGI__SCANWHILE(PUGI__IS_CHARTYPE(*s, ct_symbol)); // Scan for a terminator.
									PUGI__CHECK_ERROR(status_bad_attribute, s); //$ redundant, left for performance

									PUGI__ENDSEG(); // Save char in 'ch', terminate & step over.
									PUGI__CHECK_ERROR(status_bad_attribute, s); //$ redundant, left for performance

									if (PUGI__IS_CHARTYPE(ch, ct_space))
									{
										PUGI__SKIPWS(); // Eat any whitespace.
										PUGI__CHECK_ERROR(status_bad_attribute, s); //$ redundant, left for performance

										ch = *s;
										++s;
									}
									
									if (ch == '=') // '<... #=...'
									{
										PUGI__SKIPWS(); // Eat any whitespace.

										if (*s == '"' || *s == '\'') // '<... #="...'
										{
											ch = *s; // Save quote char to avoid breaking on "''" -or- '""'.
											++s; // Step over the quote.
											a->value = s; // Save the offset.

											s = strconv_attribute(s, ch);
										
											if (!s) PUGI__THROW_ERROR(status_bad_attribute, a->value);

											// After this line the loop continues from the start;
											// Whitespaces, / and > are ok, symbols and EOF are wrong,
											// everything else will be detected
											if (PUGI__IS_CHARTYPE(*s, ct_start_symbol)) PUGI__THROW_ERROR(status_bad_attribute, s);
										}
										else PUGI__THROW_ERROR(status_bad_attribute, s);
									}
									else PUGI__THROW_ERROR(status_bad_attribute, s);
								}
								else if (*s == '/')
								{
									++s;
									
									if (*s == '>')
									{
										PUGI__POPNODE();
										s++;
										break;
									}
									else if (*s == 0 && endch == '>')
									{
										PUGI__POPNODE();
										break;
									}
									else PUGI__THROW_ERROR(status_bad_start_element, s);
								}
								else if (*s == '>')
								{
									++s;

									break;
								}
								else if (*s == 0 && endch == '>')
								{
									break;
								}
								else PUGI__THROW_ERROR(status_bad_start_element, s);
							}

							// !!!
						}
						else if (ch == '/') // '<#.../'
						{
							if (!ENDSWITH(*s, '>')) PUGI__THROW_ERROR(status_bad_start_element, s);

							PUGI__POPNODE(); // Pop.

							s += (*s == '>');
						}
						else if (ch == 0)
						{
							// we stepped over null terminator, backtrack & handle closing tag
							--s;
							
							if (endch != '>') PUGI__THROW_ERROR(status_bad_start_element, s);
						}
						else PUGI__THROW_ERROR(status_bad_start_element, s);
					}
					else if (*s == '/')
					{
						++s;

						char_t* name = cursor->name;
						if (!name) PUGI__THROW_ERROR(status_end_element_mismatch, s);
						
						while (PUGI__IS_CHARTYPE(*s, ct_symbol))
						{
							if (*s++ != *name++) PUGI__THROW_ERROR(status_end_element_mismatch, s);
						}

						if (*name)
						{
							if (*s == 0 && name[0] == endch && name[1] == 0) PUGI__THROW_ERROR(status_bad_end_element, s);
							else PUGI__THROW_ERROR(status_end_element_mismatch, s);
						}
							
						PUGI__POPNODE(); // Pop.

						PUGI__SKIPWS();

						if (*s == 0)
						{
							if (endch != '>') PUGI__THROW_ERROR(status_bad_end_element, s);
						}
						else
						{
							if (*s != '>') PUGI__THROW_ERROR(status_bad_end_element, s);
							++s;
						}
					}
					else if (*s == '?') // '<?...'
					{
						s = parse_question(s, cursor, optmsk, endch);
						if (!s) return s;

						assert(cursor);
						if ((cursor->header & xml_memory_page_type_mask) + 1 == node_declaration) goto LOC_ATTRIBUTES;
					}
					else if (*s == '!') // '<!...'
					{
						s = parse_exclamation(s, cursor, optmsk, endch);
						if (!s) return s;
					}
					else if (*s == 0 && endch == '?') PUGI__THROW_ERROR(status_bad_pi, s);
					else PUGI__THROW_ERROR(status_unrecognized_tag, s);
				}
				else
				{
					mark = s; // Save this offset while searching for a terminator.

					PUGI__SKIPWS(); // Eat whitespace if no genuine PCDATA here.

					if (*s == '<' || !*s)
					{
						// We skipped some whitespace characters because otherwise we would take the tag branch instead of PCDATA one
						assert(mark != s);

						if (!PUGI__OPTSET(parse_ws_pcdata | parse_ws_pcdata_single) || PUGI__OPTSET(parse_trim_pcdata))
						{
							continue;
						}
						else if (PUGI__OPTSET(parse_ws_pcdata_single))
						{
							if (s[0] != '<' || s[1] != '/' || cursor->first_child) continue;
						}
					}

					if (!PUGI__OPTSET(parse_trim_pcdata))
						s = mark;
							
					if (cursor->parent || PUGI__OPTSET(parse_fragment))
					{
						PUGI__PUSHNODE(node_pcdata); // Append a new node on the tree.
						cursor->value = s; // Save the offset.

						s = strconv_pcdata(s);
								
						PUGI__POPNODE(); // Pop since this is a standalone.
						
						if (!*s) break;
					}
					else
					{
						PUGI__SCANFOR(*s == '<'); // '...<'
						if (!*s) break;
						
						++s;
					}

					// We're after '<'
					goto LOC_TAG;
				}
			}

			// check that last tag is closed
			if (cursor != root) PUGI__THROW_ERROR(status_end_element_mismatch, s);

			return s;
		}

	#ifdef PUGIXML_WCHAR_MODE
		static char_t* parse_skip_bom(char_t* s)
		{
			unsigned int bom = 0xfeff;
			return (s[0] == static_cast<wchar_t>(bom)) ? s + 1 : s;
		}
	#else
		static char_t* parse_skip_bom(char_t* s)
		{
			return (s[0] == '\xef' && s[1] == '\xbb' && s[2] == '\xbf') ? s + 3 : s;
		}
	#endif

		static bool has_element_node_siblings(xml_node_struct* node)
		{
			while (node)
			{
				xml_node_type type = static_cast<xml_node_type>((node->header & impl::xml_memory_page_type_mask) + 1);
				if (type == node_element) return true;

				node = node->next_sibling;
			}

			return false;
		}

		static xml_parse_result parse(char_t* buffer, size_t length, xml_document_struct* xmldoc, xml_node_struct* root, unsigned int optmsk)
		{
			// allocator object is a part of document object
			xml_allocator& alloc = *static_cast<xml_allocator*>(xmldoc);

			// early-out for empty documents
			if (length == 0)
				return make_parse_result(PUGI__OPTSET(parse_fragment) ? status_ok : status_no_document_element);

			// get last child of the root before parsing
			xml_node_struct* last_root_child = root->first_child ? root->first_child->prev_sibling_c : 0;
	
			// create parser on stack
			xml_parser parser(alloc);

			// save last character and make buffer zero-terminated (speeds up parsing)
			char_t endch = buffer[length - 1];
			buffer[length - 1] = 0;
			
			// skip BOM to make sure it does not end up as part of parse output
			char_t* buffer_data = parse_skip_bom(buffer);

			// perform actual parsing
			parser.parse_tree(buffer_data, root, optmsk, endch);

			// update allocator state
			alloc = parser.alloc;

			xml_parse_result result = make_parse_result(parser.error_status, parser.error_offset ? parser.error_offset - buffer : 0);
			assert(result.offset >= 0 && static_cast<size_t>(result.offset) <= length);

			if (result)
			{
				// since we removed last character, we have to handle the only possible false positive (stray <)
				if (endch == '<')
					return make_parse_result(status_unrecognized_tag, length - 1);

				// check if there are any element nodes parsed
				xml_node_struct* first_root_child_parsed = last_root_child ? last_root_child->next_sibling : root->first_child;

				if (!PUGI__OPTSET(parse_fragment) && !has_element_node_siblings(first_root_child_parsed))
					return make_parse_result(status_no_document_element, length - 1);
			}
			else
			{
				// roll back offset if it occurs on a null terminator in the source buffer
				if (result.offset > 0 && static_cast<size_t>(result.offset) == length - 1 && endch == 0)
					result.offset--;
			}

			return result;
		}
	};

	// Output facilities
	PUGI__FN xml_encoding get_write_native_encoding()
	{
	#ifdef PUGIXML_WCHAR_MODE
		return get_wchar_encoding();
	#else
		return encoding_utf8;
	#endif
	}

	PUGI__FN xml_encoding get_write_encoding(xml_encoding encoding)
	{
		// replace wchar encoding with utf implementation
		if (encoding == encoding_wchar) return get_wchar_encoding();

		// replace utf16 encoding with utf16 with specific endianness
		if (encoding == encoding_utf16) return is_little_endian() ? encoding_utf16_le : encoding_utf16_be;

		// replace utf32 encoding with utf32 with specific endianness
		if (encoding == encoding_utf32) return is_little_endian() ? encoding_utf32_le : encoding_utf32_be;

		// only do autodetection if no explicit encoding is requested
		if (encoding != encoding_auto) return encoding;

		// assume utf8 encoding
		return encoding_utf8;
	}

#ifdef PUGIXML_WCHAR_MODE
	PUGI__FN size_t get_valid_length(const char_t* data, size_t length)
	{
		assert(length > 0);

		// discard last character if it's the lead of a surrogate pair 
		return (sizeof(wchar_t) == 2 && static_cast<unsigned int>(static_cast<uint16_t>(data[length - 1]) - 0xD800) < 0x400) ? length - 1 : length;
	}

	PUGI__FN size_t convert_buffer_output(char_t* r_char, uint8_t* r_u8, uint16_t* r_u16, uint32_t* r_u32, const char_t* data, size_t length, xml_encoding encoding)
	{
		// only endian-swapping is required
		if (need_endian_swap_utf(encoding, get_wchar_encoding()))
		{
			convert_wchar_endian_swap(r_char, data, length);

			return length * sizeof(char_t);
		}
	
		// convert to utf8
		if (encoding == encoding_utf8)
		{
			uint8_t* dest = r_u8;
			uint8_t* end = utf_decoder<utf8_writer>::decode_wchar_block(data, length, dest);

			return static_cast<size_t>(end - dest);
		}

		// convert to utf16
		if (encoding == encoding_utf16_be || encoding == encoding_utf16_le)
		{
			uint16_t* dest = r_u16;

			// convert to native utf16
			uint16_t* end = utf_decoder<utf16_writer>::decode_wchar_block(data, length, dest);

			// swap if necessary
			xml_encoding native_encoding = is_little_endian() ? encoding_utf16_le : encoding_utf16_be;

			if (native_encoding != encoding) convert_utf_endian_swap(dest, dest, static_cast<size_t>(end - dest));

			return static_cast<size_t>(end - dest) * sizeof(uint16_t);
		}

		// convert to utf32
		if (encoding == encoding_utf32_be || encoding == encoding_utf32_le)
		{
			uint32_t* dest = r_u32;

			// convert to native utf32
			uint32_t* end = utf_decoder<utf32_writer>::decode_wchar_block(data, length, dest);

			// swap if necessary
			xml_encoding native_encoding = is_little_endian() ? encoding_utf32_le : encoding_utf32_be;

			if (native_encoding != encoding) convert_utf_endian_swap(dest, dest, static_cast<size_t>(end - dest));

			return static_cast<size_t>(end - dest) * sizeof(uint32_t);
		}

		// convert to latin1
		if (encoding == encoding_latin1)
		{
			uint8_t* dest = r_u8;
			uint8_t* end = utf_decoder<latin1_writer>::decode_wchar_block(data, length, dest);

			return static_cast<size_t>(end - dest);
		}

		assert(!"Invalid encoding");
		return 0;
	}
#else
	PUGI__FN size_t get_valid_length(const char_t* data, size_t length)
	{
		assert(length > 4);

		for (size_t i = 1; i <= 4; ++i)
		{
			uint8_t ch = static_cast<uint8_t>(data[length - i]);

			// either a standalone character or a leading one
			if ((ch & 0xc0) != 0x80) return length - i;
		}

		// there are four non-leading characters at the end, sequence tail is broken so might as well process the whole chunk
		return length;
	}

	PUGI__FN size_t convert_buffer_output(char_t* /* r_char */, uint8_t* r_u8, uint16_t* r_u16, uint32_t* r_u32, const char_t* data, size_t length, xml_encoding encoding)
	{
		if (encoding == encoding_utf16_be || encoding == encoding_utf16_le)
		{
			uint16_t* dest = r_u16;

			// convert to native utf16
			uint16_t* end = utf_decoder<utf16_writer>::decode_utf8_block(reinterpret_cast<const uint8_t*>(data), length, dest);

			// swap if necessary
			xml_encoding native_encoding = is_little_endian() ? encoding_utf16_le : encoding_utf16_be;

			if (native_encoding != encoding) convert_utf_endian_swap(dest, dest, static_cast<size_t>(end - dest));

			return static_cast<size_t>(end - dest) * sizeof(uint16_t);
		}

		if (encoding == encoding_utf32_be || encoding == encoding_utf32_le)
		{
			uint32_t* dest = r_u32;

			// convert to native utf32
			uint32_t* end = utf_decoder<utf32_writer>::decode_utf8_block(reinterpret_cast<const uint8_t*>(data), length, dest);

			// swap if necessary
			xml_encoding native_encoding = is_little_endian() ? encoding_utf32_le : encoding_utf32_be;

			if (native_encoding != encoding) convert_utf_endian_swap(dest, dest, static_cast<size_t>(end - dest));

			return static_cast<size_t>(end - dest) * sizeof(uint32_t);
		}

		if (encoding == encoding_latin1)
		{
			uint8_t* dest = r_u8;
			uint8_t* end = utf_decoder<latin1_writer>::decode_utf8_block(reinterpret_cast<const uint8_t*>(data), length, dest);

			return static_cast<size_t>(end - dest);
		}

		assert(!"Invalid encoding");
		return 0;
	}
#endif

	class xml_buffered_writer
	{
		xml_buffered_writer(const xml_buffered_writer&);
		xml_buffered_writer& operator=(const xml_buffered_writer&);

	public:
		xml_buffered_writer(xml_writer& writer_, xml_encoding user_encoding): writer(writer_), bufsize(0), encoding(get_write_encoding(user_encoding))
		{
			PUGI__STATIC_ASSERT(bufcapacity >= 8);
		}

		~xml_buffered_writer()
		{
			flush();
		}

		void flush()
		{
			flush(buffer, bufsize);
			bufsize = 0;
		}

		void flush(const char_t* data, size_t size)
		{
			if (size == 0) return;

			// fast path, just write data
			if (encoding == get_write_native_encoding())
				writer.write(data, size * sizeof(char_t));
			else
			{
				// convert chunk
				size_t result = convert_buffer_output(scratch.data_char, scratch.data_u8, scratch.data_u16, scratch.data_u32, data, size, encoding);
				assert(result <= sizeof(scratch));

				// write data
				writer.write(scratch.data_u8, result);
			}
		}

		void write(const char_t* data, size_t length)
		{
			if (bufsize + length > bufcapacity)
			{
				// flush the remaining buffer contents
				flush();

				// handle large chunks
				if (length > bufcapacity)
				{
					if (encoding == get_write_native_encoding())
					{
						// fast path, can just write data chunk
						writer.write(data, length * sizeof(char_t));
						return;
					}

					// need to convert in suitable chunks
					while (length > bufcapacity)
					{
						// get chunk size by selecting such number of characters that are guaranteed to fit into scratch buffer
						// and form a complete codepoint sequence (i.e. discard start of last codepoint if necessary)
						size_t chunk_size = get_valid_length(data, bufcapacity);

						// convert chunk and write
						flush(data, chunk_size);

						// iterate
						data += chunk_size;
						length -= chunk_size;
					}

					// small tail is copied below
					bufsize = 0;
				}
			}

			memcpy(buffer + bufsize, data, length * sizeof(char_t));
			bufsize += length;
		}

		void write(const char_t* data)
		{
			write(data, strlength(data));
		}

		void write(char_t d0)
		{
			if (bufsize + 1 > bufcapacity) flush();

			buffer[bufsize + 0] = d0;
			bufsize += 1;
		}

		void write(char_t d0, char_t d1)
		{
			if (bufsize + 2 > bufcapacity) flush();

			buffer[bufsize + 0] = d0;
			buffer[bufsize + 1] = d1;
			bufsize += 2;
		}

		void write(char_t d0, char_t d1, char_t d2)
		{
			if (bufsize + 3 > bufcapacity) flush();

			buffer[bufsize + 0] = d0;
			buffer[bufsize + 1] = d1;
			buffer[bufsize + 2] = d2;
			bufsize += 3;
		}

		void write(char_t d0, char_t d1, char_t d2, char_t d3)
		{
			if (bufsize + 4 > bufcapacity) flush();

			buffer[bufsize + 0] = d0;
			buffer[bufsize + 1] = d1;
			buffer[bufsize + 2] = d2;
			buffer[bufsize + 3] = d3;
			bufsize += 4;
		}

		void write(char_t d0, char_t d1, char_t d2, char_t d3, char_t d4)
		{
			if (bufsize + 5 > bufcapacity) flush();

			buffer[bufsize + 0] = d0;
			buffer[bufsize + 1] = d1;
			buffer[bufsize + 2] = d2;
			buffer[bufsize + 3] = d3;
			buffer[bufsize + 4] = d4;
			bufsize += 5;
		}

		void write(char_t d0, char_t d1, char_t d2, char_t d3, char_t d4, char_t d5)
		{
			if (bufsize + 6 > bufcapacity) flush();

			buffer[bufsize + 0] = d0;
			buffer[bufsize + 1] = d1;
			buffer[bufsize + 2] = d2;
			buffer[bufsize + 3] = d3;
			buffer[bufsize + 4] = d4;
			buffer[bufsize + 5] = d5;
			bufsize += 6;
		}

		// utf8 maximum expansion: x4 (-> utf32)
		// utf16 maximum expansion: x2 (-> utf32)
		// utf32 maximum expansion: x1
		enum
		{
			bufcapacitybytes =
			#ifdef PUGIXML_MEMORY_OUTPUT_STACK
				PUGIXML_MEMORY_OUTPUT_STACK
			#else
				10240
			#endif
			,
			bufcapacity = bufcapacitybytes / (sizeof(char_t) + 4)
		};

		char_t buffer[bufcapacity];

		union
		{
			uint8_t data_u8[4 * bufcapacity];
			uint16_t data_u16[2 * bufcapacity];
			uint32_t data_u32[bufcapacity];
			char_t data_char[bufcapacity];
		} scratch;

		xml_writer& writer;
		size_t bufsize;
		xml_encoding encoding;
	};

	PUGI__FN void text_output_escaped(xml_buffered_writer& writer, const char_t* s, chartypex_t type)
	{
		while (*s)
		{
			const char_t* prev = s;
			
			// While *s is a usual symbol
			while (!PUGI__IS_CHARTYPEX(*s, type)) ++s;
		
			writer.write(prev, static_cast<size_t>(s - prev));

			switch (*s)
			{
				case 0: break;
				case '&':
					writer.write('&', 'a', 'm', 'p', ';');
					++s;
					break;
				case '<':
					writer.write('&', 'l', 't', ';');
					++s;
					break;
				case '>':
					writer.write('&', 'g', 't', ';');
					++s;
					break;
				case '"':
					writer.write('&', 'q', 'u', 'o', 't', ';');
					++s;
					break;
				default: // s is not a usual symbol
				{
					unsigned int ch = static_cast<unsigned int>(*s++);
					assert(ch < 32);

					writer.write('&', '#', static_cast<char_t>((ch / 10) + '0'), static_cast<char_t>((ch % 10) + '0'), ';');
				}
			}
		}
	}

	PUGI__FN void text_output(xml_buffered_writer& writer, const char_t* s, chartypex_t type, unsigned int flags)
	{
		if (flags & format_no_escapes)
			writer.write(s);
		else
			text_output_escaped(writer, s, type);
	}

	PUGI__FN void text_output_cdata(xml_buffered_writer& writer, const char_t* s)
	{
		do
		{
			writer.write('<', '!', '[', 'C', 'D');
			writer.write('A', 'T', 'A', '[');

			const char_t* prev = s;

			// look for ]]> sequence - we can't output it as is since it terminates CDATA
			while (*s && !(s[0] == ']' && s[1] == ']' && s[2] == '>')) ++s;

			// skip ]] if we stopped at ]]>, > will go to the next CDATA section
			if (*s) s += 2;

			writer.write(prev, static_cast<size_t>(s - prev));

			writer.write(']', ']', '>');
		}
		while (*s);
	}

	PUGI__FN void node_output_attributes(xml_buffered_writer& writer, const xml_node& node, unsigned int flags)
	{
		const char_t* default_name = PUGIXML_TEXT(":anonymous");

		for (xml_attribute a = node.first_attribute(); a; a = a.next_attribute())
		{
			writer.write(' ');
			writer.write(a.name()[0] ? a.name() : default_name);
			writer.write('=', '"');

			text_output(writer, a.value(), ctx_special_attr, flags);

			writer.write('"');
		}
	}

	PUGI__FN void node_output(xml_buffered_writer& writer, const xml_node& node, const char_t* indent, unsigned int flags, unsigned int depth)
	{
		const char_t* default_name = PUGIXML_TEXT(":anonymous");

		if ((flags & format_indent) != 0 && (flags & format_raw) == 0)
			for (unsigned int i = 0; i < depth; ++i) writer.write(indent);

		switch (node.type())
		{
		case node_document:
		{
			for (xml_node n = node.first_child(); n; n = n.next_sibling())
				node_output(writer, n, indent, flags, depth);
			break;
		}
			
		case node_element:
		{
			const char_t* name = node.name()[0] ? node.name() : default_name;

			writer.write('<');
			writer.write(name);

			node_output_attributes(writer, node, flags);

			if (flags & format_raw)
			{
				if (!node.first_child())
					writer.write(' ', '/', '>');
				else
				{
					writer.write('>');

					for (xml_node n = node.first_child(); n; n = n.next_sibling())
						node_output(writer, n, indent, flags, depth + 1);

					writer.write('<', '/');
					writer.write(name);
					writer.write('>');
				}
			}
			else if (!node.first_child())
				//writer.write(' ', '/', '>', '\n');
				writer.write(' ', '/', '>');
			else if (node.first_child() == node.last_child() && (node.first_child().type() == node_pcdata || node.first_child().type() == node_cdata))
			{
				writer.write('>');

				if (node.first_child().type() == node_pcdata)
					text_output(writer, node.first_child().value(), ctx_special_pcdata, flags);
				else
					text_output_cdata(writer, node.first_child().value());

				writer.write('<', '/');
				writer.write(name);
				//writer.write('>', '\n');
				writer.write('>');
			}
			else
			{
				//writer.write('>', '\n');
				writer.write('>');
				
				for (xml_node n = node.first_child(); n; n = n.next_sibling())
					node_output(writer, n, indent, flags, depth + 1);

				if ((flags & format_indent) != 0 && (flags & format_raw) == 0)
					for (unsigned int i = 0; i < depth; ++i) writer.write(indent);
				
				writer.write('<', '/');
				writer.write(name);
				//writer.write('>', '\n');
				writer.write('>');
			}

			break;
		}
		
		case node_pcdata:
			text_output(writer, node.value(), ctx_special_pcdata, flags);
			//if ((flags & format_raw) == 0) writer.write('\n');
			break;

		case node_cdata:
			text_output_cdata(writer, node.value());
			//if ((flags & format_raw) == 0) writer.write('\n');
			break;

		case node_comment:
			writer.write('<', '!', '-', '-');
			writer.write(node.value());
			writer.write('-', '-', '>');
			//if ((flags & format_raw) == 0) writer.write('\n');
			break;

		case node_pi:
		case node_declaration:
			writer.write('<', '?');
			writer.write(node.name()[0] ? node.name() : default_name);

			if (node.type() == node_declaration)
			{
				node_output_attributes(writer, node, flags);
			}
			else if (node.value()[0])
			{
				writer.write(' ');
				writer.write(node.value());
			}

			writer.write('?', '>');
			//if ((flags & format_raw) == 0) writer.write('\n');
			break;

		case node_doctype:
			writer.write('<', '!', 'D', 'O', 'C');
			writer.write('T', 'Y', 'P', 'E');

			if (node.value()[0])
			{
				writer.write(' ');
				writer.write(node.value());
			}

			writer.write('>');
			//if ((flags & format_raw) == 0) writer.write('\n');
			break;

		default:
			assert(!"Invalid node type");
		}
	}

	inline bool has_declaration(const xml_node& node)
	{
		for (xml_node child = node.first_child(); child; child = child.next_sibling())
		{
			xml_node_type type = child.type();

			if (type == node_declaration) return true;
			if (type == node_element) return false;
		}

		return false;
	}

	inline bool allow_insert_child(xml_node_type parent, xml_node_type child)
	{
		if (parent != node_document && parent != node_element) return false;
		if (child == node_document || child == node_null) return false;
		if (parent != node_document && (child == node_declaration || child == node_doctype)) return false;

		return true;
	}

	PUGI__FN void recursive_copy_skip(xml_node& dest, const xml_node& source, const xml_node& skip)
	{
		assert(dest.type() == source.type());

		switch (source.type())
		{
		case node_element:
		{
			dest.set_name(source.name());

			for (xml_attribute a = source.first_attribute(); a; a = a.next_attribute())
				dest.append_attribute(a.name()).set_value(a.value());

			for (xml_node c = source.first_child(); c; c = c.next_sibling())
			{
				if (c == skip) continue;

				xml_node cc = dest.append_child(c.type());
				assert(cc);

				recursive_copy_skip(cc, c, skip);
			}

			break;
		}

		case node_pcdata:
		case node_cdata:
		case node_comment:
		case node_doctype:
			dest.set_value(source.value());
			break;

		case node_pi:
			dest.set_name(source.name());
			dest.set_value(source.value());
			break;

		case node_declaration:
		{
			dest.set_name(source.name());

			for (xml_attribute a = source.first_attribute(); a; a = a.next_attribute())
				dest.append_attribute(a.name()).set_value(a.value());

			break;
		}

		default:
			assert(!"Invalid node type");
		}
	}

	inline bool is_text_node(xml_node_struct* node)
	{
		xml_node_type type = static_cast<xml_node_type>((node->header & impl::xml_memory_page_type_mask) + 1);

		return type == node_pcdata || type == node_cdata;
	}

	// get value with conversion functions
	PUGI__FN int get_integer_base(const char_t* value)
	{
		const char_t* s = value;

		while (PUGI__IS_CHARTYPE(*s, ct_space))
			s++;

		if (*s == '-')
			s++;

		return (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) ? 16 : 10;
	}

	PUGI__FN int get_value_int(const char_t* value, int def)
	{
		if (!value) return def;

		int base = get_integer_base(value);

	#ifdef PUGIXML_WCHAR_MODE
		return static_cast<int>(wcstol(value, 0, base));
	#else
		return static_cast<int>(strtol(value, 0, base));
	#endif
	}

	PUGI__FN unsigned int get_value_uint(const char_t* value, unsigned int def)
	{
		if (!value) return def;

		int base = get_integer_base(value);

	#ifdef PUGIXML_WCHAR_MODE
		return static_cast<unsigned int>(wcstoul(value, 0, base));
	#else
		return static_cast<unsigned int>(strtoul(value, 0, base));
	#endif
	}

	PUGI__FN double get_value_double(const char_t* value, double def)
	{
		if (!value) return def;

	#ifdef PUGIXML_WCHAR_MODE
		return wcstod(value, 0);
	#else
		return strtod(value, 0);
	#endif
	}

	PUGI__FN float get_value_float(const char_t* value, float def)
	{
		if (!value) return def;

	#ifdef PUGIXML_WCHAR_MODE
		return static_cast<float>(wcstod(value, 0));
	#else
		return static_cast<float>(strtod(value, 0));
	#endif
	}

	PUGI__FN bool get_value_bool(const char_t* value, bool def)
	{
		if (!value) return def;

		// only look at first char
		char_t first = *value;

		// 1*, t* (true), T* (True), y* (yes), Y* (YES)
		return (first == '1' || first == 't' || first == 'T' || first == 'y' || first == 'Y');
	}

#ifdef PUGIXML_HAS_LONG_LONG
	PUGI__FN long long get_value_llong(const char_t* value, long long def)
	{
		if (!value) return def;

		int base = get_integer_base(value);

	#ifdef PUGIXML_WCHAR_MODE
		#ifdef PUGI__MSVC_CRT_VERSION
			return _wcstoi64(value, 0, base);
		#else
			return wcstoll(value, 0, base);
		#endif
	#else
		#ifdef PUGI__MSVC_CRT_VERSION
			return _strtoi64(value, 0, base);
		#else
			return strtoll(value, 0, base);
		#endif
	#endif
	}

	PUGI__FN unsigned long long get_value_ullong(const char_t* value, unsigned long long def)
	{
		if (!value) return def;

		int base = get_integer_base(value);

	#ifdef PUGIXML_WCHAR_MODE
		#ifdef PUGI__MSVC_CRT_VERSION
			return _wcstoui64(value, 0, base);
		#else
			return wcstoull(value, 0, base);
		#endif
	#else
		#ifdef PUGI__MSVC_CRT_VERSION
			return _strtoui64(value, 0, base);
		#else
			return strtoull(value, 0, base);
		#endif
	#endif
	}
#endif

	// set value with conversion functions
	PUGI__FN bool set_value_buffer(char_t*& dest, uintptr_t& header, uintptr_t header_mask, char (&buf)[128])
	{
	#ifdef PUGIXML_WCHAR_MODE
		char_t wbuf[128];
		impl::widen_ascii(wbuf, buf);

		return strcpy_insitu(dest, header, header_mask, wbuf);
	#else
		return strcpy_insitu(dest, header, header_mask, buf);
	#endif
	}

	PUGI__FN bool set_value_convert(char_t*& dest, uintptr_t& header, uintptr_t header_mask, int value)
	{
		char buf[128];
		sprintf(buf, "%d", value);
	
		return set_value_buffer(dest, header, header_mask, buf);
	}

	PUGI__FN bool set_value_convert(char_t*& dest, uintptr_t& header, uintptr_t header_mask, unsigned int value)
	{
		char buf[128];
		sprintf(buf, "%u", value);

		return set_value_buffer(dest, header, header_mask, buf);
	}

	PUGI__FN bool set_value_convert(char_t*& dest, uintptr_t& header, uintptr_t header_mask, double value)
	{
		char buf[128];
		sprintf(buf, "%g", value);

		return set_value_buffer(dest, header, header_mask, buf);
	}
	
	PUGI__FN bool set_value_convert(char_t*& dest, uintptr_t& header, uintptr_t header_mask, bool value)
	{
		return strcpy_insitu(dest, header, header_mask, value ? PUGIXML_TEXT("true") : PUGIXML_TEXT("false"));
	}

#ifdef PUGIXML_HAS_LONG_LONG
	PUGI__FN bool set_value_convert(char_t*& dest, uintptr_t& header, uintptr_t header_mask, long long value)
	{
		char buf[128];
		sprintf(buf, "%lld", value);
	
		return set_value_buffer(dest, header, header_mask, buf);
	}

	PUGI__FN bool set_value_convert(char_t*& dest, uintptr_t& header, uintptr_t header_mask, unsigned long long value)
	{
		char buf[128];
		sprintf(buf, "%llu", value);
	
		return set_value_buffer(dest, header, header_mask, buf);
	}
#endif

	// we need to get length of entire file to load it in memory; the only (relatively) sane way to do it is via seek/tell trick
	PUGI__FN xml_parse_status get_file_size(FILE* file, size_t& out_result)
	{
	#if defined(PUGI__MSVC_CRT_VERSION) && PUGI__MSVC_CRT_VERSION >= 1400 && !defined(_WIN32_WCE)
		// there are 64-bit versions of fseek/ftell, let's use them
		typedef __int64 length_type;

		_fseeki64(file, 0, SEEK_END);
		length_type length = _ftelli64(file);
		_fseeki64(file, 0, SEEK_SET);
	#elif defined(__MINGW32__) && !defined(__NO_MINGW_LFS) && !defined(__STRICT_ANSI__)
		// there are 64-bit versions of fseek/ftell, let's use them
		typedef off64_t length_type;

		fseeko64(file, 0, SEEK_END);
		length_type length = ftello64(file);
		fseeko64(file, 0, SEEK_SET);
	#else
		// if this is a 32-bit OS, long is enough; if this is a unix system, long is 64-bit, which is enough; otherwise we can't do anything anyway.
		typedef long length_type;

		fseek(file, 0, SEEK_END);
		length_type length = ftell(file);
		fseek(file, 0, SEEK_SET);
	#endif

		// check for I/O errors
		if (length < 0) return status_io_error;
		
		// check for overflow
		size_t result = static_cast<size_t>(length);

		if (static_cast<length_type>(result) != length) return status_out_of_memory;

		// finalize
		out_result = gixml ;
*
 return status_ok;
	}ersioPUGI__FN size_t zero_terminate_buffer(void* ------,------------, xml_encoding by Arsen) 
	{*
 // We only need to--------------- ify Kapoulk conversion does not do it for us
	#ifdef ----XML_WCHAR_MODE*
 14, by Arsenywchar by Arseny= get_d under the MI()versionif (r the MIT =ed under the MIT||ne@gm_endian_swap_utf
 * of th,ed under the MI))*
 arsen	-------length =2006- /2006-of( undetat the e	* --ic_cast< (kris*>(------)[2003, ] = 0----on 1.4
 (2003, b+ 1) *n Wegner (kristen@	----	#els**
 nd
 * of this fby Arsen the8
 * Copyrighet)
 */

#ifef SOURCE_P----_CPP
#define SOURKrist+ 1ugixml.hpp"ndifersion 1.4
 *ize-------------------14, parseugixml pload_file_impl(14, document& doc, FILE* clud, unsigned int options2014, by Arseny Kapoulkie (arsennd
!clud)  1.4
 makeIXML_NO_XPATH(* -----cludenot_foundat the e//Lice>
#	 PUGIX(canrser -  in I/O errors
 * Ct (C) 2006-CPP
#defPUGIXML_NO* ----------e <new> Licenclude.h>(
#	if.h>at the end


#ifdef _!= * -------
 * Copyrigfclosgma wapugixmn 1.4
 ifndef PUGIXML_NO_S

#ifdef pugixml.hp	f

// Formax_suffix	pragby Krisner (kristen@tima// allocport------tp://the whole>
#	) //ng.h>ownltent _MSnclude <string.h>
14, memory::ragma warning + was padded due )at the end
!between 7) // conditional expression is constant
#	pragma warn ------ut_ofdestrucpugixml.h))
#	pread <ostrin estrucf

// Forsafe due to fsafe(between , 1rning(,>
#	pugixmtional expressi
#	endi: 4793) //disa war * Copyrigct destructideon is non 4702) //ragma warning(disable: 4996) // this fio_
#enriable may be14, by Arsenyrea, by Arseny Licen------ee notice pugxml parn compileding(push// str 1.4
 doc.
#	inisable:inplace_ownon compiled----------------------g expression i, 
#	pragma wa),NO_EXCEPTIgma warning(di---------#ifnorg/
 *
 *NO_STL
	templport<typename T> struct PUGIstream_chunke (arsenncludened(__BORLANDC__)* cre nond
#endif
-
 * estruc = PUGIestruction is non-porofect d_BORLANDC__)rning	// strt
#	pranew ( or varned(__BORLANDC__) at tle may bened(PUG-
  destroy--
 * ptrd
#endif

#i_HEADER_ONLY)
C__)'_setjmp' and Cpush
#	pragma wa>(pragen@tima.//funcern -800chain pushwhostr(s insi pushopyrig push
#	pragma warnex parn -80->_puse push

#ifdef __INTEL_COMPILE insis not dn -8008  does not 4) //ut never ree warning go a: a co(0)sion i(0d
#endif
#	pragma diag_suppressag_pusway
t (C) 2006-p' makeT data[ declared _pag#	pragten WegnerT)]-----p' makral type
#endif

#i
#ifndef PUGIXML_NOe <new>
#	in_BORLANontr_noseek(std::basic_i_BORLA<T>&if deam,def _** unctCopyright (C) #elif naged
#arsenisable:holderdiag_ps(02014, _BORLANDC__)<T>_INTRLANDign())
#	psafe
#	prto arn -800lista diag_supptotalcement new
#inline))
#else* las par
#day
de
#en!c(noin.; di
 * Copyrig#	pragma war doeDC__) &&SNC__
// Using diC_ASSn -8008 ((noinline))
#else

#	pragme pushable:  insidif

# this function or vagma warn -8appendd[0]; }to/ Simple nd foERT()SERT(conERT(op doeh/diag_pe push"
attribu.ontrILE vol
#else
#	dailed[(csafe_VOLAto1] = {0}; (t char nctionag_popontr,setjmp' and CGI__Nc(noinnage>used; ding ::memcpy)constant
#eide pushng ::medue to _pending on h----->(t char gcount()CPP

#incTagma warn -8safemay sde <ailbit | eofNG_Lin case #if defi is less th	inclad 2003, , sorkaecktp://otherring>
#endine PUGI_t char bad()his nst char condi &pec(noin._USI())ing char loaded was decl
// Borlanguard against hug // insendif
 PUGIXis small enough worifnd this overflo-1] // Iwor

#ifdnd
assert+diag_pop is c< assering char loaded from memory via 			ONLY
#/diag_pop.h>
#en
#endif
cture was padded due to __declspec(align())
#	pcopyf

// Sim#endifontiguousrning(dteraction ning(di_setjmp' and C++ object destruction is nonONLY
#	le
#	pragma warningund forSOURCE_ng char loaded from memory via stacction writ to ------
// Bop://isabling this insailed[0]; }
// condition is always falseC_AS>an't aMC_VOL);else
#d[0]; }
iag_pop doed
#endif
assert(gi { n#	define PUGI__amespace +NS_BEGI impl memcpy_NS_EN, namespamcpy dne PUGI__NS_FN
#	NS_END fine PUGI__NS_END } }
#	 PUGI__NS_END=
#	define PUGI__FNGI__NO_IN.4
 MSC_VER) &*lif definenamespace im#else
due to asserversion 1.4
 * -----------------nlining controls
#if defined(_MSC_VER) && _MSC_VER >= 1300
#	dene PUGI__NO_INLINE __declspec(noinline)
#elif defined(__GNUC__)
#	define PUGI_nclude 2003, bof remaind do_VOLAir lonoinR_T_#endif
GI__NO_INLINE __decls::pos_#end po '_setMSVC-telle at tfor not dne Pgte__GI__Nios::eream>for ader incloff 2003, by Kory allocatio -
//n
PUGI__NS_BEGINporagma)
#	prag MSVC-specifhis 
// < 0c features
#if defined(_MSC_VE) && !defined(__S3E__)
#	ding(disable: 479e_t size)
s don't have it at2003, __FN void defaending on header include orry_manageme)unma2003, b||n_functi* ptr)
	{
		free(from memory via stacdefine PUGI__FN inline
#	define PUGI__FN_NO_INLImmovefault_	typedef_MSCymbol (& !defined(__Se<T>::aexce_EXCEP withC_VER < INE __
#if_NO_INLINE __a------- declared but never ury_managemend(__BORLAND anonymous namespac2014, fdef __INTEL_COMPIes inside namespaugi { 
	};

	template <typename T> allo not definiion_functionTef SOURCEugi {  depending on header include ortic dealloca_function_storag(__MEM_H_USING_LIST)
using std::memcpy;
using std::memmovanagemen(i.e. line 
#	 download newt st

// In some environments MSVCis a compiler but the CRT lacks certain MSVC-specific features
#if defined(_MSC_VEclude <stdint.h>
#edefine actualanagement_function_storage
	{ all)
#if defindefau PUGI__UGIXML_WCHAR_M<arsec dealloca constantlse
#	ifndef _UINTP.releasworkarouDEFINED
// UGIXML_WCHAR_Md(__BORLANDC__) &&ntptr_t in MSVC6 and in some WinCE versions
typedef size_t uintptr_XPATH
#	in_BORLAN <math.h>
#	include <flGI__NO_INLINE __declspec(noinlidef PUGIXML_NO_EXCEPTIONS
#		include <setjmp.h>
#	-
 * CopyriCPP
#def
// For placement new
#include <new>fdef _MS* ----------ssert(sifemplate has an>
#en NG_Lset, bailelif (me enwise 	return #	in_USI and we'll clears)
	{
		// unris a compilspecifidif

#ifndef PUGIXML_NO_STL
#	was declaredssert(s
#	iemplate  = default_usretune P-basGIXMmpleinclaEXCEe(copossibwarninnce it's faster		rettakesing st or var void default_d	return * ptdio.h>
#incMSVC-slen((); NLINlen(s);
	#flagstd::t could beMEM_Hby aODE
retulocattima.net)f _MSVER >= 1300
#	define PUGInoinli&Copyrigh&_INLINE
#4) //"

#in; *i; ++i) *dest++ = *i;
	dest = 0;
	}
#endif
PUGI__NS allocation_ng(disable: 4127dif

#ifndef PUGIXML_NO_STL
#agma warr referenced 
#	pragma warning(disable: 279) // controllinCopyright (Cs constant
#	pragma warning(disable: 1478 17Copyrigh----------------------Copyright (Cragma warning(disable: 1684) // conversion from E
#	inclu#if defined(------MSVC_CRT_VERSIONcate(
		}
__BORLANDC__ut the	void* resMINGW32ta;&& !	void* resSTRICT_ANSI__
 * ---------oat.h>openER
#	widILERsted undet* paendiatic const sizemodfine PUGI_de <std_wf__NS(_t xmlze =(data) de

#i---------ction betvert__t x_heaptatic const sizesragma arsen PUGI__static_cast<sifirst pass:gned __int16in e <sSIZEactes MSVCht (C) 2003, by Ktr2003, N
	str_t xmlf CRT lacks wideasde <s_begin = ~,n_functign())
#	pragma wargixml 
	}
trinourcection gixml paretjmp' and C++ object destruction is non-portab1ces inside ngixml or exceptd) { sta// second_alignm#else
 s);e <sonst ument -end(gixml _)
		{	static const uintpde <stdser - ver----------------
PUGI__NS_BEGIN
	static const size_t xml_memory_page_size =
	#ifdef//le: r_MSVCno
	snd!deffuncAR_MOtoI__NS 
	se_t xssioo our best betMSVCto trytatic _t xlocated_ma	3276atic =	#else
		32768
	#_t xes inside nmory);

	static const uintory_pageze =c_caASCII (we mi
	{L_MEMORate erfaced
#e und
			_ascii[4_CPP{0

/ {
#		define icond)
			[i]; ++i)		result->fri_CPPnclude <string.h>(t;
		} xml_memoryst<x

			xe: 4ml_memory_pagoat.h>gixml parMEMORY_PAG);
E_SIZEult->f xml_memory_66 /dummydint.h>
#eef xml_memory_manageme		size_t bruct xml_memory_page
	{
		E
#	includ---------bool savenclude <matatic ch.h>
#	include <float.h>
#	ifatic cnst sizeindent	
		return lhsar* sTIONS
#		include <setjmp.h>
#	endif
#endif

#falss=237 //14, gi { rER
#ugi { r expressiongma t fr_NS_ENr,whole pagruct xmconversion f insinle: e* next;
#en_setjmp' makee of '_setjmp' make_memory_page || 
#dedif

#ifndef PUGIXML_NO_XPATH
#	inng(disab<math.h>
#	incl < cuct*de <fl14, n_sizeaving mrootline)
ng expression i) 2006-20def PUGIXML_NO_EXCEPTIONS
#		include <setj, offseis_mutaConveoffseown PUGt siz#elif defineurn 0; //$ rfdef Pinputdint.h>
#e PUGI__between '||r place= 0am>
#	include UGIXMLpage* all
	struct buffer_isable: 279) // =PUGIX::ng(disable: 279) // controlling expression is cof unsigned priv warning(d = 0;
	sizetring, even if CRT lac2003, by d) { staable:alignm#else
	leter(deleter_);
#endiisable: 279) //ing expression i,		if (!memod = s;
		alignmifndef PUGIXML_NO_STL
#	unction or variable; //$ rdelete originalml_memorif we performed#	if doad new inside own restring, !			reween '&&ng expres)root->a
#ifdef __INTEL_COMPILER
#	pragmast uintptorarning(disabloffset_debuntptrdoc->#	ifndef _UINTPTR_; //$ rXML_Nt new
#includize_t dre _MSid* allocaXML_Nr::XML_N			assert(page);e <fladdingO_EXCEP_function_stomember_cast<uintptrres.emory_page_isable: 279) //;

			// rab oe = xml_memorying t resuopyrighusmemos	{
pon/ Contp://NTEL_COMPreturn ween 'himself_memory::dea||ocate(page->memory)) lse
#	ifndef _UINTPTR_setof(xml_memo
	{
		------NS_ENDpageif
space pugi
arse
#ifndef PUGIoot), _busyallocaoot), _busy--
 * clude):ge->dma wa_mp.h>
#---------------ef __		assert(ptr >= pgi { ata
		-
 * mcpy de = xml_memmp.h>
#	disable: e* next;ize +=mcpy d as nativion_functionoat.h> expre
	#end--
 )sk = 8;t* deunfortu----lyy_pacan's at propze, 
	{handar* idund from pointer to same-sized integize;

			assert(pt= 0;
 page->data &= 0;
UGI__NO_INLIoE __dec;
!= rhs[;

	raits
		voi spec(noin178 arroweed_size&
					,xml_meed_sizeFN voize);
			(void)!pt			page->busy_size = page->freed_size = 0;
					_busy_sonst si= 0;
				}
				elonst si				{
					assert(_root != unct			assert(papage);
y_size);
			(void)!ptr;

			page->fsy_size ize += size;
			assert(page->freed_size <= nd
sert(_root !=
#		define PUGI__!			assert(p_FN
#	sert(_root !=->ize +=re>busypret */

ng occupi*>		if t string length
	PUGI__FN size_espaces insND

#if !dedefine PUGI__Nze_t size = sizeo uintptris c% __declsonst si)t_cast<void*>	ll_size = (string_header) + length * sizeoonst sizr_t);
			
			// round size up to pointerten Wegner& ~(size alignment nt16_t page_offset; // 14, tree_walk + s

			if (!head=178_depthage->prev);
ic v page));

			if (!header~) return 0;

		->prev);

					// rege(s

			if (!header setup)ring o
	#ifdef PUGIXML setu
	{
		static xml_moffse

			if (!header 1);
 for wor&
	#ifdef PUGIXMtru
#endif

#ifndef e_offset = static_cast	str6_t>(page_offset);

			// full_size == 0 for l14, attribut= page-age->busy			//age-age->prev);

					// removeage->busy_size == full_sider->full_sizst-case page-ze && pag(1 <<->prev);

					// re

#ifdef __unspecified_offs_= static_cast<uint16_t>(ful***_offset == 0));
			header->full_size rt(_ator|| (page->busy_sugh void* to avoi#end= 0 && page_offset < (1 age- ?ough void* to avoid 'cast incre :e, data) + data_size;e_offset ing
			// header i!= 0 && page_offset < (1!er + t*>(static_cast<void*>(header + 1));
		}

		==ata
		uintage->busy& r 0 && page_offset < (1(gh for== r.oid* 
		PUGIptrdiff_t pagd*>(header + 1));
		}

		vs through void* to avoid 'cast increases required ali!nment of target ttic_cast<void*>(header + 1));
		}

		< through void* to avoid 'cast increases required ali<ment of target type' warnings
			// we're guaranteed the> through void* to avoid 'cast increases required ali>ment of target type' warnings
			// we're guaranteed the<s through void* to avoid 'cast increases required alic, d_memory_string_header*>(static_cast<void*>(string)) - 1;s through void* to avoid 'cast increases required ali>f it was allocated via allocastatic_cast<cheader + 1));
	_pus == full_siz which should be enough for cize == full_sioid* op doeoid* to av) :ize;

			deale whole page
			size_t full_size = header->full_previous == 0 ? page->busy_size : header->full_&&ugh fo->y_sit16_t>(fullce_memory(header,_size;

			deallocate_allocate_memory_ full_size, page);
		}

		xml_memor sizeof(chsizeeader + 1));
	asocatice xml_memory_pagdef 'cast increases required alillocator::value) ?cation_thresh : = a
	{
		static xml_m

			asze / 4;

		xint(ge(s= allocate_page(size <= lalignment hreshf (!pgh for c? xml_memory_pa0,) ret
	{
		static xml_mdef PUGIXML_Nge = page;

		ifu (!pdef PUGIXML_N return 0;

		if (size <= large_allocatnd ofthreshold)
		{
			_root->busy_size = _busy_sizedou

ge = page;

		if
		el(
		els return 0;

		if (size <= large_allocatrt pagethreshold)
		{
			_root->busy_size = _busy_sizefloainsert page at the eleted(leted  return 0;

		if (size <= large_allocat (see threshold)
		{
			_root->busy_size = _busy_sized*>(header + 1));
	aso avo(offse return 0;

		if (size <= large_allocat pagethreshold)
		{
			_root->busy_size = ml.org/
 *
 *HAS_LONGa;
	trdiff_t paglong

namge = page;

		ifl
na(
namespac return 0;

		if (size <= large_allocat'name=threshold)
		{
			_root->busy_size = _busy_size;

			/
namespace pugi
{
	/// A u'name=uct(impl::xml_memor
			page->prev = _root;
			_root->next = ute_struct
	{
		/// Default ctor
		xml_a6_t page_offset; // offsege = page;

	emptyoid deallocate_string(char_t* string)
		{
			/xml_memory_page_size / 4;

if
ge->busy_size : header-arge_allocation_tif
old ? xml_mif
:/
 *
 *TEXT(""ocation_threshold = xml_memory_page_size / 4;

hresh	xml_attribute_struct* prev_attribute_c;hreshold ? xml_memory_pate (cyclic list)
		xml_attribute_spage->fge = page;

	hashallocage->busy_size : header-function_storage
	{eader) + length *nd opt>pretructuse std::meuint16_t>(full_size ze)
	le page
			size_t full_sizest-case pge = page;

	>busynal_objecy;
which should be enough fo whole page
			size_t full_size&is function casts pointerory_page* page rh// une <= pe_alloca(aren = 0;de <std*
#earget type' warnings{
		}

		uintptr_t header;

		xml_nge(sirent;					///< Pointer to parent

		char_t*				ibute(0)
		{
		}

		uintptr_t header;

		xml_ndef PUGIXML_Nalue;					///< Pointer to any associated string data.

		xml_node_struct*		first_child;			///< First
		elsarent;					///< Pointer to parent

		char_t*					name;					///< Pointer to element name.
		char_t*		offsealue;					///< Pointer to any associated string data		return page->data;
	}
PUGI__NS_END
{
		}

		uintptr_t header;

		xml_n
namespacalue;					///< Pointer to any associated string data.

		xml_node_struct*		first_child;			///< First child
	truct, public xml_allocator
	{
		xml_document_struct(xml_m		uintptr_t header;

		char_t* name;	///< 

		ode_struct*		parent;					/able:er(reinry_page* root):nstant
#	praalignmstrcpy_insitullocate_mame,ld)
		{head}

d* allocate_memessionsser_ragma wad_mask,blic arget tyype' warnings
			// we're guaran//< Pointea_buffers;
	};

	inline xml_allocator& get_allocat(const xml_node_struct* node)
	{
		ahresht(node);

		return *reinterpret_cast<llocatemory_page*>(node->header &ml_memory_page_pointer_mask)->allocator;
	
		xml_node_st-level DOM operations
PUGI__NS_BEGIN
	inline_allocat#else
ute_struct* allocate_attribute(xml_allocator& alloc)
	{
		xml_memory_page* page;
		void* memory = alloc.allocate_memory(size child
		
		xml_node_st_struct), page);

		return new (memory) xml_attribute_struct(page);
	}

	inline xml_node_struct* allocate_node(xml_allocator& alloc, xml_node_type type)
	{
		xml_memory_page* page;;			///< Right brot_struct), page);

		return new (memory) xml_attribute_struct(page);
	}

	inline xml_node_struct* allocate_node(xml_allocator& alloc, xml_nodexml_memory_page_pointer_mask)->allocator;
_BEGIN
	struct x_struct), page);

		return new (memory) xml_attribute_struct(page);
	}

	inline xml_node_struct* allocate_node(xml_allocator& alloc, xml_node_t		return page->data;
	}
PUGI__NS_END
ge_pointer_mask)->allocator;
truct, public xml_allo_struct), page);

		return new (memory) xml_attribute_struct(page);
	}

	inline xml_node_struct* allocate_node(xml_allocator& alloc, xml_node_type type)
	{
		xml_memory_page* page;
		void* 		uintptr_t header = n->header;

		if (header & impl::xml_memory_page_name_allocated_mask) alloc.deallocate_string(n->name);
		if (header & impl::xml_memory_page_valdeleter(data)org/esult = data_offset; // offseheader i&& through void* to avoily_pa_BEGIN
	struct xtruct* poffs)lh);
	rhtring data.

		xmle_struct* chi|| = n->first_child; child; )
		{
			xml_node_struct* next = ||ild->next_s(full_size, page));

	 worer) re wor			//addiage->prev);

					// removeeinterpret_cast for worst-case pp<xml_memopheader
			ptrdiff_t pagnd-trip through void* to avoid 'cnter_mask));alignment of target type' warneinterheader is guaeinterointer-sized alignment, which should be enougaddir char_t
			return stat worhar_t*>(static_cast<void*>(heade= node_elementvoid deallocate_string(chaaddi whole page
			size_t einter		}ment)
	{
		xm 1);
turn 0;

		if (size <
		{
	(type);
st_ch->_page_child_root-st_che whole page
			size_t child)
		{
			xml_node_	str* last_child = first_child->pre->next_sibling =ptrdiff_t page_ofeinter, name(0),hild->pr	{
			node->first_cs- 1);
_c(0), next_sibling(0)->first_child = chev_sibling_c;

			laic_cast<char_>next_sibling = child;
			child->pre->first_child = child;
			child->prev_schild;
			first_child->pr
		return child;
	g_c = child;
		}
		else
		{
	siblin_rangen is  worshild->pr>t), reinterst_chreng_c = child;
		}
			turn 0;

		xml_attribute_struct* (ibling__pagd;
	#el data.

		xml_nod 0;

		xml_attriamedribute_struct* first_attribute = noxml_memory_pagxml_mde->first_attribute;

		if (first_attributev_attribute_c;

_mask)ev_attribute_c;
(st_ch(= a;
.st_ch>next_s, = a;
ry for ttribute_c = a;
		ibute_stfirst_atruct* last_attribute = first_attribute->p->first_child = ch first_attriild->prev_ode->first_attribute;

		if (first_attribu

// Helper classefull_>prev_sibling_,uct* a = ac)
	{ge->next = _root;

			_root= node_elementrs through vo(pageid 'cast increases requireype);gnmentxt_sibling = child;
			e;

		xml_node_struct*ode utilities
PUGI__NS_BEGIN
	inline uint16_t if it _swap(uint16_t value)
	{
		return static_castr
			xml_mes
PUGI__NS_BEGIN
	inline uint16_t xml_m= child;
		}
		else
		
PUGI__NS_END

// Uni;

			// ds
PUGI__NS_BEGIN
	inline uint16_t of(xm00) << 8) | ((value & 0xff0000) >> 8) | (value >emory_page* pn ((value & 0xff) << 24) | ((value & 
	}

	inline ui((value & 0xff0000) >> 8) | (value >>type result, uint32_t ch)
		{
			// U+0000..U+ccupie_swap(uint16_t value)
	{
		return sta//< Pointer to attribute name.t_child;
	ptrdiff_t pagxml_memory_page_sieinter

		xml_attribute_struct* prype);lloc_c;
	///< Presult + 4;_node_struct
	{
		/// Default ctor
attributeif
2_t)
		{ild = allocate_node(alloc, type);
// condition is value_typ>(		retu;

		r &turn *reinterpret_cast<#ende*>(n_dea1 ful worsnulative high(value_type result, uint32_t)
		{ext attribute
	};

	/// An return result +hreshold ? < 0x800)
_node_struct
	{
		/// De}
		else
		{
			nofirst_attribute next_attribute = a;
			a->prev_att xml_axt_sitor& getpret_cast< impl {
#		defi worst-case pn reg_c;

			last_ch; i;F
		ie_memor/ Coetjmp.h 0x3F)= st
	st&
			if = 0;qualelse
, = st///<		return result + 		size_t return result + 2;
ype - 1)), parent(0), name(0)s for code generatiostatic_cast<uint8_t>(0x80 | (ch & 0x3F));
				return resuze, page);
		pl {
#		defi, name(0), value(0
			else
			{
age->busyesult[0] = staticy(header, nt8_t>(0xE0 | (ch >> 12));
				result[1] = static_ not dtatic value_type high		siznstant
#	praalue_type high(val8_t>(0xC0 | (ch >> 6));
				result[tatic_cast<ustatic_cast<uint8_t>(0x80 | (ch & 0x3F));
				return result + 2;
tic vo
			// U+0800..U+FFFF
			else
tatic_cast<uesult[0] = static_cast<uint8_t>(0xE0 | (ch >> 12));
				result[1] = static_cast<uint8_t>(0x80 | ((ch >> 6) & 0x3F));
				result[2] = static_0x80 | ((ch >> 6) & 0x3F));tic_cast<uint8_t>(0x80 | (ch & 0x3F));
			return resnd
value_type any(valu		return result + 
			return result + eturnne PU
	};

	struct utf16_counter
	{
		typedef size_t valy_size;
0x3F));
			result[3] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
			return result + 4;
		}

		static value_talloc_cast<u_c; = static_cast<ult[0] = s		{
			*resulch)
		{
			return (ch < 0x10000) ? low(result, ch) : high(result, ch);
		}
	};

	struct utf16_counter
	{
		typedef size_t val

	struct utf16	static value_type low(value_type result, uint32_t)
		{
			ret		{
			*resuleturn result + 1;
		}

		static valuch);

			retualue_type result, uint32_t)
		{
			return result + 2;
		}
	};arepy;
llocate_node(alloc, type);
(0xD800 + msh);
}

 full_siruct utf16_counter
	{
		typedef size_t val_memo(0x80 | (ch & 0x3F));
				return result + 2;
			}urn *reinterpret_castmemog to eader) + length *	{
		typedef size_t v> U+0000..U+007F
			if (ch < 0x80)
			po>busy			*reUGI__NS_BEGIN		{
			rion_functionurn *reintgnment, leaving >(lue_->ragma wor
		}
			
		return a;
tle
;

		stat// fde->first_attribute;

		
	{
			// U+0800..U+FFFF
	 result, uint32_t)
		{st_chype
		xml_node_structx3F));
				return te (cyclic list)
		n result + 4;
		}

		static value_t			{
				result[0] = static_cast<uint8_t>(0xE0 esult[ >> 12));ise_typt>(0x80 atic_cast<uint
			retUGI__NS_BEGINte (cyclic list)
		xml_attribute_struct* next_attribult, uint32_t ch)ext_attribute = a;
			a->prev_attribute_}
		else
		int32_t ch)
				result[2] = static_cast<uint8_t>(0x80 |+10000..U+10FFF		static value_type any(value_type 

			deall		// U+10000..U+10FFF full_size, page);
		}

		xml_memory_page* _root;
		einterERT(nt32_t ch)
		{
			*result = static_can result +NO_INLINE xml_atst<uint8_t>(ch > 255 ? '?' : ch);

	_page)
	{
		const size_t large_allocation_threshold = 	typedef size_t val
			last_ch		static value_type any(value_type result, uint
			last_chh)
		{
			return (ch < 0x10000) ? low(result, ch) pe re>
	{
		typedef uint16_t type;
	

			*result = st_chi	typedef utf16_counter counte;
			result[1] =)
		{
			return (ch < 0x10000)
PUGI__NS_END
fer* extra_buffers;
	};

	inlineswitch (nment,N void widstd:: worspi:char_selector<declarHAR_Mzeof(wchar_t)>:eXML_WCzeof(nst xml_node_struct* node)
	sult + 4;>next_s;

		return *reinterpret_cast<xml_memory_page*>(node->headallocefaulte <typename T_allocator_memory xml_memory_page_pointeselector<stor;
	}
PUGI__NS_END

// Lowcounter;
	typedef wchar_selector<sizeof(wchar_t)>:contrzeof(wchar_t)>:pze)
			{
				uint8complate <tywchar_t)>::oc#ende <typename Traits, typename opt_swap = ct* allose> struct utf_decoder
	{
		static
	{
		xml_memory_page* pageue_type decode_utf8_block(const uint8_t* ult[2] = static_cast<uint8_t>(0x80 | fdef nt8_t>(ch >ext_attribute = a;
		*result = nment, !=;

	templatpedeund-trip throug:writer wch	static value_type high(val

		stic_cast<uint8a(urn *r<uintptr_t>(data_ll_swap eturn *ricenurn resule;

	
	#else.fer* extr= a;
 >> 12) & 0x3F))a				result[2] = static_cast<uint8_t>(0x80 |preintptr_t>(data) & 3) == 0)
					{
						// round-trip through void* to silence 'cast increases required alignment of target type' warnings
						while (agma wa>(0x80 | ((cconst uint32_t*>(static_cast<conable:nt> xml_mealue_type high(value_tst void*>(data)) & 0x80808esult, uint32_t ch)
	

	{
			// U+10000..U+10FFFFauto_ptr-l

	
#		define nt of _page)
	{
		cons =(lead_page)
	{
		cons impl  Traits::low(result, (alignt of _NS_END

#if !def 0x80)
				{
					result = & utf8_byte080..Ulocate_memory(header,t = Traet ty 255 ? '?' : ch);

	 2;
				}
			x80808080) == 0)
						{
							result = Traits::low(reinUGI_lt, uint32_befora) & 3) == 0)
					{tring och void* to avoiull_size : 0		}
ound-trip through void* to silence 'cast increases requ||lt, (.//< Poiquired alignment of target type' page bounurce)ast<unsignbe
naic_ca	charstatic_cast<unsigned int>(cu, evutf8_ utf8_by static concur::allocate_memory_oob(size_t size,)(static - 0xF0) < 0x08 && si= 2 && (datstat!
			// U+10000..U+10FFF	static value_type high(value_tlow(result, data[3]);
							data += 4;
							size -= 4;
						}
					}
				}
				// 110xxxxx -> U+0080..U+07FF
				else if & ~0xE0_cast<unsi0xF0) < 0x08 && size >= 4 && (data[
					ask));
					data += 4;
					size -= 4;
	 2;
				}
			
#if !def if (static_cast<unsigned int>(lead -	// 1110xxxx 0) << 6) | (data[1] ask));
					data += 4;
		t<const10xxxx -> U+0800-U+FFFFtic inline	#elseask));
					data += 4;
		ned int>(lead - 0xE0) < 0x10 && size >= 3 && (data[1] & 0xc0) == 0x80 && (data[2] & 0xc0)afe(ro0)
				{
					result = Traits::low(result, ((lead & ~0xE0) << 12) | ((data[1] & utf8_byte_mask) << 6) | (data[2] & utf8_byte_mask));
					data += 3;
					size -= 3;
				}
				// 11110xxx -> U+10000..U+10FFFF
				else if (static_cast<unsigned int>(lead - 0xF0) < 0x08 && size >= 4 && (data[1] & 0xc0) == 0x80 && (data[2] & 0xc0) == 0x80 && (data[3] & 0xc0) == 0x80)
				{
					result = Traits::high(result, ((lead & ~0xF0) << 18) | ((data[1] & utf8_byte_mask) << 12) | ((data[2] & utf8_byte_mask) << 6) | (data[3] & utf8_byte_mask));
				size -= 4;
				}
				// 10xxxmemory(header,= 2;
					size -= 2;
				}
						data += 1;
					size -= 1;= 2;
					size -= 2;
				}
				// 1110xxxx -> U+0800-U+FFFFic_cast<unsigned int>(nextvalue_type decsult;
		}

		static inline6_t* data, size_t invalid
				else
				{
0808080) == 0)
						{
							result = Traits::low(re<uintptNE i through void* to avoiprotomp.h>
#	endifblock( 0x80)
				{
					result = Traits::high(reusy_size)<uintptr_t>(data)block.

		xo parentxml t voiPointet32_t*pe
		xruct xml_memory_page
	{
		static xml_m	result = Traits::low(result, da::value_type decode_utf32_block(const uint32_t* data, size_t size, typename Traits::value_type result)
sult, data[0]);
	t32_t* end = data + size;

			while (data < end)
			{
				uint32_t lead = opt_swap::value ? endian_swap(*dat (data[NE i < end)
			{e decode_utf32_blockult = Traits::low(result, ((lead & ~0xE0_t* data, size_t size, typename Traits::value_type result)

			while (data < end)t32_t* end =_true U+10000..U+10FFFF
				else
				{
					result = Traits::high(result, lead);
					data += 1;
				}
			}

 == 0x80)
		sult;
		}

		static inline typename Traits::value_type decode_latin1_block(const uint8_t* data, size_t size, typename Traits::value_ty == 0x8ult)
		{
			for (size_t i = 0; i < size; ++i)
			{
				result = Traits::low(result, data[i]););
				result[ Traits:
	{ch)
		{
			 sile{
						// rotruct(p
			w nod
			}
	{
#e->nment,,size_t (ch & 0x3F));
			return resecode_wchn	while (size >	static vast<const uint32_t*>(static_c	return d utf8_byte_ize_t endast increases requnt void*>(dte (cyclic lisxml"nd)
			{
				un2_t msh = static_cast<uint32_t>(ch - 0x10l(const uint32_t* data, size_t size, typename Traits::value_type result)
		{
			return decode_utf32_block(data, size, result);
		}
							d>(0x80ypename Traits::value_type decode_wch			}
	equired alignmlt + 2;
			}n;
			nt32_t c
			elseult = Trait worst-case plead - 0xC0) < 0x20 				re 2 && (data[1] & 0xc0) ==0; i < len	{
			*result = Traits::lo			*resultlead & ~0xC0) <<char_endian_0; i < _NS_END

#if !defvoid convert_wchar_endian_ length)
	{i = 0; i < letatic_cast<uFF
				else if (static_cst_chi) result[i] = 2_t)
		{(const wchar_t* data, size_t size, typename Traits::value_type result)
		{
			return decode_wchar_block_impl(lue_type res		returnt32_t* data, size_tult = Traitss
PUG wor size, typename Traits::value_type result)
		{
			return decode_utf32_block(data, lengtode;
			his wct_parse_length; +x80 && ((ch & 0x3F));
			reture T> PUGI__FN void convert_utf_endian_swap(T* result, const T* data, size_t length)
	{
		for (size_t i = 0; i < length; ++i) result2_t)
		{a = 16,	// \0			result[0] = static_cast<usize_t
	};

	static const unsigned char chctor<sizeof(wch			data += 1;
					si_selector<sizeof(wchi = 0; i < len_t* data, size_t 
	};

	static const una-z, ic_cast<wchar_t>(endian_   0,   0,     0
	};

	static const unctor<sizeof(w_t)>::type>(data[i])));
	}
#endif
PUGI__NS_END

PUGI__NS_BEGIN
	enum chartype_t
	{
		ct_parse_pcdata = 1,	// \0, &, \r, <
	< end)se_attr = 2,		// \0, &, \r, ', "
		ct_parse_attr_ws = 4,	// \0, &, \r, ', ", \n, tab
		ct_space = 8,			// \r, \n, space, tab
		ct_parse_cdata = 16,	// \0, ], >, \r
		ct_parse_comment = 32,	// \0, -, >, \r
		ct_symbol = 64,			// Any symbol > 127, a-z, A-Z, 0-9, _, :, -, .
		ct_start_symbol = 128	// Any symbol > 127, a-z,A-Z, _, :
	};

	static_cast<uint8_t, 192, 192, 192,    192,; i < length; ++i) result[i] =  0,      0,   12,  12,  0, ; i < length; ++i) result[i] = static_cast<wchar_t>(endian_, 192, 192, 192,    192,     0,   0,  5
		0,   0,   0,   0,   0, 0,   // 16-31
	   0,   0,   0,   0,   0,_t)>::type>(data[i])));
	}
#endif
PUGI__NS_END

PUGI__NS_BEGIN
	enum chartype_t
	{
		ct_parse_pcdata = 1,	// \0mpl(const uin & 3) == 0)
					{
						/ecode_wchresult)
		{
		}
		el

	templat

		statsize;

 (data[3] & utf8_bne typename Traits::value_type decode_wchar_block_ireinterpret_ca192, 192, 192,
		192, 192, 192, 192, 192, 192reinterpret_ca  192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 19,  192, 0,   1,   00)
				{
					result = Traits"
		ct_parse_attr_192, 192, 192, 192,  192, 0,   1,   0  192, 192, ,ct_par, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 19 == 0x80)
				{
					result = Traits
	{
		ctx_special_pcdata = 1,   // Any symbol >= == 0x8 < 32 (except \t, \r, \n), &, <, >
		ctx_special_attr = 2,     // Any symbol >= 0 and < 32 (except \t), & Traits::value_type des
PUGblock(const ui192, 192, 192, 192, 192, 192,  t32_t*
	typep' makes a fu= 8;
an_swarecursiv
			py_skipruct xml_	static
		3,   page)
		{
			i 192, 192, 192, 192, 192,    192, 192, 192, 192, 1] =
	{
		3,  3,  3,  3,  3,  3,  3,  3,     3,  0,reinterpret_ca 2,  3,  3,     // 0-15
		3,  3,  3,  3,  3,  3,  3,  3,     3,  3,  3,  3,  3,  3,  3,  3,     // 16-31
		0,  0,  2,  0,  0,  0,  3,  0	}
			}

			return result 3,  3,  3, chartypex_t
	{
		ctx_special_pcdata = 1,   // Any symbol >= 0 and 2,  3,  3, t \t, \r, \n), 0,  0,  3,  0,  3,  0,     // 48-63

		0,  20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,    // 64-79
		20, 20

		static inlin, 20, 20,    20, 20, 20, 0,  0,  0,  0,  20,    // 80-95
		0,  20, 20, 2	return decod0, 20,    20, 20, 20, 20, 20, 20, 20, 20,    // 96-111
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 0,  0,  0,  0,ize, typename Tremov	data += 4; & 3) == 0)
					{
						/_memory_p20, 20,    20, t32_t ch) = a;
		}
			
		return  20, 20, 20, 20, 20, 20,    20, 20, 20aits::low(result
	inline xml_arse_cdat!_type d operations
PUGI__NS -= 3;
				}
				// 11110xxx -> U+10000..U+10FFFF
				else if (d alig
				}
			}
de
#en;
					data += 4;
					size -= 4;
		, 20, 20,ine typename Traits::valf8_byte_mask0x80 && (data[3] & 0xc0) == 0x80)
* root): _royte_mtype decode_utf16_bloc);
				}signed int>(next - 0xDC00) < 0x400)
					{ typename Traits::valuene PU		{
			retta[3] & 0xc0) == = Traits::high(result, 0x10000 + ((lead & 0x3ff) < typename Traits::value>(c) < 128 ? tabl, 20, 20, 20
	};
	
#ifdef PUGIXM// 10xxxxxx or 11111xxx -> invalid
				else
								}
					else
			ne PU if (static_cast<unsigned int>(l				}
					else
	er
	{
		#	defin, 20, 20, 20type dst<const uint32_t*>(static_ca			}

			ret// full_size == 0 for large stri20, 20, 20, 2192, 192, 192, 192,
		192, 192, 1
		20, 20, 20,192, 19
		else
	, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 2192, 192, 19bol > 127, , 20, 20, 20, 20, 20, 200; i < his w16,	// \0, ], >, \r
		ct_parse_gned int>(c) < 1ic_cast<wchar_t>(endie_t 0, 0, 0, 0, 0,           // 112-127

		192,   0,   0,   0,      MPL(c, ct, table) (tabounter;, 192, 192, 192, 192,    192, 192, 192, 192,   0,   0,   0,      e_endian() ? encotatic const unsigned char chaing_utf16_b
	{
		55,  0,   0,   0,   0,   0,  92, 192, 192, 192, x_table)

	PUGI__selector<sizeo92, 192, 192, 192, igned int ui = 1;_cast: encodterpret_cast<unsigned char*>(&ui) == 1;
	}

	PUGI__FN xml_encodPUGIXML_NO_XPATHhartypex_table[25red "depreze;
			aoid* memory = xml_memory::allocate(size + xml_memory_page_alignmenurn 0; //$ rd0 == 0xef &&age ulkinvalidtp://templats/
#	incl two strime Traits::value_type resul, 20,    202, 192, 1ator = _root->allocator;

			return pa<uintptin 0 &&
			// Uf unsigned 
#	incl192, er
	{
		typegnment, leaving memo allocator;

ype result, uint32_t)
		{
: high;
			// U+ (sizeofdo = statf unsigned extra	_busy_sh void* (urn wcpage*&e: 40 && d1 =frag d1 =gs
		iedundaso				}		{
	return buftrin typenction	{
		typedef size_t value_typ
#defurn *reinting_u0xef &&*ding_ut encoding_utf16_be;
		ifail, but is >d3 =eturn resexml_memused; die it's zero terminated),alue_ze)
			{
		lue_ing(disable:ing_uator = _root->allocator;

			return page;
		}

		stast uintpave = a;; = a;6 uie: 4: encar_t			_e NULL  == 0xmemor,   -ome en PUGIiona,   );
	mismatche T> llions bre ttecte)
tencodtop levelcture
			xmaddi| (chc value_ty8
	else if (st	// rep const uintemory_page
			xml_memory_page* e*& out_page)
		{
			if (_be block with somee <fl0xff) rd1 == */
-
 *;

ze_t sry;
			O_EXCEPTIage->memor* rooncoding =
#endi_function_stopage*&har ncoding with utf i{
		// gn())
#	prddding_utf16_le;_cashe/ Simple ing_ue_memory(size_t sizef32 encod_push/d earlfail, but isoc(siznness
		if (encod =ding_u page)
		{
			if (page
	template <> struct wchar_select2, 192, _b;

		retur0)
				{
					result = Trnst sizeR_MO, 192licit encoding is reqhresholc_cast<uint8_t>(0x80 | (ch & 0x3F));
			return result + 4;
		}

		static value_t			{
				result[0] = static_cast<uint8_t>(0xE0 | (ch >> 12));
				result[1] = static_
#ifdef __SNype result, uint32_t ch)
	r thi U+10000..U+10FFFFa;st uiific en= static_cast<uic) < 120 | (ch >> 12));
				ress requested		PUGI_n res_VOLATILE uint8_t d0ct* alla
			retu? data[3];node_struct
	{
			statich >> 6) & 0x3F));ch >> 1d but ne
	};

	struct utf16_counter
	{
		typedef size_t valbe;

		// only do autodetection if ns requested
		if (encoding != encoding_auto) return encoding;

		// skip encoding autodetection if input buffer is too small
		if (size < 4) return encoding_ution, Appendix F.1)
		const uint8_t* data = static_cast<const uint8_t*>(contents)

		PUGI__DMC_VOLATILE uint8_t d0 = data[0], d1 = data[1], d2 = data[2], d3 = data[3];

		return guess_buffer_encoding(d0, ast<uint8_t>(0x80 | ((ch >> 6) & 0x3F));
				resulp page freed, just reset sizes
		ml_mem/ \para 2;
		}thr (kris deaimhildding_auto) retuecode_wch,  3o, 20	char_* deM_VER	if py.) // strulength +usy_size)MODE
* end =gned int>(lead - ml_e	}

		N void wideMODE
	PUeturn (le == ene push/pop tf(xml_enral  le, xml_encoding rpl {codi+=true;
	}re == encoding   // 16-392, 192, 1wap(encoaway
#endif_header
	{
		uint16_t page_offset; // > struct wchar_selector<2templat/ on
		reit encoding i	size 0;

	true;
	}

#ifdef PUGIXML_WCHAR_istre	PUGI__FN booCurth; +search&& d2 xt.ng(disable:20, 20, 20	size_mutable)[0]ator& get_stre_IS_CHARTYP		const  endrue;
	}
ion_failed[(cAbsol			e
		; e.g. '/foo/bar's));
	 out_l

		.: highre == ++	sizeway
#endifFN bool convert_bufse2 == 0=rt_bufgned int>(lead*har_t*>(data);ar_t*>(conte 
			cha*>(data namespa const_cast<char_t*>(data)
	);

		

			out_buft_wchar_endian_swap(th = && 		char_t* buffer =!ffer, data, length);

			)
	 static_cast<conn_swap(buffocate((length + char_t* data = staticext_attribute =atic_>(data);

		((length + 1) * sit_wchar_an_swap(bufferffer, data, lenan_swap(buff_IS_CHARTYP_endian_swap(buff'.'= st(char_t)));
+ 1
			if (!buffer) retude_utf8_blockis_mu32_be);
	}

	PUGI__an_swap(bufft->be;
	}
_encoding(uint length + 1;
		}

		r*f(char_t)));+1eof(v

		return true;
	}2

	PUGI__FN bool convert_buffer_utf8(c	}

		char_t*& out_buffer, size_t& out_length, const void* con) // condit
			// U+0800..U+FFFFjif (is_mu	else
			{
				resj;ter>:j, 192,    192, 192, ef __SNnd
gth, (ch >> 12));
				re		xmlr of suing_uata, lengt depending on he it at	char_t* buffer =- static_cast<oding(d0,ef __SNXML_WCHAR_sub conten

//_castj
		// first pass: get length in wchar_t units
		stents);

return fat> xml_memeturn faf(wchart* dad but neool get_mutable_buffere (ascii) blocks
		ing get_wchar_etraoad arse_a		if (!head& (!head
						/(!head. << 16 = -_WCHARUGIXML_WCHAR_arg- 1);	PUGI__FNze_t lenglock(da 1);
		assert(c && d1 ==* root): _root(rCHAR_MODif (tor<2>
	{
f(wchar_t)>::tycutents);
		s++lock(data, dahar_writdone (fdef __SNC__;

		asfor_eafalseculead - 0n + length);_buffer_oend _buffer_ length * sizeof_allocatorchar_t)>return trchar_t*>
	{
r_t)));
		if (	template <typenamze_t ] & 0xc0)p)
	{
		conset_cast<wcharoding(uintt*>(ue_type;

		ing(d0, int16_t*>(ue_type;

		<const 
#if !def;
		if (// Borl	retC++PUGIXar
		const utic consfirst pass: get le;
		= 0x80	char resut*>(size;

byte_masconst u
		if (	--= static_cast<const u
		// firscoding_utf16_l_t data_lt size, xc0) == 0x80	charing(d0, d
		// first pass: get length in<wchar_wr code
#enduallocmory::allocat
#if !defined(_MSC_ock(data, dataa_lesult = Trait;

		ash = le	char_t*	_utf32_l16 inpute_s:value_		/// Default ctor
		/// \paraeinterde type
		xml_node_struct(impl::xml_memory_page* page, xml_node_type type): heade
		ctterpret_cast< worst-case
		}
			
		return a;
 worst-case p    // 64-79, prev_sibling_c(0), next_sibling(0),t_child;
			
		if (fir;

			92, 192, (!page->data &_size(rtring occupies whole page
	};

	struct xml_allocator
	{
		xmory::allocate(s setuding_auto) return encoding;

		unsigned int 14, 

		ed->data 0x78 &&st uint32{
		}
age* allocate_paglignm  192outputdeleterst uint32,pe obe

		xml_memory_pngth, 

			out_buffer = buffer;
			out_length =eturn true;
	}

	t = 0;
					_busy_size = 0;
				}
				else
				{
				opt_swap> PUGI__FN bool convert_buffer_utf32(char_t*& out_buffer, size_t& out_length, const void* cohar_t* allocate_s_size(ro size = siin =

	t
		}

		xml_memory_page* allt);

		// firstar_t units
		size_t length = utf_decoder<wchar_co
					page->next->prev = page->prev;

				tring occupies whole page
	};

	struct xm	char_t* buffer = static_cast<char_t*>(xml_memory::allocate((length + 1) * sizeof(char_t)));
		if (!buffer) nse. St);

		// fir6_t page_offset; // ptrdiffr);
		wchar_

		void* 
		typedef uint for worst-case pang_utf1 0 && d2ing(disable:0, 20, 20,lengtconvert_wchar_endi_VER < 1300 // MSVC6 6_be;
		if (d0 == 0x3c && d1)e_memorying(disable:amespaces
#		dGI__FN bocounter;
	typedef wchar_selector<
#	incle <typename Td) { staiter;

	template <tywchar_t)>::writer wchar_writer;

	sizeof(7FF
			else i0..U+007F
			if (ch < 0x80)
			xml_memory_page*>(nold -1 xml_memof suit-mespace impl {
				uint8_t lead = *data;

		 lead = *data;

				// 0xxxxxxx -> U+0000..U+007F
				if (length = data_length;

		// allocate 
	{
		xml_memory_plength
		char_tesult[fer = static_caype decode_utf8_bloclength, (ascii)  = next;
		}

		for (xml_node_struct* child = n->firsts
PUGld; )
		{
			xml_node_struct* next = child->next_sibling;

			destroy_node(child, alloer>::decode_latin1_block(data, data_length, o_memory(n, sizeof(xml_node_struct), r sizer) ret// f for worst-case paddi<xml_memofer(calloc, xml_node_type type = nongth);
		*oe	}


#	dgh(result, ch);
		}
	};ngthresult + 1;
		}
atic_ca result, 192, 192, 1tf_decoder<wchar_count);
				else
			{
				res  19coding		192,  static_cast<uint8_t>(0xEesult + 1;
		}
t_paratic_cast<uintding(		}

			ret, data) + data_size;

 xml_encoding encoding, cons_new>(header)
		out_length = led - 0const obegin + turn false 1) * si;
		}

		static va)., 192, 192,    192_t lea).;

		out_buffer+ 1;
		}
	};

	struct u
	PUGI__FN<xml_memory_page*>(header & xml_mnd-trip through void* to avoid 'cI__FN booI__Falignment of target type' warnut_lenheader is guaut_lenointer-sized alignment, which should be enougconst v char_t
			return statct utar_t*>(static_cast<void*>(headefer_utf8(out_bvoid deallocate_string(chawapping istf16_be || encoding == encodin//< Pointer to attribute namding is age, data) + data_size;xml_memory_page_siut_lenge{
		typedef uint

		// only endian-swapping ite_struct* p = stdx800)
			{pt_falseibute (cyclic list)
		xml_attribute_struct* next_attribut_len		xml_memory_page* page = allocate_page(si_utf16(out_buffer, out_length, contents, size, opt_false()) :
				convge_size : size);
		out_page =ze, opt_t (!page) return 0;

		if_utf16(out_buffer, out_length, contents, s<= large_allocation_
		t:
				convot->busy_size = _busy_size;

			// insertze, opt_tnd of linked list
			page->prev = _ ? encoding_utf32_le : encoding_utf32_be;

			return (nand of_encoding == encoding) ?
				convert_buf
		else
	ze, opt_trt page before the end of linked ? encoding_utf32_le : encoding_utf32_be;

			return (nart page encoding == encoding) ?
				convert_bufleted evenze, opt_t (see deallocate_memory)
			a ? encoding_utf32_le : encoding_utf32_be;

			return (na (see ize);

		assert(!"Invalid encoding");
ding == encodint = page;
			_root->prev = pag ? encoding_utf32_le : encoding_utf32_be;

			return (na pageize);

		assert(!"Invalid enc		return page->data;
	}
PUGI__NS_END

namespace pusize, opt'name=value' XML attribute structu ? encoding_utf32_le : encoding_utf32_be;

			return (na'name=ize);

		assert(!"Invalid encoding");
uct(impl::xml_memory_pafer, out_ler(reinterpret_cast<uintptr_t>(page)), name :
				convert_buffer_utf32(out_buffer, out_length, contenallocate buffer of suitable length		uintptr_t header;

		charut_lenseemory_struct*		parent;					/

		// only endi(oen size, is_mh, contents, sdn ?_node_struct* node)
dn	result =gth,::low(result, lead);
					data += 1;
					size -= 1;
 :ntents, sitf16_be || encoding == encodin_t*>	
		xml_node_st = utf_decoder<utf8_writer, opt_swap>::decode_utf16_block(d_attribute_structgth, obegin);

		assert(oend == obegin + length);
		*oend = 0;

		out_buffer = buffer;
		out_length = length +  child
		
		xml_node_strue;
	}

	template <typename opt_swap> PUGI__FN bool convert_buffer_utf32(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, opt_swap)
	{
		const uint32_t* data = ;			///< Right brotrue;
	}

	template <typename opt_swap> PUGI__FN bool convert_buffer_utf32(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, opt_swap)
	{
		const uint32_t* data = _BEGIN
	struct xrue;
	}

	template <typename opt_swap> PUGI__FN bool convert_buffer_utf32(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, opt_swap)
line void destroy_node(xml_node_struct* n, xml_al length + truct, public xml_allorue;
	}

	template <typename opt_swap> PUGI__FN bool convert_buffer_utf32(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, opt_swap)
	{
		const uint32_t* data = static_cauffer = buffer;
		out_length = length + 1;

		return true;
	}

	PUGI__FN size_t get_latin1_7bit_prefix_length(const uint8_t* data, size_t size)
	{
		for (size_t i = 0; i < size+ 1;

		return true;
	}
intptrfer_utf8(out_b_node_struct*		parent;					///<r to any associated string data.

		xml_nodint8_t*>(contents);
		siz	
		xml_node_struc// get size of prefix that does not need utf8 conversion
		size_t pref child
		
		xml_node_struc// get size of prefix that does not need utf8 conversion
		size_t pref;			///< Right brother// get size of prefix that does not need utf8 conversion
		size_t pref_BEGIN
	struct xml_buffer
	{
		char_t* buffer;
		xml_extra_buffer* next;
	};

	struct xmint8_t*>(contents);
		siztruct, public xml_allocatin1_7bit_prefix_length(data, data_length);
		assert(prefix_length <= data_lengts: get length in utf8 units
		size_t length = prefix_encoding_utf32_le && re == encodiut_lenconst void* contents;
		}

		staticonst 		// first panext;
		}

		for (xml_node_struct* child = n->firstint8_tld; )
		{
			xml_node_struct* next = child->next_sibling;

			destroy_node(child, allo
		// second pass: convert latin1 input to u_memory(n, sizeof(xml_node_struct), reinte_struct*erpret_casc = a;
		buffer, size_t& out_length, xml__writer>::decode_latin1_bloc&, \r, ', "
		ct_par: _wrapcoding, _siz

	 // 1le == encodistfix, postfix_length, obegin + prefix_length);

		ass for worst-case paefry for worst-case p32_t chlength);ref*oend = 0;32_t chr = buffer;
		out_lening get_wcharin + prefix
// Unicode utilities
Pin + prefhild-	static value_type any(gth)e : encgnmehs.sion requirellocd = 0;required
		if coding_utf8)< 8) | ((value & 0xff0000) >> 8) * contents, size_t <uint16_t>(((valuable)
	{
		// fast path: no conversion requireif i	if (encoding =||encoding_utf8) ng_utf16et_mutable_buffer(ouffer(out_buffer, ouintptr contents, size, is_mut*
		typedef uint PUGI__sion requiro parent

	sion table_buffer(out_buffer, ou
		*oend =* contents, size_t ->ng_utf16_le : encoding_utf16_be;

			return (ncoding witing) ?
	>(&sion _t* deBCC32= utf_decoder<return (native_encodil is_mutable)
	{
 is_little_endian() ? encod++_mutable);coding_utf16_be;

			rsion requiredrsion requir92, 192, 192, 192, ssociated string data.

		xml_nodnvert_buffer_/ source encoding is utf32
		iigth, xml_et_length =endian() ncodinge obegin =++e obegin = reinteenco
			return (native_encodiue());
		}

		// source encoding is utf32
--_mutable);sion  == encoding_ulingion rx10000) >> 10;
			:encoding_ <> struct w
			xml_encoding native_encoding = is_little_endian() ? encoding_utf32_le : encod--g_utf32_be;

			return (native_encoding == enc--ing) ?
				convert_buffer_utf32(out_buf
		enum { value = 0 };_size == full_satin1_block(postfix, postfix_length, ding");
		return false;
	}
#endif

	PUt = Traits::low(result, ( Any symbol > 127,r_t*& out_buffe		20,ize_t& out_length, xml_encoding encodinegin(const wchar_t* str, size_t length)
	{
		esult, uint32_t ch)
	ool convert_buffer(char_t*& out_buffer, size_t& out_length, xml_encoding encoding, const ding");
		return fas pointers through void* to avable)
	{
		// fast path: no conversion red alignme	if (encodge_allocacoding_utf8) return get_mutable_buffer(out_buffer, out_length,cast<uint8_t*>(buffer);
		u proper (pointer-sizedder<utf8_writer>::decode_wchar_block(str, lengthng_utf16_be ||E_IMPding == encoding_utf16_le)
		{
			xml_encoding native_enco	}

		uintptr_t headerle_endian() ? encoding_utf16_le : encoding_utf16_bfor (size_t urn (native_encoding == encoding)required apret_cast<uint8_t*>(buffer);
		ubuffer, out_length, contents, si
		std::string r:
				convert_required ar_utf16(out_buffer, out_length, contents, size, opt_true()L_NO_STL
	PUGI__FN e
		buffer[size] = 0;
	}
	
#		if (encoding == encodingfor (size_(str, length,k(str, lengt				}
					else
			ssociated string data.

		xml_node_struct*hild = child;_t size)
	{
		const uint8_t* _utf32_be;

		tf_decoder<wchar_coe_encoding == encoding) ?
				convert_buffer_utf32(out_buffer, out_lpl(const char* str, size_t size)
	{
		const uint8_	convert_buffer_utf32(out_bhresholdut_length, cont32_t ch)
	e, opt_true());ent of target ty wchar_t units
		size_t length = utf_decoder<wchar_counter>::decode_utf8_block(data, rt_buffer_latin1(oullocate resulting string
		std::bas size, is_mutable);

		assert(!"Invalid encoe = a;
			a->prev_erpret_ttribute_c = a;
		out_

		ry_page*>(header & xml_memorylt;
	}
#endif

	inline bool strcpy_insit&, \r, ', "
		ct_patring occupies wtaticlength);
		*oend = 0;

		out_buffe,allow(statice_t length, uintptr_t allocated, char_t* target)
	{
		assert(target);	}

	PUGI__FN bool convert_buffer(char_t*& ngth = strlength(target);

r, size_t& out_lengthffer memory if possible
		if (!alling get_wclt;
	}
#endif

	
// Unicode utilitiesev_attribute_c;

		// fast path: no conversion required
		if (encoding == encoding_utf8) return get_mutable_buffer(o& (target_length < reuse_threshold || target_lengt<uint16_t>(((varget_length / 2);
	}

	PUGI__FN bool strcpy_insitu(chng_utf16_be || encoding == encoding_utf16_le)
		{
			xml_encoding native_encoding = is_lse_threshold || target_lengting_utf16_le : encoding_utf16_be;

			return (native_encoding == encoding) ?
				conse_threshold || target_lengtbuffer, out_length, contents, size, opt_false()) :
				convert_buffer_utf16(out_buffer, out_length, contents, size, opt_true())arget_length / 2);
< reuse_threshold || target_lengt		if (encoding == encoding_utf32_be || en;

		//6) & 0x3F));ue;


		// source encoding is latin1
		if (eev_attribute_c;
dest && strcpy_insitu_allow(source_le_utf32_be;

			g zero terminator)e_encoding == encoding) ?
				convert_buffer_utf32(out_buffer, out_le;
		}
		else if (dest && strcpy_insitu_allow(source	convert_buf		{
tf16_be;
ata +=
		{
			// 

	struct utf16old buffer,oundary
			silocate neopt_true());
		}

	nt8_t>(0xtruct(p
				res			// w	{
			old bu		// allocate new buffer
			char_t* buf = alloc-coding_utf32_lt copy the new data (including zero terminator)
			memcpy(dest, source, (source_rt_buffer_latin1(out(char_t));
			
			return true;
		 size, is_mutable);

		assert(!"Invalid encoXML_NO_XPATH_busy_size IXML_NO_):ize_t s+;
		ret

		ou decla,;

							 279) // controll_auck(const uder_mask) alloc->deallocate_strinheader is pagexml_node_struct(impl::xml_f _Msize_t strlengcontents, size, opt_trction ->deallocate_strindescri_EXCE
	struct gap
	counter;__NS_BEdef wchar_sele* -------: result,"Noroot ="atic_cast<chSTL
#	include <istre gap).
		Fusy_sasions utf8(pse evious gap.
	was decl gap).
		E
	{dst),   fromge->d/= 0;

			if (end) // thunction or va gap).
		C
	{
ons ragma wardefaul
			if (end) // thenow allocated gap).
		I

		oroot ==occt vo
					if (end) // thunrecognized_tagrt) to [old_gap_starcons----e tag silepse previous gap.
	badunit gap already; cxml_enco0 && d1 =:writer wch/proces_encod(__vingion
			if (end) // th		
			// 0x// "merge" two gaps
					// 0/ Collapse all gaps, ree)
	// "merge" two gaps
		CDATAtptr
		// Collapse all gaps, r00..U+00// "merge" two gaps
			end = s;ta, s:writer wch/ Collapse all gaps, r_t lead// "merge" two gaps
		Pve [old_gap_end, current_pos) to [ostare);
	}
cast<char*>(s) - reintern s -;
		if (tag/ Collapse all gaps, rage->busy_// "merge" two gaps
		h void* age->busy/ Collapse all gaps, rntptsize;
			}
			else return s;h = 	};
	
	PUGI__FN char_t* strre)
		{
	_xml_encohar_t* s, S s --h = t* soxml_encopse previous gap.
	n encoding_utf32_lehar_t* s, Unmemo#endifdef _  19s:ding_unt, ltt* s) void* 		re
#	inclpse previous gap.
	no && d3 == size;
			}
			eNo		end = s;for (;;)
		{
			cast<wchar_wre;

				kn:deaCollapse ;
		wchar_writer::va= 0 && d3 == == 0 && d3 ==tu_al-------ge->prev)	 bug workaroncoding native_enco) - 'a') <rpret
							ut;					/BORLANDCndian_swap(out_buffer;

			) - 'a') <reet t			break;
						else (ch | ' ') - 'a' + 10);
						return stre;

						ta
		uint16_t full_block(const ui						c;
			}
			// U+080(statict32_t*th = length + f16(c1] & 0xc0) / sizeof(uint16_t); Traits::valuu allocated via alloca			return stre;


#	pragma w{
  = 10 *ck
			s32_le;
		if (d0initialory_sentinelalue_	}
------STATIC_ASSERT 0 && d1 == 0x3c) erpret_cast_dea0 && d1 == 0x3c) gnment, leaving_deaurn *reinterpret_cast<align d1 =<o __declsn or varign())
#	praign upward8;
	lue_tb
		arning(__
#	psionsymbol iseader) + length *h utf16ge, xml_node_type type): heade not mak+n requir		++stre;
				}

			#if-_all & ~ncel
							return star_t*>(utf8_writize, xml_mered = ast<ch
			
u					ok for utf16 < followed by nodeurn *reinterpret_cast(page
			

			rn or variable PUGI__n strth + 1) 		retbusyNED
// urn *reinterpret_cast<.h>
#e))
#	pragma war doeaddielse if (to a wast<	retays us		ch = *++stre;
					}
f (*stre  += 1;
	5
		0,   0,   0, 192, 192, 1intpttup(ch == ';')
								return resul than utf8 since it's zer, uint32_t)
		{

			// U+0800..U+FFFF
				return stre;


						e		ucsc = 10 * ucsc + 32_le;
		if (d0#	defin source page
							{
ign upwardlocate urn *reinterpret_INTEL_COMPIL			retur<const
#	ifndef 
#def may be un					g.ping_utf16_les, :(chanone@gmail.					g.plink1_blelse retu, repy'rert, ...)d f
	}0 && d1 =urn result= 0;

	(this may fail, but is better than utf8 since it's zer++stre == 'o' && *++stress
		if (encoditurn isbetter thspecific enrn stre;
		d
  encoding wivoid* allocate_memory_oob(size_= '<';
					+way
#endif (*++stre =dynams, stre - rt(pe utch == ';')
	 (ing tef und(PUGg all sym = 10 * 	{
		typedef size_t vaaddiessiotype;

		static value_type low(value_type result, uint32_t)
		{
			return result + 1;
		}

 = 10 * ucsc + ++stre == resu++stre ==re - sre;
				}
				OM detected, = 10 * 
					return stdef size_t value_type			}
				pragmer::a; stre == 'u'csc = 10 *  'u' && *++stre == 'o' && *_push/dity macro f
				break; 'u' && *++strurn resul_INTEL_COMPIessiof (*stre ==& endch == (lue_typpragma  = 10 *  char = 10 * 		{
			, data) + p page freed, just reset sizes
					poding_utf16_le;
) - 'a') <
#	iUGI__NO_INLINE __decunter, opt_swap>::decode_utf32_block(datdef PUGIXML_NO_EXCEPTIONS
#		include <setjmp.h>
#	tre;

					i32_be;

			0; i < count; ++iize / s= 0;
	}dianness
		if (endian_swap(out_buffer, oomment)) ++s;
		
			if (*s == '\r') // Either a s		wchar_writer::value_type obegin = reinterpredef PUGIXML_NO_EXCEP
				
				if (*s == '\n') g.push(s, 1);
			}
			else if (s[0] == '-' && s[1] == 'nse. S
		uint8_t* obegin = reinter '>')) // comment ends here
			{
	(buffer);
		un compiled
			}
			else if (*s == 0)
// Forctle_tive
	 Arseny(  3, ;
	const  reqgixml.org/
 *
 * This library is distributeemory_page_* strconv_cdatpushp"

#in14, by Arseny Kapoulk/ replace fie <sne wit
#	include <wchare block witeprecated"rn false;2003, LER
#	praPP

#include "p== '-' && s[1] == '-' && ENDSWITH(s[2], '>')) // comment ends here
			{
&& ptr sizeof(chart_buffe*s++ = '\n'; // replace first one with 0x0a
				
				if (*s == 'oat.h>
#	ext;

		size, "rbt)
	== '\n') g.push(s, 1);clude <matize / s
#	ifere
			{
				*g.flush(s) = 0;
				
				return s + 1;
			}
			else if (*s == 0)onst size_t x		return 0;
			}
			else ++s;
		}
	}
	
	typedef char_t* (*strconv_pcdata_t)(requir__NS_BEGIN
	stgap g;L
	template <typename opt_trim, typename opt_eol, typename opt_escape> struct strconv_pcdata_impl
	{
		static char_t* parse(xef && d1 == 0xbb && d2 == 0xbf) return encoding_utf8;

		// look for <, <? or <?xm in vario			if (*s == '\n') g.push(s, 1);ck with some&ap
				{
					if (*++stre == 'o' && *++stree utf16 encoding with utf16 with specific endianness
		if (encoding == encodi}

			case.flush(s);

					if (opt_trim::value)
						while (endable: 14--
 * UGI__IS_CHARTYPE(end[-1], ct_space))
							--end;

					*end = 0;
					
					return s + 1;
				}
				else if (opt_eol::value && *s == '\r') // Either a single 0x0d or 0x memory;
			dianness
		if (enc// fone with 0x0a
					
			 xml_memory_pa'\n') g.push(s, 1);
				}
				else if (opt_escap78 17::value && *s == '&')
				{
					s = strconv_escape(s, g);
				}
				else if (*s == 0)
				{
					char_t* end = g.flush(s);

					if (opt_trim::value)
						while (end > begin && PUGI__IS_CHARTYPE(end[-1], ct_spact_spa0x0a
					
					if (*s == 			return stre;


		{emplate <typename opt_swap> PUGI__FN bool convert_buffer_utf32(char_t*& out_buffer,ding_auto) returwap)
	{
		const uint32_t* data = static_cast<const uint32_t*>(coxE0)ar* so&
		mat->data_bomode_uf (*s == ! replace filuf;1ents);
		sizeBOM alwaye = p				en 'replcodelt +  U+FEFFpage*jus)
i { nusing 	
			if (*s ==
		ml.org/
 *
 * This library 	def PUGIXML_Nbom)
	xfef_sizeon xml_mst uint32.ize +=ion_functionage->pre(data					rp"

#inlse>::parse;
		case 5:'\xef', '\xbb		case f'pl<opt_t
#	inc{
				if retue 2: return strc<unscreases requresuurn sth
		iwriter wche opt_ncoding_utf1se>::parse;
		case 5:te (cyclic lis<?xml oad new=\"1.0\ncodkaround foe <stdlib.h>
#includ, opt_f_t* data = statipt_true>::parse;
		dpl<opt_fa=\"ISO-8859-1e); return _false, opt_true>::pars?		ca> strcon	//_true, opt_true, opt_raw)
	}

	typedef char_t* '\n strconmpl<opt_ntents);
		size_t data_length = size / sizeof(uint32_t)st<vo first pass: get length in wchar_t units
		size_ escapes trim)tf_decoder<wchar_counter, opt_swap>::decode_utf32_block(data, data_length, 0);

		// allocate buffer of suitable lengthstatic_cast<char_t*>(xml_memory::allocate((length + 1)
		{
		}

		xml_memory_page* allocatet bitmask for flags (eol escapes trim) input to wchar_t
		wchar_writer::value_type obegin = reinterpret_cast<wchar_writer::value_type>(buffer)	while (true)
			{
				while (!PUGI__IS_CHARTYPE(*s, ct_parse_attr_ws | ct_space)) ++s;v_cdata(char_t* s, char_t endch)


		char escapes trim)if (*s == 0)
			{
				rPE(*str, ct_space));
				
				g.push(s, str - s);
			}

			while (true)
nv_pcdata_t)(char_t*);
		e 2: return strct from pagI__Fs ut"w" : "w	templypename Traits,  from page->da opt_eol, typr_ws | ct_space)) ++s;
				
				if (*s ==IS_CHARTYPE(*s, ct_space))
					{
		{
			gap g;* str = s + 1;
						while (PUGI__IS_CHARTYPE(*str, ct_space)) ++str;
						
						g.pusE(*s, ct_parse_pcdata)) ++s;;
					}
				}
				else if (optL_escapLe::value && *s == '&')
				{
					s = strconv_escape(s, g);
				}
				else if (!*s)
				{ecode_wchar_b/ &apos;
	igned int>(ch -
	struct gaps++ = '\'';
						++stre;

			// U+0800..U+FFFF
			else
			{
				result[0] = static_cast<uint8_t>(0xxE0 .U+007F
			if (ch < 0x80)
			{
				*resultt wchar_t, 192, 1tic_cast<uint8_t>r_t));
			buffer[length] = 0;

			out_buffer = buffer;
			out_length = lader interp
 *
 *FUNCTIONignment #endif
		;

	static const uintptr_t xml_mem32_be;

			= 7;

if (optnterpret_e_pointer_mask = ~(		/// Default ctor
	
				}
				else ++s;
			}
		}

		staGI__NO_INLI		}
	conv_pcdapec(nSERT(sizeof(wchaote)
		{
			gap g;
.= end0,  turnion w			while node_struct* apf (*s == end_quote)
						else ++s;
			}
	
	static c
			{t* parse_eol(char_t* s, char_t end_quote)
		{ll_sigap g;

& ENDS{
				while  (*s == '\r')
				{
					*s++ = '\n';
					
					if (*s == '\n') g.HARTYPE(*s,	{
					*g.flush(s) = 0;
			& *s == '&')n s + 1;
				}
				else if6_t page_offset; // ef __		else ++s;
			}nvererpret_managf (strrformancs(urn buf;onPUGI__IS_rt, ...)t);ARTYPE(*s, ct_parse_y_managemene 1: return strconestruction is not<coagma waname (this may 	}
				break;
		dianTEL_COMPI				
				if (*s ==ARTYPE(*s, ct_parse_		else ++s;
			}icenerpret_ARTYPE(*s, ct_parse	ch = *++sush(s) = 0;
*g.flush(s) = 0;
		}

		// source enc
				
				if (*s ==conv_escape(s, g);
				}
		PUGI__FN strconv_att			{
					return 0;
				}
				el_escape::value &&			whilesult;
		} same-sized in
		c	data = 0;MSCse(){
			void* resICC (in>busy_sizestdbusy_sy.kautf_decods
		}non-ft for p)child->pr 		ifgbol ither a sage);E __aoad news (d* r7/IC8		retearlicode_uUGI__IS_CHARTYbidirer a salurn (nati>(en _I 1;ca code)
 = _::decode_latin1_bloe_offset);

			ribute_impl<opt_false>::parse_s	else // cancel
				ribute_impl<opt_false>::parse_simple;
		case 1:  return sL_NO_STL
	PUGI__FNte_impl<opt_true>::parse_simple;
		case 2:  return strconv_attribute_impl<opt_false>::parse_eol;
		case 3:  return strconv_at_cast<xml_memory_pate_impl<opt_true>::parse_simple;
		case 2:  return strca) deleter(data);eol == 0x20 && parse_wconv_alt;
		}
	UNPRO_CCibute == 0x80);
		
		switch ((optmask >> 4) & 15) // get bitmask for flags (wconv strconv_attribute_impl<opt_false>::parse_simlse>::parsk for flcase 1:  return strconv_attribute_impl<opt_true>::parse_simple;
		case 2:  return strconv_attribute_impl<opt_false>::parse_eol;
		t_true>::parse_wnorm;
		case 10: rttribute_impl<opt_true>::parse_eol;
		case 4:  return strconv_attribute_impl<opt_false>::parse_wconv;
		case 5:  returt_true>::parse_wnorm;
		case 10: rept_true>::parse_wconv;
		case 6:  return strconv_attribute_impl<opt_false>::parse_wconv;
er to same-sizedXPATHplat// STL: re: 140 && d3 == _rootBEGIN
| (lefin			re_tot;					/ral type
#endif

#ie_struct* chi()s == 0)T// secotus, ptrd		// fast patlocate se if (sh end;ld->nex(buffemplatenline xde <ml_parse_result make_parse_result(xml_parse_status status, ptrdiff_t offset = 0)
	{
		xml_parse_result resng_utf		result.status = statung s_result make_parse_result(xml_parse_status status, ptrdiff_t offset = 0)
	{
		xml_parse_result res< error_offset;
		xml_parse_sta
		restus error_status;
		
		// Parser utilities.
		#define PUGI__SKIPWS()			{ while (PUGI__IS_CHARTYPE(lt;
		result.status ral type
#endif

#ief ___be &trdiff_tt = 0)t;					/T		returnlrror_of resut;
		resrE()		rt_buffer_utf3ral type
#endif
I	retur suitPred> I min(*s, ct_sI  1);, I		{_t offse	{ wchareonverarsenIncoding le 1);value_type rIe, ole ((X)XML_We, ose, od

	= '&')_castred(*it, *
		3,   '&')
		,   // A version 1.4
    // 16-31
		0}
		#define PUGI__SR(staturef_deco !(X)) ++s; }de_utf8_ble
#enth + 1 PUGI_>lt =_be &* 1);++, *-(hexndian_swap( m, error_status = errI uniqust<char_t*>(0)
		#defineory_E
#\r') 

	fine PUGI__CHECK_ERROR(err= sta PUGI_::alml_attue_all_ERROR++_IS_CHARTYPsert(oe			{ // get na ((X)) ++s; <sizE
#t_trte		for (;;ILE(X)gi { name	}

	  *++stre mergeml_pars== 0 && d	buffer[lentatus(sta)
		#d
					*s++ status(statt_tru '&')
*++gi { namif (*s =#	define Pconst s
		// Seco may be unpvolathe(hex _NS_ENDlt + 8;
	l			if *s == '&') reinterS_END }_WCHAfset = m, error_status = err, sta0, 20,ackerpre& !(X)) ++s; }
	I targetf32_be;
<![...]]>
		// <!.. 0) ' || *roup) PUG->parent; }
		#define PUGI__SCANFOR(X)			{ wCANFOR(X)		OR(statulue_ty*s, sor&& !(X)) ++s; }
		#define PUGI__S, Tignment o
				if]>
		// <!..) ++s; }
		#define PUGI__ENDSEG()			{ ch = *s; *sopyrigT= 0 roupOR(err, m = 0; ++s;val,ups
		 XML specifica ...ov (ch froe typenverhar_t* s)
		(X)) ++}
	i
	}ength instatus(== 0 mpiler bugn wchar_t ef __SNIion_ (truNFOR(s[0]ed for ENype, _t* s)
	const  PUGI__'?' && s[1](ype, _writr_t)));
		if (*ype, s)== '!' && s<const uype,--);
		if (				s += f encype, > deassible types[3] == '-PUGI__THROW_ER (ascii) bry_pad varia16 *  == 0 && d&& s[1==t; }
		#define PUGI__SCANFOR(X)			{ whief __partitmask !(X)) ++s;midd#	ifoctype, s);

				s++;
I#elif eqbege PUGI__THR)
		#defineI HROW_ =s);

		tus_b

			retgroups of theexpe 0: 		re 		xmlfine PUGI__Cs);
!ne PUGI_	cons	{
		_wrisist*	{
	) --	{
	lue &* s)
		{_t*>(xmer = stan s;
	] == '!' &++	s++;c));
			#enount;elifle;
		if if (sI lts;
		HROW_ERgt);

 (s[0] == '<tf_de;;ents);
		sizebe;rconvsc = 16 *t
	ncodiigh 200		rerce)110xxx -> Ureplaeft onroup ction
	// nes)			{ ch =// ne<char_t*>(x! ++s; HROW_ER* s[2] r_t)));
		if (irst // nest] == '!' &m)	{ i// ne,le (*s)++				s += ne PUbrea
#	defir_writer:se_doctype_ignore(s);
					ise if return s;
				}
				elf (!s) (s[0] == ']' &)
				assert(; --)
		 == '>')
				{
	()
			_wri	}
	ore section end
					s  s);
'-')(s[0] == 'eturn s;((s[0] == '<' & s[2] se s++;
			}

			PUGI__THROW_ERRORscanUGIXRT_VE& s[2] == '				s+= 3;

er = st)
				 <!-- .XML specificalse
 s);

s[2] == '[rse_docty
				{
har_t crse_resul
	PUGI__FN boed fo_VERro
		 can't termibyfor seny _ignoarea[2] == '[')
					{
/ allocate buffertoplevendchf (s[0] turn s;)
		<' || s[0<const m)	{ i				// i& s[id* defauROW_ERROR(sc, ct		// ignore
						s = parss++ =s++;

 s[2] turn s;				// i');
						s	if (s[0] =					asser// ne == 0|| s[else s++;++;
					}g (forbidden), otoplevelcast<unsigned ir doctype
				if (!*s) PUGI__THROW_ERROR(me bas3(Igth = e, s);

				 fol
		#define PUGI__SCANWHILE(= 0; ++s; );

		*th = while (*s (!toplevel || obegin +  ++s; n s;
= '>') while (*s_bad_doctype, OW_ERROR(status (!toplevel || endch != '>') PUGI__THROW_E);
					if (!s) return s;
				}
				else if (*s == >')
				{
					return s;
				}
				else s++;
			}
 foll-y_page_<= 4FN void wide ...  basrn enc66 /some_CRT_Vpace pN
#	de == '>
				{);

		n s;
I__SClignment boundary
			si) // '<!-...'ni(s[0] =page->frte	{
k
			++s;

ult =/  1);		if (*s == '-') // GI__PUSH				nd a new n2ool de on	{
					+ (*s == '}

		-
						);

		}

		ch						cursor->value = s; 
			++e.
						
			++UGI__OPTSE		cursor->value = s;  a new node on'<!--...'
	T(parse_c	{
					++s;);
					if (!s) return s;
				}
				else if (R(status_bad_doctype, s);

				s++;locator& allR(st l || ;

				if PUGI__CHECK_ERROR(er32				s = parse_docty/ '<!-.] == '-' &&  s);

ine PUGI__E_CHECK_ERRORODE(2r->value = sinate pr			return0] == 
						if  (PUGI__status_ba			{
			{pace p (< = > '&')e, s);urn s;nown tstatus_badomment, s);

			
					, &HROW_ER&');
				 (PUGI__loop onrminat== pealloc'<' || '<' &&ERROR(er						tus_bad_dus_bad_doR(sta| s[0ating '-'.ength in 				{
			rimitive group
se PUGI__THROW_ER(X)) ++HROW_ERad_comment, sert(oenturn s;
		 bug
#	prag					ROW_ERRor ter		++s;

	/ DOCTYPE cons// quote'A' && *+OR(sta	// '<![ng '-'.

] == '>f (page == _root) page-ize_== 'p') /uef PsomeASTse 0: hresHAR_MOstac				re
		}
	}

	inline xxstatierpret_bloc) && , obeg	cursor->value = #	pragma  = 0;
		ontrogixml.org/
 *
 *MEMORY retur_PAGE_SIZata_im
						{
							s = strconv_cda_true, opt4096);
			}
			ndif

/h, obclass			cursurn resul32_be;
 the offset.

			127, a-z, page->fgth ='p' && *++spubliczeofml.org/
 *
 *NO_EXCEP
			Sursojmp			e*static_ pagebe) ||
			}
			el;
						}
			(
						{
							// addingdisable:g ']]>'.)
	char_t*& out_be utf16	pragm++ = 0; /es furtherCANFOR(s[0] == ']' && s[1] == ']	WITH(s[2], '>&g
			{v_pcdata_implh, obe-
 * nvert_utf_ethrow			returnaged
#endif
;
				 = xmlue = _capacitl is PUGIXMLng_utfgi { na stack
#i= rei(void0x6d) retu	++stif (ch page*&lt + 1;
			{eturqu== 0xe = 			if 0; // Zn-portabsed; di-
 *n ma1ter::a

					s += (s[1]f (!buf) rets segment.	}

	if
#UGI__SCANFOR(XML specifica-
 * Copcoding = g_VOLA+ting ']]>'.
	cludiast ']>'.
[0] ==f (s[0]de <stdint			else if (*s == '[')
			{						PUGI__|| !de	++s;
					>			else PUGI__THsult		}:			else PUGI__Tnown tag						PUGI__' && EN s[5] == 'P' && +;

		t_ca	cursor->value = ,TSET(tep over 
						{
							// 		els	else
#		define the offset.

		ject destruction is nonnt) PUGI__ return ontentse = 
	static conchar_t*>(x '>')fic endiasizeof(wchar_t)>				{
			 '>')f (s[0] == 'D' && [1] == 'O' &&;
					}
	(parse);
		& *++s=='D' && *++tor.
					{
 Scan for terminating '-
 * result)
					{
						// Scanif (!buf) retu
		3,  se PUGI__TH				else // Flagged for discard, 
				ifWITH(s[2], '>>
					
najm0] =ITH(s[2], '>,
				s +_true, opt				//)
				{d				}length iv_pcdata_ir_writer::value_e == encodi ++mark;

	
#			PUGI___
#	pra							*sol793) /							*snew#	define _failed[(conITH(s[2], '>'));
						PUGI__CHECK_ERROR(status_bad_cdata, s);

				 s);

s;
sk, char_}

					s += (s[1] == '>' ? 2 : 1); // Step 2, 1s;
		}s;
 cursor =}

					s += (s[1] == '>' ? 2 : 1); // Step over //eturn enulkinRROR(statu			elsendcsiblinze + (sizeofpalignm0interclude <string.h>
prag_badk, char_tdata, s);
			}
			else i= ']' && ENDSde, op	{
	s[2], '>'));
	 he utftart, ...)type_isiblinid* alze_t 	e_strunlyfirst_as;
e))
				{
=if (!PUGItep over the 

		 == 'D' && -HECK_ERROR ']' && ENDSWgma wara				*oad new ult)ng encobize; quesuUGIXh
				asODE
	// Con;
			);

					cursor->valuef_cursorsize + (sizeof 3,  3,  3,  3startin s);strcasue = s; 20, 20, 20, = '[targ		reragma o/ no need fNE inol++ bugndch == '-')  cursor =>HECK_ERROR(sta 0 &define     3,  3gnizCK_ERROR(status_bn -8066 /ratiy_size;t endcize + ha') rome envsiblinif (s[0ory::, ct_symbor_t)));
		if ('';
						+);
			ult;3,  3,  3s);

					PUGI__P
			har_writere the offset.

						iic value_type ((c) == 0 &nts);


				{
allocate buf (*++tion mark
		611) /++ =	retng stitt courati_page_ (s[0] =t disable the warnings ins		}
				else
'?')
		}
				elto a compiler_t data_l<wchar_writer::value_;
			else PUGI__THROWatic_c code)
	
						}
			pec(ote)
	s = parse_d66 /RT_V			*++ =			if 
						{
							// t_leng 192, 192, 1 return falsdisable_parse_
#ifdef __SNC				PUGI__PUSHNODE(node_pi - 0x				cursor->n
#ifdef __INTEL_COMPILEstaticecause _lenga compiler bu) && !defipage*&			elata += 1; than u92, 192, 192pe))
				{
		
					{
	lse if (s[= '>');

			ith [rh
#endif
)
				{
					PUGI__SKIPWS();
;

										PUGI__Ct>(lead - 0x target;
_t* value = s;

					PUGI__SCANFOR(s[0] == '?' && ENDSWITH(s[1], '>'));
					PUGI__CHECK_ERROR(status_baesult.status = statuNODE();
				}_SCAg.push(else
					 step over >
			e, s);urn resul
					get)' || *HARTYPthising ?(*ENDSEG declared but ne~r->value = value;
					 properly
		PUGI_->				PUG;

	ared but neverPUGI__POPNODE();		PUGI_ing_wcPUGI__POPNODE();

	dif

// Iore value andnd a 			cursor->value = valu
					cfor tag end
				PUG*ursor->par[0] == '?' && ENDSWITH(s == '			cursor->valor->value =  s);
[2ndiftag end
				PUGIe == encodiag end
				PUGIrt_buffeturn s;nd a end a 	PUGI_CANFOR(s[0] == ']' && s[1] == ']' && ENSWITH(s[2], '>'));
							PUGI__CHE2 : 1);
			//XML_NO_s);
 + unctencottribute =_false>::paristers0] we cET(parses[1rconv_pcdd) { sta

ck.THROW_ERR&e == encodin_strconcoding&rt_buff}
					else // Flagged for discard, , 192, but we still havencor_t* mark = s;
&WITH(s[2], '>'));nv_pcdata_impl<opt					}ttribute_t s	xml_parse_rsize; with [rhs_beg				iGI__IS_CHARTYPEult.statdata))
					{
			S	}
		valueew node on the treevalue);
			ame_alloc + ((cbuffer);
		u			elseexit offse_uses68
	

// Strd(PUGe* page =upli		if ml_memory_page* page 		}
								*s;
#endi	PUGI__POPNODE();

		minating ']ize)
	{k = 16;
	static const utringtricmrly)
		if(CE_PUGIXML_CPP

#include "p return arget[2] | ' ') == 'l'laration)
				; // Sca__FN bool strequ					// ncoding_utf3PUGIXML_CPP
#di, s);
					s += (*s == '>');I__SCANWHILE(PUGI__IS_CHARTYPE(*s, ct_symbol)); // Scator.
						PUGI__ENDSEG(); // Saecode_utI__IS_CHARTYPE(*; // Scan ENDSWITH		}
				fNDSEGway
#endif			PUGI__Srconv_att_memoucsc = 16 *uess_buffer_encod, = s;

		(* roo+= (*s == '>');
ex__ISi&& ENDSWITYPE(*s, ct_symbol)); /		
								if (PUGI__IS_CHARTYPE(*offse//< P_s;
*sarget
se
				{
#	ifndef  s; // ?esult, uint32_t cutf3art_symbol)) // <..						xml_att	= s;

		 = ! s; //way
#endif						if (!a) PUGI__THROW_ERROR(status_ouoffseuse68
	ucsc = 16 *
		Make space font, left  declared but nevend a new n		{
			while ((X)) ++he tree.

				if tor.
						PUGI__ENDSEG(); // Sase if (s[0] =< '<' && s[1] name = s; // SavE consists ofet.

									PUGI__SCANWHILE(PUGI__IS_CHARTYPE(*s, ct_symbol(X)) ++function_storage
	{UGI__CHECK_E // Scan for a terminator.
									PUGI__CHE

	== ';'GI__POPNODE()			else o__CHECK_ERROR(status_bad_attribute, forr') //< P sourct_space)
		*o.			reture_t size, opt_ alloc_):== ';')somehe tre --
				tring
	ret							{					}
									ngs
		iresu terminatorresuo.= s;

		XML specifica
#	ifndef 
											else if (*s == '[')
			{//ne@gmail.
				nator ?> (cursor->pare' || *anagement_fuENDSWITH}

			case 'nts))
						}" -or- '""'.
						
									; // Step overin 'chanagement_on "''" -or- '+r the quote.
allow non to== ';') // &ae structurSave char in 'ch', terminate & step over.

		RROR(status terminator? length);
ry_string								iencod(						s = strcon_CPP

#include "p,5
		3, the start;
											// Whi 0 && endarget[2] | ' ') == 'l'k
#ifdef __page_		}
		
				el										ing std::edunda countuestion maxml_memult = ch'"' || *s if (ation)
				 defined(on "''" -or- 't_space))
						{
	// everything eptr_t xll be detected
								f (declaration)
		ne Pn "''" -or- ,...'
				pagehe quote.
	t_space))
						{
							LOC_paces, / and UTES:
						RROR(ste if ') // '<... #=".e == encodina terminator.// full_+s=='D' && *++	{
			while (!s + 1;
	{
		xml_parse_result'
									} }
#	define NDSWITH
										s++;
							'.
											++s; // e terminae* page =e_t nce

										ch = *s;
									
				pare paghar to avoid bmbol)) PUGI__THR== '\'') // '<... #=".YPE(*s, ct_symbol defined(/ Scan for a	if (*s == '>')
									{#define SOUR After this line the loop coway
#endifame = s; /else if (*s == 0 && endchespace.
age, dat(*s == 0 && e
// Unicode utilieft for perforlse if (*s == 0 && endch ==

			

			o>value = s; // 		}
								else PU<uint16_t>ERROR(status_bad_start_element, s);
	st ch		}

							// !!!
						}
						elsee space fo
										s++;
								 s;

					esult.status ;
				}
eft for per == '>');
		te_ss code)
		;

	static const}
				/har in 'ch',)); /r thisf (page == _root) page-re
		}
	}

	ifset; // offset s -c chI__FN bool conver; // Scafer = buffer;
		, prs == '\'')
			{_end, prs[0] 		}
					sR(status_baid widen_aing// Seconnt, s);tain nested gr
						nt, s);;

			return (native_encoding == enbe;

a92, 192
	};

f_t _endiaSEG()arseml.org/
 *
 * This library  reintercschr(f_t inatop"

#in& endch ==_end_element_mius_bad_*s == '/')
					{
						++s;

bad_GI__THROW_ERROR(statuPUGI__THROW_ERRORame;
						if (!name) PUGI__THROW_ERboolSVC6atus thebug= utf_decod ((cone;

				i__ISse 3: r.4
s_FN voirn false*pt_castsult :				}
d_ele == emismatch, s);
				 0) PUGI__THROW (PUGI__IS_CHconselse
s symbol_
#	ow__atang =				ifi_t* slt->ne (s[0]RY_PAGE_SIZEic astatusult->freturn tchte_impl<opt_true>:t)
 */
def PUGIXML_espa - 'A'GI__26sult, uint32_t nv_pcdatch | ' 'ontipret_cncoding native_e= '>');
			tf(xml_;
		}
	};s_bad_"
		cta__CHECK_ERROR(status_bad_att memory for.t32_t ch)
nvert_buffer_			}
						else ifHROW_ERROR(staata < end)
c->allocate_striturn is_little_en_ERRa.lt + 2;
			}ove s coun3,  3,  se PUGI__TH
				uint8_t lead = *		while (size)
			{*data;

				// 0xxxxxn wchar_t units
					// we stepped ove
							
					}
				(PUGI_tic_cast<const uint8_t*>iter;

	template <typ_t* value = s		}
		ser - versionffer;
		out_lengn(contents);
		size_tf (s[0] == '<false;

		//						sn end
					s
			nment,  wchar_t_t leagth);
					}
					elsreturnng(d0, d1,size;$ redun			}
						else if urata < end(*s == '>')s) return s;
	)
	{
		const uint1 uint16_t*>(contents);
		size_tlength = size / sizeof(uint16_t);
th + 1) * sizeof(char_t)));
		in wchar_t ulocate bufr_counter, opt_swap>::decode_utf16_bl				if (h
		char_t* buffer = st!ENDSWITc0) == 0x80n[1] & 0xc0)inator.

					PUGI__SROR(status_ba(PUGI__IS_CHARe == encodinffer) e opt_pe decode_ut			// we stepped overprimitive grnt8_t* d= _busy_size;

			// in  192he (!sh == ';') us_bad_sta child
		
		HROW_ERR
#def>'.
						PUGI__THROW_++e == encodin'
		 buffer = state termina)	return error_offse((value & 0xff0000	retur_sibt_parse_attr  ln	return 0;
			linat92, 192, '<' || s[1] != 'r// Pop.
 // n st if ( | par			iftf_dedef PUGIXML_Nn rerhlt[0< lim_p++) l'
	l		}
						else(!PUGI__OPTSET(parer>:ta))jE(*s,; j
					el r		}
						else
						onlectoratic_he anc

	rn encodme enion mark	elserequired allocatrret_ca_memory_pI__SKmmse_aNODE(no>'.
						= mark;
		_t* e		}
					EG(); // Sa			s = mark;
								ET(parse_fragment))
 may be unedundant, lealue = s; // Sa note shareR(stad1 =nt, ull,    20s= pa);
		h);eak;
 == 0 && d3 == 0x3= mark;
		ode on thentreeions of the	s += coun   0,   ord // Ap s, charn
				s = m/ sizeof(uint16_t);pend a new noPUGI__IS_CHAR)
							(PUGI__OPTSE_buffer = buffer;
		out_leng			{
		s; // Saf (s[0] !=eat
		ce_ws_pcdat_parse_attr_ta))
				PU&	ct_paation2_t ch);

		// f| !*s)
					{
tring rreak;
eturn s;			ifak;t size;
			
		gap(): en
			asnment, l+s;
					}
						elsex		ctx_special_pcdata ding wchar_encod== st		{
		 convert_buffer_endiaA-Z, _, :
			{
					*s++ // fast (ch >>s)
		{.U+007F

		// allocate buffer of suitable leh && nar_encodingwchar encod_t* s)
		{		return urn (s[0] == '\xef' && s[1] == 'r_t
		wchar_writer::vf') ? s + 3 : s;
	e any(vat*>(contents);ed but never 16_t>(full_size < (1 < 1 : s;
s;
						}else
		static char_t* parseull_size
					*s++  20, 200] == '\xef' && s[1] == '\xbb' && s[2] == '\xbf') ? s + 3 :vious attr	#endif
 == node_element) return true;
de_struct* node)
		{
			while (noe_struct* a				xml_node_type type = sn get_mutable_b				nline xt bom = 0xfeffretupter 					elseparse_status status, p
						elseiff_t offse
						else 0)
	{
		xml_parse	{
ptimhar*			g.push(+s;
 fdef Pfdef (*s == '?')-
 * lodianneom = 0xfeff;l to parerly-out for erpty documents
			i1;

					to LOCo chilata, size_lotreeo ']' && ENDs
#ifk)
	isstart_s (s[0] != 'PUGI__		{
		,SET(paret last c ']' && END_documetrue
	{
	ine PUGI__hseader & impl child-OW_ERROR(status_bah = *s; //(!*s) break;art_symbolst_r	}

		s, begintrconv_pcdata units
		siz;
						
						++s;
				);
			out_length = lst uist_root_child =ic_cast<coe afteW_ERROR(status_bar_t*>(xmcreate peds up parsing)
			
			}

			// checr_t*>(xmid* contents, size_herwise we woarsing
			xml_node_ip_bom(if (s[0		// get buffer = staticld of thecoding_utf16_l++;
					}
			->first_child ? root->first_chrue
	{
	 go < endvel deeak;
] == '-' && s : 0;
	
			// create p_cast<ator& get_allocatorh - 1] = parse output
			char_++;
					}
		t->first_child ? root->first_chng
			parser.parse_tree(buffer_data, root, optmfer_data = pcreaget last c? s + 3 :
			// check th_t* buffer_data = parse_skipine PUGI__a new node on thor state
	ta_impl<opt_true, lata_cdata | parslT(parse) || PUGI__OPTS			// since we rrmoved la/pop bracket 		{
							if '<'/' |>firr0;
			ult.status = statuYPE(*s, ctsk)
		{
			// allocator object is a part of document object
			xml_allocator& alloc = *stati* last_root_child =? s + 3 :t->first_child  ?make buffer zeroE(*s, d_parsed = la:sert(resulttype result,ruct* first_root_c* roo :buffer : 0)ast_roo>(buffer);
		wc92, 192source encoding gen_nask)
	{
	ta);
		}
eak;DC_IEC_559ta;
			(FLT_RADIX - 0crea2onv_atn maMAX_EXPrse_resu128t(status_no_NT_DIGrse_resul4pcdataun *++{
		retuf; nd o32turn; } ue <asof deallt - boccurs ck offseot_ch
	-1			refustrcn resx7fc0lt.olue && *s ==
			'C' &p"

#inallocll_t* 							PUGvo, opore 
		els----rse_.ffset > 0 && h == /offse'));
						s_pcdata_single))
			ise_sib&& endchreshome;
					;
		}

		void* release()
		{
			void* result = data;te_string(ch!_is_sibhresho_cast<sPUGI__FN xmfpvalueifyv_attribute_iFP_NANfdef PUGIXMLncoding();ODE
		creaeturn _cast<size_t>(result.offset) == length - 1 && endcv= '>')xml_doctring rvor tv		}

			return result;
		}	{
			while (!PUse
	nusize_tomemory_ph voiaeplautput facilities
	PUGI__FN xml_encoding get_write_native_encoding()
	{
	#ifdef the lf		eleODE
		? s + 3 :ODE
	ch && namte (cyclic lis0CHART name (he lHAR_MODE
		
			return result + 1;NaNvalue && *s ==ncodin> 016) return is_litInss
	yCHARTte (cyclic lis- with specreturn get_wchar_encoding();
	#else
		return e
	#else
		retuINFINITE) ? encoding_utfZEROif

ounter;8;
	#endif
	}
es further (skieturn ts
		size_t oding_utf16_be;

		/xplicit enco32_le : ts
		size_t  utf32 encoding with utf32 with specific endianness
		if (encodingxplicit encoe;
g is requested
		if (encod0def PUGIXype decode_utf8_bloctype typeml_encoding get_write_encoding(xml_encoding encoding)
	{
ze_t lenv
			while (nosize_t get_valid_lener if ite wc_le : encoding_utf16_be;

		//  if it*8_t*>(sizeof(wch utf32 encoding with utf32 with specific endianness
		if (encoding , xml_node_st s);
						((value & 0xff0000(encoding == encodoffse	// Output facilities
e[0] == encodin!t
			cas

	//tion if = static_cast<uint8

	trun		if ----s)
		 step overcodin)
		#define PUGI__++s == '[')_impl<d[-1 char'0')		if				PR_T_Ds);
	, data) + dncludes mantissa digit
			{
				orm6 ui0.xn_sw&& s[10. = 0;i		{ctype_igxponsibli	PUGI__FN xml_encoding get_write_nat&& defined* release()
		)
	140_t* r_root_chilWIN32_WCEn strconv_attitive(cncoding == encod()))
		_

			reurn get_wchar{
		// Copyright (C) 2 if (op

		
		//#elif ()))
		scapC__)
#

			reypedef unsigned fdef_cast< two s(); ig'<![
			repush(secvt_sdeleter_)block(data, l<utf8_wDBL
			+	if &t to utff
PUgT(par
		xml_mml_enc red_t*>n------struc xml_encoding eoding == encod
				{
	
					+ xml_memory_pllTSET(pastruclse
()))
		{f _UINTPTR_T_DEFINt to utf') //to utf16
UGIXML_MEMORY_PAGE_S8_t* dest = r_u8;
			uint8_t* end = utf_decoder<utf8_writer>::decode_wchar_block(data, length, dest);

			return static_cast<size_t>(end - da scih ==fic

HAR_MOncodin> deaIEEE|| encodidecimaluct xm

	f to nativ"%.*e" || encodtf16_be
	#else
		re			uint16_t* e <= encoding_ue)
			{
			block(data,
		if (d0 == h);

			re PUGI/ Coine@g
			e = 0;
	 bet
			re == '!')age_p_end

			o'e strcon= '-') P2_le)
		{
		ocate_page(sngth, dest)atoi convert to natioctype
* parse_dtst ug()))
		{
				:		++s;f16_located_madecode_wchar_blocst char'-' ?ve utf16
h
	
									 PUGI__()))
		st c!_endi
		ttle_endineed_en. strcrn stre;ivretu()))
		{by 10deteh, coeport nte Stestatnt16ng_utf32_le :ing_utf32_0			ref()))
		// Secoconvert 
		// DOry_oobr ENing_ut()))
		{
			coengtcom)
 * Report()))
		;

			// convert );

			rconvert to native utf32_decoder<utf16_writer>::decode_wcha()))
		k(data, length, dest);

			// swap is, char_t endch)
	 (*s == '!')(encoding == encoding_wc_decoder<utf8_w			{
							if (*s != '>') PUGxml_memar) ret g == e		{
			xml_mem	char_t* name =

		as			result-g == encoding_wchar) retuend - dest)::valr) retta, size_txml_memory_page_tyhar_t* d
		if (d0 == ()))
		{+		uint32_tchar = 0;
			nt8_t* enecessa3
			rt);

			// swap r 
		
			uint32_			if est = r_u8;
			uint8_t* end = utf_<utf8_w)
		{
			uint						of convert n encoding&);

			r encodingign())
#	pragma warutf16_le;of sui!memoPE(ch, csable: 4g == e <= page->busy_silace endin	{
	)
		{
			uint			s g_utf32_b encodg_utf32_b: -gth - i; + 4			if e char in 'ch', terminate & step over.

						if (c__declspec(ali r in 'ch*s == 0 && earget[2] | ' ') == 'l
						at the end!PUGI__FN siz()			er - version_chidest); if icodin* ptr*s++ 			xm0');
						ding) convert_s++ = utf32_b<offs
				if (= encodi0g_utnment boundary
			si PUGI__Ce whole chustart_element PUGI__*decode_wcht
			char_t* targe();

						iPUGI__SKIPWS();

						i = utf_den mandian<= 9pe
				i encod = utf_dec?			// swap++ :t* dest =e_t>(end -				PUGI		{
			e_t freeda<opt_fading == encod uint8_t*>e if (PUGI__Id - desERROR(
			uint16_t*.g_utf16_::decode_d - des);
		ing == e

			retu convert to native __FN vois = parsent16_t* dest =e_t>(end - destI__THROW_ERRORrn static_cast<size_tdest, staticf16_le : encod		uint16_t* endock(reinterpret_cast<const uint8_t*(data), length, dest);

			// swap// Seconing = is_littlecom)
 * Reporest) * sizeotree
							uint16_t* r_TR_T_D()		:
					d_end_element, s);on)
				// f<< 8) | ((value & 0xff0000fdef memory_pto	PUGI__Frn str			if (endch != '>') nd - dest);XML_N	}
ibutedhitsy_siztch, s);
------IS_ThisTYPEve thmemoryt_y_siz)		ifg_utf3ize, xml_memoryg)
	{
		if 				}
				'-'	if (native_encod					oding n 20, 20, 20, 20, 20, 20i encondant, le|| encoding 
							h
	{
		fa6_be;

		}
> deaa				++s;ne{
			ze_t leng_utf32_le : encodX		{
		[0]2_bex_
			onv_at
			uint() ? .'0, 20coding_latin1)
		{
			ui1t8_t* dest = te allocator statee_encoding = || encoding == encoding_utf32_le : encodXvert_utfecode_utf8_blif (native_encoding != enizeof(uint32ng) convert_utf_endi.'				{
						(native_encodength, dest);

			return static_cast<size_t>(end - dest);nested groups = en
			ar* ie_endian() ? encoding_utf32_le : encoding_utf32_be;

			if (native_encod
									}
				, data) + data_size;rn get_;

			

			// swap ifary
			xml_encoding native_encodfdef P		}
		rn str(disable: st);

			// swap if necessaoding n<size_t>(node_sible_encoding != encme_alloc				if (!name) PUGI__THROW_ERROR(statusto= '\utf32_hitespmismatch, s);
	atypeo native uile (PUGI__IS_CHARTYPE(*s_buffer_outpu

			// swap if screncoreturn t(ing_utf18_t PUGI__THROW_ERROtep over.
									PUGI__rn get#elif NODE();
	e <= page->bnagement_function_storage
	{UGI__CHECK_E	PUGI__FN sizize_t snamespace impl { : 0;003, b>l terminal procesen Wegnernecessaryition_failed[(c Save quote cze;
on-har to avoid b* sizeof(cterminate & step over declared but never uh == '>')
						{
							// end of )
		ize_t sator& get_allocator is_littleNE inll be detecom)
 * Repordffer_latengtge* pag		{
			xml_memW_ERRORize_t s,

			/ad_attribute, s);
								}ize_t s_ATTRIBUTES:
					lse
coding le,ing user_encoding): writ sizeof(size_t freed_size;

		char da::val sizeof	ass procesta[1];
	};

	struct maining buffer  1;
	}

	PUGI__ (!PUGI__OPTS;			///<
		_nea;
 uint8_t* r_u8, uint16_t* r_ufloor (encod+ 0.5	}

		// source encoding native_encodi_n---- uint8_t* r_u8, uint16ssumeme aser.write(data,== ePTSE== 0 &-0ters [-0.5, -0] 20, 20,ei};
PUSHNOgt;
						Pi warnetween +0, res-0ext =::value_o convert in sui, res+o conv+& name[0] == e utf32 = rt i
		(encodi && namhunkf
	}
: path, can just write datah utf implementation
		if 		reid* to extra_buff
						else
e =
	#ifdef PUGIXMe->header & impl ?cessary)
						sopy the= stati
		}
	#le && re =and form a complete codepoint gma e>::pa discard start of last codepointth = strlength(tar = sequence (i.e. \t, \r, uffer = buffer;
ength

					ssert(': strconE
		static  ? pg nativhar enco is_li= statusbusy_siz_uri_ ++sIS_CHode to the tree.

		prefixxml_memory_pufsizeibute(s, ch);ize = 0;
				}
			}
odetection if no exrminating ']]>'.ffer + bu// Me chunk_size;
					}
tus_bafsize(e) osize_
	stru
#defith * sizeof(c	}

		var_t*>(xml_memory::alos -h(targwrite(cha		}
								else PUt is a par, 20,    20, 20,
	{
		xml_parse

						// iteratea, bufcapac(result <= 				
					ssert(ename Traits::valnscodick(reinterpret_cast<		static );
			void [5y
			:ncodith
		char_t* oid w+ 6A[...fix
		voiddeallocatcopied + 1] =size_t len							s += (*s th = strlength(tar= 0;
			sert(oend == obegin + ncoding ize = 0;
				}
			}
[...'		192, 1le && re ==e;

			retth -=ding) rm_pcdata))
	ENDSEG(); /Traits::high(res(e) . chuna += 1;
			_comment,
			if ( 0) returnaata < ene push/pop h -=p	}
						else if (PUGI__OPTSEte (cyclic list)
		xml_attribute_struct* next_atcapacity) flush();

		gth in utf8 characters
		return utf_decsize + 0] = d0;
			buffer[bufsize + 1]data,;
			buffer[bu// Dpe dec		buffer[b versions applemoryxml_node_struc)
				{fferfix
			return result + 1;
		}

		sfsize + 2] = p_bom(charbufsize += 3;
		}

		void write(char_t d0, char_t d1, char_t d2, char_t d3)
		{
			if (bufsize + 4 > bufcapacity) flush();

			buffer[bufsize + 0] = d0;
			buffer[bufsize + 1] = d1;
			buffer[bufsize + 2]  start of last codepoint if necessary)
						size_apacity) fluse->header & implt \t, '); // '...copiedr[bufsize + 0] r : 0);				
				if (*s == end_}
					};

	)
				r>::deconk_size);ory_paggi { namespace impl {
#		cupies wh_pcda
					SCANFt ch); // Save cha eof(c*i- dest)); > bu_utf32_le : encodionstbe;

							ch = *s; //uld notbuffered_wr _cdata		// dea_encstrean() ? e encoding_utf32_le : encodin}
	be;

			iansion: x4 (d[(co

	is_littl	enumUGI__THROW_ther gdle large c not cencodi return ++;
					}
				10240pret_ca may be unsaor END
		xml_b	enum
		UTPUT_STACK
			#el
		// con_le : encodither  (neifdef PUGIXMLther ap_utf(eno native utf32
			ui not cng, get_wchar_size_t length, xanstype
			buffer[bufPUGI__THROW_ERRO;
	PUGI__THROW_ERROck(const uistatic as" -or- '""'.
						to buffer  = d4;
			buffer[bufsize + 5	}

				return stre;
	------DMC_VOLATILEcursor->nm expe += 6sion: x4 (har_t* data)
		{
			write(da
			chag buffer  > bufcoate((len		bufcapacity * dedo

	& s[1] +;
					}
		fsize + 1 > bufcapacity) f;
	
		} scratch	{
		while (*s)
to[ a usual s]{
		uld not/ convert to native utf32
			ui		uint16_t data_u16[ore value andcausebl_t datean:			switch (*s)			cursor->valch (*s)
			{
	ribuPointer this attribute.
			offseg)
	{
		ursor->har_t buffes += (s[1] == '>' ? ch (*s)
g == e			case 0: break;
				case '&':
					): writrite('&',as declared but nern get_wchar
					break;
				case '<':
					writer.write('&',data, le	case 0: break;
				case '&':
					nd_attribu
					writer.write('&',					}';');
					++s;
				if (typehresholta[1];
	};

	struct end - dest)								ory_pag	++s;
					break;
				case '<':
					writer.write('&', r_t>(etrite('&', 'q', 'u', 'o', 't', c_cast<c					assert(ch < 32);

					writer.w_SCANWiscard start ofst<chze;
ic_cast<c						s += (*s def PUGIXML_Nde tyUGI__THROW_ERROR(status_ar_t));
			Jenkin (d0e-at-a-timtargsh (http://en.wikip*s =.org/ for/unsigne_de tyrformanc# int flags)

			{| PUGI__OPTSET(parse_tri	xml_encodinrtypex_
					if (PU s[1]ck(reinterpret_cast<const strelse s++;d text_out = utf_<< 1
#definexml p^ar_t* s)>> 6est = r_u 192, 192,char_t* s)
	{3data + size		writer.write1_WCHARconst char_t* s)
	{592, 192,)	return error_offset = m, error_status = if defined(_Tc) =w_// s is not a ;
		}

		void write(ding == get_write_nativENDSWITHld buffer,else
			{f') ? s + 3 :0{
		
				ch (*s)bufferse
	ing_utf_cast<con$$			{
				eclad_doctypeTfirst_
		caeclaTe;

n-PODpage*wese, optricmp / sddus_baalt* data,const uintptr_80 // symbol is declared but never used; dit_deaad_attribute, s);
								})
		 not mak(contents);
		Tr in 'ch',  does not makT8);
		}W_ERROR(statu	assert(ssert(h == '>')
						{
							//   3,  3,  3,     // 16-31
		0,  0,  2, / s is not a  since it terminates ribute aocatta, size_ngth = strlength(targ	{
			const uint8_tes further (skiributeize_tc_cast<ch/pop bracket doe it terminatesh, true#', static_cast<c>&& s[2] =ite(a.name()[0] ? a.n'l', 'default_name);
			writer.write('=', '"');

	 == eoutput(writer, a.value(), ctx_data, ldefault_name);
			writer.write('=', '"');

flush(output(writer, a.value(), ctx_			{
		default_name);
			writer.write('=', '"');

			{
	output(writer,const char_t* data, size_t len in some WinCE versions
typedef sizeef __BOalloe it terminates T
			ypex_t typvar->~ode, u& ENDSWITH(s[1], '>'));
nt i				
				if (*s == end_== 0)
			for (unsignea; a = a.next_attributeribute a = node.nt i = 0; i  ');
			writer.write(a.name()[0] ? a.name() : defau== 0)
			for (unsigneoup(s, endch, true#', static_cast<c*>

		
	
	te
			PUter, a.value(), ctx_special_attrindent, flags, depth);
			break;
		}
			
		cas == eelement:
		{
			const char_t* name = nod_buffered_wrindent, flags, depth);
			break;
		}
			
		caflush(element:
		{
			const char_t* name = nod unsigned intindent, flags, depth);
			break;
		}
			
		ca			{
	element:
		{
			const chd of PCDATA ock
			s"Ing_utftopped at / endrimitive(s);
		 (xml_attribute a = node.ge_allmat_raw)ze_t size)
		{
			if (size gs & format_raw)eze *sert== 0) return;

			// fast path, justencoding == get_write_native_encoding())
				writer.write(data, size * sizeof(char_t));
			else
			{
				// convert chunk
				size_t result = convert_buffer_output(scratch.data_char, scratch.data_u8, scratch.data_u16, scratch.data_u32, data, size, encoding);
				assert(result <= sizeof(scratch)
			{
				if (*ata
				writer.write(scratch.data_u8, result);
	lookup
		void write(const char_t* data, size_t length)
		{
			if (bufsize + length > ribute a = node.in 'ch', t(staGI__maining buffer contents
				flush();

				// handle large chunks
				if (length > bufcapacity)
				{
   // 16-31
data))
					{
			nd, reintfer) ret__PUSHNODE(node_element); (xml_attributec_cast<chatic / \pstatic(sta
			if (bu;

			/ode.first_chUGI__CHECK_			
				for (xmlribute_BEGINevf32_be;
ing())
					node_outp(xmldoar_tv		tying())
					node_oun = ed_tic_casfullformat_indent) != 0 && (fl 192, 192, 192,

t_raw) == 0)
					foun&& (flN void widen++s;

					if signed int optmsk)
		{
,     // 0__int 1 :_raw) == 0)
					for (unsignse if (PUGInt i = 0;!=			if bufff_deco

					ize + 4 > butring r+s;
;

		for (xml_attribute);
		statith = )
		{
			if (bu;

			// fastn; n = n.next_sibling())
					node_output(w
						// roE consists of nested 	//writer.8);
		} ');
			writer.write(a.name()[0]t_indent) != 0 && (fled int depth)statusonst char_t* name t_indent) != 0 && (flags & foat_raw) == 0) 		writeStep over('\n');
			break;

		cawrite(inat_raw) == 0)  (*s != 0 &&		writer.write('<', '/');
				writer.write(namde.first_child(); n; n = n.nexter.write())
						tf8_block(rein>(buffer);
		wchar_wvalue);
			c_cast<c
	{ + '0'), static_cast<c	for (xml] ? aut_cdata(writta, fl- 1);r_t* parse_defaultthis seg
			if (node.tloc(sute_struct* a = appee('<', '?');ributbe;
< depth; ++i) writer.write(inds_ba1);
unct)
	{value(oswas declared but nev_pcdata, flags);
										s++;
									name);
.write(' ');
				wchild;
			fir	s++;
								this seg*s == 0 && endch == '>')
								{
					ssert(oe=t_raw) == 0) writpage->freedelse if (*s == 0 && endch =_t*>(xml_memory::agth + 1 = 0;
;
			}
			else
size_t ctx_
										s++;
							alue(), ctx_	}

lue()[,writer;

			if (nRROR(sushr_t* )
		{
			if (bufsize __CHECK_ERROR(status_bad_attribute, ? en// igno			{lse PUGI__THRO = xmlSCANFOR(s[0]writer.write('T', 'Y'ty) f, 'E');
_OUTPUT_Sng
: -1]CANFOR(s(1.5x ru| ' ') =fault:
);
SCANFOR(s[0laration(+node child/ 2	char_t* paion_storion mark
		ion)array)
	tricmp / strcash == '?')' ');
				w_VOLATIoup(s, endch, truebuffer_te, a->value);

	tep overCANFOR(sP

#inc' ');
			buff
		for (xmltype == node_elementEOF are wrong,
 mark = s + 9;		{
									++s;ert(oenark, ct_sp	lue_typ
			}
				if (curso;
		de_type creturn false<size_t>(end -*\n')encodd2;
			bbute, s); //$ redundant, left fta, flags);0, &, \r, te('?', '>');___CHECK_ERROR(status_bad_attribute,page->freed/ Sa	writer.write('T', 'Y', 'P', 'E');

	fault:
			assert(!"Invalid node type");
		}
	}

	inoctype)) ref denative_encoding())
				wr_
					} & utf8_b#	pragma we chive_c>child)
	sion: x2 (-> utf32ild = child.next_sibling())
		{
			xml_node_type type = child.type();

			if (type == node_declaration) return true;
			if (type == node_element) , const xml_no>')) // CDA		return false;
	}

	inline bool allow_insert_child(xml_node_type parent, xml_node_type cault:{
		if (parent != no const xml_no<size_t>(end -W_ERRORte(nod || child ve_cbute(); a; a = a.next for a ired
//(); c; c 
			}

			
	_doPUGI__THROW_ER;
				//wri	
		iter.write(node.valll terminatot.append_chil		uint16node.first_ch)
	{
	ibute, s); //_node_t<}

			reoid*octype (PUGI__ISh = lenloc(siz= '>');

			20, 2YPE(*s, csymbol
				{
		ssert(ci < depth; ++i) writer.write(indent)			
		iter.write(nodreturn make_parse_re = data nt:
		case l_parseiter.write(n;

			if (node.value(				node_output(w, s);

							PUGI__POPN ? nodest.append_childetriter, node, flags);
			}char_t>());
				assert(ccg)
	{
		)) // '<#...'
					{
	 node on the tree.
						cursts, siz + '0'), static_ca
		{
ta, lengtosus_ba						ut_cdata(writts, siz)
		{
			if (bufsi_							*snode typ_node_struault:178 (nattrnode typcity node)nction wml_nod declared bue '<':
enum lexeme_
		defaullex_nif (= 0,_type_mas, '>;

		res;
		res;

		reng s;

		reg#	prara || type == _buffcdata || typea;
	}lue with conversioplu node_cdatminget_integer__pagply;

		re{
		;

		revar_ool _type_ma__NS_bracevalue;
ionalhile (PUGI__IS_quo(flaflush()_type_mas == e

		if (slash

		if (rn get	s++;

		retu__NS_squarTYPE(*s, ct_spacCHARTY= 'x' || s[1] == 'X')	s++;

		if (aluea

		if (axoot)};

	s

		returnt

		return (s[0	{
		if (!value) rcolconst char_eo	retu[0] == '?' && ENDSW_memlocatw node to the tree.

		}

			wrr.
									PUGIut_cdata(writdef PUGIXML_'>':(node.valu()[0])'m', 'p', ';');
				
// Unicode utils++);
	me en		}

		void wri== get_write_native_encoding())
				writer.write(dment, s);
							}ar_t* me enst char_t* dataT(parse_fragme
			writer.writdef Pode to the tree.

			16(char_GIXML_WCHAR_MODE


	msultror_ofase));
	#else
		st<unsigned iegin &&  def)
_memory_st<unsigned on)
			{
				no						if (!a) Pdef P		{
			while (queryget)cur(double	PUGI__FN vconte	{
										P	if (endch != 'pragm		//if ((flags & format_16(char_e terminator.8 // fite(const char_t* data)
__SKIPW16(chadest, static_utf32_le : encodincu32[be;

			if 
		return ssume ut_memorype = sta some dy; colporf;
for a <unsigned int>utf16(cha	s = parse_quf
	, cursor, optmsk,0DATA on		if (!valud ma_integ
				if 
			PUGI_OC_ATTRIBU'>'DATA onconvelse ata = st=urn 0;// Eat whiECK_+=OR(statun static_cast<float>n functions
	PU	size_t data_lengtod(value, 0));
	#end_WCHAR

	PUGI__FN bool get_value// skip BOM to
			const chse
		r<turn static_cast<float>(strtod(value, 0));
	#endif
	}

	PUGI__FN bool get value with st char_t* value, bool def)
	{
		if (!value) return def;

		//ng sook at first char
		char_t first!turn static_cast<float>(strtod(value, 0));
	#endif
	}

	PUGI__FN bool ges;
		resst char_t* value, bool def)
	{ long def)
	{
		ifn	// skip BOM tohar
		char_t first=turn sta
		if (!value) static_cast<float>(value) fdef PUGIXML_W;
	#else
		r+	#ifdef PUGI__MSVC_CRT_VERSION
			returnt getoi64(value, 0, bhar_t first-	#ifdef PUGI__MSVC_CRT_VERSION
			return_base
	#else
		#ifdef PUGI__MSV*_CRT_VERSION
			return _strtoi64(value, 0r_t* va
	#else
		#ifdef PUGI__MSV|	#ifdef PUGI__MSVC_CRT_VERSION
			return{
		toi64(value, 0, base);
		#else$	#ifdef PUGI__MSVC_rn static_est);

			return s
	}
__FNs - OW_ERR section end
		);
	#else
		return;
		*eturn def;

ss xml_buffered_writer
	{
	ue);

	#PUGIXML
	#EMORY_OUTPU			// Weary
			d1;
	est);

			return 0, b:decode_
			retu// qe_endian // Eat whitrn wcsSION:			{
				_wcstoui64(value, 0, base);
		#else
			return wcstoOW_ERRORMODE
		#ifdef PUGI__MSVC_CRs);
	16(char_t*ase = get_integer_base(vat* s = e) return def;

		int base = get_integer_base(value);

	#ifdfdef PUGIXML_WCHAR_MODE
(	#ifdef PUGI__MSVC_CRT_VERSION
			return
		while (
	#else
		#ifdef PUGI__MSV)	#ifdef PUGI__MSVC_CRT_VERSION
			returnCHARTYPE(*stoi64(value, 0, base);
		#else[	{
	#ifdef PUGIXML_WCHAR_MODE
		char_t wbuf[= 'x' || s[1
	#else
		#ifdef PUGI__MSV]uf);

		return strcpy_insitu(dest, header, he
	#endif
	}

	PUGI__FN bool set_value,uf);

		return strcpy_insitu(dest, heade_int
	#else
		#ifdef PUGI__MSV/turn static_cast<float>(s/rtod(value, 0));
	#endif
	}

	PUGI__FN bool gern (s[0] == st char_t* value, bool def)
	{
		if (!value) return def;

		//uintptr_t& header, lue, 0));har_t first.turn static_cast<float>(sturn 0;

	PUGI__FN bool set_value_convert(char_t*& dest, deof(wchar data_length = sest);

			return sast<floecode_utf8_bl_WCHAR_MODE
		#ifdef PUGI__MSVC_CRT_VERSION
	SIONe_t siz					N
			return _wcstoui64(value, 0, base);
		#elseize_t>(ern wcstoull(val#endif

	// set value with conversiion functions
	PUGI__FN b*s == st char_t* value, bool def)
	{
		if (!value) return def;

		//ol set_value_convhar
		char_t first@	#ifdef PUGI__MSVC_CRT_VERSION
			returnar_t* value, i
	#else
		#ifdef PUGI__MSV"turn ste
		r\'turn stef __SNursor->--------
	PUGN
			returnf, "%g", value	#ifdef PUGI__MSVC_CRT_VERSION
			urn _wcstouest,s[0] tf16_blrt(char_t*
		#endif
	##endif

	// set value with conversion fun					UGIXML_WC get_integer_base(value);

	# wchar_t units
		
		if (!value) return def;

		//e))
			s++;	PUGI__THROW_ERalue, 0));
der_maske
		r:turn static_cast<float>(s:ffer(dest, header, header_mask, buf);
	}

	PUGI__FN bo basee) return def;

		int base = get_integer_base(value);

	#ifdef PUGIXML_WCHAR_d of PCDATA one = get_integer_base(value);

	ptr_t header_mask, double value)
	{
		char buf[128];", value);

		return set_value_buffer(dest, header, header_char_t*>(xml_me28];ue_buffer(des _strtoui64(valu base);
		#else
			return strtoull(value, 0,out_result)
	{
	#iask, unsiglue);
	
		return set_value_buffeconvert(char_t*& dest, uintptr_t& header, uintptrchar_t*& dest, uintptr_t&ue);

	#ifdef PUGIXML_WCHAR_MODE
		#ifdef PUGI__MSVC_CRT_VERSION
			return _wcstoui64(value, 0, base);
		#else
			return wcstoull(value, 0, base);
				// allocate bufue, 0, bneed_en*'RSIONt d3, chartlt =ncuffe:*!ENDSWITocate buf;
	#endife, 0,		typedefcast<char4(file, 0, SEEK_END);
		lenf PUGI__MSVC_CRT_VERSIONtell, let's use
			return f off64_t lengalue, 0, base);
			#else
			return strtoull(value, 0, base);
		#endif
	#eo64(file,ndif
	}
#endif

	// set value with conversion functions
	PUGI__FN br_t header_mask,def;

		int base = get_integer_base(value);

	#ifdef der_mask		ifwith converder_mas_cast<unsast<ct_space))
		urn def;

	#ifded int>(str
										PUGI__POPNODt voidint>if

		// check for I/O errors
		iint>(wcst										PUGase));
	#else
		
		}

		!value) return def
		casetatic_cast<f__FN bool setgth)(result) != lengthsert(!" status_lexeme == lex_string || _cur_/**
 * pugixmquotedml pars);

			return- version 1._contents;----}
	}------enum ast_type_t
	{----C) 2unknown,by Arsenop_or,	apoul// left or righ014,e (arsenandkapoulkine@gmaiandcom)
 * Report bequalkapouline@gmai=com)
 * Report bnot://pugixml.ine@gmai!
 * This library ilessixml.org/
 *<com)
 * Report bgreate.kapouline@gmai>MIT License. See noti_or://pugixmlthe end

 * This library i
 *
 *rser, which work is b
 * This library iads and download n+com)
 * Report bsubtractuted under th-com)
 * Report bmultiplyuted under th*com)
 * Report bdivide This work is /include "pugixml.hos and download n% * This library isegat.h>
#include <#include "pugixml.union This work is |com)
 * ReporpredicDE
#	includapply e <math.h to set; next points#	ifUGIXML <math.h * Reporfil
 * This (kriselectdlifrome@gmaiwherecom)
 * Reporjmp.h>_posinvuted und
#endif

#ifndef PUGIXML_NO_; proximity <isttion invarian
 * Reporl pars----stanUGIXM#	in parseude <new * Repornumberlude <new>

#i warniMSC_VER
#	pragmar plabl.h>
#incl7) // coSTL
#	inunc_lasUGIXML(krisast()ion is constaif

//uted undif

//ing(disable: 43couew>
agma clspe(@gmang(disable: 43is and d// idpragma warning(disalocal_name_0ch is:
 '_s-tjmping(disable: 43n '_setjmp'1and C++ object desragma warning(disatjmpspace_uri' andragmreachabl-uriing(disable: 43nreachable cod
#	p	pragma warning(disable: 4702) // unrea' and or variadisable: 4996) // t
#	prsable: 47ragma warning(disa
#inclng(disablfdef _ing(disable: 43
#inclpiled as unctionragma warning(disaconcaUGIXML_CP warni_COMP,com)
, siblings unmanaged
#endarts_with>

#ifdreference7) // functiILER
#	pragma watainice atsable:ng exng(disable: 279) // controsub
#inclbefor.h>f __ble: 147- 1786)tant
#	pragma warning(disable: 1478af
 * Th function wasisabltant
#	pragma warning(disable: 14782>

#ifble: 147tant
#	pragma warning(disable: 14783 type
#endif

#if defined, third unmanaged
#endif
lengthde
#	prion wasdeclarn unmanaged
#endif
declareon or vever used; disisable: 4702) // unormalize_chablee
#	pray
#endi-chabldisable: 4996) /y
#endif

#ion or vaORLANDC__
#	prragma warning(disatranslDE
#	in// ragma warONLY)
#	pragma warn -8080 // symbooleastructureC__
//e the warning go awayec(align()no	pragma warning(disatru.h>
#inclatesing(disable: 43fals.h>
#inclagma struction is nonang#	pragma waEL_COMPILER
#	pragma warninng(disablewarnidisable: 4996) /warninpiled as npress=e: presence of '_seume: 4611) sumstant
#endif

//floy.kapoulupprC_VE_COMPILER
#	pragma eis depression UGI__Nstant
#endif

//rougs and d// if destant
#endifstepkapoulkineprocessfdeffndef Pith GI__efine PUGI___root#	endif
#endifine  nodsion Copyright (Cxis2014, by Araticancestorlkine 
#define PUrserselfGI__STATIC_ttributeGI__STATICchildGI__STATICdescendnew>(cond) ? 1 : -1] = cond) { static confollow__NO]; }

// Digital_ was de Mars C++ agma warn Mars C++ pare= {0}; (voidpreced for passing #ifdef __round for passing ) { 

// 	yright (ndifte) 2014, by Aine PUGI_nonded frTILE
#endam

// Borland 006-2ndifbug workaround for commy via storkaround for pig on header includetexng on header in order (can't alalluse some compiler_in char load

// Simtemp war <tatic  N>t douctstatic od for14, by AstaticMSC_VEstatic stati#elsned(__BORLANDC__) && !defin:memcpy;
using sUSING_LI<N>::TATI = N-----
	class xpath_agma dif
 by private: * Rthe wde G_LIST)	char d for-----C_VER)ret && !deres
#ifor(C) 2GI__ /(C) 2
#		include C_VER)ng std::SC_VER) est
#	define treeif defed(__Mursion s certain MSVC* _@gma-----EGIN namespace pude <stnamespace impl {
#	dUGIX
#	defi#ifnd#ifd by Aonal elue PUGI__MSVC#include <new-----:memcpC_VE_t*t does  PUGIdefine PUGI__FN_NO warning(push)-----double is con_VER) && _M // co PUGI__MS expression espace th anony*ith anony_VER) && f defiest PUGI__MSVC_CR(f defC++ /agma warn/f defined/pi targe 279) se
#	if defineine PUGI----- _data
#	defis certain MSVC(:memcps certain MSVC&----ce pugi { namesp& operator=ace impl { namespace {
#	#		dORLANDC__)T lackCompined std:C__ commory_eq(EGIN namespace plhs,pl { namespace * rtr_tce impl { na-----xt& c_VER) || _MSC_stack&NLINck_VER) ||_NO_&NO_INng(die
#	d namespalued for l*
 lhs->S3E__)(), r*
 *ptr_t in MSVC--------if (lthe Ms cert for not _ute_&& andf size_t uintptr_t;
#} }
#e
#	d
typede= size_t uintpC__
//er -and 
	typedef unsigned } }
#---------O_IN(tptr_evalnsigned (c,.h>
#C6 a_t;
	typedef unsigned __i
#		d		else NS_BEGIN
	typedef unma warn_int8 uint8_t;
	typis consigned __int16 uint16_t;
	typis consigned __int32 uint32_{
		return mall
#endif

// Memory allocation
Pl parser -t8 uint8_t;
	typ-------endif
PUGI__e pugi {l objRT(ccapGI__ cr(h>
#.resultversions>
	strufdef _Mler b6_t;
	typdif

igned __i#endif	{
		static arer b32 uint32_ion allocate;
		stigned __int16 uint16s, rs		static
 *nction x

// Memory allocation
PUr_t;
#define nt_function_storage<T>endif
PUGI__
	struct xml_memory_management_function_storage
s certorage<T>_rawallocation_functorage<T>locate;
		staticagement_function_stfunction dealloocate = default_deallrage
PUGIace impl { na/ uintpiocats.begin();D
!
// Send uti++lisigned _l_memory;
PUGI__NS_ENDr

/r String utiritiesr
PUGI__NS_BrGIN
	// name T> <typename T> deallocation_iment_function_storage
	
typuint1_INLINE// No(*ligned __unction_gned
		return strlen(s);
	#endif)signed _---------ates	static }<int> xm-------agma #endifml_memory_ndif
PUGI__NS_BEGIN
	typedef unallocate;
	te
		assertswat16_tnt32 		static eturn / fun_stonst char_t* sNS_BEGIN
	typedef unsigned signed __int16 uint16_t;
	typedef unsigned __int32 uint32_t;
PUGI__NS_END
#endif

// Memory allocation
PUGI__Ne <typename T>
	struct xml_memory_management_function_storage
	have anlage<T>::deallo
		return mall	static deallo	typedef xml_memory_management_function_storage<int> xm Get string length
	PUGI__FN size_t strlength(const char_t* s)
	{
		assert(s);

	#ifdef PUGIXML_WCHAR_MODE
		return wcslen(s);
	#elsl_VER)verew
#inclto				retu	}

	// Compare two strings
.#endi()
	PUGI__FN bool strequal(const char_t* ssrc, const char_t*  dst)
ault_deallocate(void* ptr)
	{
	e <typename T>
	struct xml_memory_management_function_storage
	{
		static alocation_function allocate;
		static deallolhs[count] == 0;
	}

	// Get length of wide string, even if CRT lacks wide character support
	PUGI__FN size_t strlength_wide(const wchar_t* s)
	{
		assert(s);

	#ifdef PUGIXML_WCHAR_MODE
		}

	// Compare two strings
	PUGI__FN bool strequal(const char_t* st>(end - s);
	#endif
	}char_t* assert(!"Wrongfineds";
	
	rc, const char_t*char_t*	define PUGI__FN_NO_INLINE PUGI__NO_INLINErel#endif

// uintptr_t
#if !defined(_MSC_VER) || _MSC_VER >= 1600
#	include <stdint.h>
#else
#	ifndef _UINTPTR_T_DEFINED
// No native uintptr_t in MSVC6 and in some WinCE versions
typedef size_t uintptr_t;
#define _UINTPTR_T_DEFINED
#	endifllocate(size_t size)
	{
		return malloc(size);
	}

	PUGI__FN void defory_management_function_storage<T>::allocate = default_allocate;
	template <typename T> deallocation_function xml_memory_management_function_storage<T>::deallocate = default_deallocate;

	typedef xml_memory_management_function_storage<int> xml_memory;
PUGI__NS_END

// String utilities
PUGI__NS_BEGIN
	//name T>
	struct xml_memory_manageAR_MODE
		return wcslen((lhs[i] != 	return wcslen(s);
	#else
		const wchrlen(s);
	#endif	while (* string, even if CRT lacks wide character support
	PUGI__FN size_t strlength_wide(const wchar_t* s)
	{
		assertt(s);

	#ifdef PUGIXML_WCHAR_MODE
		return wcslen(s);
	#else
		const wchar_t* end = s;
		while (*end) end++;
		return static_castnst char_t* src, const char_t* dst)
	{
/ Memoryf size_t uintptr_t;
#define );

	#ifdef PUGIXML_WCHAR_mplate <typename T> deallocation_function xml_memory_man(lhs[i] != rhs[i])
				return false;
	
		e;

	typedef xml_memory_management_function_storage<int> xml_memory;
PUGI__NS_ENDI__FN size_t strlength(const char_t* s)
	{t uintptr_t xml_memory_page_type_mask = 7;

	struct xml_aCHAR_MODE
		return wcslen(s);
	#else
		const wchar_t* end = s;
		while (*end) end++ool strequal(const		result->busy_size = 0;
			result->freed_si);

	#ifdef PUGIXML_define _UINTPTR_T_DEFINED
#	endif
PUGI__<typename T> deallocation_function xml_memory_management_function_storage<T>::deallocate = default_deallocathave anrmory_managemen		return false;
	c const uintptr_t xml_memory_page_value_allocated_mask = 8;
	static const uintptr_t xml_memory_page_type_mask = 7;

	struct xml_a);
	#els
	struct xml_memory_page
	{
		static xml_memory_page* construc, re* root): _root(root), _busy_size(root->busy_size)
		{
		}

	* allocator;*);

		buffer_holder(void*src, const char_t* dst)char_t*voidlude <SION _MSC_#endift_function_s& nson wze_t first_t
#if !defined(_expr0
#	include <stdint.h>
#NTPTR_T_DEF*);

ns.t(pa()@tim;

ct(void* t(page)

/1se
		t(page)t(pa = 

			retu- page;se
		rT_DEFINED
/ uintpas*
 n String u +ge* page)
		{
		__GNemove_if...il.cweler fort oile even i	xml_memory:i uintast;e*& !locatUGI__NS_Bit,llocr* allocator;

		VER >=  c(*catelen(ize;
	
	rn strcmp(memor_t in MSVCN
	PUGI__FN void* default_aname T>	if (_busy);
	}

	PUGI__FN vmoryGIN
	// 	*t_pa++ = *iage)
		f
	}

#ifdef ate_memory_t;
PUGI__NS_END
{
		vod* buf = _root->dat= 0;
 sizens.truarnie(t_pa;
	
= xml_memory_page::construcsl_memory_pary);
			assert(page);

		;
			page->allocator = _root->allo
typ

			retu=n page; ------ge)
		{
	, xml_memor!defined(_
#	 =	define data usy_si =data ->definr* allocator;_page::constructsser;

		(void)!) // ffalse;
	
	page = xml_memory#	defpusht size, xml_memory_page* ce implmlnst char c& age->busy_siz__NS_Emory vipl { namt xml_me* 				a->busy_size = _!assert(ptr > }
#	else
#		defina
 *  a.e: 479 page fr// TUGIXMar
#	d st char cine Ps corresponf __#	ifst char cs that declpage-reachabls->busy_sizat is, "xmlns:..."il.cage);"_size = _ragma warninC++ b PUGIXML_TEXT(ert(pag))defi remo[5];
0S_BEprev->next ':')ssert(ptr >= page->switch (GIXMLr* allocatorcase = 0;land C++ uresprev);
r//pug removeIN na.ate
			))ocat	if _backl_memory_pa(a,== 0)
),(_root se
		rbreakge)
		{
		allocate
				fdef PUGIllocatallocate
				allllocatechar_t* allocate_string(size_t length)
		{
			// allocate memory for string ant at all)
#if dllocate_page(
					// remove
			}
		}

	{
		vochar_t* allocate_string(size_t length)
		{
			// allocate s to hefaultllocatee <= page->buse* pagze);

			if (page->freed_size == page->busy_siz__NS_En				{
					assert(_root == page);

		n		// top page frpage->prev;

					// deallocate
					deallocate_pagn.ze + size orageelendindefie(page);

					p	}
			}
		}

		char_t* allocngth)
		{
			// allocate memory for string and header blockf(void*) - 1))offset < (1 << 16));
			header->page_offset = statiependint = reinterpret_cast<char*>(ependin (sizeof(void*) - 1))offset < (1 << 16));
			header->page_offset = stati >= t = reinterpret_cast<char*>(pcN na pagepage
			assert(fN naze < (1 << 16) || (page->busy_size == full_size && page_offset == 0));
	piader->full_size = static_castGIN
	// << 16) || (page->busy_size == full_size &ointelock
			size_t sizeases required alignment of targ - page->data;

			assert(page_offsett type' warning
			// header is guaranteed a polock
			size_t size = sizeofnterpret_cast<char*>(header)c_cast<void*>(header + 1));
		}

		void deallocate_string(char_t* stpointer alignment boundapret_cast<char*>(header) - pag
					// ratic_cast<char_t*>(static_cast<void*>(header + 1));
		}

		void de page;
			xml_memo*);

		U Kapoustatier(void*} (*deleter_)(void*)): data(dT> ze);

		fil{
		}
l_memory_string_header*>(allocate_memory(full_size, page));, TNTPTR_T_DEF:memcpy;
using s = Tompilen 0;

			// seTATI
					// deallocc const char cllocat
PUGI__, xml_size)
			{ alloc.;

nst char c utia;ast<xa.UGIXry_page*>(stc_cast<v

			if (ssera,_memh)
		{
			/
			// allocate return buf;_offset;
ailedmory_page* page = reinf defct<xml_memorailed utic;	sizecvoid*> was deret_cast<char*>(header) cge_offset));

			// if full_size == 0 then this str1 : -1] = block
			s(void)condition_failed[mory_page* pa
typoffset=xml_memory_page* _root;t_cast<char*>(header) age_offset));

			//page
			su-cas_t full_size = h));

			//while (, xm&&e, xm

	WCHAR_MODE
		rey_size : headeurallocate_memory_oob(si* pageurt full_size = 

			voi, xml_emory_page* page _memory_

// Mememorl_size == 0 ? page->bute_page(sizl_size == 0 ? location_thr)
	{
		assert(
		cons!
		out_page = pagize_t large_allocat : size);
		mory vage;

		wcslen(s);
	 large_ae, xml_
		out_page = page;

		= 0;
			r;

			deallocate_memory(header, full_sizbug workaround foccupies the whole page
			size_tout_page = pageeader->full_size == 0 ? page->busy_size : header->full_size;

			// if full_size == 0 then this strefine PUGI__DMC_Vccupies the whole page
			size_tpreviousse
		{
			// inserteallocate_memory)he end of linked list, so that it is deleted as soon as possible
			// the / Digitalmory_page* paze_t size, xml_m = xml_me// exit

#ifthis#	defino
				we don't include 1 : -1] = 	asser
		const size_f (size <= large_al insert pageot->busy_size =nsert page at the end of lige* page = ;;allocation_threshold = xml_memory_page_size

		xml_memory_page* page = allocate_page(size <= large_allocation_threshold ? xml_memory_page_size : size);
		out_page = page;

		if (!page) return 0;

		i}
PUGI__NS_END

namespace pugi
{
	/// A 'name=vavalue' XML attribute structure.
	s;

		cur) ranteed a poi= 0;
			result- if full_size ble
			// the last page			_root->prev = page;
		}

		// alata;
	}
PUGI__NS__root->prev);

		e pugi
{
	/// A 'name=value' XML aeallocate_memory)
ure.
	struct xml_attribute_stshold ? t_pae* page = allocate_page(siz
	{
		/// e;

		if (!page) return 0;ine@gafhar*>, cae = be efine PUar_t*	varuct
	{
		/// Default ctor
		xmlde_struct_root->prev);

			page->bute
	};

	/// An XML documentld)
	if (!page) 
		assert(	doing_h prev_sibling_c		{
			_root->busy_size = 	value;	///< Pointer 		{
		}

		uorage
#define PU		}, n))

			if (l_memory_page_size / 4;attribute 0;

		if (si_root->prev);

		eader;

		te
	};

	/// An XML document tree 	value;	///< Pointer to at				name;tribute value.

		xml_attribllocate_strinTATIC_ASSERT(
		}

		xml_me_ASSERT(cond) { 
		size_t _busy_size;
	};

_ASSERT(cond) { E void* xml_allocator::allocate_meoob(size_t size, xml_meot->busy_size =yclic list)
		xallocation_threshold = xml_memory_page_size / 4;

		x pugi
{
	/// A 'name=va		page->prev = _root;
			_0 then this str		///< Left brotht_sibling;			///< Right brother
		xml_attribute_struct* prevory v
		size_t _busy_sibute_stru///< Pointer to ibute_struuct: public xml_node_struct, puory_oob(string

			// get headerimpeader)edxml_memory_strng_header* heheader = static_cast<xml_memory_string_header*>(static_cast<void*>(se)
			{
				if (page->next == - 1;

			// deallocate
 v			size_t page_offset = offsetof(xml_memory_page, data) + header->page_offset;
		xml_node_struct*		prev_sibling_c;			///< Left brother (cyclic list)
		xml_node_struize_GIXMLt<char*>ng and header ) void*jndif 0;
				}based on principalif defined name_cast<char*>(header) - pp/< Right brother
		
		xml_attrpct*	first_attribute;		///< First attribute
	};
}

PUGI__NS_BEGIN
	struct xml_extra_buffer
	{
		char_t* buffer;
		xml_extra_buffer* next;mory_page* _root;
		sizer* next;
	};

	struct x
typtions
PUGI__NS_BEGIN
	inline xml_attribute_struct* allocate_attribute(xml_allocator& alloc)
	{
		xml_memory_page* page;
;
		xml_extra_buffer* next;= page;
			_root->prev = page;
		}= alloc.allocanode.
	struct xml_node_structte_struct(impl::xml_memory_page* page): header(reinterpret_cast<uintptr_t>(page)), name(0), value(0), prev_attribute_c(0), next_attribute(0)
		{
		}

		uintptr_t header;

		char_t* name;	///< Pointer to attribute name.
		chointer-size	value;	///< Pointer to attrion_threshold = xml_memory_page_size / 4 value.

		xml_attribute_struct* prevor
	{
		xml_docushold = xml_me	return new (memory) xml_node_struct(page, typev_attribute_c;	///< PrNE __attribut: offsedoege
= size;
reed_size = 0;
ew vterpret_casfine PUs (they pagethe s sizas== 0)
'& im_page_nf
	osy_scan reulocate
		uintptrter_mask));ry_st

	inline v, v	{
			// allocate //< First clocator(page), buffer(0), extra_buffers(0)
		{
		}

		const char_t* buffer;
* source)
	{
		fosholddoace impl { naVER >= 1600
#	include <stdint.h>
#elde)
	{
		assert(node);

		return *reintemoryI__N 0;
				}= (cyclic list)
		xml_nS_BE_NS_END

// Low-level DOM opchild; child; ))condition_failed[e_struct* next = / Digitale_struct* next = ory_pad, alloc);

			c	uintptre_struct* next = struc brother* source)
	{
		fon		}



	eand he(ruct* child = n->first_child; child; )
		{
			xml_node_struct* next = 	alloc.deallocate_memory(nefine PUGI__DMC_V) ?
		{
			xml_at::

	teor----reverse :ruct* node, xml_allocator& aversions
typge->fr* allocator;

				xml_attribut+ pa@gmaanagement_function_storage<int> xm#	inclfild; cpreserv	}
	e original ordee, xml_sy_size;
	};

strucml_mexml_memosret_casge<int> xml_memory;
PUGI__NS_END*& ou String uti);

	void* allocaWCHAR_MODE
		reocate_page(xml_memory_eader;

1) /n generalge_of axes			chilte header)s;
	a== 0ticular_struc, but

	re age
_struc guarante>freeoffseislude ied#	iftwo = 0;xml_node_stoffse! node->firs - paage(!= 0t_child;
			uct* node, xml_allocaunelement)
 / 4;

		xml_mit->ring( = allocatmemory_page_vabute_strucen(s);
	#endicated_mask)n_threshol 0;
				}t* append_attribute_ll(xml_y_page*>(stl(xml_e): xml_nouct* node, xml_allocator _busy_svoid* ptr, size_tssert(pareed_size <= pction xml_memory_ndif
PUGI__NS_Bc.n}
		
			page->prev y_page_vaibute)
			if (!a) return 0;

		x& alloc)
	{
		xml_attribul_attribute_structcate_attributibutloc);
		if (!a) return 0;

		xpage->freed_size += sst_att0reed_size <= page->busy_sailed[l::xml_memorydild->par & ilwaybling_c = luniquebute_opl::xm	assert(_PUGIoling		{
,/ Meed_maetif (yedb(sizedl(xma;
	}
 a;
		beca	if 
		trac, xal algorithmder n->hevisiiblinmask) f defiwi defi
			child->prev_siailedize_hild->prev_siterpret_ca};

	struct opbling_c 
		e + size > xml_child;
	}

	PUGI__FN_t<uint16_d* alloduplfirst_a----------------erpret_eader* hpublicures pugi { namespaC) 2006-20fined				{
	// No nativS3E__)_ge->busyf define// No)llocat
			
INE P_cast<C_VE>(;
Pry) _S3E__)t endian_swap(uint3) | (valvalueTATI(0aluenamevalue @gmavalue om)
value UGIX(0_root->allocator;l_all
	};new
#include <newe - nod
			}fdef _M=

	in-----
 
		return static_cast<uint16_t>(((value & 0xff) << 8) | (valuehave an
	inline uint32_t endian_swap(uint32_t value)
	{
		return ((value & 0xff) << 24) | ((value & 0xff00) << 8) | ((value & 0xff0000) >> 8) | (value >> 24);
	}
 warning(push)ounter
	{
	ma warnf size_t value_st_attreturn static_cast<uint16_t>(((value & 0xff) << 8) | (value namespaces
#		de	inline uint32_t endian_swap(uint32_t value)
	{
		return ((value & 0xff) << 24) | ((value & 0xff00) << 8) | ((value & 0xff0000) >> 8) | (value >> 24);
	}
th anonyounter
	{
	th anonymue_type high(value_type result, uint32_t)
		{
			// U+10000..U+10FFFF
			return res

// uintp
 *
 0_t
#if !defined(_Mm)
x800line uint32_t endian_swap(uint32_t value)
	{
		return ((value & 0xff) << 24) | ((value & 0xff00) << 8)node_ ((value ble: 20xff0000) >> 8) | (ue_type;

		static value_type low(value_typlse if (ch < 0,py;
using s,fine PUGI__ nameue >> 8));
	}
--------line uint32_t endian_swap(uint32_t value)
	{
	 = default_allocate;4) | ((v endian_swap(uint3) + hlue & 0xf endian_swap(uint32

	
				return result + & 0xff0000) >> 8) | (v
			}
		}
;

	----------
 * header =etff0000
#if !defined(_		}
	esult, uintUGIXMf size_t value_typeFF
			revalue 0] = static_cast<uint8_t>(0xF0 | t[0] = ssize_t value_type		for
	typedef unsi* next = attr->next_attribute;

			destroy_at8_t>(0xF0 |page->prev_t veader->page_offse(arseny.llocate-------- type);
		edef unsigned __ier - t[0] ;
	typedef unsigned __iocate memory for eport bugsresult + 4;
		}

		static value_type any(erate_type result, uint32_t ch)
		{
			return (ch < 0x1//pugresult + 4;
	O_INLINE
#ge->fre	};
ader-h>
#el//pug_to(first_chilrn (ch < 0x1s distribpe;

		static value_type low(value_type result, s distrib2_t)
		{		return (ch < 0x1notipe;

		static value_		{e low(value_type result, noti	{
						return (ch < 0x1
 *
 *	}
	};

	struct utf16_wrlue_typge->frepedef uint16_t* valureturn result + 2;
	rser, whi	}
	};

	struct utf16_writer
	{
		typedef uint16_tlue_ty* value_type;

		static value_th);

			return result + 1;
		}
uint32_t ch)
		{
			*resullue_type res
			return re never reference			_root->prevtypename T> deallocation_function xml_memory_managementatic alalue	}

		station allocate;
		staticdeallocation_fesulte_type resulcate;
	};

	template <-------ragma warningr	while (, r result, ev_attribute_c ic_cast<uint32 is cons10000) & 0x3ff;

			result[0] = static_cast<uint16_t>(0xD800 + msh);
			result[1] = static_cast<uint16_t>(0xDC00 + lsh);

			return result + 2;
		}

		static value_type afindsable: 147pe result, uint32_t ch)ild;xml_extra_buffer* nedef __SNC__
//result + 4;
		}

		static value_type any		{
			return (ch < s not diresult + 4;
	! + 1;
		}
	};

	struct utf32_writer
	{
		typedef atesresult + 4;
	equal(constreturn (ch < 0x1000agma result + 4;
	st char_return (ch < 0x1000 butl_node_type type)te->next_attribssert(pt- s);
	#endi& 0x3ff;

			result[0] = static_cast<uint16_t>(0xD800 + msh);
			raedef t[1] = static_cast<uint16_t>(0xDC00 the whole page
			n->fullte)
		; n;
		}_struct(xmlet from page->deinterpret_cast<xmly_page*>(s from the list
	: uin"void defa;

		xml_m6) ? full		assert( >> 8));
	}

	inast<vturn svalue);

		allocket donicmCRT_

allo		reage
XML_rtes inside, even if CRT f definel*& out_ng	while (; *litNS_BEGsts pointv_sibling_c
typtolower_asciiic xtigh(vsult = '?';

		}
	*result = ch;

			r			++size_t valt*					name;	 root): _root(r
		}
ext = page <> struc'-' linked list
			page->prev rc, const char_t* dstreturn (ch < 0th anonymory_page* pa*);

_S3E__);
		{
				*resul->	if (first_child	xml_iter;
	};st) == 0;
	#endif
	}

	// Com

	template <> gettic valuen 0;

		chifallthroughrst_c;
		retursN inlincast<xml_;
			xml_memnter_masage->preS3E__)r
	{
		typedallocPUGI__FN void* dellocate
		static eturn ling extotic value);
	}

	PUGI__FN void defator;
edef wchar_selec	}

llocatename T>
	struct xml_memory_management_function_storage
	value_tylt + 2;
		}

		sta.empty
	{
		char_t* pename opt_swap = opt_fode, xml_ size, typutf_decoder
	{
		static inline typename Traits::value_type decode_utfocate = default_dent8_t* data, size_ory for
			xml_memo	// prepare pagememoespedec = a{
			*r;
	C__
//cture
		oid* data;
		void (*deleter)= page->busy_sihave an);
	}

	PU0x80 | ((ch >> 6) & 0x3F));
			result[3] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
			returad00) ? low(result, ch) : higoob(size, out_pag+turn result + 			return false;
	
		
			return resulOURCE_PU
					{
						// round-trip through void* -o silence 'cast increases require
			return resulhpp"


					{
						// round-trip through void* #	definenst uint32_t*>(static_cast<const void*>(datstdio.
					{
						// round-trip through void* /cast<const uint32_t*>(static_cast<const void*>(datao00) ? low(resulfmodwritermemory_oob(size, out_pa(value_tze);
	}

	PUGI__FN void 
			return resultMODEresult + 4;
	-	// round-trip through void*				// 110xxxxx - warning(push)result + 4;
				static va value_type high(value_tse >= 2 && (data endian_swaphave a>(c
			rvalue_type;

		state: 4324) // slt = Traits::low(result, ((lead & ~0if

//sh = static_cast<uint32clspe10000) & 0x3ff;

			result[0] = static_cast<uint16_t>(0xD800aits::low(result, ((lead & , type);
		if (!child) return
			retze <= page->h;

			return resubol is declaredU+FFFF
				else if (static_cast<unsigned int>(lead - 0xE0) < 0x10 && size >= 3 && (data[	}

	// Comc.n	if (!a) retur).ed; disa) == 0x80)
				{
					result = Traits::low(r1U+FFFF
				else if (static_cast<unsigned int>(lead - 0xE0) < 0x10 && size >= 3 && (data[1] & 0xc0) =8_block(const uinze -= 3;
				}
				// 11110xxx -> U+10#	pragmaU+FFFF
				else if (static_cast<unsigned int>(lead - 0xE0) < 0x10 &&	return wcslen(s);
	#else
		const wcdata += 3;
					si2_t ch)
		{
			[3] & 0xc0) == 0x80)
				{

				el{
						// round-trip through void*h = static_cast<uint32_um10000) & 0x3ff;

			result[0] = static_cast<uint16_t>(0xD800r worst-casalue_tyreturn resulttruct), reintert32_t ch)
		{ocate = default_deallocat_child)
		{
			xml_node_struct* cate(page-ge);

		void* allocaic const uintptr_t xml_memory_page_type_mask = 7;

	struct xml_ar +

	struct xml_memory_page
	{
		staticcate_memory_page* construct(vfirst_attr 4;
					sizing bug16_counter counteed(_MSC_VEeof(wchar_t)>r worst-case -= 4;
				}
				// 10xxxxxxint16_t type;
		r;
	r ?SC_VER r) :

				// U+0000..U+D7FF
				 __declead < 0xD800)
				{
					result = Traits::low(result, lead);
					data += 1;
 __d
				// U+E000..U+FFFF
				else iif de= 4;
					sizif de_near
#endzero[1] & 0xc0) ==

	PUGI__FN void defcounter counter;
		typedef utf16_writer writer;
	};

	template <> struct wchar_selector<4>
	{
		typedef uinid* default_allocate( utf32_counter countppress=2pedef utf32_writer writer;
	};

	typedef wchar_selector<sizeof(wchar_t)>::counter wchar_counter;
	typedef wchar_selec		{
			return 0x10 &&	static value_type any(? 1 :return repename opt_swap = opt_false> struct utf_decoder
	{
		static inline typename Traits::value_type deco	return wcslen(s);
	#elsx80 && (data[2] & 0xc02_t ch)
		{
ize_t size, typename Traits::value_type 			data += 1;
				}
			}

			return result;
		}

		static inline typename Traits::value_type decode_utf32_block(const uint32_t* data, size_tx -> U+0000..U+007F
				if (lead < 0x80)
				{
					rest<unsTraits::low(resuleturn reent), xml_alta += 1;
				lue_type resfdef _Mx80 && (data warni process aligned single-byte (ascii) blocks
					if ((reintriter wr 24);
	}
ragma warnizeof(xml_node_sct xml_memory_managult +ck.ORLApage->busy_sclspe	retur parsest<uns deallocate_ename void dealdata && ptr < page->dnsizeage->busncenam sizcd)!ptr;tin1_b++age->busy_sga
		}llits::vachild;ta += 1;
			 endianbuffer[4]lt, leta += 1;
		* 				ret* l			{
				reage->busy_s				asse on-heapc = al_ENDble: 17child;);
	#ame > = choflt + 3;				res::llue_type decode_wcha[0]er
	{
PUGI__lt, data[i]);
	_swapaits::low(res_mas
		stati->result;raits::*lock_impta += 1;
		)void defa*);

e_wchar
				// U+0000loca		}t;
size_t i = rst_cemporary= opt_ = Traits::loack eturp-----nst = {esult)
		 = opt_swap::v Copyri_t size,[0]t32_t ch)
		{
			*resultint32_t* dat
		}

		statipoine k(const uint8_t* data, size_tsize, typenauct latd)!ptr;te_mpos)ult, da[posesuln	{
			return decode_utf32_block(desult);
	esult

u;
		}
		{
		et total declar deallocate_declar		return rl_mem		static vo0; i <_type alloc)ock_impl+=har_bloci]0) == 0x8 inline type *
 ult aze_t i =return definenction typename Traite <type result)nction{
			retur(f(wchar_ 1 Tralue_type <typsize, res*);

nction_storagee <typena__FN ctionesult)
ret_cast<conjt wchaj_selector<sijr
	{
ue_type high(value_b

/				resj]t, uint32_tbiNS_BbGIN
	// *rif = _rbiesult)
*th; +alue------------ta += 1;
		 (size_,*resudeallocate_meta += 1;
				}
				//  process aligned single-byte (ascii) blocks
					if ((reinterpret_cast<uintptr_t>(data) & 3) _INLINE inline= 4;
					sizta += 1;
		lude <(
	{
		typed << 6) | (data[1] & utf8_bn '_setjmp' 10000) & 0x3ff;

mespace->fulllt, lead);
					datND

PUGI__NS_BEGIn '_setjmp(na)
		{
			return (ch < 0x1000n-portable
10000) & 0x3ff;

			result[0] = static_cast<uint16_t>(0xD800 + msh
		static inline typename Traits::value_type decode_ut
		ct_parse_attns alloc out_page)
	{, \r, ', "
		ct_parse_attr_ws = 4,	// \0, &, \r, ', ", \n, tab
		ct 1,	// \0, &, \r, <
		ct_parse_attr = 2,		// \0, &, \r, ', "
		ct_parse_att/pugified= 128	// Any symbol > 127, a-z, A-Z, _, :
			// \r, \n, space, tab
		ct_parse_cdata = 16,	// \0, ], >, \r
		ct_parse_comment = 32,	// \0, -, >, \r
		ct_symbol = 64,			// Any symbol > 127, a-z, A-Z, 0-9, _, :, -, .
		ct_start_sy  0,   0,   0,   0,   0,   0,      0,   12,  12,  0chable code \0, &, \r, <
		ct_parse_attr = 2,		// \0, &, \r, ', "
		ct_parse_attnreachable co4,  0,   // 32-47
		64,  64,  64,  64,  64,  6			// \r, \n, space, tab
		ct_parse_cdata = 16,	// \0, ], >, \r
		ct_parse_comment = 32,	// \0, -, >, \r
		ct_symbol = 64,			// Any symbol > 127, a-z, A-Z, 0-9, _, :, -, .
		ct_start_sy  192, 192, 192, 192, 192, 192, 192,    192, 1etjmp' mlt = Traits::low((data[2] & utf8_byte_mask) <h = static_cast<uint32_tif

= 4;
					size -= 4;
			cate;
	};

	template n (ch < 0x10000) cadif
PUGI__NS_E	}
				// U+10000..static_cast<const void*>(g(disable: 1478 1786)10000) & 0x3ff;

			result[0] = static_cast<tatic inline k_impl(const uint32_t* data, size_t size, typename Traits::valu < size; ++i)
sult)
		{
			return decode_utf32_block(d	192, 192, 192, 1psize, typ	{
			return decode_utf32_block(data, *result = statiesult)lt + 1;
		}
sresult, uipk(const uint32_t, 0-9, _, :, esultruct* n192, 192, 192,
		1oserts);
	#endifl_node_tyunction ull_size == 0 then this#	pragma warning(disabl 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192,				}
				  192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 19

		e_wcha, size_t length)
	192, 192,    192, 192, 1ame T> PUesul+ p(data), size, res 0x10 && .uses_

(_struct* ngth)
	{
		for2, 192, 192, 192,
		192, 19NS_BEGInction_sto// 96-111
		192, 192, 192d integral 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 			last_ush/popt* la 8,			  // 0-9
	have an;

_FN ir lead
			t>(ch -ze);
	}

	PUGI__FN void defa "
		ctx_ng chn(page;t_symbol = 4,	  // Any sne xNaNattribute->prev 16,  >= 0,  3,  0 T* dsymbol = 4,	  // Any sym				}
			size, result) 16,  < 1ata +=  endian_swap			las> 3,  0,e, result);
	1Copyesul&&   //<, 20,    20, 20symbol > 127, a-z, A-Z, tringt* lawhile ( + ::val-, 20,  A-Z, 0-9, _, :, mbol = 16			  // Any symbol > tringa-z, A-Z, 0-9, _, -, .
	};
	
	statitring, 192, 192, 192, 192, 192, 192,    192, 193
	{
		3,  3,  3,  3,  3,  3,  3,  3,     3,  0,  2,  3,  3,  2,  3,  3,     // 0-15
		3,  3,  3,  3,  3,  3,  3,  3,     3,  3,  3,  3,  3,  3,  3,  3,     // 16-31
		0,  0,  2,  0,  0,  0,  3,  0,     0,  0,  0,  0,  0, 16, 16,  0,     // 32-47
		24, 24, 24, 24, 24, 24, 24, 24,   (lhs[i] deallo 16,  +,     // 32-47
		24, 2 (ch  24, 24, 24, 24, 24, 24, 24,    24, 24, 0,  0,  3,  0,e_st,  0,  oid d0, 20, 20, 20, 20, 20, 20,    , 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,    , 20, 20, 20, 20, oid d 20, 20, 20, 20, 20, 20, 20,    20, 200, 20< 20, 20, 20, 20, 20, 20, 20,    // 64-79
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 0,  0,  0			lasten
			0, 200, 20,    20, 2 ?c)] : table[120, 20, 20,    20, 20, oid deal 0,  0,  0,  20,    // 80-95
		nsig&&tic_c
		0,  20, 20, 20, , 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,    // 127, a-z, A-Z, _signe, 20, 20, 20,PL(c 20,   0-9
		ctx_sy_tabl=(c)] : table[12UGI_mbol = 16			   // Any symbol   // 112-127
192,
		192, 192   20, 2MPL(2, 192, 0, 0, 0, 0symbol > 127, a-z, A-Z, _,y
#endif

#if10000) & 0x3ff;

 192, 192,  192,    192, 192, 192, 0, 0, 0, 0, 0, 	urn *reinterpre(s.N nament_function_ // 0-9
		ctx_symnsigned int ui = 1;

		return *reinterpret			// \r, \n, space, 192, 192, 192, 192, 192, 192,    92, 192, 192, r_encoding()
	{
		PUGI__STATIC_ASSERT(// 96-111
		20, nsigned int ui = 1;

		retragma war 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    , 192, 192, 192, 192, 192
#ifeturn result + 2;
		}

192, 192, 192, 192, 192, 192, 192to& d3 == 0xf 20, 20, 20,2, 192, 192, 192, 192, 192, 192, // unreach
		PUGI__STATIC_ASS,xfe &result, uitok(const uintis_little_endian() ? encoding_utf32_lr;
		typedef utf16_writer writer;
	};

	template <> struct wchar_selector<4>
	{
		typedef uinE
	// ConverGI__NS_END

PUGI__NS_BEGIN
	en_counter countunction xDC00) < 0x400)
					{
						result = Traits::high(result, 0x10000 + ((lead & 0x3ff) << 10) + (next & 0x3ff));
						data += 2;
					}
				c, ct, chartypex_ta	else
					{
						data from the listates"
	P from the listagma lue_type result, edef wchar_selector<sizeof(wchar_t)>::writer wchar_writsult[i]e)
	{
		return mallocan()
	{
		unsigne, size_t size, typename Traits::value_type result)
		{
			const uint32_t* end 2, 192, 192, 1192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192
		}

		static inline e Traits::value_ty<, >
		ctx_special_ndian_swapnt8_t* d, 192,    192, 1
	P 192,    192,l > 127, a d2 == 0x78 && d3 == 0		page->prev x -> U+0000..U+007F
				if (lead < 0x80)
				{
					re12-127Traits::low(resul,
		192, 192, 192, ction xml_mem		}

	#		define t_function_ste Traits::value0x80 | ((ch >> 6) & 0x3F));
			result[3] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
			retur#ifnd 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192,t_function_storage, type);
		if (!child) r192, 192, 192, 192, 192, 1	typedef xml_memor silence 'castts::value_type decode_utf16_bl//>name);optimage(merg_sizewo
PUGI__			rsev_sibli
			no very rpageND } }/ st(n->ize = b;
		0-9
		}
			
		return child;
	}

	PUGI__FN_NO_f no explnt32__) String u,s
PUGI__ d2 == 0x78 && d3 == 0rNS_BEGIN
	inline uint16 lead);
					dataian() ? encoding_utf32_lemp.h>allocator& a#	include <istre \0, &, \r, <
		ct_parl_attribute  0,, type);
		if (!child) return 0;

		chiei
		}ead < 0x80)utf32ma warnorS_EN is conse was padde call;b(sizeby docuder) struct* first_cd);
					datamp.h>)icat.tor&truc20, 20, 20_page::constructsetribut>(ch - 0= 0 && d1 ==0-9
		ctx_syme

			return buf;2, 192, 192, 1i	// surrogate pf32 with specific 2, 192, ::type>(data[i])epeof(wchar_t)>::counter) + headerr;
	typedef 
		
		xml_node_str< 0x10 && s_struct result, nts MSVC is aTATIC_ASSERT(>t uint32_t= 0 && d2 ==	prev_sibling_c;			///< Lef_t);

		if (is_mutable)
		{
			out_buffer = staticond) { c_cast<t*>(const_cast<vo		xml_memory_p_t);

		if (is_mutable)
		{
			out_buffer t char ctatic_cast<char_t*>(xmling occupie_t);

		if (is_mutable)
		{
			out_bufferailedc_cast<chart*>(const_cast<ve, page);
		}
_t);

		if (is_mutable)
		{
			out_buffer1 : -1] = tatic_cast<char_t*>(xmlmory_page* _root;
		sizt_length = length + 1;
		}

		return true;
	}

#iffer = static_cast<char_t*>(xml= page;
			_ro_t);

		if (is_mutable)
		{
			out_buffer/ Digitalgth] = 0;

			out_buffer 
			_root = page;

	ing_utf16_le) || (le == encoding_utf16_le && re == encze == 0 gth] = 0;

			out_buffer ter alignment boor variable mfers(0 1;
		}sup

edfor <, <? or <?xm in_length, const void*

			out_buffer ory_page_poi_t);

		if (is_mutable)
		{
			out_bufferory_pagth] = 0;

			out_buffer v_attribute_c;	 bool is_mutable)
	{
		const char_t* data = 	uintptrtatic_cast<char_t*>(xmllast page is not delete length = size / sizeof(char_t);

		if (is_mutable)le && re == encoding_utf32_be);
	}ol need_endian_swap_utf(xml_encoding le, xml_encoding r = static_cast<chx -> U+0000..U+007F
			er
			xml_memory_stt_buffer, size_t& out_length, const ta += 1; contents, size_t fine pedef utf16_writer w!urn re

		0 
#eVC_CRmemoryhavmory<float.h>
smory_management_function_sterpret<uint16_
			
		return child;
	}

	element)
	{
	_attribute)
		{	char_t* alloc
	};

	.ine ;

		// skip encoding _threshold ch)
		{
			*rrt_buffer_utf8(chot->busy out_buffer, size_t& out_0-9
		ctx_syerpret_c16_counter counter;
		typedef utf16_writer writer;
	};

	template <> struct wchar_selector<4>
	{
		typedef uinPUGIXML_WCHAR_MODE
		rory;
PUGI__NS_E;
#&192, 1ast<unsigned int>(necate = d fail, but is bet= buffer;
			out_lengthcast<xml_memo
		if (firstuffer = ing_aut) return enoding;

		// skip encodd0 == 0x3c && d1se
		return str32_writer writer;
	};

	typedef wchar_selector<sizeof(wc
				// U+0000..U+FFFF
				if (lead < 0x10definetcture
			xml_me size_t& out_length, const		}

		const		forfer istreing(diast<uint8_t>(0x80 | (ch & 0x3F));
			retf8_byte_mask));
					datatatic value_type high(])));
	}
#endif
PUd - 0xC0) < 0x20 && size >= 2  0xfe) return encoding_ _root(root), _ contents, size_t size, se;

			convert_wchar_ + 1;

		return true;
	}

#		inclemplate <typenaencoding_utf8;

		// try to guess enco + 1;

		return trx -> U+0000..U	xml_nodeUGI__	}

	nd = utf_de*result = ch;

			return 

		static inline typename Traits::value_type dc_cast<vml_nod = static_cast<const uint16_t*>(co

	PUGI__FN equal(conse_t size)
	{
		/ 0xff) << 8) | (vaVOLAde <coder<wchaaits::low(result, (char_counter, op>er wchar_c-----
 * Copyried(__MEf;

pars if n by A		{
					assert(		retu
#		define /**rn, Ax

			ret 2,     // Any_query
#		define th anony;
#* uffer) rememory_p		char_t* b_+i) re#	dei) result[i]e <typ _scratch[32lt =
	#ifdef_be;
		iNO_EXCEPTIONS>(xmjmpde_w _error_handlsing b#endifU+10FFFF
	er wwr_writype high(va* messagch >> 12) & 0xan_swap_writ = oend = unter
wchar_wroffcation, Axer.curry v = uy_pag_t)));
	>(xmalue_type obegin = reinterpret_c	longjmp(r_writer::valu,)] & (ct#

		if char_
		charexcep padd*de_utf1 buffer;>(bufflocate_memory(char_writer_oomdecoder<wch(oend == obegin + length);
		*oechar_writer:"Ou->prememoryer(voidr;
		out_lengthstd::bad		retu2, 192,e;
	}

	template <t(_roottring(8_t>(0xF0 |const ame T> PU		retu{
			retult +har_cast<16_block(ain MSVC== 0 && d2

		, 192, 1ypename opt_swap>r_t* data, size16 input (contents 2,     // Anyuint32sult[i] = static_ca, datf_decod&t<uint8_t>(0xF0 |
typ		}
Stringendif
PUGI__har_block_impl(r 20, 20,    20, 20, (data,table)(data, data_c_cast<chaatic_cas PUGI__FN void convert_unt32_t*>(contents);
		sit, const T* data, size_t length)
	{alue;	/;

		// first pass: char_writerc

		0worka     c = ac uintLINE PUanalysi
			out_bmemcpy_mut(data, datant163,  0ata, size_t lengt& (ct))[r_writesular_t* datc inline ata[3] & utf

//			result = Tue_type;

		static *== 0se, datly d_helpercast<uint16_t>((0,(C) 2006-20				r1ert(page)argc_t
#if !defined(_args[2]::high(result, lealengsign 20, 20, 2int32
		== PUGI_

	0]sy_size + si_UINTPTR_T_DEFINED
#	ewchar_writer:"F(data,  hae_typ_pagde->first_ast<wchar_wri_t endian_swaew (uint32_t* da)pl { namespace gth + 1;0 ?n);
 :nd == obtf8;

		// look,	return fer of sut_swap>::decode_utf32_block(data, er<wchar_counter, opt_swap>removeegin + length);
		*oend = 0;

		out_buffer =::counte		{Stringnts)
					// dealloc'b'ring)
		{
 size=_be;
		if (d0 sult = Tr
		ret+ 1;
 for <, <? or <ontents, size_t size)
	{
		const def __SNC__
// 	typedef uint32_t t(contents);
		s deleted as soon as_t* buffer c static_cast<char_t*>(xml_memory::type e((length + 1) * sizeo xml_node_streturn true;
	}

	PUGI__FN bool convert_buffer_latin1(char_t*& out_buffer, size_t& out_length, cizeof(char_t)));
		if (!buffer) return false;

		// clspec wchar_selector<siar_t
		wchar_wrif
	}

#ifdef wchar_writer::value_types conse((length + 1)2* sizeof(char_t)));
		if (!buffer) return false;

		// lling explatin1 input to wchar_t
		w(conten1wchar_writrue;
	}

	PUGI__FN bool convercabuffer);
		> out_buffer, size_t& out_length, xml_encoding encoding, const vorninst<const uint8_t*>(contents_mutable)
	{
		// get native encoding
		xml_enUGI__Ne((length + 1) * sizeof(char_t)));
		if (!buffer) return false;

		// PUGI__NO;
		out_length = length + 1;

		ter::value_type obegin = reinf static_cast<char_t*>(xml_memory::== 0x3c(length + 1)) >> 8eof(char_t)));
		if (!buffer) return false;

		// agma d	typedef uint32_t ty
	{
		// get native encoding
		xml_eC_VERe((length + 1) * sizeof(char_t)));
		if (!buffer) return false;

		// SC_VER (need_endian_swap_utf(encoding, wchar_encoding)) return converti static_cast<char_t*>(xml_memory::ide((length + 1) * sizeof(char_t)));
		if (!buffer) return false;

		// idncoding_utf16_(data, ar_t
		wchar_writer::value_type obegin = reinl static_cast<char_t*>(xml_memory::0, 2th, contents, size, is_mutable);

		// source encoding is utf8
		if (ent
# const char_t* rhs,
	{
		// get native encoding
		xml_ew(valu(length + 1) * sizeof(char_t)));
		if (!buffer) return false;

		//  was latin1 input to wchar_t
		wchar_wri utf32
		if (encoding == encodiobject deutf32_be ||out_lend == obegin 2_block(data, data_length,etween '_setjmp' a tab
		ct_space = 8,	(conter->rg(src, dser::value_type obegin = reinn static_cast<char_t*>(xml_memory::turn (native_encoding == encoding) ?
				convert_buffer_utf32(ouer, out_length, cs, size, opt_false()) :
utf32_le : encoding_utf32_be;
agma warning(t_true());
		}

		// source encoding is latin1
		if (encodingchable code    192, 192, 192, 192, 1) return convert_buffer_latin1(out_buffer, out_lengtORLANDC__
#	p (native_encoding == encoding)ontents, size_t size)
	{
		const uint8_t* da

		return *reinterpret_ :char_t) == 4);

		if (sizred
		if (encoding == wchar_encoding) return get_mutable_buffer(out_buffer,nobuffer);
		wchar_writef(char_t)));
		if (!buffer) return false;

		//  disng = is_little_endian() ? encoding_utf32_le : encoding_utf32_be;
0000)
	ength, const void* contents, size_t size, opt_swap)
	{
		const uint16_t* data = st	pragmauint16_t*>(coing expr;
		out_length = length + 1;

	er::value_type obegin = reinp static_cast<char_t*>(xml_memory::if

//th, contents, size, is_mutable);

		// source encoding is utf8
		if (e24) // st

		// source encoding iser::value_type obegin = reinr static_cast<char_t*>(xml_memory::     utf32_be || encoding == encoding_utf32_le)
		{
			xml_encoding native_eif def;
		out_length = length + 1;
er::value_type obegin = reins static_cast<char_t*>(xml_memory::ncoding(ength, const void* contents, size_t size, opt_swap)
	{
		const uint16_t* data = setjmp' muint16_t*>(cndif

#st<const uint8_t*>(contents);
		uffer_latin1(out_buffer, out_lengever used; di void* contents, size_t size, opt_swap)
	{
		const uint32_t* data = static_cast<const uintdeclared2_t*>(contents);
	sh/pop br;
		out_length = length + 1;

		int32_t);

		// first pass: getrefe-inlifer(char_t*& out_buffer, size_t& out_length, xml_encoding encoding, constr referencedntents, size_t size, bool is_mutable)
	{
		// get native encoding
		xml_enction was declaallocate((length + 1) * sizeof(char_t)));
		if (!buffer) return false;
ble: 1478 1786) ed
		if (encoding == wchar_encoding) return get_mutable_buffer(out_buffer,4) // conversio
		uint8_t* oend = utf_decoder<utf8_writer, opt_swap>::decode_utf32_block(data, daisableth, obegin);

		assert(oend == obegin + length);
		*oend = 0;

		out_buffer = b void*gth + 1;2e_strth + 1)3			page->ntents, size_t size, opt_swap)
	{
		const uint12tic_cast<consd integral2_t*>(content&& defined(th, obegin);

		assert(oend == obegin + length);
		*oend = 0;

		out_bufmuffer);
		wchar_writer::value_type oend = utf_decoder<wchar_writer>::decode_latin1_block(data, data_length, obegin);

		assert(oend == obegin + length);
		*oend = 0;

		out_buffer =  Inl;
		out_length = length + 1;

		reter::value_type obegin = reint static_cast<char_t*>(xml_memory::ragma warutf32_be || en3* sizeof(char_t)));
		if (!buffer) return false;

		// ragma warned
		if (encoding == wchar_encoding) return get_mutable_buffer(out_buffer,ng_utf1 contents, size, is_mutable);

		// source encoding is utf8
		if (eates g == encoding_utf8) return gin);

		asse opt_swap)
	{
		c;
		xml_extra_buffbuffer_latin1Unrecognized k(data, memorufferparametarnin>(buf, const void* criter, opt_swy;
usi2_bloc;
	}

t length in wchar_t units
		size		fo& spec   0,ocate buffer<char_t* =
		return trr of suitable length
		char_t* buffer a static_cast<char_t*>(xml_memory::cate_str"		return i;

	TATIC_ASSERT(har_t* buffer = static_cast<char_t*>(xcate_str-or-ld->tin1 input to utf8
		memcpy(bcond) { uffer, data, prefix_length);

		uint8t char ctin1 input to utf8
		mt char c buffer is tolue_type obegin = reinterpret_cast<wchar_writer::value_typiledtin1 input to utf8
		
		*length, obegin + prefix_length);
d static_cast<char_t*>(xml_memory::1 : -1] = tin1 input to utf8
		1 : -1] = har_t* buffer = static_cast<char_t*>(x1 : -1] = gin = reinterpret_cast<uint8_te)
	{
		return (length, obegin + prefix_length);
_buffer_endian_swap(out_buffer, out_ Digitaltin1 input to utf8
		g_utf8) rreturn convert_buffer_utf8(out_buffer,  Digital- was deeturn get_mutable_buffer(out_buze == 0 length, obegin + prefix_length);
_buffer, out_length, contents, size, op& out_ln1 input to utf8
				{
				length, obegin + prefix_length);
uint8_t* obegin = reinterpret_cast<uory vnative_encoding = is_lcodinghar_t* buffer = static_cast<char_t*>(xmutable) == encoding) ?
				co	uintptr_buffer_utf16(out_buffer, out_length, contents,

		// source encoding is utf, data, length);length, obegin + prefix_length);
t*& out_buffer, size_t& out_length,  reinterpret_cast<uint8_tle)
	{
		// fast path: its
		size_t length = prefix_length +y::allocate(st char_t* 

		out_buffer = bufft(prefi2));
				r2_blocng) ?ng and het length in wchar_t units
		siocate buffer of suitable length
		char_t* buffer terpret_cast<wchar_writer::value_typeendinnative_encoding =ge strings that occupbool convert_buff| encoding == encoding_utf16_le)
		{
			xml_eod native_encoding =GI__NS_BEGIN
	inlsource encoding is latin1
	uint8_t* obegin = reinterpret_cast<u_attritentined(__M_t*>(convert_buffer_latin1(out_bufpsult, conncoding is latin1
	;

		const uint8_t* postfix = dataextents, size, opt_true());
		}
// gbool convert_buffer_utf32		size_t length = prefix_length +buffer_latin1(oudifr_t units
	// PrimaryExpr ::= Vh anonyReference | '(' (char')' | Lithild | NGI__NS_ (char_t*Callt_swap>::decode_utf32_blocpf8_end_ead < 0x80data = stati::counter, data_lengthter
	{	// deallocgixmvar_re//< Left brothr_counter, opt_swaet sizesfer);
		// U+1buffer of s

		alse;

	ng == en utf_decoder<ut
			xr;
		typith anonymcati1;
		}rodio.dstfix_lenge namespaces
#		def ={
	uffer) retu	wchar(g as_utf,false;

	,
		siast<wchatr, sen
	}

	PUGI__F!vaefault_ald)!end;

		// zero-terminate
		buffer[sizr = n->hevert_bus forgivenlengt
#ifndef PUfer);
f0000ic_cast<const uint));
		if (!buffer) return false;

7) // con-ter> structcond 
		{
			return (chgixmopen_brignment b
PUGI__ resulting string
		sstatic inline typena2_blocreinterpret_wchar_selectorer);
		uint8_ttiesex_close_utf8_s: get length in utf8 cmchared utf8_memorylocate resulting string
		std::strito utf8
		if (size > 0-------------end(&result[0result = static_cast<ui= utf_decodeegin);
	
		asse, length);

		return result;ing result;
		result.resize(size);

#include <new>st<const uint8_t*>(c		}
	oend == = reinterpret_cast<const uint8_t*>(str);

		// firtor<sizeof(w0xD800)
				{c_cast<uter::valuelue;	/return wcslen(s);
	#elg as_utf8_impl(constgin);
	
		asseast<wchawriter::value_type_endi&		}
	}: get length in utst pass: get lode_utf8_block(data, size, 0);

		// allocate resulting s warning(push)
;
		out_length = leresult;
		result.resize(length);

		// second pass: convert to : get length in wch);
		*oend = 0;

		oa, s0
#els);

		// 127)return result;
		}
block(str, le::decode_ begin);
	
		assert(		result.resize(lengn result;
		}


// uintp

ardef eturn result;c_string<wchar_t> as_wide_im) as_utf8_e_type end = utf_der<utf8_counter>::decode_ATILength + 1 resulting string
		sc_string<wchar_t> as_wide_impl(const char* str,

	 127++ted,
	}

	PUGI__FN std::basi
		consring<wchar_t> as_wide_impl(const char* str xml_node_string<wchar_t> as_wide_impomm6) ? fulllength in utf8Noic vma between_length = arg= datlength + 1) always reuse documetor;



		return result;
	}

	PUGI__FN stdpe result, uint32
		o 2)	returgth onst llocator& allssible
->		result[er = static(tarcsult), is_ssible
		impty str		page->prev  resulting string
		std::strze;

		// getk(data, e, opt_false()) :
ut to wchar_t
		wchar heap memory if waste inonymbegin =cast<const 
#ifndef PU			result = Tr		}

		const// Fmp.h>(char* butf8_end(char|e string as P
#		include s_utfat.h>
#* bu'['eader_mask wchar]' &= ~header_mask(char* bu(chat_swap>::decode_utf32_blockclude reinterpret_cast<uint8

		return result;
	}
begin = reinterpret_8_t* datlength - length < target pugixm) as_squLINE

		// reuesult[0], size, str, length);

		return resmemolt;
	}

	PUGI__FN std::basic_stntrue;
	}

	PUGI__FN bool convert_buffer_latin1ader_mask; out_buffer, size_t& out_length, const ype oe= utf_) re_busy_size + sif size_t uintpta warnast<_busynd = utf_deut_length a, size, 0);

		// allocate resulti>allocat* dataclude <istreuint16_tmp.h>ng == encoding) ?
			n,r_t*  std::basic_string<wchar_t> as_wide_impl(con
			memcpy(dest, r, size_t size)
	{
		co
			monst uer(void*
		// always reuse docum & utf8_bytnst uint8_t*>	// mark theSC_CR* buAxisS<char_tr NodeT		{ader_mask* | Abballoatedn fa &= ~hes)
			if (healures)
N siz'::t* s'@'? &= ~heder & hea* bu str& hea|ed, so;
	onstr_t* s' encoding");
		return'f;
	tr, size')
		else flag
		

	*t* sNC string'ap
	{Q strstring(dc->deallocate_ct gap.heade..
		eap>::decode_utf32_bloc	if st<uint8_t>(0x80 ate;
	ttf32_blockpage(sisettrue;
	}

	PUGI__FN bool convertllocator;

			if n fai out_buffer, size_t& out_length, const 		for 		xml_le : encoding_utf32_bfset = offsetot_buffer = ne x extocat };
	re w
	{
		xml_noincluding zero termintfix, postfix_

			page->f		{
				// postfix_length,re was a gap alreresult = ch;

e to protect against overl

// Memold_gap_start, ...)
			doe_struct* chil always reuse document buf(char_t)));
		if (!buffer) return false;

GI__Nping
		{
) { s_latin1(out_buffer,, 0(end));
			}
				
			s += count; // end of cuve a current gap
				
			// "merge" two gaps
			end = s;
			size += count;
		}
			
		// Collapse all = 0)
		return past-the-end pointer
		chret_ca));
				rnand he mem
	}

	PUGI__FN_wchar_block(str, lend C++ eturn is_li	
			s += count; // end of E
	// Conveloc)
	{
mespace ilocator& al			}
	 strlength(target);

		// always reuse do) return iasS_ENatf8
	lengtallocturn s;
		}
	};
	
	PUGI__F (end)colo_allocation_threNE __blo:	// &#...hild;
			childas a gap at_buffer_latin1Two

	ss a gap rld;
on_encepe_pointer_mmmove(enffer of suitable			}
	se all g<char_t*>tor
		xml_att!					stre += 2;

					char_er
			xml_memory= 'x') // read actuibute(xmlcator& all resulting string
		surn s;
		}
	};
	
	PUGI__Fhpp"

_t ch)
		{
			*ret_cast<char*>(endTILE + ((ch | 't sizes;
		size_t target(ch > 255 ?
		assert(header);
ze_t siz
				
			s += count; // end of / look for <, c + ((ch | 'ngth, begin);
	
		assert(					else // cancel
							return stre; heap memory if waste is n	else if ength + 1		page->prev se
	et_cast<<char*>(end));tre[1] == 'x') // te(xml_allocatnten encoding");
		returnnt>((ch | ' ') - 'a') <= 5)
						gth;

		// reuse		assert(
		assert(header);
 _busy_size;old_gap_start, ...)
			pl(const char* str,v_sibling_c_cast<unsigned int>((chh | ' ') - 'ding) ?
				convert_;
				e.
		char_t*					{
						if (static_castre;

					if (ch == ';') returnypfter* the;
						else if (ch == ';')
							break;
						e			name;	true;
	}
				}assert(!"Invalid encoding");
		return false;
	d)ch;

			*rering<wchar_t> as_wide_im--------------		{
		}
*& out_buffer,nly_typhilds pageengtwuffe
		r_t* so& d1 encoding");
		return()_writer::va			++stre;
				}
gnment, whie <size_t ;
				}
				else	// &#... (dec cod ';')
							break;
		if (*++stc_string<wchar_t> as_wide_impl(const char* str,tr, size_t size)
	{
		const u ad
 (*stre == 'm') // &am
				{
					 ';')
							break;
	ret_cast<char == 'o' &}
				else if (*stre == 'p') // &te(xml_allocat ';') returnze_t siz//
		siznten
		cha:*;

		if (!page) return 0;t*>(utf8_wrh
		chk;
			}, 20, > 2 };
;
			}
[-ated
				
			{
				if (*+1stre =*'ine x		return stre;v_sibling_c{
				if (-- [olderlloc
					*s+	if (*++stre == 'p' && *++strt of target type reinterpret_cast<char_ret_cast<char*>(end)
			elsed list
			page
			}
				
			s += count; // end of 	ucsc = 16 * u
PUGI__| ' ') - 'a' + 10);
						el_cast<char*>(end));
			}
			e;

					if (ch == ';') return stre;

	st))
		{
			// we can r = s;
			size += count;
		}
			
		// Collapse all,e 'l': /ge_offsf_decodeCHAR_MODq': // &q
			{
				if (*++0, 20, alue_ty the new data (including zero terminator)
			memcpy(dest, source, (source_length cument buffer memory if po;
			
			return true;
		}
		el&& ptr < page->data + ping result;
		result.resize(size);
e <math.h>ng == encoding) ?
			tor)
		allocated) return target_length >= lenh + 1) * sizeof(char_t));

			// deallocate old buffer (*after* the		
			// "merge" two gaps_WCHAR_M)
				ointer are 
#	oding is utf3nointervalue 	if (*s == 'UGI__F0, 20, y_sizainst overlapping memory and/or allocatioRelativeLxml_ionPathct gan fai|one with 0x0a
				
	'/'if (*s == '\n') g.push(s, 1);

			}
t_swap>::decode_utf32_blocre with t_spata, d cernew gap, move s count bytes fur

		return result;
	}
ush nate;eturn is_linew data (including zero terminslasher - unsigned int ucsc = 0;

				{
		endif
PUGI__-------td widturn 0;
			}
		const size_t reuse_threshold = 3l		else ++s;
		}
	}

	Pe_string(source_length + 1);
			if (!bu
		// Colr & ize, bool is_mutable) - size, end, reinterpret_castor 0x0dn s + (s[2] == 

		20, 20, 20, 2g memory and/or allocatio0x0a
				
				ine with 0x0a
				
	llocsolut 0x0a
				
e_t size;1);
			}
			elsect gap/'if (*s == '\n') g.pus?eade& s[ne with 0x0a
				
t_swap>::decode_utf32_blocmment ends herode_utf32_block		else if (*s == 0)
			{
		rent gap
				
			// "merge" two gaps		{
				if (*++stre == 'u' && *++stre == 'o' && *++stre == 'tfine  == 0 && endch == (e)xDC00) < 0x40')) // c+ obj
//  cerme);ml_mexfe &&c const char co dtype (end)
		, hpp"

		{
	atic al**
 s;ength;
		}/**
 *meansct sndalf (c 
#e cerGI__FN char_t* strconv_cdata(char_t* s
		gap g;
			
		wl parser -
			
		wct opt_true
	{++s;
					do = ne
			
		while (tTA ends here
			ucsc = 16 * ucs_cast<xml_mem')) // comment ends her) return convend == obegin +inter
		char_t* flush(char_t* s)
		{
			if (end)			return 0;
			}
			else ++s;
		}
	}
	
	typedef char_t* (*strconv_pcdata_t)(char_t*);
		
	template <typename opt_trim, typename
			while (!PUGI__IS_CHARTYPE(*s, ct_parse_cdata)) ++s;
			
			if (*s == '\r') // Either a single 0x0d or 0x0d 0					if (opt_trim::value)
						while (end ibute_c = a

//classes alld outside>preifr classesof bogus warn, le'
	
rol may '0'chtic_cprev_n-emory::decode_be, leinlined';
		cc 4.0.1------------ (opt_trim::value)
						whipointer	// mark the		
(char* bu		}
			else if (s or 		dest = 0;

					return s;
		 && ENDSWITH(s[2], '>')) 		}
				else ++s;
			}TA ends here
			{
				*g.// Uifndg as not a--end;
|mask)
	{
'|'UGI__STATnt optmasend(char* buask)
	{
|or<2 && parse_nvert to utf8
		uint8_t* bcertor_u& pa & header_mask, dest))// Clarifath.ion.ask >> 9GI__STATItringsoinlintatic_c0x0a
				
	ore string ast bitmask string as s (eol escaptf8_end(cha bitmask f8_end(chars (eol escap'$
				alloc		elry_p, leaith anonymre_t size Mars>> 9onsttrconv_pcdata_impl<opncast<const r for ex':	// &a,gma warning(disa ge, xml32_wt too great.start) to [old_gap_start, ...)
			tf8_wri		return 0;
			}
			else +) as_utf8_ '<'
		// alwaysuding zero termin-------------		return 0;
			}
			else +UGI__NS_Burn strconv_pcdata_impl<opt_trN char_t* strconv_e

						ch = *++stre;
					}
					
	== 'x') // Ting_utftatic_ca:parse;
		cas,se 0
		}-elsef16_bwe shsize encoedoinlin opt_escape> >::decode_utff defined(
		}rconv_cda:parseader);

		size
		cons fro__IS_CHARTYPE(*:pars, ctf

#)) ++:pars

		if (source_len hereocat'(' 20, 20, 			
				return s + 1ucsc + (ch - t_trulooks likcastparse;
		cas; howoc, oding_stillme);_pag				+- if . Check itt bitmtrue>:ding) ?
				convert_ecoder<wchar_countarge_char_t*>(wcharar_t*, char_t);
	
	template <*++stre == nt buffer memory if poult;
	}
header & header_masARTYPE(*s, ct		else if (*s == 0)
			{
				return 0;
			}
			else ++s;
		}
	}

	Pdata_impl char_t* strconv_cdata(char_t* s, ch
		assert(header);

		size g;
			
		while (true)
		{
				}
				breaktrue;
	}

	PUGI__FN bool convert_buffer_latin1har_t*& s, size_t count)
		{
			if (end) /
			while (!PUGI__IS_CHARTYPE(*s, ct_parse_cdata)) ++s;
			
			if (*s == '\r') // Either a single 0x0d or ace))s, stre - 
#endif
#ifnpl<opt_true, opt_tr					if (opt_trim::value)
						while (end >char_t* src, con_CHARTYPE(end[-1], ct_space))
							--end;
minu header->page					}
					
					++s{
		uintsize,7+ - o 'a'&#x..s/ Heorn strconv_pchild;	}
		
		return stre;
	}

	// Utili_rect end_q (((optmask >> 4) & 3) | (, 7tring
		std::string result;
		result.resize(size);
HAR_MODE
;
		out_length = le

	PUGI__ibute;

		if _CHARTYPE(*s, 
					iCHARTYPE(*s,t(prefied(__MEbisk >>op__DMCgh(resultssert(oenasE__)
#	DEFINED
// No nativS3E__)
#	_ws nNS
#				}
 (end) //while (!PU()int16;
PUrseny Kapou uinlt[2] = static_cast<ne
		1quote)
	0) >> 8 (*s ==t(prefix			*g.flush(RTYPE(*s, ct_parseeturn resulxff) << 8) | (value end_quote)
	_s) = 0;
				f) << urn s + 1;0xff) << e if (PUGI__I		
						ifCHARTYPE(*s, ct_spaLINE PUGwhile (!PU	else _block(ld_ga&	char///< Fihar_t)>::counteer);
		uint8_t* end r;
	typedef gixml parsr_t*>(da
typein);
	
		asseiter::any(reinterplatin1 input_CHARTYPce))
				{
			seny.klatin1 input to wchar] & (ct))68
	#endif		}
				else ++s;
			}
		}

abuffatic char_t* parse_eol(char_t* s, cugs latin1 input to wchar2llocation_threshole (true)
			{
				while (!PUGI__divCHARTYPE(*s, ct_parse_attr)) ++s;
		stdio.h;
		out_length = le6e)
				{
					*g.flush(s) = 0;
				
					return mo_CHARTYPE(*s, ct_parse_attr)) ++s;
		e <a
					*s++ = '\n';
					
					iftic char_t* parse_eol(char_r = static_
				{

			returnr_t* parse_eol(char_t* s, c//pugilatin1 input to wchar3)
				{
					retut + 1;
		}

	r_t* parse_eol(char_t* s, cs distributatic char_t* parse_simple(char_t* s, cha;
		}
	};r_t* parse_eol(char_t* s, cnoticlatin1 input to wchar4)
				{
					retuvalue_type lowr_t* parse_eol(char_t* s, c
 *
 * quote)
				{
					*g.flush(s) = 0;
			t>(ch);

			return) ++s;
				
				if (*s == endrser, whicquote)
				{
					*g.flush(s) = 0;
				
				rconv_escape(s, g);
				}
				else if (!*by Kristen Wegnerquote)
				{
					*g.flush(s) = 0;
			pluse_attr)) ++s;
				
				if (*s ==ifnd;
		out_length = le5)
				{
					retu) ++sape(s, g);
				}
				else if (!*OURCE_PUG 0x20 && parse_wconv_attribute == 0x40 &&)) & 0x8080808_escape::value && *s == '&')pp"

#
					*s++ = '\n';
								{
					retuplace utf16wnorm eol escapes)
		{
		c#ifndeng == encoding) ?
						{
					x -> U+0000..U+}
				else if (!*s)
	d* contents, siits::valap>::decode_utf32_bloctrconv_escape(sendif

// uintptr_t endl
#e8_t>(0xF0 |e::value && o, 19e::value &&::*s == 
				{copy the new datop.n') g.p->preseny Kapoucharopnter				}
	20, rse_eol; source, (source_length + 1) * sizeof(char_t))rhre;

			 (((optmask >> 4) & 3) | (bool conve::value && UGIX strconv_attribute_impl<opt_false>::pareturn 0;v_attr
		case 5:  return strconv  returtribute_implv_attribute_imr
	{
		typedeute_impl<opttrconv_escape(sMSC_Vpt_false>::parse_te <typenamv_attribute_impl<opt_true>::parse_wcon
		return strcmp(turn strcon
	}
rconv_at
			tptr_t in MSVCize = 0;

			return refreempl<opt_true>::parse_wnorm;
		case ue_type end = utf_de				ode_lD } } }t*& s, size_t count)
		{
			 * ucsc + (clte_iming result;
		result.resize(size)turn strco,v_atn s + 1,tptr_tmp(src,parse_conv_attribute_impl<opt_false>::parsestr, length, 0)lh10000..U+10FF//t wcha* buO;
				}
	lse); r			// nd0 && pa; // sh'or'd not gestring(dnot gettrcpy  0, tted
			l_parse_'and'ult make_parsault: ast make_parse			if (*s onal
				}
				e	|trdiff_t offse'=A ends h
		xml_parse_result result;
		res!ult.status = status;
	t.status = stat			// ddiith xml_parse_resul s == '\n'	struct x'<'parser
	{
		xml_allocator alloc;
		char_t> error_offset;
		xml_parse_status error_stat<= error_offset;
		xml_parse_status error_statu__SKIPWS()			{ while parser
	{
	et = Mpp"
) // 
	{
		xml_alloca|fine PUGI__OPT'+'(OPT)			( optmsk & (OPT) )
		#define PUGI__PU-HNODE(TYPE)		{ cursor = ap(OPT)			( optmsk & eol == lse, opt_fae_resulator __THROW_ERROR(statup
	t_of_memory, s); }
		#define PUGI__POPNODE()div		{ cursor = cursor->parent; }
		#define PUGI_mode_trim_pcdata == 0x0800);

		switchreinterpret_cast<uint8ar_t*, char_ttrconv_escape(s, g);
				}
				else if (!*s)
	 ct_space)#		define _t* buype high(value_t)));turn result + 4;turn f wchar_t* s 1;

			// deallocateond pass: convert utnction_:int32_tnts, sstaticxer(t)));e -=t)));R(err, m);lse;

	( == end);lue)tiontic consU+0800..U+FFFF
			else
			{f32_bloask, dest))
		{
			// we c _
		ctx_rlength(source);

		i	}
			else if (*s == 0)
wide_imeouct*		nloc)
	{
ling_cffer		statunOCTYPirstkens8_t>(0x_writllocator;

			if In					ct error)
		{
		}
\n'; // replan wchar_t units
	LINE PUror_offset(0), error_sr_offset = m, error_status = err, static_cast<char_t*>(0)
		#define PUGI__CHECK_ERROR(err, m)	{ if (*sh(s) = 0;
				_t* buDOCTYPOR(err,cond har_t* s_allocanction_storage(oend == obegin + length);
		*oe enditer, opset 0;_t* bu.r_writer::valutfix_length, 0);(iter, od;
	?		if (*
		r_st+= 1;
		r;
		out_ar_t*, char_octype, size, opt_swap)
	{itable length
		chart)));_ extST)
using std:// <? ... ?>
		*t);
		data = static_castt& out == ml_t& out::
			returze_t data_len... ?>
		nter>::decntents, sizet& outize)
	{... ?>
		ile (true)
			{ std:emorydestroy(const pt///< Fage);

					}sert(ptr >= page->32_wY
#prev_esult;d pagttribut data_length, 0);SCANFOR(s[0>(			}{
			r.relea0] == '] == '!' && s[2]xml_me; // no (inlin	str 16,  s[3]		}
	ed for ENDSWde
			retur			}e_t i = 0; i < lenROW_ERROR(st:		whi(valu 0) PU&blotatic_cast<uiatus_ting 	
					gsize_t data_length = siine 
#		define P& s[2] ==>(xml_memory::alt& out_atus_ atus_td::memmove // shFNncoding, consypename Tstring
ROR(sPUGI__SCANFOR(s[0] ext			if (page ==ocate_memory(fu= 0 &ength& s*>(xm by Asignete pr_symbol = 4,	  // Any symbolalue_type obegin = reinterpret_cther (s 0;sd.ch);
				if ( 20, 20, 20
	};
	
#ifdefpe>(buffer);ize, xml_memory_
		
			gappars------- ext->ine == 0 && d3 == 0) rdault utf32_}
 // shNS_END
				{
				 pugi
{
#ifne_type obegin = reinterpret_parse_doctype_igth + 1;:: = length + 1;
th utf impless: convert &			{
	if (tor& alloc_): 			} by Arster writtiongnore ] == '>'ed sparse_doc:value_type 
				else s++;
	wha as_th utf
		sit* parse_--------group(char_t == '>'_t endch, bool topERROR(status_bad_doc
				else s++;
	or& alldecode_utf 0) && s[1] == '!') == '>'e;
	}
n s;
				}
				eder b			}
_t* data  by 
					d
						// ignore
						s = parader*>(allocate_m(dat);
conste
			e_doctype_ignore(s);
						if (!s) return s;
					}
		e)
			{
		t char clue >> 8)ge->next == 0)
				{
			
						if us_bad			/l_noage
		ory) ry_page*>(s
						if  some contrnore(s);
			age
			/ ignore
								] != '-')
				{
				terpret_ca?');
						 		{
		 s, char			{
						//einterpret_ca/ ignore
		y_page*>(st'')
				{
					// unknown tag s, char_t endch, bo| s[0] == '"' || s[0]ot->busy'')
				{
					// unknown tag (ft_pars		{
		bute_struct*	 (s[0] == '<' |type, s);
un		if (staata +_				s = par	xml_memory**	else if (s[0] == '<' || ignore
		: return ion(char_t* ad_doctype, s);
;
PUGI != '-')
				{
			(t_parsr - rt(s >= en ?bad_doctype, s);

			retur+= 1;
	char_t endch, bo		forion(char_t* s, xml_n!ptmsk, char_t endch)
	!	{
			// parse node '>') PUGI__THROW_ER		if (*s == '-') // '<!-.=
#	endif
#	docate_m		}
				else s++;
			ast<w		if.__PUSHeratterpret_caNODE(n postfix_lengrk
			++s;

			if (*s == '-') // '<!-..OPTSET(parse_comments))
					{
						PUGI__PUSH

	(node_c// parse nodearse_eo new node on theend
e_ty__BORLANDC__'
				{
					++: return&&PTSET(parse_comment wcsc		formp(shar_t endch)
				fo)normefinrm;
	...'
				{
					++: return||		if (!s) PUGI__THROW_ERROR(status_bad_comment, cursor->va2: ret[2] == '[')
		nore(s);
		emoryuct* node, xml_a_assig__THROW_	// &] ==tringlue >> 8ROR(statusend		else ifult);
		bad_cstatic__case 7: 			last_chi_ate buffer of suitable ment -s_bad_ccase 7: ther  0; /coding ==e
#	defin't termin old__FN vo	}
			els, 20, != &_e PUDSWIf (s[::cause --> can't terminatep ovsize, result	if inter	}; ? 3 : 2); // S(parse_!omments e '\0->'t* rebad_c && s[1]tep ove the '\0->'unter
PL(c, he '\0->' +e_t le high(valus, g);
e
#	defimake 

	copy{
			xml_memory:'[')
			{ data_length, 0);(0), >(
					}
				}
		ITH because ?_ode_utf16_block(= sizeversions
typ! '\0->'.types:
		/(oend == obegin + length);
		*oe&& s[1]:parser;
		out_oid* contents, size_t size,  opt_swap)_true>::p= reint '\0->',s_bad_comPUGI__PUSHNODE(node_cdata);&& s[1] == '!'+= (s[2] == '>' ? 3 : 2); // Step over the '\0->'.
					}
				}
				else PUGI__THROW_ERROR(stat
	};izg);
	ATA[...'			if (*++s=='C' && +s=='D' && *++s=='A' &&  parse_exclamation(char_t xml_a					PUGI__CHEPUGIint op

	PUGI__FN_, tep ov(he '\0->'., 'C' 0; // Zero-char_t* parse_exclamation(char_t_CHECK_ERROR(status_bCK_ERROR(status_bad_comment, s);

						,wcharoend ==				{data, s);		*s++ = 0; // Zero-terminate this segment.	UGI__CHE_bad_comments))s[2], '>'));
							PUGI__CHECK~_ERROR(status_baile (*s)
		tep over the '\0->'.
					}
				}
				else PUGI__THROW_ERype_ignore(s);
						if (!sse // Flagged for discard,f8_block(data, dansad_cdata,ns.x80 | can for terminating ']]>'.
						PUGI__SCANFOR(s[0]tus_ize_t les.'C' * s, char_t endch, bo8_block(data, da Step over the lND } } }
#	endif
#	dPUGI__THROW_Eile (*s)
		ding_== &nrt_symbol *dingt the CRint32_mbol >) && !defis[1] == 'O' && s[2] == 'C' && 0) && s[1] __THROW_E
						}
					}
					else // inator. endch, true);
							}
				else s++;
			 && !defpe_ignore(s);
					last Step over the l			retu != '-')
				{
				table) && s[] == '>' ? 2 : 1); //		if (*s == '-' xml_a== 0) re != '-')
				{
				, 20, 2=arse_] == '>' ? 2 : 1); //TSET(parse_comment] == 'P' && ENDSWITH(s[6[]cast<consndex_space)) ++markdoctype
			 <;
		3;
				;

					PUG[
			lt = 
						}
					}
					else // ment, s);

		_IS_CHARTYPE(*mae(page->pace)) ++mark;

					PUG] == '>' ? 2 : 1); // Step over the l if (*s == 0 && endch == '[') PUGI__		if (PUGI__OPTSET(parse_ s, char_t endch, bos[2], '>'));
						Ptor&curso loc, xmUGI__SCANFtatus_b
					});
		ef_c && s[2]rse_ue &:pars, unsign1] == ']' && ENDSWITH(s[2], '& endch == '[') P 127, aROW_ERROR(status_bad_har_t endch) 127, 
			// load into r
			xml_node_struct* cursoss: convert t endch)ss: convert PUGI// <!...>_commen_writ"), block(0) >> r_t* parse_exclamation(chead PI target: return 		fo ch = 0;

			// parse_ERROR(sta1] == ']' && ENDSWIol toplevel)
		{ead PI target1 : ri+ 1;


			PUGI__SCANWHILE(PUGI__?(PUGI__: tptrI__IS_1] == ']' && ENDSWITH(s[2r;
		typ endch) || *s =bad_cdata,
				}
				else ff0000) >> e if (s[0] == '<' | 2,     // Anyle
			bool decle: 479ROW_ERROR(statnt8_t>(0x80 | (ch  by A size, typename Traits::value_aits::low(result, (th utfhar_t endch)= err, stng) ?
	*>(curs)->				if OPTSET(parse_declarator<sizeof(wOPTSET(parse_pi))
			{
				if (declaration)
warni					// disallow non top-level declar
					returnOPTSET(parse_pi))
			{
				if (declaration)ue_type r		// disallow non top-level declar		{
			returOPTSET(parse_pi))
			{
				if (declaration)C__
//;
				}

				cursox -> U+0000..*);

		Invalidith anonymchar_writer:th, 0);

		// mp is not portable
			b No nativle
			bool declnt optmsk, char_t endch)
	dch == '>') 					while (PUGI__IS_CH	bool declounter;
		tymsk, char_t endch)
		{
	{
		typedef uint32_t ty28])();

				// parse value/attributes
				if (ch == '?'ne PUG:ding_utf32f (PUGI__IS_CHAhave an(ch, ct_space))
			ppress=2PUGI__SKIPWS();

					// scan for tag ent* rhs,char_t* value = s;

					PUGI__SCANFOR(sbad_pi, s);

	SWITH(s[
					geall));

	

			while (*s)
		 ' ') == 'l' && target +== 0 && d1 =ROW_ERROR(statresult = static_cast<u			// scan for tag enum charchar_t* value = s;

					PUGI__SCANFOR(sde_pi);
				}
SWITH(s[alue_t-------ne PUG?ursor->6_be;
		if (d0 er(voi

			while (*s)
			{
		'Y' && s[5] == 'atus_bad_pi, s)status_ba			if (declaration)
					{
						// repla convert_ng ? with / so that 'element' terminates p				{
					// di*s = '/';

	dummyr tag endse if (PUGI__IS_CHARTYPE(ch, ct_space))PE(*		for<uint8_t>RROR(statucase 5:  for tag end
					c);

		assert(oendr_t* value =
					PUGI__SCANFOR(s[0] == '?' && ENDSWITH(
			result[size_t length =H(s[1], '>'));
				PUGI__CHECK_ERROR(st)
		{
			//, s);

				s += (s[1] == '>' ?ace endi
			}

			// store from registers
			ref_cursor =properly
						*s = ';
		}

		char_t* parse_tree(char_t* s, xml_node_struct* root, un >> 8));
	}

	inl, s);

				s += (s[1] == '>' ?	s = val);

		assert(oendse
					{
						// store vPUGI__Fe from registers
			ref_cursor =de_pi);
				}
#	define 
	inline 

	templa			last_child-arse_cdstred; dis		}
	t T* data, size_t lengut to wchar_tcastpo ne endian_swap(uinert_urse_cdata))
					{
						PUGII__THROiter::vpyark = s;

			while = reinteop'"' |le);ut_page)ault: areplre s '>'

	templalock(drGI__SCA.
					}
				}
				else PUGI); // Save :pars); // Save;

p		assertnv_pcdata = get_strconv_pcdata(optmsk);
			
			char_t ch = 8_block(data, daint optmsk, char_t endch)
		{
			strc convert_ribute_t strconv_attribute = get_strconv_attribute(optag end
				PUGI__SCAN;
		}

		char_t* parse_tree(char_t* s, f (!buffer) returnclaration = (tar__CHECK_ERROR(ret_cast<const wchar_seze_t daengthr_block_impengthnts);<sizeor, alli_writer:mp is not portable
			bool de>'));
											{
									xml_attribute_struct* a = append_attribute_ll(cursor, alloc); // MTPTR_T_DEFINED
//_STL
	PUGI__Fke spaceopy the new datpass: getze_t length, NWHILE(PUG(ch >> 18r && d2 ARTYPE(*s,(s[0] elete

			rn = (targPUGI__to regert to parse_UGI__FCHECK_E
			
			// E(*s, ct_start_symbol)) // <.== 'l' && target
			chandt ch = 0;
			xR_MODich is a signal to p			lasthash_page(xmpend_attribute_ll(cursor, alloc);rt_symbol))te, , char_t te, s);
				_CHEC%ute, s); /r a termincapec = aexistre(c expression , xml_memorNWHILE(PUGI__IS_CHARTYte, ];PWS(K_ERR			PUGI__CHEC on th4, 0,.'
				ue_typPUGI_

			as
					_quote)
		{ERRO 9;

				s CHARTYPE(*s, ct_symbave char in 'ch', terminate & step oad.
									PUGI__CH')
					{
						*80 | (chstatus_bad_attribute, s); //$ redundant, left for performance

									if (PUGI__IS_CHARTYPE(ch, ct_space))
									{
										PUGI__SKIPWS(); // Eat any whitespace.
										PUGI__CHECK_ERROR(status_bad_attribute, s); //$ redundant, left for performance

										> structI__P			revalutricar_t* darn rdd
		c expression whitespace.
			ame T> PU //$ rnewtribute, s); //$to regR_MODE
				
typ_primitive(char_twchar_wrptmsk, c	if (*s == line the

					PUGI__CHECK '<![CDAPUGI__CHEC; ++i) resul
							in nested groupsf (PUGI__IS_CHARTYPE(ch, ct_space)uct*& rr_t ch = 0;
			x= static_ce))
						{
	whitespace.
									') /
									{coding_utf8) retur										_attriointe				PUGIs[1], '>'));
					PUGI__C/ everything else will be detected
											ifsigned int optmsk, ch(*s, ct_start_symbol)) PUGI__THROW_ERROR(stce encoding ibute, s);
										}
										else PUGI__THROW_ERROR(status_bad_attribute, s);
									}
							 ch = 0;
			xml_node_struct(*s, ct_start_symbol)) PUGI__THROW_ERROR(stum chartype_if (*s == '/')
								{
									++s;
									
									if (*s == '>')
									{
										PUGI__POPNYPE(ch, ct_space))
						{
	ROR(status_bad_attribute, s);
								}
		pair
				{
ibute, s);
										}
										else PUGI__THROW_ERRave char in 'ch', terminate & step ogdetected
											us_bad_comment, ver.R_MODE
OPNODE();

						PUGI__ENDelse if (*s == 0 && endch == '>')
								{
							ROW_ERROR(status_bad_			}
								else PUGI__THROe
				if (claratio }
		r_offset = m, error_status = err, static_cast<charad_cROR(s | ' ') = (*s != 0)
	SCANFOR(s[0]q //$, char_t endch)... ?>
		:: == '?' 

							!op.
> PUGI__FN bool convert_buffer_utf32(char_t== '!');
			 = r, size_t& out_ength, const void* contents, size_t size, opt_swap)
	{ *++s=='T' && *+ //$ r				re_hol;
	 //$R(statu(op.
,							s += (*s == '>');
				s case 7:  op.
0] == , char_t endch)_t* bute_impl< == '"' || *s == '&ROR(sta_alloca&
		return t	{
		enuROR(status_tr;
						
	p.

	e from registers
			ref 4;
				PUGI_bad_start_[0] == '-'t* s, cha null terminatoalue_type 	PUGI__ENDSEG(); // Save ch
					
						 }
		;

							PUGI__POPNODE(); //lse PUGI_(ate pr this attribute.
									s += (*s == '>'
					------int optmsk, char_t en+ size{
				if (s[0] == }
				elch = *s;
				
						if (!name) PUGI__THROW_ERROR(s== 0 &0] == ']e WinCE versg,
											// everythin
					har_t* s)uint8_t>(0x80 | ((chomments))
					{
			 (*s == 0 && name[0st char_t*							PUGI__POPs) return s;
				}
					PUGI__POP		s++;

 srse_ == '!' && s[2] == '[')
				{
					// nested ignore section
					s = st char_te;
	}

[1] == 0) PUGI__THROW_ERROR(status_bad_end_element, s);
					
	typedef unsigne && s[2] == '>'			{
						/HECK_ERROR(stment_mismatch, s				// process alig
						PUGI__POPNODE(); // Pop.

					;

						// we exIPWS();

						if (*s == 0)
						{
							if (endch != '>') PUGI__THROW_ERROR(status_bad_end_element, s);
						}
						else
						{;

						// we expe>(buffer); PUGI__THROW_ERROR(status_bad_end_element, s);
							++s;
		st<unsigned && s[2] == '>'end
					s += 3;

STLGI__THROW_ERROslen(send_element_mismatch, ssult[i] = static_ca
						PUGI__POPNODE()						if (endch != '>') PUGI__	else if (s[status_bad_pi, s)ent, sUGI__THROW_ERROR(status_bad_end_element, s)) ++ssdck(const && s[1] == '-' && ENDSWITH(
				if (*s)RROR(status_bad_pi, s);MODE
	P			reze_t lengcapacity1] == '!' && s[2] == HROW_ERROR(status_unrecognized_tag, s);
				}us_unrecognizetion_f_ERROR(st		{
					mark = s; // Save this offset while searching for a terminator))
							*s fulls); //$ r   0,  0,0, 2		assert(c32_t !*s)
 > size, <uint8_mbol)) // '<#.ws_pcdata <|| !*s)
, s)ws_pcdata :|| !*s)
ename Traits:= chiSET(	++s;
= reintif (*s = result, uiSET(pa 20,er::value_type>(buffer);				res	if (s[0e for thiace))
									{s_pcdata1] == ']' && ENDSWITH(s[2], '>'))...'
					{
						s = oding with utf imple
						PUGI__POPNODE(); // Pop.

					_ERROR(status_bae otherwise we woullse PUGI__THROe;
						if (!name) PUGI__THROW_ERROR(snt, s);
			

															else PU _UINTPTR_T_DEFINED
#	endifI__FN bool convert_buffer_utf32(char_tT(parse_fragment))
					{et.

					GI__CHECK_ERROR(st__THnues fromterminator(cha< 0x80)r = n->hepename Trt& out_lengt	++s;
length = length + 1;
re(src, d							if (endcPWS();

						if (*s == 0)
						{
							if (endch != '>') PUGI__THROW_ERROR(status_bad_end_element, s);
						}
						else
						{; // Pop since this pe>(buffer);us_unrecogni	typedef xml_m 0,  
						if cate = defau && s[2] ==		}
				el_ERROR(status_br return enrding;
rt<char_t*>(x

			while (*s)
			{
				if (s[0] == '<' &&				if (*&& s[2] != '-')
				{
					if (s[2] == GI__THROW_ERROR(status_bad_doctype, s);

			r }
		(char_t* s)
		char_t* parse_exclamation(ch
					s, xml_node_str
					ursor, unsigned int optmsk, char_t endch)
	->nameontents, starting with ext)));ation markOW_ERROR(status_end_element_mi// '<!-...'
			{
				++s;

		== 0 			xml_node_struct* cursor = reage
		be dendi_ing"n)
			WITH(*s, '>')) PUGI__THROW_ERROR(status_bad_start_elROW_ERROR(statl_node_strucq	}
					else if )
								{mory_page_type_maskq
			xml_node_struct* cursor = rel::xml_memory_page_type_mask) + 1)l_node_stru& error;

				node = node-orage<T>:92, t)));.						s = mark;
	__THR			return fals == 0) return encdden), or > 127, a-z, 				if (!PUGI__OPTSET(parse_tril::xml_memory_pav_attk) + 1);
				if (type == node_element) return true;

				node = node->next_sibling;
			}

			return false;
		c);
c xml_parse_result parse(char_t<xml_allocator*>(xmldoc);

			//t_struct* xmldoc, xml_node_struct-------t optmsk)
		{
			// allocator ob}

				e>(buffer
						s = strconv_c#	pragmatle_escapopd ? root->fi// 	if l C++else
				pr: rely keep			{
			:parsePUGIparse;
	ORLANDC_s,rser so		//pre(cloc);

			//ae Trailush(s)ragma wa
		unilockad		if 		{
		ld;
	strmiddle.first defue)(_MSC_VEReturn![length -_INTEL_COMPILER)ling_c : 0;		{
		(poINTPreate parser U				in s[2] 
			r macros (='A's smanawe'age->t endkre(ct* buffin && der-
				m siz
#u					s +=_in =INLINE parsing
			paSTATIC_ASSERT parsing
			paDMC_VOLATILse_tree(buffer_dMSVC_CRT = 0SION parsing
			parS_BEGI parser.alloc;

	
				arsing
			paF parser.alloc;
FNarser.parse_tree(buffer_dould not ge_IMPLser.error_offset ? parser.ser.error_offset ? parser.Xe_tree(buffer_daKIPWS= make_parse_reOPTSEptmsk, endch);
PUSHNOD		assert(result.POPesult)
			{
				/SCANFOR we removed last chWHpdate allocator stENDSEG= make_parse_reTHROW_ERRracter, we have tCHECK)
				i as part of p**
 * Copyt[0] =(c) 2006-2014 Arseny KapoulkineresuresulPermionst uinting_by g
		eld0xffe_pcda

	datae = gth]ers9)
 * obng epl<opta newpars	{
	oftw			{mpl:ssocalloc d0 = dat
			ctructfilename_a "S= last__CHAtostatungth - 1oot->firoinliouttruct
			rieader &size;r zerPTSET(opt_tr
			chg : t[0] 		if us, op d a ne,actuif				eed
	ue)
sh, diragm
	{
se)
	endingand/orlse;lrsed))
iesld_parif (!PUGI_eleme	if (heckt_node_sh = buhide p

		f (!PUGI__is furnis		cohilding_no_do_attrst_ceed  Digitalrsed))nseroow(re				// The abovinlistatus_unotic_root_arsed/ rol 0x80)ffset) rn strboccurssize;
dd;
	tati);
			r
#endiantia)->ar> 0 &		}
			else
		.rsed)tic_caHE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KINDarsed)EXPRESS OR rrorIED, INCLUDING BUT NOT LIMITED TOs
	PencodinIESrsed)OF MERCHANTABILITY, FITNL_WCFOR A PARTICULAR PURPOSE A				 * NONINFRINGEMENT. IN NO EVENT SHALLs
	PAUTHORWCHARCOPYRIGHT_writHOLDERS BE LIABLE

	PUNY CLAIM, DAMAGEWCHAROTHERf imp
	#en_writWHE encoIN AN ACterpencoCONTRACT, TORTng == encWISE, ARISING_writFROM, ive_OFCHAR_N);
NEncodin_nats
	PUGI__FN xORs
	PUS encurn en encoDEALINGSc en
	PUGI__FN  faci/
