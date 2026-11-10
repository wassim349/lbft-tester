/* conseil attendu : pour INT_MIN : écris */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_putnbr_fd(int n,int fd){if(n<0){ft_putchar_fd(45,fd);n=-n;}if(n>9)ft_putnbr_fd(n/10,fd);ft_putchar_fd(n%10+48,fd);}
