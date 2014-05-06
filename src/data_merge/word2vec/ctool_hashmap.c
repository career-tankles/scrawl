/*
 * ctool_hashmap.c
 *
 *  Created on: 2011-08-02
 *      Author: ddt
 */

#include <stdint.h> /* int64_t and uint64_t */
#include <stdlib.h> /* NULL */
#include <stdio.h>
#include <string.h>

/******************************************************************************/
/*                                                                            */
/* options                                                                    */
/*                                                                            */
/******************************************************************************/

#define _CTOOL_HASHMAP_ALIGN_

/******************************************************************************/
/*                                                                            */
/* defines                                                                    */
/*                                                                            */
/******************************************************************************/

typedef struct _ctool_hashmap_item_t ctool_hashmap_item_t;
struct _ctool_hashmap_item_t {
    uint64_t key;
    ctool_hashmap_item_t *next;
};

typedef struct {
    uint64_t total_count;
    uint64_t collision_count;

    uint64_t item_asize;
    ctool_hashmap_item_t **item_array;

    uint64_t mblk_asize;
    uint64_t mblk_count;
    void **mblk_array;

    uint64_t byte_asize;
    uint64_t byte_count;
} ctool_hashmap_sdata_t;

/******************************************************************************/
/*                                                                            */
/* functions                                                                  */
/*                                                                            */
/******************************************************************************/

#ifdef _CTOOL_HASHMAP_DEBUG_
static void *debug_malloc(size_t size) {
    void *ptr;
    ptr = malloc(size);
    fprintf(stderr, "malloc %016llx %llu\n", (unsigned long long) ptr,
            (unsigned long long) size);
    return ptr;
}

static void debug_free(void *ptr) {
    fprintf(stderr, "free   %016llx\n", (unsigned long long) ptr);
    free(ptr);
}
#endif

#define HMAP ((ctool_hashmap_sdata_t *) hmap)

void *ctool_hashmap_init(uint64_t item_asize, uint64_t mblk_asize,
        uint64_t byte_asize) {
    void *hmap;
    uint64_t i;
    uint64_t size1, size2, size3, size4;

    if ((item_asize == 0) || (mblk_asize == 0) || (byte_asize == 0)) {
        return NULL;
    }

    size1 = sizeof(ctool_hashmap_sdata_t);
    size2 = item_asize * sizeof(ctool_hashmap_item_t);
    size3 = mblk_asize * sizeof(void *);
    size4 = size1 + size2 + size3;

#ifdef _CTOOL_HASHMAP_DEBUG_
    hmap = debug_malloc(size4);
#else
    hmap = malloc(size4);
#endif

    if (hmap != NULL) {
        HMAP->total_count = 0;
        HMAP->collision_count = 0;

        HMAP->item_asize = item_asize;
        HMAP->item_array = (ctool_hashmap_item_t **) ((uint8_t *) hmap + size1);

        HMAP->mblk_asize = mblk_asize;
        HMAP->mblk_count = 0;
        HMAP->mblk_array = (void **) ((uint8_t *) hmap + size1 + size2);

        HMAP->byte_asize = byte_asize;
        HMAP->byte_count = byte_asize;

        for (i = 0; i < item_asize; i++) {
            HMAP->item_array[i] = NULL;
        }
        for (i = 0; i < mblk_asize; i++) {
            HMAP->mblk_array[i] = NULL;
        }
    }
    return hmap;
}

void ctool_hashmap_drop(void *hmap) {
    uint64_t i, mblk_count;
    void *mblk;

    mblk_count = HMAP->mblk_count;
    for (i = 0; i < mblk_count; i++) {
        mblk = HMAP->mblk_array[i];
        if (mblk != NULL) {

#ifdef _CTOOL_HASHMAP_DEBUG_
            debug_free(mblk);
#else
            free(mblk);
#endif

        }
    }

#ifdef _CTOOL_HASHMAP_DEBUG_
    debug_free(hmap);
#else
    free(hmap);
#endif
}

void ctool_hashmap_usage(void *hmap, uint64_t *total_count,
        uint64_t *collision_count, uint64_t *mem_size) {

    *total_count = HMAP->total_count;
    *collision_count = HMAP->collision_count;
    *mem_size = sizeof(ctool_hashmap_sdata_t)
            + HMAP->item_asize * sizeof(ctool_hashmap_item_t)
            + HMAP->mblk_asize * sizeof(void *)
            + HMAP->mblk_count * HMAP->byte_asize;
}

