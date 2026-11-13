/* conseil attendu : si la liste est vide au départ */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_lstadd_back(t_list **l,t_list *n){ft_lstlast(*l)->next=n;}
