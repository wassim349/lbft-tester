/* conseil attendu : qu'est-ce que lstdelone doit libérer */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
void ft_lstdelone(t_list *l,void (*d)(void*)){d(l->content);}
