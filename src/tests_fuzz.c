/*
** Tests aleatoires : des milliers d'entrees au hasard, comparees a la libc
** (ou a une reference ecrite a la main). Ca trouve les cas auxquels personne
** n'a pense. La graine est affichee : TESTER_SEED=<graine> rejoue la meme serie.
*/
#include "framework.h"
#include "available.h"
#include "libft.h"
#include "ref.h"
#include "fuzz.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DESC "tests aleatoires (%d appels)"

/* chaine aleatoire faite des caracteres de alpha, longueur 0..maxlen */
static void	rnd_str(char *buf, int maxlen, const char *alpha)
{
	int	len = t_rand_range(0, maxlen);
	int	n = (int)strlen(alpha);

	for (int i = 0; i < len; i++)
		buf[i] = alpha[t_rand_range(0, n - 1)];
	buf[len] = '\0';
}

static int	rnd_int(void)
{
	switch (t_rand_range(0, 5))
	{
		case 0: return (INT_MIN + t_rand_range(0, 3));
		case 1: return (INT_MAX - t_rand_range(0, 3));
		case 2: return (t_rand_range(-20, 20));
		case 3: return (t_rand_range(-100000, 100000));
		default: return ((int)t_rand());
	}
}

#define FAIL_SEED(...) do { t_fail(__VA_ARGS__); \
	t_fail("graine %u : relance avec TESTER_SEED=%u pour rejouer", t_seed(), t_seed()); } while (0)

void	fuzz_atoi(void)
{
#ifdef HAS_ft_atoi
	TEST(DESC, FUZZ_N)
		for (int k = 0; k < FUZZ_N; k++)
		{
			char	s[40];
			char	*end;
			long	v;

			if (k % 3 == 0)
				snprintf(s, sizeof(s), "%.*s%c%d", t_rand_range(0, 3), " \t\n\v\f\r",
					"+- "[t_rand_range(0, 2)], abs(rnd_int() % 1000000000));
			else
				rnd_str(s, 12, " \t\n+-0123456789a\v");
			v = strtol(s, &end, 10);
			if (v < INT_MIN || v > INT_MAX)
				continue ;			/* depassement : comportement indefini, la moulinette ne teste pas */
			if (ft_atoi(s) != atoi(s))
			{
				FAIL_SEED("ft_atoi(%s) : attendu %d, obtenu %d", t_esc(s), atoi(s), ft_atoi(s));
				break ;
			}
		}
#endif
}

void	fuzz_strlcpy(void)
{
#ifdef HAS_ft_strlcpy
	TEST(DESC, FUZZ_N)
		for (int k = 0; k < FUZZ_N; k++)
		{
			char	src[32], a[40], b[40];
			size_t	size = (size_t)t_rand_range(0, 34);
			size_t	ra, rb;

			rnd_str(src, 30, "abcdefgh");
			memset(a, '.', sizeof(a));
			memset(b, '.', sizeof(b));
			rb = ref_strlcpy(b, src, size);
			ra = ft_strlcpy(a, src, size);
			if (ra != rb || memcmp(a, b, sizeof(a)))
			{
				FAIL_SEED("ft_strlcpy(dst, %s, %zu) : retour %zu (attendu %zu)%s",
					t_esc(src), size, ra, rb, memcmp(a, b, sizeof(a)) ? ", dst different" : "");
				break ;
			}
		}
#endif
}

void	fuzz_strlcat(void)
{
#ifdef HAS_ft_strlcat
	TEST(DESC, FUZZ_N)
		for (int k = 0; k < FUZZ_N; k++)
		{
			char	src[24], a[64], b[64];
			size_t	size;
			size_t	ra, rb;

			memset(a, '.', sizeof(a));
			rnd_str(a, 20, "xyz");
			memcpy(b, a, sizeof(a));
			rnd_str(src, 20, "abc");
			size = (size_t)t_rand_range(0, 44);
			rb = ref_strlcat(b, src, size);
			ra = ft_strlcat(a, src, size);
			if (ra != rb || memcmp(a, b, sizeof(a)))
			{
				FAIL_SEED("ft_strlcat(dst de longueur %zu, %s, %zu) : retour %zu (attendu %zu)%s",
					strnlen(a, 64), t_esc(src), size, ra, rb,
					memcmp(a, b, sizeof(a)) ? ", dst different" : "");
				break ;
			}
		}
#endif
}

