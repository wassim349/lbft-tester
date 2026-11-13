#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_putchar_fd(char c,int fd){(void)fd;write(1,&c,1);}
