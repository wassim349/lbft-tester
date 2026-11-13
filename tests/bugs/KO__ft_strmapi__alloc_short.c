/* conseil attendu : de combien d'octets le résultat a besoin */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char *ft_strmapi(char const *s,char (*f)(unsigned int,char)){size_t i=0;char*p=malloc(ft_strlen(s));if(!p)return 0;while(s[i]){p[i]=f(i,s[i]);i++;}p[i]=0;return p;}