void *ctool_hashmap_buf(void *hmap, uint64_t value_len) {
    uint64_t item_len;
    void *mblk, *item;

#ifdef _CTOOL_HASHMAP_ALIGN_
#if __WORDSIZE == 64
    item_len = (value_len >> 3) * 8;
    if (item_len < value_len) {
        item_len += 8;
    }
#else
    item_len = (value_len >> 2) * 4;
    if (item_len < value_len) {
        item_len += 4;
    }
#endif
#else
    item_len = value_len;
#endif

    item_len += sizeof(ctool_hashmap_item_t);
    if (item_len > HMAP->byte_asize) {
        return NULL;
    }

    if ((HMAP->byte_count + item_len) > HMAP->byte_asize) {
        if (HMAP->mblk_count >= HMAP->mblk_asize) {
            return NULL;
        }
#ifdef _CTOOL_HASHMAP_DEBUG_
        mblk = debug_malloc(HMAP->byte_asize);
#else
        mblk = malloc(HMAP->byte_asize);
#endif
        if (mblk == NULL) {
            return NULL;
        }
        HMAP->mblk_array[HMAP->mblk_count] = mblk;
        HMAP->mblk_count++;
        HMAP->byte_count = 0;
    } else {
        mblk = HMAP->mblk_array[HMAP->mblk_count - 1];
    }

    item = (uint8_t *) mblk + HMAP->byte_count;
    HMAP->byte_count += item_len;

    return (uint8_t *) item + sizeof(ctool_hashmap_item_t);
}

void ctool_hashmap_add(void *hmap, uint64_t key, void *value_buf) {
    uint64_t item_id;
    ctool_hashmap_item_t *key_item, *add_item;

    add_item = (ctool_hashmap_item_t *) ((uint8_t *) value_buf
            - sizeof(ctool_hashmap_item_t));
    add_item->key = key;
    add_item->next = NULL;

    item_id = key % HMAP->item_asize;
    key_item = HMAP->item_array[item_id];

    if (key_item == NULL) {
        HMAP->item_array[item_id] = add_item;
    } else {
        while (key_item->next != NULL) {
            key_item = key_item->next;
        }
        key_item->next = add_item;
        HMAP->collision_count++;
    }
    HMAP->total_count++;
}

void *ctool_hashmap_get(void *hmap, uint64_t key, void *value_buf) {
    uint64_t item_id;
    ctool_hashmap_item_t *key_item;

    item_id = key % HMAP->item_asize;

    if (value_buf != NULL) {

        /* get value of duplicated key */
        key_item = ((ctool_hashmap_item_t *) ((uint8_t *) value_buf
                - sizeof(ctool_hashmap_item_t)))->next;
    } else {
        key_item = HMAP->item_array[item_id];
    }
    while (key_item != NULL) {
        if (key_item->key == key) {
            return (uint8_t *) key_item + sizeof(ctool_hashmap_item_t);
        }
        key_item = key_item->next;
    }

    return NULL;
}

int ctool_hashmap_travel(void *hmap, int(*func)(void *data, void *value_buf),
        void *data) {
    int rcode;
    uint64_t item_id;
    ctool_hashmap_item_t *key_item;

    uint64_t item_asize = HMAP->item_asize;
    ctool_hashmap_item_t **item_array = HMAP->item_array;

    /* for each entry */
    for (item_id = 0; item_id < item_asize; item_id++) {
        key_item = item_array[item_id];
        while (key_item != NULL) {
            rcode = func(data,
                    (uint8_t *) key_item + sizeof(ctool_hashmap_item_t));
            if (rcode != 0) {
                return rcode;
            }
            key_item = key_item->next;
        }
    }
    return 0;
}

uint64_t ctool_hashmap_key(const uint8_t *byte_array, uint64_t byte_count) {
    uint64_t hval = 0xcbf29ce484222325ULL;
    const uint8_t *byte_end;

    byte_end = byte_array + byte_count;
    while (byte_array < byte_end) {
        /* xor the bottom with the current octet. */
        hval ^= *(byte_array);
        byte_array++;

        /* multiply by the 64 bit FNV magic prime mod 2^64. */
        hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7)
                + (hval << 8) + (hval << 40);
    }
    return hval;
}

/******************************************************************************/
/*                                                                            */
/* end of file                                                                */
/*                                                                            */
/******************************************************************************/

