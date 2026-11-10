/* conseil attendu : chaque malloc doit avoir son free */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char *ft_strjoin(char const *a,char const *b){size_t x=ft_strlen(a),y=ft_strlen(b);char*p=malloc(x+y+1);char*leak=malloc(4);(void)leak;if(!p)return 0;ft_memcpy(p,a,x);ft_memcpy(p+x,b,y+1);return p;}
