/* conseil attendu : puis mets toujours un '\0' */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
size_t ft_strlcpy(char *d,const char *s,size_t z){size_t l=ft_strlen(s),i=0;while(i+1<z&&s[i]){d[i]=s[i];i++;}return l;}
