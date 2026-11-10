#include "framework.h"
#include "available.h"
#include "libft.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fuzz.h"

static void	free_str(void *p)
{
	free(p);
}

/* ================================ substr ============================= */

void	suite_substr(void)
{
#ifdef HAS_ft_substr
	struct { const char *s; unsigned int start; size_t len; const char *exp; } c[] = {
		{"hello", 1, 3, "ell"}, {"hello", 0, 5, "hello"}, {"hello", 0, 100, "hello"},
		{"hello", 1, 100, "ello"}, {"hello", 4, 1, "o"}, {"hello", 5, 2, ""},
		{"hello", 10, 2, ""}, {"hello", 2, 0, ""}, {"", 0, 5, ""},
		{"hello", 0, (size_t)-1, "hello"}, {"hello", UINT_MAX, 3, ""},
	};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_substr(%s, %u, %zu)", t_esc(c[i].s), c[i].start, c[i].len)
		{
			char	*p = ft_substr(c[i].s, c[i].start, c[i].len);

			if (!p && c[i].exp[0] == '\0')
				t_fail("renvoie NULL : il faut renvoyer une chaine vide \"\" allouee");
			else
				CHECK_STR(p, c[i].exp);
			if (p)
				CHECK_ALLOC(strlen(c[i].exp) + 1);
			free(p);
		}
	TEST("ft_substr : protection des malloc")
		FAIL_LOOP(3, ft_substr("hello world", 2, 5), free_str);
	fuzz_substr();
#else
	t_missing();
#endif
}

/* ================================ strjoin ============================ */

void	suite_strjoin(void)
{
#ifdef HAS_ft_strjoin
	struct { const char *a; const char *b; } c[] = {
		{"abc", "def"}, {"", "def"}, {"abc", ""}, {"", ""},
		{"hello ", "world !"}, {"a\nb", "\tc"},
	};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_strjoin(%s, %s)", t_esc(c[i].a), t_esc(c[i].b))
		{
			char	exp[64];
			char	*p;

			snprintf(exp, sizeof(exp), "%s%s", c[i].a, c[i].b);
			p = ft_strjoin(c[i].a, c[i].b);
			CHECK_STR(p, exp);
			if (p)
				CHECK_ALLOC(strlen(exp) + 1);
			free(p);
		}
	TEST("ft_strjoin : protection des malloc")
		FAIL_LOOP(3, ft_strjoin("hello ", "world"), free_str);
	fuzz_strjoin();
#else
	t_missing();
#endif
}

/* ================================ strtrim ============================ */

void	suite_strtrim(void)
{
#ifdef HAS_ft_strtrim
	struct { const char *s; const char *set; const char *exp; } c[] = {
		{"  hello  ", " ", "hello"}, {"xxhelloxx", "x", "hello"},
		{"abcHELLOcba", "abc", "HELLO"}, {"  hello world  ", " ", "hello world"},
		{"xhexllox", "x", "hexllo"}, {"hello", "", "hello"}, {"hello", "xyz", "hello"},
		{"     ", " ", ""}, {"", "abc", ""}, {"", "", ""}, {"aaa", "a", ""},
		{" \t\n hello \n\t ", " \t\n", "hello"},
	};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_strtrim(%s, %s)", t_esc(c[i].s), t_esc(c[i].set))
		{
			char	*p = ft_strtrim(c[i].s, c[i].set);

			if (!p && c[i].exp[0] == '\0')
				t_fail("renvoie NULL : il faut renvoyer une chaine vide \"\" allouee");
			else
				CHECK_STR(p, c[i].exp);
			if (p)
				CHECK_ALLOC(strlen(c[i].exp) + 1);
			free(p);
		}
	TEST("ft_strtrim : protection des malloc")
		FAIL_LOOP(3, ft_strtrim("  hello  ", " "), free_str);
	fuzz_strtrim();
#else
	t_missing();
#endif
}

/* ================================= split ============================= */

static void	free_split(void *p)
{
	char	**tab = p;

	if (!tab)
		return ;
	for (size_t i = 0; tab[i]; i++)
		free(tab[i]);
	free(tab);
}

static const char	*split_desc(char **tab)
{
	static char	buf[512];
	size_t		len;

	if (!tab)
		return ("NULL");
	len = (size_t)snprintf(buf, sizeof(buf), "[");
	for (size_t i = 0; tab[i] && i < 20 && len < sizeof(buf) - 40; i++)
		len += (size_t)snprintf(buf + len, sizeof(buf) - len, "%s%s",
			i ? ", " : "", t_esc(tab[i]));
	snprintf(buf + len, sizeof(buf) - len, "%sNULL]", len > 1 ? ", " : "");
	return (buf);
}

