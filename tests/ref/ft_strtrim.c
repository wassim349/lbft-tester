#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char *ft_strtrim(char const *s,char const *set){size_t a=0,b=ft_strlen(s);while(s[a]&&ft_strchr(set,s[a]))a++;while(b>a&&ft_strchr(set,s[b-1]))b--;return ft_substr(s,a,b-a);}
