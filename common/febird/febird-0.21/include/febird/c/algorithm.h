#ifndef __febird_c_algorithm_h__
#define __febird_c_algorithm_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
# pragma warning(disable: 4127)
#endif

#include "config.h"
#include <stdarg.h>
#include <stddef.h>

#include "field_type.h"

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

typedef void (*void_handler)();

typedef void (*array_handler)(char* _First, ptrdiff_t count, ptrdiff_t field_offset);

typedef void (*array_handler_v)(char* _First, ptrdiff_t count, ptrdiff_t field_offset, va_list val);

typedef ptrdiff_t  (*array_handler_r1k)(const char* _First, ptrdiff_t count, ptrdiff_t elem_size, ptrdiff_t field_offset, va_list key);
typedef ptrdiff_t  (*array_handler_r2k)(const char* _First, ptrdiff_t count, ptrdiff_t elem_size, ptrdiff_t field_offset, ptrdiff_t* res, va_list key);

//! when _First[_IdxUpdated] was updated, call this function to keep heap form
typedef void (*array_update_heap)(char* _First, ptrdiff_t _IdxUpdated, ptrdiff_t _Bottom, ptrdiff_t field_offset);

typedef struct febird_arraycb
{
	array_handler sort;

	array_handler sort_heap;
	array_handler make_heap;
	array_handler push_heap;
	array_handler  pop_heap;
	array_update_heap update_heap;

	array_handler_r1k binary_search;
	array_handler_r1k lower_bound;
	array_handler_r1k upper_bound;
	array_handler_r2k equal_range;

	const char*  type_str;
	ptrdiff_t    elem_size;
	ptrdiff_t    field_offset;
	int          is_byptr;
	field_type_t field_type;
	field_type_t field_type_idx;
} febird_arraycb;

void FEBIRD_DLL_EXPORT febird_arraycb_init(febird_arraycb* self, ptrdiff_t elem_size, ptrdiff_t field_offset, field_type_t field_type, int is_byptr);

#define febird_offsetp(ptr, field_name) ((char*)&((ptr)->field_name) - (char*)(ptr))

#define febird_arraycb_init_m(self, first, field_type) \
		febird_arraycb_init(self, sizeof(*(first)), febird_offsetp(first, field_name), field_type, false)

#define febird_arraycb_init_t(self, elem_type, field_type) \
		febird_arraycb_init(self, sizeof(elem_type), (ptrdiff_t)&((elem_type*)0)->field_name, field_type, false)

#define febird_arraycb_handle(acb, fun, first, count) (acb).fun(first, count, (acb).field_offset)

#define febird_acb_sort(     acb, first, count) febird_arraycb_handle(acb, sort     , first, count)
#define febird_acb_sort_heap(acb, first, count) febird_arraycb_handle(acb, sort_heap, first, count)
#define febird_acb_make_heap(acb, first, count) febird_arraycb_handle(acb, make_heap, first, count)
#define febird_acb_push_heap(acb, first, count) febird_arraycb_handle(acb, push_heap, first, count)
#define febird_acb_pop_heap( acb, first, count) febird_arraycb_handle(acb,  pop_heap, first, count)

//! @{
//! va_arg ... is Key
ptrdiff_t FEBIRD_DLL_EXPORT febird_acb_binary_search(const febird_arraycb* acb, const void* first, ptrdiff_t count, ...);
ptrdiff_t FEBIRD_DLL_EXPORT febird_acb_lower_bound(const febird_arraycb* acb, const void* first, ptrdiff_t count, ...);
ptrdiff_t FEBIRD_DLL_EXPORT febird_acb_upper_bound(const febird_arraycb* acb, const void* first, ptrdiff_t count, ...);
ptrdiff_t FEBIRD_DLL_EXPORT febird_acb_equal_range(const febird_arraycb* acb, const void* first, ptrdiff_t count, ptrdiff_t* upp, ...);
//@}

//////////////////////////////////////////////////////////////////////////


void FEBIRD_DLL_EXPORT febird_sort(void* first, ptrdiff_t count, ptrdiff_t elem_size, ptrdiff_t field_offset, field_type_t field_type);

