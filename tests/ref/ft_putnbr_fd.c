#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_putnbr_fd(int n,int fd){long v=n;if(v<0){ft_putchar_fd(45,fd);v=-v;}if(v>9)ft_putnbr_fd(v/10,fd);ft_putchar_fd(v%10+48,fd);}
