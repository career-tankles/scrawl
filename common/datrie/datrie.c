#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

#include "datrie.h"

#define DAT_MAGIC           0x20131026
#define DAT_VERSION         1

#define DAT_SCALE_NODES		UCHAR_MAX
#define DAT_BLOCK_NODES		(1<<18)

/* check zero as none */
#define DAT_BASE_ROOT       1

typedef struct da_node {
    uint32_t	base;	/* children base */
    uint32_t	check;	/* check parent */
    uint32_t	value;	/* saved value */
} da_node_t;

typedef struct dt_node {
    char 		key[DAT_KSIZE_MAX];
    int			ksize;
    uint32_t	value;
} dt_node_t;

struct dat {
    size_t		total;	/* total of nodes alloced */
    size_t		nodes;	/* num of nodes */
    da_node_t	*da;	/* double array */
    size_t		dt_nodes;	/* num of nodes */
    dt_node_t	*dt;	/* dict table */
};

typedef struct dat_head {
    uint32_t magic;
    uint32_t version;
    uint32_t nodes;
} dat_head_t;


static int grow(dat_t *p)
{
    assert(p);

    /* overflow */
    if (p->nodes + DAT_SCALE_NODES < p->nodes)
        return -1;

    if (p->nodes + DAT_SCALE_NODES < p->total)
        return 1;

    unsigned int new_total = ((p->nodes + DAT_SCALE_NODES) / DAT_BLOCK_NODES + 1) * DAT_BLOCK_NODES;
    da_node_t *new_da = (da_node_t *)realloc( p->da, new_total * sizeof(new_da[0]));
    assert(new_da);
    if (!new_da)
        return -2;

    memset(new_da + p->nodes, 0, (new_total - p->total) * sizeof(new_da[0]));
    p->da = new_da;
    p->total = new_total;
    return 0;
}

dat_t *dat_new()
{
    dat_t *p = (dat_t *)calloc(1, sizeof(p[0]));
    return p;
}

void dat_free(dat_t *p)
{
    assert(p);

    if (p->da)
        free(p->da);
    if (p->dt)
        free(p->dt);
    free(p);
}

int dat_add(dat_t *p, const char *k, unsigned v)
{
    if (!p->dt)
        p->dt = (dt_node_t *)malloc(DAT_DICT_SIZE_MAX * sizeof(p->dt[0]));
    assert(p->dt);

    int ksize = strlen(k);

    dt_node_t *dt = &p->dt[p->dt_nodes++];

    memcpy(dt->key, k, ksize);
    dt->ksize = ksize;
    dt->value = v;

    return 0;
}

da_node_t *dat_get_internal(dat_t *p, const char* k, unsigned ksize)
{
    assert(p && k);

    unsigned off;
    da_node_t *parent = &p->da[DAT_BASE_ROOT];
    da_node_t *cur = parent;

    for (off = 0; off < ksize; ++off)
    {
        unsigned char ch = k[off];
        if (parent->base + ch >= p->nodes)
            return NULL;

        cur = &p->da[parent->base + ch];
        if (cur->check != parent - p->da)
            return NULL;

        parent = cur;
    }
    return cur;
}

static int dat_add_internal(dat_t *p, dt_node_t *dt, unsigned off)
{
    assert(p && dt);

    unsigned char ch = (unsigned char)dt->key[off];

    da_node_t *parent = dat_get_internal(p, dt->key, off);
    assert(parent);

    if (parent->base == 0)
    {
        if (p->nodes <= ch) 
            parent->base = DAT_BASE_ROOT + 1;
        else
            parent->base = p->nodes - ch;
    }

    da_node_t *cur = &p->da[parent->base + ch];
    if (cur->check)
    {
        /* exist */
        assert(cur->check == parent - p->da);
        return 1;
    }

    /* as dt is sorted by off and key, da increasing monotonously */
    assert(p->nodes <= parent->base + ch);

    p->nodes = parent->base + ch + 1;

    cur->check = parent - p->da;
    assert(cur->check);

    if (dt->ksize == off + 1)
        cur->value = dt->value;

    if (p->total < parent->base + DAT_SCALE_NODES || grow(p) < 0)
        return -1;

    return 0;
}

int dat_get(dat_t *p, const char* k,unsigned *p_v)
{
    assert(p && k && k[0]);

    int ksize = strlen(k);

    if (ksize > DAT_KSIZE_MAX)
        return -1;

    da_node_t *da = dat_get_internal(p, k, ksize);
    if (!da)
        return -1;

    if (p_v)
        p_v[0] = da->value;

    return 0;
}

static int dat_build(dat_t *p)
{
    assert(p);

    grow(p);

    int i, off, flag;
    for (off = 0; off < DAT_KSIZE_MAX; ++off)
    {
        flag = 0;
        for (i = 0; i < p->dt_nodes; ++i)
        {
            dt_node_t *dt = &p->dt[i];
            if (dt->ksize <= off)
                continue;
            if (dat_add_internal(p, dt, off) < 0)
                return -1;
            flag = 1;
        }
        if (!flag)
            break;
    }
    return 0;
}

int dat_dump(dat_t *p, const char *dat_file)
{
    assert(p && dat_file);

    FILE *fp = fopen(dat_file, "w");
    if (!fp)
        return -1;

    int ret = 0;
    if (!p->dt_nodes)
    {
        ret = -2;
        goto out;
    }

    if (dat_build(p) < 0)
    {
        ret = -3;
        goto out;
    }

    assert(p->nodes);

    dat_head_t h;
    h.magic = DAT_MAGIC;
    h.version = DAT_VERSION;
    h.nodes = p->nodes;

    if (fwrite(&h, sizeof(h), 1, fp) != 1)
    {
        ret = -4;
        goto out;
    }

    if (fwrite(p->da, sizeof(p->da[0]), p->nodes, fp) != p->nodes)
    {
        ret = -5;
        goto out;
    }

out:
    fclose(fp);
    return ret;
}

int dat_load(dat_t *p, const char *dat_file)
{
    assert(p && dat_file && !p->da);

    FILE *fp = fopen(dat_file, "r");
    if (!fp)
        return -1;

    int ret = 0;
    dat_head_t h;

    if (fread(&h, sizeof(h), 1, fp) != 1)
    {
        ret = -2;
        goto error;
    }

    if (h.magic != DAT_MAGIC || h.version != DAT_VERSION)
    {
        ret = -3;
        goto error;
    }

    if (!h.nodes)
    {
        ret = -4;
        goto error;
    }

    struct stat st;
    fstat(fileno(fp), &st);

    if (st.st_size != sizeof(h) + h.nodes * sizeof(p->da[0]))
    {
        ret = -5;
        goto error;
    }

    p->da = (da_node_t *)malloc(h.nodes * sizeof(p->da[0]));
    if (fread(p->da, sizeof(p->da[0]), h.nodes, fp) != h.nodes)
    {
        ret = -6;
        goto error;
    }
    p->nodes = h.nodes;

    fclose(fp);
    return 0;
error:
    free(p->da);
    p->da = NULL;
    fclose(fp);
    return ret;
}
