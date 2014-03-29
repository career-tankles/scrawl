#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include "uthash.h"

#include "HashTable.h"

//#define HASHTABLE_MEM_TEST
#ifdef HASHTABLE_MEM_TEST
static int new_nums = 0;
#endif

#ifdef HASHTABLE_MEM_TEST
static int new_nums = 0;
#endif
typedef struct _KV_ENTITY_IMP{
    char key[KV_KEY_MAX];
    char value[KV_VALUE_MAX+3];  // type + len + data
    UT_hash_handle hh;
#ifdef HASHTABLE_MEM_TEST
    _KV_ENTITY_(){
        fprintf(stderr, "NEW: %d\n", ++new_nums);
    }
    ~_KV_ENTITY_(){
        fprintf(stderr, "DEL: %d\n", new_nums--);
    }
#endif
} kv_entity_imp_t;

kv_entity_imp_t* hashtable_imp_ = NULL;


HashTable::HashTable()
  : hashtable_(NULL)
{
}
HashTable::~HashTable()
{
}

int HashTable::Init()
{

    return 0;
}
int HashTable::Destroy()
{
    if(HASH_COUNT(hashtable_imp_) > 0)
    {
        kv_entity_imp_t* el=NULL, *tmp = NULL;
        HASH_ITER(hh, hashtable_imp_, el, tmp) {
            HASH_DEL(hashtable_imp_, el);
            delete el;
            //free(el);
        }
        HASH_CLEAR(hh, hashtable_imp_);
    }
    return 0;
}

static int split_key_value(char* s, char** key, char** value)
{
    assert(s);
    char* p = strstr(s, "\t");
    if(p) {
       *p = '\0';
        *key = s;
        *value = p+1;
        return 0;
    }
    return -1;
}

// line format: Key\tValue
// Key, Value is string
int HashTable::Create(const char* txt_file)
{
    assert(txt_file);
    FILE* fp = fopen(txt_file, "r");
    if (!fp) {
        fprintf(stderr, "open file %s failed: %s\n", txt_file, strerror(errno));
        return -2;
    }
    int records = 0;
    char*key, *value;
    char line[256];
    while (fgets(line, 256, fp)) 
    {
        int res = split_key_value(line, &key, &value);
        if(res == 0)
        {
            Add(key, value);
            records ++;
        }
    }
    fclose(fp);
    fprintf(stderr, "Process %d records\n", records);
    return -1;
}
 
// line format: Key\tValue
// Key is term, Value is type
int HashTable::Create(const char* txt_file, int)
{
    assert(txt_file);
    FILE* fp = fopen(txt_file, "r");
    if (!fp) {
        fprintf(stderr, "open file %s failed: %s\n", txt_file, strerror(errno));
        return -2;
    }
    int records = 0;
    char*key, *value;
    char line[256];
    while (fgets(line, 256, fp)) 
    {
        int res = split_key_value(line, &key, &value);
        if(res == 0)
        {
            int v = atoi(value);
            //fprintf(stderr, "ADD: %s -> %d\n", key, v);
            Add(key, v);
            records ++;
        }
    }
    fclose(fp);
    fprintf(stderr, "Process %d records\n", records);
    return 0;
}
 

int HashTable::Add(const char* k, const void* v, const int size, KV_ENTITY_TYPE type)
{
    assert(k && v && size>0 && size<KV_VALUE_MAX);

    //kv_entity_imp_t* t = (kv_entity_imp_t*)malloc(sizeof(kv_entity_imp_t));
    kv_entity_imp_t* t = new kv_entity_imp_t;
    assert(t);
    strncpy(t->key, k, sizeof(t->key));
    t->value[0] = type;
    t->value[1] = size;
    memcpy(t->value+2, v, size);
    t->value[size+2] = '\0';

    HASH_ADD_STR(hashtable_imp_, key, t);
    return 0;
}

int HashTable::Get(const char* k, void* v, int* size, int* type)
{
    kv_entity_imp_t* t = NULL; 
    kv_entity_imp_t** p = &t;
    HASH_FIND_STR(hashtable_imp_, k, *p);
    if (t)
    {
        if(v)
        {
            assert(size);
            int min_size = *size<t->value[1]?*size:t->value[1];
            memcpy(v, t->value+2, min_size);
        }
        if(type) *type = t->value[0];
        if(size) *size = t->value[1];
        return 0;
    }
    return -1;
}
int HashTable::Del(const char* k)
{
    kv_entity_imp_t* t = NULL; 
    kv_entity_imp_t** p = &t;
    HASH_FIND_STR(hashtable_imp_, k, *p);
    if (*p)
    {
        HASH_DEL(hashtable_imp_, *p);
        return 0;
    }
    return -1;
}

