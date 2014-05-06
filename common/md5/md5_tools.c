#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md5.h"

int main(int argc, char *argv[])
{
    if(argc <= 1) {
        printf("Usage: %s str\n", argv[0]);
        return 1;
    }

    MD5_CTX md5;
    MD5Init(&md5);         
    int i;
    const char* encrypt = argv[1];
    unsigned char decrypt[16];    
    MD5Update(&md5,(unsigned char*)encrypt,strlen((char*)encrypt));
    MD5Final(&md5,decrypt);        
    printf("s=%s md5=", encrypt);
    for(i=0;i<16;i++)
    {
        printf("%02x",decrypt[i]);
    }
    printf("\n");

    unsigned char md5_val[33];
    memset(md5_val, 0, sizeof(md5_val));
    MD5_calc((unsigned char*)encrypt,strlen((char*)encrypt), md5_val, 33);
    printf("s=%s md5=%s\n", encrypt, md5_val);

    return 0;
}

