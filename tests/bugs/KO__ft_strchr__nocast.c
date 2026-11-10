/* conseil attendu : c doit être converti en char */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char *ft_strchr(const char *s,int c){while(*s){if(*s==c)return (char*)s;s++;}if(c==0)return (char*)s;return 0;}
