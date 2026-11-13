#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
t_list *ft_lstlast(t_list *l){while(l)l=l->next;return l;}