int HashTable::Load(const char* hashtable_file)
{
    assert(hashtable_file);
    FILE* fp = NULL;
    if((fp = fopen(hashtable_file, "r")) == NULL)
    {
        exit(-1);
    }
    int records = 0;
    while(!feof(fp))
    {
        kv_entity_imp_t* el = new kv_entity_imp_t;
        assert(el);

        size_t r = fread(el, sizeof(kv_entity_imp_t), 1, fp);
        if(ferror(fp))
        {
            delete el;
            exit(-2);
        }
        if(r == 0){
            delete el;
            break;
        }
        fprintf(stderr, "load %s type:%d size:%d \n", el->key, el->value[0], el->value[1]);
        HASH_ADD_STR(hashtable_imp_, key, el);
        records ++;
    }
    fclose(fp);
    fprintf(stderr, "HashTable Load %d records\n", records);
    
    return 0;
}

int HashTable::Dump(const char* hashtable_file)
{
    FILE* fp = NULL;
    if((fp = fopen(hashtable_file, "w")) == NULL)
    {
        exit(-1);
    }
    
    int size = 0;
    int records = 0;
    kv_entity_imp_t* el=NULL, *tmp = NULL;
    HASH_ITER(hh, hashtable_imp_, el, tmp) {
        size_t b = fwrite(el, sizeof(kv_entity_imp_t), 1, fp);
        size += b*sizeof(kv_entity_imp_t);
        records ++;
    }
    fclose(fp);
    fprintf(stderr, "HashTable Dump %d records, %d bytes\n", records, size);

    return 0;
}

int HashTable::Add(const char* k, char v)
{
    return Add(k, &v, sizeof(char), KV_ENTITY_TYPE_CHAR);
}

int HashTable::Add(const char* k, unsigned char v)
{
    return Add(k, &v, sizeof(char), KV_ENTITY_TYPE_UCHAR);
}

int HashTable::Add(const char* k, int v)
{
    return Add(k, &v, sizeof(int), KV_ENTITY_TYPE_INT);
}

int HashTable::Add(const char* k, unsigned int v)
{
    return Add(k, &v, sizeof(unsigned int), KV_ENTITY_TYPE_UINT);
}
int HashTable::Add(const char* k, double v)
{
    return Add(k, &v, sizeof(double), KV_ENTITY_TYPE_DOUBLE);
}
int HashTable::Add(const char* k, const char* v)
{
    return Add(k, v, strlen(v), KV_ENTITY_TYPE_STR);
}

int HashTable::Get(const char* k, char& v)
{
    int type = 0;
    int size = sizeof(char);
    int ret = Get(k, &v, &size, &type);
    if(ret == 0) assert(type == KV_ENTITY_TYPE_CHAR);

    return ret;
}
int HashTable::Get(const char* k, unsigned char& v)
{
    int type = 0;
    int size = sizeof(unsigned char);
    int ret = Get(k, &v, &size, &type);
    if(ret == 0) assert(type == KV_ENTITY_TYPE_UCHAR);

    return ret;
}
int HashTable::Get(const char* k, int& v)
{
    int type = 0;
    int size = sizeof(int);
    int ret = Get(k, &v, &size, &type);
    if(ret == 0) assert(type == KV_ENTITY_TYPE_INT);

    return ret;
}
int HashTable::Get(const char* k, unsigned int& v)
{
    int type = 0;
    int size = sizeof(unsigned int);
    int ret = Get(k, &v, &size, &type);
    if(ret == 0) assert(type == KV_ENTITY_TYPE_UINT);
    return ret;
}
int HashTable::Get(const char* k, double& v)
{
    int type = 0;
    int size = sizeof(double);
    int ret = Get(k, &v, &size, &type);
    if(ret == 0) assert(type == KV_ENTITY_TYPE_DOUBLE);
    return ret;
}
int HashTable::Get(const char* k, char* v, int& size)
{
    int type = 0;
    int ret = Get(k, &v, &size, &type);
    if(ret == 0) assert(type == KV_ENTITY_TYPE_STR);
    return ret;
}


