/* conseil attendu : ces fonctions reçoivent UN caractère */
/* erreur classique : prototype d'un exercice de piscine, sans inclure libft.h */
int	ft_isprint(char *str)
{
	int	i;

	i = 0;
	while (str[i])
	{
		if (str[i] < 32 || str[i] > 126)
			return (0);
		i++;
	}
	return (1);
}
