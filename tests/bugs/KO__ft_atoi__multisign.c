#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
int ft_atoi(const char *s){int sg=1;long r=0;while(*s==32||(*s>=9&&*s<=13))s++;while(*s==45||*s==43){if(*s==45)sg=-sg;s++;}while(*s>=48&&*s<=57)r=r*10+*s++-48;return (int)(r*sg);}
