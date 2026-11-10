#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_striteri(char *s,void (*f)(unsigned int,char*)){unsigned i=0;while(s[i]){f(i,s+i);i++;}}
