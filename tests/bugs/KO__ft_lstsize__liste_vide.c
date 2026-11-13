#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
unsigned int ft_lstsize(t_list *l){unsigned n=1;while(l->next){l=l->next;n++;}return n;}
