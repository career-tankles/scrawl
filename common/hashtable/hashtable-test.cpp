#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "HashTable.h"

void HashTable_int_Test(HashTable& hashtable, bool insert=true, bool get=true);
void HashTable_str_Test(HashTable& hashtable);
void HashTable_char_Test(HashTable& hashtable);

int main(int argc, char*argv[]) {
   HashTable hashtable;
   hashtable.Init();

   HashTable_int_Test(hashtable, true, false);
   HashTable_char_Test(hashtable);
   HashTable_str_Test(hashtable);
   hashtable.Dump("hashtable.dat");
   hashtable.Destroy();

   HashTable hashtable2;
   hashtable2.Load("hashtable.dat");
   HashTable_int_Test(hashtable2, false, true);
   hashtable2.Destroy();

   HashTable hashtable3;
   hashtable3.Create("hashtable.txt", 0);
   HashTable_int_Test(hashtable3, false, true);
   hashtable3.Destroy();
    
   return 0;
}

void HashTable_int_Test(HashTable& hashtable, bool insert, bool get)
{
    int ret;
    const char **name;
    const char * names[] = { "ibob", "ijack", "igary", "ity", "ibo", "iphil", "iart", 
                                "igil", "ibuck", "ited", NULL };
    int i = 0 ;
    for(name=names; insert && *name; name++, i++) {
        if(i%2 == 0) {
            int si = -(i+1);
            ret = hashtable.Add(*name, si);
            printf("added int %s->%d, ret:%d\n", *name, si, ret);
        }
        else{ 
            unsigned int ui = i+1; 
            ret = hashtable.Add(*name, ui);
            printf("added uint %s->%u, ret:%d\n", *name, ui, ret);
        }
    }

    i = 0;
    for(name=names; get && *name; name++, i++) {
        if (i%2 == 0) {
            int value;
            ret = hashtable.Get(*name, value);
            if(ret == 0)
            printf("GET: %s->%d ret:%d\n", *name, value, ret);
        }else{
            unsigned int value2;
            ret = hashtable.Get(*name, value2);
            if(ret == 0)
            printf("GET: %s->%d\n", *name, value2);
        }
    }
    hashtable.Del("sbob");
}

void HashTable_str_Test(HashTable& hashtable)
{
    const char **name;
    const char * names[] = { "sbob", "sjack", "sgary", "sty", "sbo", "sphil", "sart", 
                      "sgil", "sbuck", "sted", NULL };
    int ret;
    int i =0 ;
    for(name=names; *name; name++, i++) {
        ret = hashtable.Add(*name, *name, strlen(*name));
        assert(ret == 0);
        printf("added str %s->%s, ret:%d\n", *name, *name, ret);
    }

    for(name=names; *name; name++) {
        char value[256];
        memset(value, 0, sizeof(value));
        int type = 0;
        int size = sizeof(value);
        ret = hashtable.Get(*name, value, &size);
        if(ret == 0)
        printf("GET: %s->%s size:%d type:%d\n", *name, value, size, type);
    }
}

void HashTable_char_Test(HashTable& hashtable)
{
    const char **name;
    const char * names[] = { "cbob", "cjack", "cgary", "cty", "cbo", "cphil", "cart", 
                      "cgil", "cbuck", "cted", NULL };
    int ret;
    int i = 0;
    for(name=names; *name; name++, i++) {
        if(i%2 == 0) { 
            char c1 = -(i+1);
            ret = hashtable.Add(*name, &c1, sizeof(char), KV_ENTITY_TYPE_CHAR);
            printf("added char %s->%c %d, ret:%d\n", *name, c1, c1,ret);
        }
        else{
            unsigned char c2 = i+1;
            ret = hashtable.Add(*name, &c2, sizeof(unsigned char), KV_ENTITY_TYPE_UCHAR);
            printf("added uchar %s->%c %d, ret:%d\n", *name, c2,c2, ret);
        }
    }
    i = 0;
    for(name=names; *name; name++, i++) {
        if(i%2 == 0) {
            char value;
            ret = hashtable.Get(*name, value);
            if(ret == 0)
            printf("GET-char: %s->%c\n", *name, value);
        }else{
            unsigned char value;
            ret = hashtable.Get(*name, value);
            if(ret == 0)
            printf("GET-uchar: %s->%c\n", *name, value);

            // get data's size and/or type
            int size = 0;
            int type = 0;
            ret = hashtable.Get(*name, NULL, &size, &type);
            if(ret == 0)
            printf("GET-uchar's size and/or type: %s size:%d type:%d\n", *name, size, type);
        }
    }
}


