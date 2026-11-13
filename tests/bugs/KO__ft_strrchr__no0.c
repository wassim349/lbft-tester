/* conseil attendu : le '\0' final fait-il partie de la chaîne */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char *ft_strrchr(const char *s,int c){const char*r=0;while(*s){if(*s==(char)c)r=s;s++;}return (char*)r;}
