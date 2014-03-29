#ifndef _DATRIE_H_
#define _DATRIE_H_

#define DAT_KSIZE_MAX		32
#define DAT_DICT_SIZE_MAX	(1<<26)

typedef struct dat dat_t;

dat_t *dat_new();
void dat_free(dat_t *p);
int dat_add(dat_t *p, const char *k, unsigned v);
int dat_get(dat_t *p, const char* k, unsigned *p_v);
int dat_dump(dat_t *p, const char *dat_file);
int dat_load(dat_t *p, const char *dat_file);

#endif
