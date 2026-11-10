#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
t_list *ft_lstmap(t_list *l,void *(*f)(void*),void (*d)(void*)){t_list*r=0,*n;while(l){n=ft_lstnew(f(l->content));if(!n){ft_lstclear(&r,d);return 0;}ft_lstadd_back(&r,n);l=l->next;}return r;}
