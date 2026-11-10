/* conseil attendu : ça plante quand malloc renvoie NULL */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char *ft_strdup(const char *s){size_t l=ft_strlen(s)+1;char*p=malloc(l);ft_memcpy(p,s,l);return p;}
