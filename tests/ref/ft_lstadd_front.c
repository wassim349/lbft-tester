#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_lstadd_front(t_list **l,t_list *n){n->next=*l;*l=n;}
