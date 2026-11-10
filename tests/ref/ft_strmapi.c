#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char *ft_strmapi(char const *s,char (*f)(unsigned int,char)){size_t i=0;char*p=ft_strdup(s);if(!p)return 0;while(p[i]){p[i]=f(i,p[i]);i++;}return p;}
