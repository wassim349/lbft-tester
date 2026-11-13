/* conseil attendu : comment les octets sont comparés */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
int ft_strncmp(const char *a,const char *b,size_t n){size_t i=0;while(i<n&&(a[i]||b[i])){if(a[i]!=b[i])return a[i]-b[i];i++;}return 0;}
