#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
int ft_isalnum(int c){return (ft_isalpha(c)||ft_isdigit(c));}
