#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
size_t ft_strlcat(char *d,const char *s,size_t z){size_t dl=ft_strlen(d),sl=ft_strlen(s),i=0;if(z<=dl)return dl+sl;while(s[i]&&dl+i<z-1){d[dl+i]=s[i];i++;}d[dl+i]=0;return dl+sl;}
