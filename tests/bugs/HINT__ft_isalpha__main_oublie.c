/* conseil attendu : il ne doit pas y en avoir dans le rendu */
#include "libft.h"

int	ft_isalpha(int c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

int	main(void)
{
	return (!ft_isalpha('a'));
}
