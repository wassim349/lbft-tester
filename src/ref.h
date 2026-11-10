#ifndef REF_H
# define REF_H

/*
** Versions de reference ecrites a la main (strlcpy/strlcat n'existent pas
** partout dans la glibc, et split/strtrim/substr n'existent pas du tout).
*/

# include <stdlib.h>
# include <string.h>

static inline size_t	ref_strlcpy(char *dst, const char *src, size_t size)
{
	size_t	len = strlen(src);

	if (size)
	{
		size_t	n = len < size - 1 ? len : size - 1;
		memcpy(dst, src, n);
		dst[n] = '\0';
	}
	return (len);
}

static inline size_t	ref_strlcat(char *dst, const char *src, size_t size)
{
	size_t	dlen = strnlen(dst, size);
	size_t	slen = strlen(src);
	size_t	room;

	if (dlen == size)
		return (size + slen);
	room = size - dlen - 1;
	if (slen < room)
		room = slen;
	memcpy(dst + dlen, src, room);
	dst[dlen + room] = '\0';
	return (dlen + slen);
}

static inline char	*ref_strnstr(const char *big, const char *little, size_t len)
{
	size_t	llen = strlen(little);
	size_t	i;

	if (!llen)
		return ((char *)big);
	i = 0;
	while (big[i] && i + llen <= len)
	{
		if (strncmp(big + i, little, llen) == 0)
			return ((char *)big + i);
		i++;
	}
	return (NULL);
}

/* ecrit dans out (taille >= strlen(s) + 1) */
static inline void	ref_substr(const char *s, unsigned int start, size_t len, char *out)
{
	size_t	l = strlen(s);

	if (start >= l)
	{
		out[0] = '\0';
		return ;
	}
	if (len > l - start)
		len = l - start;
	memcpy(out, s + start, len);
	out[len] = '\0';
}

static inline void	ref_strtrim(const char *s, const char *set, char *out)
{
	size_t	a = 0;
	size_t	b = strlen(s);

	while (s[a] && strchr(set, s[a]))
		a++;
	while (b > a && strchr(set, s[b - 1]))
		b--;
	memcpy(out, s + a, b - a);
	out[b - a] = '\0';
}

/* renvoie le nombre de mots ; words[i] pointe dans buf (modifie) */
static inline size_t	ref_split(char *buf, char c, char **words, size_t max)
{
	size_t	n = 0;
	size_t	i = 0;

	while (buf[i])
	{
		while (buf[i] && buf[i] == c)
			buf[i++] = '\0';
		if (buf[i] && n < max)
			words[n++] = buf + i;
		while (buf[i] && buf[i] != c)
			i++;
	}
	return (n);
}

#endif
