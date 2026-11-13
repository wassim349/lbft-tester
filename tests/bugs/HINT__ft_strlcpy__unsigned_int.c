/* conseil attendu : regarde le type des tailles */
/* prototype de la piscine, sans inclure libft.h : les tests passent, mais c'est hors sujet */
static unsigned int	len_of(char *s)
{
	unsigned int	i;

	i = 0;
	while (s[i])
		i++;
	return (i);
}

unsigned int	ft_strlcpy(char *dest, char *src, unsigned int size)
{
	unsigned int	i;

	i = 0;
	while (size > 0 && i < size - 1 && src[i])
	{
		dest[i] = src[i];
		i++;
	}
	if (size > 0)
		dest[i] = '\0';
	return (len_of(src));
}
