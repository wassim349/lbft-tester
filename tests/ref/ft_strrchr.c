#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char *ft_strrchr(const char *s,int c){const char*r=0;while(*s){if(*s==(char)c)r=s;s++;}if((char)c==0)return (char*)s;return (char*)r;}