void FEBIRD_DLL_EXPORT febird_pop_heap(void* first, ptrdiff_t count, ptrdiff_t elem_size, ptrdiff_t field_offset, field_type_t field_type);
void FEBIRD_DLL_EXPORT febird_push_heap(void* first, ptrdiff_t count, ptrdiff_t elem_size, ptrdiff_t field_offset, field_type_t field_type);
void FEBIRD_DLL_EXPORT febird_make_heap(void* first, ptrdiff_t count, ptrdiff_t elem_size, ptrdiff_t field_offset, field_type_t field_type);
void FEBIRD_DLL_EXPORT febird_sort_heap(void* first, ptrdiff_t count, ptrdiff_t elem_size, ptrdiff_t field_offset, field_type_t field_type);

ptrdiff_t FEBIRD_DLL_EXPORT febird_lower_bound(const void* first, ptrdiff_t count, ptrdiff_t elem_size, ptrdiff_t field_offset, field_type_t field_type, ...);
ptrdiff_t FEBIRD_DLL_EXPORT febird_upper_bound(const void* first, ptrdiff_t count, ptrdiff_t elem_size, ptrdiff_t field_offset, field_type_t field_type, ...);
ptrdiff_t FEBIRD_DLL_EXPORT febird_equal_range(const void* first, ptrdiff_t count, ptrdiff_t elem_size, ptrdiff_t field_offset, field_type_t field_type, ptrdiff_t* upper, ...);

void FEBIRD_DLL_EXPORT febird_sort_p(void* first, ptrdiff_t count, ptrdiff_t field_offset, field_type_t field_type);

void FEBIRD_DLL_EXPORT febird_pop_heap_p(void* first, ptrdiff_t count, ptrdiff_t field_offset, field_type_t field_type);
void FEBIRD_DLL_EXPORT febird_push_heap_p(void* first, ptrdiff_t count, ptrdiff_t field_offset, field_type_t field_type);
void FEBIRD_DLL_EXPORT febird_make_heap_p(void* first, ptrdiff_t count, ptrdiff_t field_offset, field_type_t field_type);
void FEBIRD_DLL_EXPORT febird_sort_heap_p(void* first, ptrdiff_t count, ptrdiff_t field_offset, field_type_t field_type);

ptrdiff_t FEBIRD_DLL_EXPORT febird_lower_bound_p(const void* first, ptrdiff_t count, ptrdiff_t field_offset, field_type_t field_type, ...);
ptrdiff_t FEBIRD_DLL_EXPORT febird_upper_bound_p(const void* first, ptrdiff_t count, ptrdiff_t field_offset, field_type_t field_type, ...);
ptrdiff_t FEBIRD_DLL_EXPORT febird_equal_range_p(const void* first, ptrdiff_t count, ptrdiff_t field_offset, field_type_t field_type, ptrdiff_t* upper, ...);


//---------------------------------------------------------------------------------------
#define febird_sort_field_c(first, count, field_name, field_type) \
	febird_sort(first, count, sizeof(*(first)),	febird_offsetp(first, field_name), field_type)

#define febird_sort_value_c(first, count, field_type) \
	febird_sort(first, count, sizeof(*(first)), 0, field_type)

//---------------------------------------------------------------------------------------
#define febird_sort_field_pc(first, count, field_name, field_type) \
	febird_sort_p(first, count, febird_offsetp(*first, field_name), field_type)

#define febird_sort_value_pc(first, count, field_type) febird_sort_p(first, count, 0, field_type)

//---------------------------------------------------------------------------------------
#define febird_push_heap_field_c(first, count, field_name, field_type) \
	febird_push_heap(first, count, sizeof(*(first)), febird_offsetp(first, field_name), field_type)

#define febird_push_heap_value_c(first, count, field_type) \
	febird_push_heap(first, count, sizeof(*(first)), 0, field_type)

//---------------------------------------------------------------------------------------
#define febird_pop_heap_field_c(first, count, field_name, field_type) \
	febird_pop_heap(first, count, sizeof(*(first)), febird_offsetp(first, field_name), field_type)

#define febird_pop_heap_value_c(first, count, field_type) \
	febird_pop_heap(first, count, sizeof(*(first)), 0, field_type)

//---------------------------------------------------------------------------------------
#define febird_make_heap_field_c(first, count, field_name, field_type) \
	febird_make_heap(first, count, sizeof(*(first)), febird_offsetp(first, field_name), field_type)

#define febird_make_heap_value_c(first, count, field_type) \
	febird_make_heap(first, count, sizeof(*(first)), 0, field_type)

//---------------------------------------------------------------------------------------
#define febird_sort_heap_field_c(first, count, field_name, field_type) \
	febird_sort_heap(first, count, sizeof(*(first)), febird_offsetp(first, field_name), field_type)

#define febird_sort_heap_value_c(first, count, field_type) \
	febird_sort_heap(first, count, sizeof(*(first)), 0, field_type)

