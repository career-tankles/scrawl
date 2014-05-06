/*
 * ctool_hashmap.h
 *
 *  Created on: 2011-08-02
 *      Author: ddt
 */

#ifndef _LIB_HASHMAP_H
#define _LIB_HASHMAP_H

/******************************************************************************/
/*                                                                            */
/* function                                                                   */
/*                                                                            */
/******************************************************************************/

void *ctool_hashmap_init(uint64_t item_asize, uint64_t mblk_asize,
        uint64_t byte_asize);
void ctool_hashmap_drop(void *hmap);
void ctool_hashmap_usage(void *hmap, uint64_t *total_count,
        uint64_t *collision_count, uint64_t *mem_size);

void *ctool_hashmap_buf(void *hmap, uint64_t value_len);
void ctool_hashmap_add(void *hmap, uint64_t key, void *value_buf);
void *ctool_hashmap_get(void *hmap, uint64_t key, void *value_buf);

/* returns, 0 continue, other break and return. */
int ctool_hashmap_travel(void *hmap, int(*func)(void *data, void *value_buf),
        void *data);

uint64_t ctool_hashmap_key(const uint8_t *byte_array, uint64_t byte_count);

#endif /* _LIB_HASHMAP_H */

/******************************************************************************/
/*                                                                            */
/* end of file                                                                */
/*                                                                            */
/******************************************************************************/

