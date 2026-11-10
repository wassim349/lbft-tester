#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
size_t ft_strlcat(char *d,const char *s,size_t z){size_t dl=0,sl=ft_strlen(s),i=0;while(dl<z&&d[dl])dl++;if(dl==z)return z+sl;while(s[i]&&dl+i<z-1){d[dl+i]=s[i];i++;}d[dl+i]=0;return dl+sl;}
