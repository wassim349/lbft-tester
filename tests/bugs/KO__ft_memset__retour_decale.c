#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void *ft_memset(void *b,int c,size_t n){unsigned char*p=b;while(n--)*p++=(unsigned char)c;return p;}
