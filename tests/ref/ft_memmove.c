#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void *ft_memmove(void *d,const void *s,size_t n){unsigned char*a=d;const unsigned char*b=s;if(a==b)return d;if(a<b)while(n--)*a++=*b++;else while(n--)a[n]=b[n];return d;}
