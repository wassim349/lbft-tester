/* conseil attendu : malloc et free sont déclarées dans <stdlib.h> */
#include "libft.h"

char	*ft_strdup(const char *s)
{
	char	*p;
	size_t	n;

	n = ft_strlen(s) + 1;
	p = malloc(n);
	if (!p)
		return (NULL);
	ft_memcpy(p, s, n);
	return (p);
}
