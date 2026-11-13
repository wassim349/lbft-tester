/* conseil attendu : que reçoit-elle en 2e argument */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_striteri(char *s,void (*f)(unsigned int,char*)){unsigned i=0;char c;while(s[i]){c=s[i];f(i,&c);i++;}}
