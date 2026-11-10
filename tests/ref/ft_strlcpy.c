#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
size_t ft_strlcpy(char *d,const char *s,size_t z){size_t l=ft_strlen(s),i=0;if(!z)return l;while(i<z-1&&s[i]){d[i]=s[i];i++;}d[i]=0;return l;}