void	fuzz_strnstr(void)
{
#ifdef HAS_ft_strnstr
	TEST(DESC, FUZZ_N)
		for (int k = 0; k < FUZZ_N; k++)
		{
			char	big[20], little[6];
			size_t	len = (size_t)t_rand_range(0, 18);
			char	*e, *g;

			rnd_str(big, 14, "aab");
			rnd_str(little, 4, "ab");
			e = ref_strnstr(big, little, len);
			g = ft_strnstr(big, little, len);
			if (e != g)
			{
				FAIL_SEED("ft_strnstr(%s, %s, %zu) : attendu %s, obtenu %s", t_esc(big),
					t_esc(little), len, t_ptr(e, big, strlen(big), "big"),
					t_ptr(g, big, strlen(big), "big"));
				break ;
			}
		}
#endif
}

void	fuzz_memmove(void)
{
#ifdef HAS_ft_memmove
	TEST(DESC, FUZZ_N)
		for (int k = 0; k < FUZZ_N; k++)
		{
			unsigned char	a[48], b[48];
			int				so = t_rand_range(0, 31);
			int				d = t_rand_range(0, 31);
			int				n = t_rand_range(0, 16);
			void			*r;

			for (int i = 0; i < 48; i++)
				a[i] = b[i] = (unsigned char)t_rand();
			memmove(b + d, b + so, (size_t)n);
			r = ft_memmove(a + d, a + so, (size_t)n);
			if (r != a + d || memcmp(a, b, sizeof(a)))
			{
				FAIL_SEED("ft_memmove(buf+%d, buf+%d, %d) : %s", d, so, n,
					r != a + d ? "ne renvoie pas dst" : "contenu different de memmove");
				break ;
			}
		}
#endif
}

void	fuzz_cmp(int which)
{
#if defined(HAS_ft_strncmp) || defined(HAS_ft_memcmp)
	TEST(DESC, FUZZ_N)
		for (int k = 0; k < FUZZ_N; k++)
		{
			char	s1[12], s2[12];
			size_t	n = (size_t)t_rand_range(0, 12);

			rnd_str(s1, 8, "ab\x80\xff");
			memcpy(s2, s1, sizeof(s1));
			if (t_rand_range(0, 1))
				rnd_str(s2, 8, "ab\x80\xff");
# ifdef HAS_ft_strncmp
			if (which == 0 && t_sign(ft_strncmp(s1, s2, n)) != t_sign(strncmp(s1, s2, n)))
			{
				FAIL_SEED("ft_strncmp(%s, %s, %zu) : attendu %d, obtenu %d", t_esc(s1),
					t_esc(s2), n, strncmp(s1, s2, n), ft_strncmp(s1, s2, n));
				break ;
			}
# endif
# ifdef HAS_ft_memcmp
			if (n > 9)
				n = 9;
			if (which == 1 && t_sign(ft_memcmp(s1, s2, n)) != t_sign(memcmp(s1, s2, n)))
			{
				FAIL_SEED("ft_memcmp(%s, %s, %zu) : attendu %d, obtenu %d", t_escn(s1, n),
					t_escn(s2, n), n, memcmp(s1, s2, n), ft_memcmp(s1, s2, n));
				break ;
			}
# endif
		}
#endif
}

void	fuzz_chr(int which)
{
#if defined(HAS_ft_strchr) || defined(HAS_ft_strrchr) || defined(HAS_ft_memchr)
	TEST(DESC, FUZZ_N)
		for (int k = 0; k < FUZZ_N; k++)
		{
			char	s[16];
			int		c = t_rand_range(0, 3) ? "abc\0"[t_rand_range(0, 3)] + 256 * t_rand_range(-1, 1)
				: t_rand_range(-300, 300);
			size_t	len;

			rnd_str(s, 12, "abcd\x80");
			len = strlen(s) + 1;
# ifdef HAS_ft_strchr
			if (which == 0 && ft_strchr(s, c) != strchr(s, c))
			{
				FAIL_SEED("ft_strchr(%s, %d) : attendu %s, obtenu %s", t_esc(s), c,
					t_ptr(strchr(s, c), s, len, "s"), t_ptr(ft_strchr(s, c), s, len, "s"));
				break ;
			}
# endif
# ifdef HAS_ft_strrchr
			if (which == 1 && ft_strrchr(s, c) != strrchr(s, c))
			{
				FAIL_SEED("ft_strrchr(%s, %d) : attendu %s, obtenu %s", t_esc(s), c,
					t_ptr(strrchr(s, c), s, len, "s"), t_ptr(ft_strrchr(s, c), s, len, "s"));
				break ;
			}
# endif
# ifdef HAS_ft_memchr
			if (which == 2 && ft_memchr(s, c, len) != memchr(s, c, len))
			{
				FAIL_SEED("ft_memchr(%s, %d, %zu) : attendu %s, obtenu %s", t_esc(s), c, len,
					t_ptr(memchr(s, c, len), s, len, "s"), t_ptr(ft_memchr(s, c, len), s, len, "s"));
				break ;
			}
# endif
		}
#endif
}

