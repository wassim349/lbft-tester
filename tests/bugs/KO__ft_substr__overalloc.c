/* conseil attendu : regarde ce que devient ta taille d'allocation */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char *ft_substr(char const *s,unsigned int st,size_t len){size_t l=ft_strlen(s);char*p;if(st>=l)return ft_strdup("");p=malloc(len+1);if(!p)return 0;ft_strlcpy(p,s+st,len+1);return p;}
