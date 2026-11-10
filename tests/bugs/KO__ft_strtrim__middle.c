#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char *ft_strtrim(char const *s,char const *set){char*p=malloc(ft_strlen(s)+1);size_t i=0,j=0;if(!p)return 0;while(s[i]){if(!ft_strchr(set,s[i]))p[j++]=s[i];i++;}p[j]=0;return p;}
