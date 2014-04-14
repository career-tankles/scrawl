

#include <stdio.h>
#include "url_decode.h"
#include "url_encode.h"

#include <string>


    void urldecode(char *p)  
    {  
    int i=0;  
    while(*(p+i))  
    {  
       if ((*p=*(p+i)) == '%')  
       {  
        *p=*(p+i+1) >= 'A' ? ((*(p+i+1) & 0XDF) - 'A') + 10 : (*(p+i+1) - '0');  
        *p=(*p) * 16;  
        *p+=*(p+i+2) >= 'A' ? ((*(p+i+2) & 0XDF) - 'A') + 10 : (*(p+i+2) - '0');  
        i+=2;  
       }  
       else if (*(p+i)=='+')  
       {  
        *p=' ';  
       }  
       p++;  
    }  
    *p='\0';  
    }  

int main(int argc, char** argv)
{

    char* src = argv[1];
    char obj[100] = {0};

    unsigned int len = strlen(src);
    int resultSize = URLDecode(src, len, obj, 100);
    printf("result: %s\n", obj);

    std::string url = argv[1];
    urldecode((char*)url.c_str());
    fprintf(stderr, "local:  %s\n", url.c_str());

    URLEncode(src, len, obj, 100);
    printf("url-encode: %s\n", obj);
    

}
