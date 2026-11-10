/* conseil attendu : vérifie le débordement avant malloc */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void *ft_calloc(size_t a,size_t b){void*p=malloc(a*b);if(!p)return 0;ft_bzero(p,a*b);return p;}
