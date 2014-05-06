#ifndef __febird_radix_sort__
#define __febird_radix_sort__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
# pragma warning(disable: 4127)
#endif

#include <stddef.h>
#include <assert.h>
#include <vector>
#include "config.hpp"

namespace febird {

	class FEBIRD_DLL_EXPORT radix_sorter
	{
	public:
		typedef int (*codefun_t)(int ch_, const void* codetab_as_locale);

		struct radix_sort_elem
		{
			void* obj;
			const unsigned char* key;
			ptrdiff_t link;
			ptrdiff_t addr;
		};
	private:
		std::vector<radix_sort_elem> ov;
		const unsigned char* codetab;
		codefun_t codefun;
		ptrdiff_t max_key_len;
		const int ValueSize;
		const int CharSize;
		const int radix;

	public:
		/**
		 * @param first sequence start
		 * @param last  sequence end
		 * @param getpn requires:
		 * 			integral getpn.size(*first), return size in bytes!
		 * 			const unsigned char* getpn.data(*first)
		 * @param codetab if NOT NULL, sorted by lexical codetab[getpn.data(..)[*]]
		 * @param radix	  radix of alpha set
		 * @param codefun if NOT NULL, code = codefun(getpn.data(..)[*], codetab_as_locale)
		 */
		template<class RandIter, class GetPn, class CharType>
		radix_sorter(RandIter first, RandIter last, GetPn getpn, const CharType* codetab
					, ptrdiff_t radix = 256 // don't default large (1 << sizeof(CharType)*8)
					, codefun_t codefun = NULL
					)
	       		: radix(radix)
				, ov(std::distance(first, last))
				, codetab((const unsigned char*)codetab)
				, ValueSize(sizeof(typename std::iterator_traits<RandIter>::value_type))
				, CharSize(sizeof(CharType))
		{
			// when sizeof(CharType) == 4, must provide codetab
			// assert(sizeof(CharType) == 4 && codetab != NULL || sizeof(CharType) <= 2);
			assert(sizeof(CharType) == 4 || sizeof(CharType) <= 2);
			ptrdiff_t j = 0;
			max_key_len = 0;
			for (RandIter iter = first; iter != last; ++iter, ++j)
			{
				ptrdiff_t keylen = getpn.size(*iter);
				radix_sort_elem& e = ov[j];
				e.obj = &*iter;
				e.key = getpn.data(*iter);
				e.link = j + 1; // link to next
				e.addr = keylen;
				assert(keylen % CharSize == 0);
				if (max_key_len < keylen)
					max_key_len = keylen;
			}
			this->codefun = codefun;
			max_key_len /= sizeof(CharType);
		}

		void sort();

		static const unsigned char* get_case_table();
	}; // radix_sorter

	// 先排序最长的 Key, 后排序较短的 Key
	//
	template<class RandIter, class GetPn>
	void radix_sort(RandIter first, RandIter last, GetPn getpn, const unsigned char* codetab = NULL, ptrdiff_t radix = 256, radix_sorter::codefun_t codefun = NULL)
	{
		radix_sorter sorter(first, last, getpn, codetab, radix, codefun);
		sorter.sort();
	}

} // namespace febird

#endif

