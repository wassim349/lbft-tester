/* conseil attendu : quel maillon doit changer */
#include "libft.h"
void ft_lstadd_back(t_list **l,t_list *n){if(!*l)*l=n;else (*l)->next=n;}
