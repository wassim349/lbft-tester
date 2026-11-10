#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_lstadd_back(t_list **l,t_list *n){if(!*l)*l=n;else ft_lstlast(*l)->next=n;}
