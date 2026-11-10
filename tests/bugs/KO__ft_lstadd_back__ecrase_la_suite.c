/* conseil attendu : ajoute new après le DERNIER maillon */
#include "libft.h"
void ft_lstadd_back(t_list **l,t_list *n){if(!*l)*l=n;else (*l)->next=n;}
