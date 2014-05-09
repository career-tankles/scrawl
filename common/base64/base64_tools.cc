

#include <iostream>
#include <string>
#include "base64.h"

int main(int argc, char** argv)
{
    std::string s = "abcd";
    if(argc > 1)
        s = argv[1];

    std::cout<<"input: "<<s<<std::endl;
    std::string encoded = base64_encode(s);
    std::cout<<"base64_encode:" <<encoded<<std::endl;

    std::string decoded = base64_decode(encoded);
    std::cout<<"base64_decode:" <<decoded<<std::endl;

}

