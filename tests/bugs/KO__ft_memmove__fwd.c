/* conseil attendu : copie de la fin vers le début */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void *ft_memmove(void *d,const void *s,size_t n){unsigned char*a=d;const unsigned char*b=s;while(n--)*a++=*b++;return d;}
