#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char *ft_substr(char const *s,unsigned int st,size_t len){size_t l=ft_strlen(s);char*p;if(st>=l)return 0;if(len>l-st)len=l-st;p=malloc(len+1);if(!p)return 0;ft_memcpy(p,s+st,len);p[len]=0;return p;}
