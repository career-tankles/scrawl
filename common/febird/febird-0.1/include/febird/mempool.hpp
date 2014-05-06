#ifndef __penglei_mempool_hpp__
#define __penglei_mempool_hpp__

#include "valvec.hpp"

/// mempool which alloc mem block identified by
/// integer offset(relative address), not pointers(absolute address)
/// integer offset could be 32bit even in 64bit hardware.
///
/// the returned offset is aligned to align_size, this allows 32bit
/// integer could address up to 4G*align_size memory
///
/// when memory exhausted, valvec can realloc memory without memcpy
/// @see valvec
class MemPool : private valvec<unsigned char> {
    typedef valvec<unsigned char> mem;
    struct link_t { // just for readable
        size_t next;
        explicit link_t(size_t l) : next(l) {}
    };
	struct HugeLink {
		size_t next;
		size_t size;
	};
    link_t* flarr; // free list array
    size_t  fllen; // free list length
	size_t  nFree; // number of free bytes
	size_t  hugelist;

    static const size_t list_tail = ~size_t(0);

public:
    static size_t align_to(size_t len) {
        return (len + align_size - 1) & ~size_t(align_size - 1);
    }
    enum { align_size = 4 };

    explicit MemPool(size_t maxBlockSize) {
        assert(maxBlockSize >= align_size);
        assert(maxBlockSize >= sizeof(HugeLink));
        maxBlockSize = align_to(maxBlockSize);
        fllen = maxBlockSize / align_size;
        flarr = (link_t*)malloc(sizeof(link_t) * fllen);
        if (NULL == flarr) {
            throw std::bad_alloc();
        }
		nFree = 0;
        std::uninitialized_fill_n(flarr, fllen, link_t(list_tail));
		hugelist = list_tail;
    }
    MemPool(const MemPool& y) : mem(y) {
        fllen = y.fllen;
        flarr = (link_t*)malloc(sizeof(link_t) * fllen);
        if (NULL == flarr) {
            throw std::bad_alloc();
        }
		nFree = y.nFree;
        memcpy(flarr, y.flarr, sizeof(link_t) * fllen);
		hugelist = y.hugelist;
    }
    MemPool& operator=(const MemPool& y) {
        if (&y == this)
            return *this;
        this->~MemPool();
        new(this)MemPool(y);
        return *this;
    }
    ~MemPool() {
        if (flarr)
            free(flarr);
        flarr = NULL;
    }

	using mem::data;
    using mem::size; // bring to public...
    using mem::shrink_to_fit;
    using mem::reserve;

	// keep flarr
    void clear() {
		hugelist = list_tail;
		nFree = 0;
        std::uninitialized_fill_n(flarr, fllen, link_t(list_tail));
		mem::clear();
	}

    void erase_all() {
		hugelist = list_tail;
		nFree = 0;
        std::uninitialized_fill_n(flarr, fllen, link_t(list_tail));
		mem::erase_all();
	}

    void resize_no_init(size_t newsize) {
        assert(newsize % align_size == 0);
		assert(newsize >= mem::size());
        mem::resize_no_init(newsize);
    }

    template<class U> const U& at(size_t pos) const {
        assert(pos < n);
    //  assert(pos + sizeof(U) < n);
        return *(U*)(p + pos);
    }
    template<class U> U& at(size_t pos) {
        assert(pos < n);
    //  assert(pos + sizeof(U) < n);
        return *(U*)(p + pos);
    }

    // param request must be aligned by align_size
    size_t alloc(size_t request) {
        assert(request % align_size == 0);
        assert(request > 0);
        size_t res = list_tail;
        if (request <= fllen * align_size) {
			size_t idx = request / align_size - 1;
			res = flarr[idx].next;
			if (list_tail != res) {
				assert(nFree >= request);
				flarr[idx] = at<link_t>(res);
				nFree -= request;
			}
		}
		else {
			res = hugelist;
			size_t* prev = &hugelist;
			while (list_tail != res) {
			   	HugeLink* h = (HugeLink*)(p + res);
				if (h->size >= request) {
					size_t remain = h->size - request;
					if (remain)
						sfree(res + request, remain);
					*prev = h->next;
					nFree -= request;
					break;
				}
				res = h->next;
				prev = &h->next;
			}
		}
		if (list_tail == res) {
			ensure_capacity(n + request);
			res = n;
			n += request;
		}
		return res;
	}

    size_t alloc3(size_t pos, size_t oldlen, size_t newlen) {
        assert(newlen % align_size == 0);
        assert(newlen > 0);
        assert(oldlen % align_size == 0);
        assert(oldlen > 0);
        assert(pos < n);
        assert(pos + oldlen <= n);
        if (pos + oldlen == n) {
            ensure_capacity(pos + newlen);
            n = pos + newlen;
            return pos;
        }
        else {
            size_t newpos = alloc(newlen);
            memcpy(p + newpos, p + pos, oldlen);
            sfree(pos, oldlen);
            return newpos;
        }
    }

    void sfree(size_t pos, size_t len) {
        assert(len % align_size == 0);
        assert(len > 0);
        assert(pos < n);
        assert(pos + len <= n);
        if (pos + len == n) {
            n = pos;
        }
	   	else if (len <= fllen * align_size) {
            size_t idx = len / align_size - 1;
            at<link_t>(pos) = flarr[idx];
            flarr[idx].next = pos;
			nFree += len;
        }
		else {
			HugeLink* h = (HugeLink*)(p + pos);
			h->next = hugelist;
			h->size = len;
			hugelist = pos;
			nFree += len;
		}
    }

	size_t free_size() const { return nFree; }

    void swap(MemPool& y) {
        mem::swap(y);
        std::swap(flarr, y.flarr);
        std::swap(fllen, y.fllen);
		std::swap(nFree, y.nFree);
		std::swap(hugelist, y.hugelist);
    }

	template<class DataIO>
	friend void DataIO_loadObject(DataIO& dio, MemPool& self) {
		typename DataIO::my_var_uint64_t var;
		self.clear();
		if (self.flarr)
			::free(self.flarr);
		self.flarr = NULL;
		self.fllen = 0;
		self.nFree = 0;
		self.hugelist = list_tail;
		dio >> var;  self.hugelist = var.t;
		dio >> var;  self.nFree = var.t;
		dio >> var;  self.fllen = var.t;
		self.flarr = (link_t*)malloc(sizeof(link_t) * self.fllen);
		if (NULL == self.flarr) {
			self.flarr = NULL;
			self.fllen = 0;
			self.nFree = 0;
			self.hugelist = list_tail;
			throw std::bad_alloc();
		}
		for (size_t i = 0, n = self.fllen; i < n; ++i) {
			dio >> var;
			self.flarr[i].next = var.t;
		}
		dio >> static_cast<mem&>(self);
	}

	template<class DataIO>
	friend void DataIO_saveObject(DataIO& dio, const MemPool& self) {
		typename DataIO::my_var_uint64_t var;
		dio << typename DataIO::my_var_uint64_t(self.hugelist);
		dio << typename DataIO::my_var_uint64_t(self.nFree);
		dio << typename DataIO::my_var_uint64_t(self.fllen);
		for (size_t i = 0, n = self.fllen; i < n; ++i)
			dio << typename DataIO::my_var_uint64_t(self.flarr[i].next);
		dio << static_cast<const mem&>(self);
	}
};

#endif

