#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
unsigned int ft_lstsize(t_list *l){unsigned n=0;while(l){n++;l=l->next;}return n;}
