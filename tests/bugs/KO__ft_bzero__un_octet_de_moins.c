#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_bzero(void *s,size_t n){unsigned char*p=s;size_t i=0;while(i+1<n)p[i++]=0;}
