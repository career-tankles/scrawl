
#include <iostream>

#include "storage.h"

int main()
{
    std::string msg("this is a test!");
    std::string filename = "test.txt";

    storage store;
    int res = store.open(filename);
    std::cerr<<res<<std::endl;
    res = store.store(msg.c_str(), msg.size());
    std::cerr<<res<<std::endl;
    res = store.close();
    std::cerr<<res<<std::endl;
    
}

