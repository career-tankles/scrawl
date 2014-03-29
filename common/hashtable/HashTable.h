#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#define KV_KEY_MAX      64
#define KV_VALUE_MAX    64

enum KV_ENTITY_TYPE
{
    KV_ENTITY_TYPE_NOSET = 0,
    KV_ENTITY_TYPE_CHAR,
    KV_ENTITY_TYPE_UCHAR,
    KV_ENTITY_TYPE_INT,
    KV_ENTITY_TYPE_UINT,
    KV_ENTITY_TYPE_DOUBLE,
    KV_ENTITY_TYPE_STR,
};

typedef struct _KV_ENTITY_{
    // interface
    // null
} kv_entity_t;

class HashTable
{
public:
    HashTable();
    ~HashTable();

    int Init();
    int Destroy();

    int Add(const char* k, char v);
    int Add(const char* k, unsigned char v);
    int Add(const char* k, int v);
    int Add(const char* k, unsigned int v);
    int Add(const char* k, double v);
    int Add(const char* k, const char* v);

    int Get(const char* k, char& v);
    int Get(const char* k, unsigned char& v);
    int Get(const char* k, int& v);
    int Get(const char* k, unsigned int& v);
    int Get(const char* k, double& v);
    int Get(const char* k, char* v, int& size);

    int Add(const char* k, const void* v, const int size, KV_ENTITY_TYPE type=KV_ENTITY_TYPE_NOSET);
    int Get(const char* k, void* v, int* size, int* type=NULL);
    int Del(const char* k);

    int Load(const char* hashtable_file);
    int Dump(const char* hashtable_file);

    int Create(const char* txt_file);
    int Create(const char* txt_file, int t=1);

private:
    kv_entity_t* hashtable_;
};

#endif

