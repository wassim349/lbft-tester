/* conseil attendu : en quoi c et les octets sont convertis */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void *ft_memchr(const void *s,int c,size_t n){const unsigned char*p=s;while(n--){if(*p==c)return (void*)p;p++;}return 0;}
