#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_putstr_fd(char *s,int fd){write(fd,s,ft_strlen(s));write(fd,"\n",1);}