void	fuzz_itoa(void)
{
#ifdef HAS_ft_itoa
	TEST(DESC, FUZZ_N)
		for (int k = 0; k < FUZZ_N; k++)
		{
			int		n = rnd_int();
			char	e[16];
			char	*g;

			snprintf(e, sizeof(e), "%d", n);
			g = ft_itoa(n);
			if (!t_streq(g, e))
			{
				FAIL_SEED("ft_itoa(%d) : attendu %s, obtenu %s", n, t_esc(e), t_esc(g));
				free(g);
				break ;
			}
			free(g);
		}
#endif
}

void	fuzz_substr(void)
{
#ifdef HAS_ft_substr
	TEST(DESC, FUZZ_N)
		for (int k = 0; k < FUZZ_N; k++)
		{
			char			s[16], e[16];
			unsigned int	start = (unsigned int)t_rand_range(0, 14);
			size_t			len = t_rand_range(0, 9) ? (size_t)t_rand_range(0, 14) : SIZE_MAX;
			char			*g;

			rnd_str(s, 12, "abcdef");
			ref_substr(s, start, len, e);
			g = ft_substr(s, start, len);
			if (!t_streq(g, e))
			{
				FAIL_SEED("ft_substr(%s, %u, %zu) : attendu %s, obtenu %s", t_esc(s), start,
					len, t_esc(e), t_esc(g));
				free(g);
				break ;
			}
			free(g);
		}
#endif
}

void	fuzz_strtrim(void)
{
#ifdef HAS_ft_strtrim
	TEST(DESC, FUZZ_N)
		for (int k = 0; k < FUZZ_N; k++)
		{
			char	s[16], set[5], e[16];
			char	*g;

			rnd_str(s, 12, "ab -");
			rnd_str(set, 3, "a -");
			ref_strtrim(s, set, e);
			g = ft_strtrim(s, set);
			if (!t_streq(g, e))
			{
				FAIL_SEED("ft_strtrim(%s, %s) : attendu %s, obtenu %s", t_esc(s), t_esc(set),
					t_esc(e), t_esc(g));
				free(g);
				break ;
			}
			free(g);
		}
#endif
}

void	fuzz_strjoin(void)
{
#ifdef HAS_ft_strjoin
	TEST(DESC, FUZZ_N)
		for (int k = 0; k < FUZZ_N; k++)
		{
			char	a[16], b[16], e[32];
			char	*g;

			rnd_str(a, 12, "abc\n");
			rnd_str(b, 12, "xyz ");
			snprintf(e, sizeof(e), "%s%s", a, b);
			g = ft_strjoin(a, b);
			if (!t_streq(g, e))
			{
				FAIL_SEED("ft_strjoin(%s, %s) : attendu %s, obtenu %s", t_esc(a), t_esc(b),
					t_esc(e), t_esc(g));
				free(g);
				break ;
			}
			free(g);
		}
#endif
}

void	fuzz_split(void)
{
#ifdef HAS_ft_split
	TEST(DESC, FUZZ_N)
		for (int k = 0; k < FUZZ_N; k++)
		{
			char	s[24], buf[24];
			char	*words[24];
			char	c = ",, a"[t_rand_range(0, 3)];
			size_t	n;
			char	**g;
			size_t	i;

			rnd_str(s, 20, "ab, ");
			memcpy(buf, s, sizeof(s));
			n = ref_split(buf, c, words, 24);
			g = ft_split(s, c);
			i = 0;
			while (g && i < n && g[i] && strcmp(g[i], words[i]) == 0)
				i++;
			if (!g || i != n || g[n] != NULL)
			{
				FAIL_SEED("ft_split(%s, '%c') : attendu %zu mot(s), %s", t_esc(s), c, n,
					!g ? "obtenu NULL" : i < n ? "mot different ou manquant" : "trop de mots (pas de NULL final ?)");
				for (size_t j = 0; g && g[j] && j < 30; j++)
					free(g[j]);
				free(g);
				break ;
			}
			for (size_t j = 0; g[j]; j++)
				free(g[j]);
			free(g);
		}
#endif
}
