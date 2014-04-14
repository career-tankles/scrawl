

#include <stdio.h>

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

int main()
{
    std::string url = "http%3A%2F%2Fwww.qingkan.net%2Fbook%2Fwudihuanling%2Ftxt.html";
    urldecode((char*)url.c_str());
    fprintf(stderr, "%s\n", url.c_str());
}
