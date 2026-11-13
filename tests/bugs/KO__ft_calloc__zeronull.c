/* conseil attendu : avec une taille 0, compare */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void *ft_calloc(size_t a,size_t b){void*p;if(!a||!b)return 0;if(a>(size_t)-1/b)return 0;p=malloc(a*b);if(!p)return 0;ft_bzero(p,a*b);return p;}