//---------------------------------------------------------------------------------------
#define febird_lower_bound_field_c(first, count, field_name, field_type, key) \
	febird_lower_bound(first, count, sizeof(*(first)), febird_offsetp(first, field_name), field_type, key)

#define febird_lower_bound_value_c(first, count, field_type, key) \
	febird_lower_bound(first, count, sizeof(*(first)), 0, field_type, key)

//---------------------------------------------------------------------------------------
#define febird_upper_bound_field_c(first, count, field_name, field_type, key) \
	febird_upper_bound(first, count, sizeof(*(first)), febird_offsetp(first, field_name), field_type, key)

#define febird_upper_bound_value_c(first, count, field_type, key) \
	febird_upper_bound(first, count, sizeof(*(first)), 0, field_type, key)

//---------------------------------------------------------------------------------------
#define febird_equal_range_field_c(first, count, field_name, field_type, upper, key) \
	febird_equal_range(first, count, sizeof(*(first)), febird_offsetp(first, field_name), field_type, upper, key)

#define febird_equal_range_value_c(first, count, field_type, upper, key) \
	febird_equal_range(first, count, sizeof(*(first)), 0, field_type, upper, key)

#ifdef __cplusplus
} // extern "C"

//---------------------------------------------------------------------------------------------
#define febird_sort_field(first, count, field_name) \
	febird_sort_field_c(first, count, field_name, febird_get_field_type(&(first)->field_name))

#define febird_sort_value(first, count) \
	febird_sort(first, count, sizeof(*(first)), 0, febird_get_field_type(first))

#define febird_sort_field_p(first, count, field_name) \
	febird_sort_field_pc(first, count, field_name, febird_get_field_type(&(*(first))->field_name))

#define febird_sort_value_p(first, count) \
	febird_sort_p(first, count, 0, febird_get_field_type(*(first)))

//---------------------------------------------------------------------------------------------
#define febird_push_heap_field(first, count, field_name) \
	febird_push_heap_field_c(first, count, field_name, febird_get_field_type(&(first)->field_name))

#define febird_push_heap_value(first, count) \
	febird_push_heap(first, count, sizeof(*(first)), 0, febird_get_field_type(first))

//---------------------------------------------------------------------------------------------
#define febird_pop_heap_field(first, count, field_name) \
	febird_pop_heap_field_c(first, count, field_name, febird_get_field_type(&(first)->field_name))

#define febird_pop_heap_value(first, count) \
	febird_pop_heap(first, count, sizeof(*(first)), 0, febird_get_field_type(first))

//---------------------------------------------------------------------------------------------
#define febird_make_heap_field(first, count, field_name) \
	febird_make_heap_field_c(first, count, field_name, febird_get_field_type(&(first)->field_name))

#define febird_make_heap_value(first, count) \
	febird_make_heap(first, count, sizeof(*(first)), 0, febird_get_field_type(first))

//---------------------------------------------------------------------------------------------
#define febird_sort_heap_field(first, count, field_name) \
	febird_sort_heap_field_c(first, count, field_name, febird_get_field_type(&(first)->field_name))

#define febird_sort_heap_value(first, count) \
	febird_sort_heap(first, count, sizeof(*(first)), 0, febird_get_field_type(first))

//---------------------------------------------------------------------------------------------
#define febird_lower_bound_field(first, count, field_name, key) \
	febird_lower_bound_field_c(first, count, field_name, febird_get_field_type(&(first)->field_name), key)

#define febird_lower_bound_value(first, count, key) \
	febird_lower_bound(first, count, sizeof(*(first)), 0, febird_get_field_type(first), &key)

//---------------------------------------------------------------------------------------------
#define febird_upper_bound_field(first, count, field_name, key) \
	febird_upper_bound_field_c(first, count, field_name, febird_get_field_type(&(first)->field_name), key)

#define febird_upper_bound_value(first, count, key) \
	febird_upper_bound(first, count, sizeof(*(first)), 0, febird_get_field_type(first), &key)

//---------------------------------------------------------------------------------------------
#define febird_equal_range_field(first, count, field_name, upper, key) \
	febird_equal_range_field_c(first, count, field_name, febird_get_field_type(&(first)->field_name), upper, key)

#define febird_equal_range_value(first, count, upper, key) \
	febird_equal_range(first, count, sizeof(*(first)), 0, febird_get_field_type(first), upper, key)



#endif // __cplusplus

#endif // __febird_c_algorithm_h__


