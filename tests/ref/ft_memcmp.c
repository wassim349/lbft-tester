#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
int ft_memcmp(const void *a,const void *b,size_t n){const unsigned char*x=a,*y=b;while(n--){if(*x!=*y)return *x-*y;x++;y++;}return 0;}