static void	check_split(char **got, const char **exp)
{
	size_t	n;
	size_t	i;

	n = 0;
	while (exp[n])
		n++;
	if (!got)
	{
		t_fail("renvoie NULL, attendu %d mot(s)", (int)n);
		return ;
	}
	i = 0;
	while (i < n && got[i] && strcmp(got[i], exp[i]) == 0)
		i++;
	if (i != n || got[n] != NULL)
	{
		char	expd[512];
		char	*e = (char *)split_desc((char **)exp);

		snprintf(expd, sizeof(expd), "%s", e);
		t_fail("attendu %s", expd);
		t_fail("obtenu  %s", split_desc(got));
	}
}

void	suite_split(void)
{
#ifdef HAS_ft_split
	struct { const char *s; char c; const char *exp[8]; } c[] = {
		{"hello world 42", ' ', {"hello", "world", "42", NULL}},
		{"   hello   world  ", ' ', {"hello", "world", NULL}},
		{"hello", ' ', {"hello", NULL}},
		{"hello", '\0', {"hello", NULL}},
		{"a,b,,c,", ',', {"a", "b", "c", NULL}},
		{",,,a", ',', {"a", NULL}},
		{"", ' ', {NULL}},
		{"     ", ' ', {NULL}},
		{"xxxx", 'x', {NULL}},
		{"  a  ", ' ', {"a", NULL}},
		{"tripouille", 'i', {"tr", "pou", "lle", NULL}},
	};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_split(%s, %s)", t_esc(c[i].s), t_escn(&c[i].c, 1)[1] == '"'
			? "'\\0'" : t_escn(&c[i].c, 1))
		{
			char	**tab = ft_split(c[i].s, c[i].c);

			check_split(tab, c[i].exp);
			free_split(tab);
		}
	TEST("ft_split : protection des malloc (chaque malloc echoue tour a tour)")
		FAIL_LOOP(10, ft_split("aa bb cc dd", ' '), free_split);
	fuzz_split();
#else
	t_missing();
#endif
}

/* ================================= itoa ============================== */

void	suite_itoa(void)
{
#ifdef HAS_ft_itoa
	int	c[] = {0, 1, -1, 9, -9, 10, 42, -42, 100, -100, 123456,
		INT_MAX, INT_MIN, INT_MIN + 1};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_itoa(%d)", c[i])
		{
			char	exp[16];
			char	*p;

			snprintf(exp, sizeof(exp), "%d", c[i]);
			p = ft_itoa(c[i]);
			CHECK_STR(p, exp);
			if (p)
				CHECK_ALLOC(strlen(exp) + 1);
			free(p);
		}
	TEST("ft_itoa : protection des malloc")
		FAIL_LOOP(3, ft_itoa(-12345), free_str);
	fuzz_itoa();
#else
	t_missing();
#endif
}

/* =========================== strmapi / striteri ====================== */

static unsigned int	g_idx_seen[64];
static int			g_idx_n;

static char	f_add_index(unsigned int i, char c)
{
	if (g_idx_n < 64)
		g_idx_seen[g_idx_n++] = i;
	return ((char)(c + (char)i));
}

static char	f_upper(unsigned int i, char c)
{
	(void)i;
	if (c >= 'a' && c <= 'z')
		return ((char)(c - 32));
	return (c);
}

void	suite_strmapi(void)
{
#ifdef HAS_ft_strmapi
	TEST("ft_strmapi(\"hello\", majuscule)")
	{
		char	*p = ft_strmapi("hello", f_upper);

		CHECK_STR(p, "HELLO");
		if (p)
			CHECK_ALLOC(6);
		free(p);
	}
	TEST("ft_strmapi(\"aaaaa\", c + index) : bon index passe a f")
	{
		char	*p;

		g_idx_n = 0;
		p = ft_strmapi("aaaaa", f_add_index);
		CHECK_STR(p, "abcde");
		if (p)
			CHECK_ALLOC(6);
		CHECK(g_idx_n == 5, "f appelee %d fois au lieu de 5", g_idx_n);
		free(p);
	}
	TEST("ft_strmapi(\"\", f) : chaine vide allouee")
	{
		char	*p = ft_strmapi("", f_upper);

		CHECK_STR(p, "");
		if (p)
			CHECK_ALLOC(1);
		free(p);
	}
	TEST("ft_strmapi : protection des malloc")
		FAIL_LOOP(3, ft_strmapi("hello", f_upper), free_str);
#else
	t_missing();
#endif
}

