#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char *ft_strnstr(const char *b,const char *l,size_t n){size_t i=0,j;if(!*l)return (char*)b;while(b[i]&&i<n){j=0;while(l[j]&&b[i+j]==l[j]&&i+j<n)j++;if(!l[j])return (char*)b+i;i++;}return 0;}
