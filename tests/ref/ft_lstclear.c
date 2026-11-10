#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_lstclear(t_list **l,void (*d)(void*)){t_list*x;while(*l){x=(*l)->next;ft_lstdelone(*l,d);*l=x;}}
