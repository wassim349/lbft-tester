/* conseil attendu : faute de frappe : c'est return, pas retrun */
#include "libft.h"
#include <stdlib.h>

char	*ft_strjoin(char const *s1, char const *s2)
{
	char	*p;
	size_t	a;
	size_t	b;

	a = ft_strlen(s1);
	b = ft_strlen(s2);
	p = malloc(a + b + 1);
	if (!p)
		retrun (NULL);
	ft_memcpy(p, s1, a);
	ft_memcpy(p + a, s2, b + 1);
	return (p);
}
