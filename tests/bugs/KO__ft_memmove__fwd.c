/* conseil attendu : les zones se chevauchent */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void *ft_memmove(void *d,const void *s,size_t n){unsigned char*a=d;const unsigned char*b=s;while(n--)*a++=*b++;return d;}
