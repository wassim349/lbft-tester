/* conseil attendu : ajoute les nouveaux maillons à la FIN */
#include "libft.h"
#include <stdlib.h>
t_list *ft_lstmap(t_list *l,void *(*f)(void*),void (*d)(void*)){t_list*r=0,*n;void*c;while(l){c=f(l->content);n=ft_lstnew(c);if(!n){d(c);ft_lstclear(&r,d);return 0;}ft_lstadd_front(&r,n);l=l->next;}return r;}