static void	f_iter_upper_even(unsigned int i, char *c)
{
	if (i % 2 == 0 && *c >= 'a' && *c <= 'z')
		*c -= 32;
}

static void	f_iter_index(unsigned int i, char *c)
{
	*c = (char)('0' + i % 10);
}

void	suite_striteri(void)
{
#ifdef HAS_ft_striteri
	TEST("ft_striteri(\"hello world\", majuscule si index pair)")
	{
		char	s[] = "hello world";

		ft_striteri(s, f_iter_upper_even);
		CHECK_STR(s, "HeLlO WoRlD");
	}
	TEST("ft_striteri(\"abcdefghijkl\", index) : bon index et adresse")
	{
		char	s[] = "abcdefghijkl";

		ft_striteri(s, f_iter_index);
		CHECK_STR(s, "012345678901");
	}
	TEST("ft_striteri(\"\", f) ne plante pas")
	{
		char	s[] = "";

		ft_striteri(s, f_iter_index);
		CHECK_STR(s, "");
	}
#else
	t_missing();
#endif
}

/* ================================ put*_fd ============================ */

typedef struct s_putarg
{
	const char	*s;
	int			n;
	char		c;
}	t_putarg;

#ifdef HAS_ft_putchar_fd
static int	w_putchar(int fd, void *a) { ft_putchar_fd(((t_putarg *)a)->c, fd); return (0); }
#endif
#ifdef HAS_ft_putstr_fd
static int	w_putstr(int fd, void *a) { ft_putstr_fd((char *)((t_putarg *)a)->s, fd); return (0); }
#endif
#ifdef HAS_ft_putendl_fd
static int	w_putendl(int fd, void *a) { ft_putendl_fd((char *)((t_putarg *)a)->s, fd); return (0); }
#endif
#ifdef HAS_ft_putnbr_fd
static int	w_putnbr(int fd, void *a) { ft_putnbr_fd(((t_putarg *)a)->n, fd); return (0); }
#endif

#define CHECK_OUT(fn, arg, exp, explen) do { \
	char out_[256]; size_t n_ = t_capture(fn, arg, out_, sizeof(out_)); \
	if (n_ != (size_t)(explen) || memcmp(out_, exp, explen) != 0) \
		t_fail("a ecrit %s (%zu octets), attendu %s (%zu octets)", \
			t_escn(out_, n_), n_, t_escn(exp, explen), (size_t)(explen)); \
} while (0)

void	suite_putchar_fd(void)
{
#ifdef HAS_ft_putchar_fd
	char	cs[] = {'a', 'Z', '\n', ' ', '~'};

	for (size_t i = 0; i < sizeof(cs); i++)
		TEST("ft_putchar_fd(%s, fd)", t_escn(&cs[i], 1))
		{
			t_putarg	a = {.c = cs[i]};

			CHECK_OUT(w_putchar, &a, &cs[i], 1);
		}
#else
	t_missing();
#endif
}

void	suite_putstr_fd(void)
{
#ifdef HAS_ft_putstr_fd
	const char	*c[] = {"hello", "", "hello\nworld", "42 !"};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_putstr_fd(%s, fd)", t_esc(c[i]))
		{
			t_putarg	a = {.s = c[i]};

			CHECK_OUT(w_putstr, &a, c[i], strlen(c[i]));
		}
#else
	t_missing();
#endif
}

void	suite_putendl_fd(void)
{
#ifdef HAS_ft_putendl_fd
	const char	*c[] = {"hello", "", "a b c"};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_putendl_fd(%s, fd)", t_esc(c[i]))
		{
			t_putarg	a = {.s = c[i]};
			char		exp[64];

			snprintf(exp, sizeof(exp), "%s\n", c[i]);
			CHECK_OUT(w_putendl, &a, exp, strlen(exp));
		}
#else
	t_missing();
#endif
}

void	suite_putnbr_fd(void)
{
#ifdef HAS_ft_putnbr_fd
	int	c[] = {0, 5, -5, 42, -42, 100, 1000000, INT_MAX, INT_MIN};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_putnbr_fd(%d, fd)", c[i])
		{
			t_putarg	a = {.n = c[i]};
			char		exp[16];

			snprintf(exp, sizeof(exp), "%d", c[i]);
			CHECK_OUT(w_putnbr, &a, exp, strlen(exp));
		}
#else
	t_missing();
#endif
}
