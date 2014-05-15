#ifdef __TEST_PHP

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "ptree.hpp"
using namespace febird::objbox;
using namespace std;
ssize_t readn(int fd, void* vptr, size_t n)
{
    ssize_t nleft;
    ssize_t nread;
    char *ptr = (char*)vptr;

    nleft = n;
    while (nleft > 0)
    {
        if ( (nread = read (fd, ptr, nleft)) < 0)
        {
            if (errno == EINTR)
                nread = 0; /* and call read() again */
            else
                return -1;
        }
        else if (nread == 0)
            break; /*  EOF */ 

        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft);
}

int get_file_content(const char* filepath, char** content)
{
    int ret = -1;
    int fd;
    char *buf = NULL;
    struct stat st;

    fd = open(filepath, O_RDONLY);
    if (fd < 0)
    {
        return -1;
    }

    if (fstat(fd, &st) != 0) 
    {
        goto ERROR;
    }

    if (st.st_size <= 0)
    {
        goto ERROR;
    }

    buf = (char*)calloc(1, st.st_size + 1);
    if (buf == NULL)
    {
        goto ERROR;
    }

    ret = readn(fd, buf, st.st_size);
    if (ret != st.st_size)
    {
        free(buf);
        ret = -1;
        goto ERROR;
    }

    *content = buf;

ERROR:
    close (fd);

    return ret;
}
int main(int argc, char **argv) {
    php_array root;
    char *line = new char[1024 * 1024];
    get_file_content("test_cast.txt", &line);
    const char *beg = line;
    const char *end = line + strlen(line);
    obj* p_obj = php_load(&beg, end);
    root[obj_ptr("results")] = obj_ptr(new obj_array);
    //root[obj_ptr("products")] = obj_ptr(new obj_array);
    //root[obj_ptr("errno")] = obj_ptr(new obj_long(0));
    obj_array& arr0 = obj_cast<obj_array>(root[obj_ptr("results")]);
    arr0.push_back(obj_ptr(dynamic_cast<php_array*>(p_obj)));
    string output_result;
    output_result.reserve(409600);
    php_save_as_json(p_obj, &output_result);
    //php_save_as_json(&root, &output_result);
    fputs(output_result.c_str(), stdout);
    fputs("\n", stdout);


}
#endif
