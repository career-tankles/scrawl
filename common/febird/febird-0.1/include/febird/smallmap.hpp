#ifndef __febird_smallmap_hpp__
#define __febird_smallmap_hpp__

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <memory>
#include <stdexcept>

template<class Mapped>
class smallmap {
public:
    explicit smallmap(int indexSize) {
        index = (short*)malloc(sizeof(short) * indexSize);
        if (NULL == index) throw std::bad_alloc();
        p = (Mapped*)malloc(sizeof(Mapped) * indexSize);
        if (NULL == p) {
			free(index);
			throw std::bad_alloc();
		}
	//	std::uninitialized_fill_n(p, indexSize, Mapped());
        for (int i = 0; i < indexSize; ++i)
            new(p+i)Mapped();
        n = 0;
        c = indexSize;
        memset(index, -1, sizeof(short) * c);
    }
    ~smallmap() {
        std::_Destroy(p, p + c);
        free(p);
        free(index);
    }
    Mapped& bykey(int key) {
        assert(key >= 0);
        assert(key < c);
        assert(n <= c);
        if (-1 == index[key]) {
            assert(n < c);
            index[key] = (short)n;
		//	assert(isprint(key) || isspace(key));
            p[n].ch = key;
            return p[n++];
        }
        return p[index[key]];
    }
    void resize0() {
		if (n <= 16) {
			for (int i = 0; i < n; ++i) {
				Mapped& v = p[i];
				assert(-1 != v.ch);
				assert(-1 != index[v.ch]);
				index[v.ch] = -1;
				v.resize0();
			}
		} else {
			memset(index, -1, sizeof(short) * c);
			for (int i = 0; i < n; ++i)
				p[i].resize0();
		}
        n = 0;
    }
    bool exists(int key) const {
        assert(key >= 0);
        assert(key < c);
        return -1 != index[key];
    }
	Mapped& byidx(int idx) {
		assert(idx < n);
		return p[idx];
	}
    Mapped* begin() { return p; }
    Mapped* end()   { return p + n; }
    size_t size() const { return n; }
private:
    short*  index;
    Mapped* p;
    int  n, c;
};



#endif // __febird_smallmap_hpp__


