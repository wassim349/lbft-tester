#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
t_list *ft_lstnew(void *c){t_list*n=malloc(sizeof(t_list));n->content=c;n->next=0;return n;}
