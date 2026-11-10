#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_lstclear(t_list **l,void (*d)(void*)){t_list*x=*l,*y;while(x){y=x->next;ft_lstdelone(x,d);x=y;}}
