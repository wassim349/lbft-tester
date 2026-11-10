#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_lstiter(t_list *l,void (*f)(void*)){while(l){f(l->content);l=l->next;}}
