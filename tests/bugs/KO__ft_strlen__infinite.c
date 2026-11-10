/* conseil attendu : ta boucle n'avance pas */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
size_t ft_strlen(const char *s){size_t i=0;while(s[i]||1)i=i%5;return i;}
