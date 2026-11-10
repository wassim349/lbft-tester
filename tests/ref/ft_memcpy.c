#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void *ft_memcpy(void *d,const void *s,size_t n){unsigned char*a=d;const unsigned char*b=s;if(!d&&!s)return d;while(n--)*a++=*b++;return d;}
