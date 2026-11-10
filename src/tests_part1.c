#include "framework.h"
#include "available.h"
#include "libft.h"
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ref.h"
#include "fuzz.h"

/* ================================ ctype ============================== */

static void	test_ctype(const char *name, int (*ft)(int), int (*ref)(int), int strict01)
{
	TEST("%s(c) pour c de 0 a 127", name)
	{
		for (int c = 0; c <= 127; c++)
			if ((ft(c) != 0) != (ref(c) != 0))
			{
				t_fail("%s(%d) %s : attendu %s, obtenu %d", name, c,
					t_escn((char[]){(char)c, 0}, 1), ref(c) ? TR("vrai") : TR("faux (0)"), ft(c));
				break ;
			}
	}
	TEST("%s(c) pour c de 128 a 255 et EOF (-1)", name)
	{
		for (int c = -1; c <= 255; c++)
		{
			if (c == 0)
				c = 128;
			if ((ft(c) != 0) != (ref(c) != 0))
			{
				t_fail("%s(%d) : attendu %s, obtenu %d", name, c,
					ref(c) ? TR("vrai") : TR("faux (0)"), ft(c));
				break ;
			}
		}
	}
	if (strict01)
		TEST_SOFT("%s renvoie 1 quand c'est vrai (conseille, pas bloquant)", name)
		{
			for (int c = -1; c <= 255; c++)
				if (ft(c) != 0 && ft(c) != 1)
				{
					t_fail("%s(%d) renvoie %d : la libc renvoie juste \"non zero\", "
						"mais certains testeurs exigent 1", name, c, ft(c));
					break ;
				}
		}
	else
		TEST("%s(c) pour c de -1 a 255, valeur exacte", name)
		{
			for (int c = -1; c <= 255; c++)
				if (ft(c) != ref(c))
				{
					t_fail("%s(%d) : attendu %d, obtenu %d", name, c, ref(c), ft(c));
					break ;
				}
		}
}

void	suite_isalpha(void)
{
#ifdef HAS_ft_isalpha
	test_ctype("ft_isalpha", ft_isalpha, isalpha, 1);
#else
	t_missing();
#endif
}

void	suite_isdigit(void)
{
#ifdef HAS_ft_isdigit
	test_ctype("ft_isdigit", ft_isdigit, isdigit, 1);
#else
	t_missing();
#endif
}

void	suite_isalnum(void)
{
#ifdef HAS_ft_isalnum
	test_ctype("ft_isalnum", ft_isalnum, isalnum, 1);
#else
	t_missing();
#endif
}

void	suite_isascii(void)
{
#ifdef HAS_ft_isascii
	test_ctype("ft_isascii", ft_isascii, isascii, 1);
#else
	t_missing();
#endif
}

void	suite_isprint(void)
{
#ifdef HAS_ft_isprint
	test_ctype("ft_isprint", ft_isprint, isprint, 1);
#else
	t_missing();
#endif
}

void	suite_toupper(void)
{
#ifdef HAS_ft_toupper
	test_ctype("ft_toupper", ft_toupper, toupper, 0);
#else
	t_missing();
#endif
}

void	suite_tolower(void)
{
#ifdef HAS_ft_tolower
	test_ctype("ft_tolower", ft_tolower, tolower, 0);
#else
	t_missing();
#endif
}

/* =============================== strlen ============================== */

void	suite_strlen(void)
{
#ifdef HAS_ft_strlen
	static char	big[10001];
	const char	*cases[] = {"", "a", "hello", "hello\0world", "\200\377abc", " \t\n"};

	for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); i++)
		TEST("ft_strlen(%s)", t_esc(cases[i]))
			CHECK_INT(ft_strlen(cases[i]), strlen(cases[i]));
	memset(big, 'x', 10000);
	TEST("ft_strlen(chaine de 10000 caracteres)")
		CHECK_INT(ft_strlen(big), 10000);
#else
	t_missing();
#endif
}

/* ========================== memset / bzero =========================== */

void	suite_memset(void)
{
#ifdef HAS_ft_memset
	TEST("ft_memset(buf, 'A', 5) sur un buffer de 10")
	{
		char	a[10], b[10];

		memset(a, '.', 10);
		memset(b, '.', 10);
		memset(b, 'A', 5);
		ft_memset(a, 'A', 5);
		CHECK_MEM(a, b, 10, "buf");
	}
	TEST("ft_memset(buf, 0, 10)")
	{
		char	a[10];

		memset(a, 'z', 10);
		ft_memset(a, 0, 10);
		CHECK_MEM(a, "\0\0\0\0\0\0\0\0\0\0", 10, "buf");
	}
	TEST("ft_memset(buf, 300, 4) : c converti en unsigned char (300 → 44)")
	{
		unsigned char	a[6], b[6];

		memset(a, 0, 6);
		memset(b, 0, 6);
		memset(b, 300, 4);
		ft_memset(a, 300, 4);
		CHECK_MEM(a, b, 6, "buf");
	}
	TEST("ft_memset(buf, 'x', 0) ne touche a rien")
	{
		char	a[4] = "abc";

		ft_memset(a, 'x', 0);
		CHECK_MEM(a, "abc", 4, "buf");
	}
	TEST("ft_memset renvoie le pointeur de depart")
	{
		char	a[4];

		CHECK_PTR(ft_memset(a, 'x', 3), a, a, 4, "buf");
	}
#else
	t_missing();
#endif
}

void	suite_bzero(void)
{
#ifdef HAS_ft_bzero
	TEST("ft_bzero(buf, 5) sur un buffer de 10")
	{
		char	a[10];
		char	b[10];

		memset(a, 'z', 10);
		memset(b, 'z', 10);
		memset(b, 0, 5);
		ft_bzero(a, 5);
		CHECK_MEM(a, b, 10, "buf");
	}
	TEST("ft_bzero(buf, 0) ne touche a rien")
	{
		char	a[4] = "abc";

		ft_bzero(a, 0);
		CHECK_MEM(a, "abc", 4, "buf");
	}
#else
	t_missing();
#endif
}

/* ========================== memcpy / memmove ========================= */

void	suite_memcpy(void)
{
#ifdef HAS_ft_memcpy
	TEST("ft_memcpy(dst, \"hello\", 6)")
	{
		char	d[10];

		memset(d, '.', 10);
		ft_memcpy(d, "hello", 6);
		CHECK_MEM(d, "hello\0....", 10, "dst");
	}
	TEST("ft_memcpy copie aussi les \\0 (donnees binaires)")
	{
		char	d[6];

		memset(d, '.', 6);
		ft_memcpy(d, "a\0b\0c", 5);
		CHECK_MEM(d, "a\0b\0c.", 6, "dst");
	}
	TEST("ft_memcpy(dst, src, 0) ne touche a rien")
	{
		char	d[4] = "abc";

		ft_memcpy(d, "xyz", 0);
		CHECK_MEM(d, "abc", 4, "dst");
	}
	TEST("ft_memcpy renvoie dst")
	{
		char	d[4];

		CHECK_PTR(ft_memcpy(d, "ab", 3), d, d, 4, "dst");
	}
	TEST_SOFT("ft_memcpy(NULL, NULL, 3) ne plante pas (teste par certains correcteurs)")
		CHECK(ft_memcpy(NULL, NULL, 3) == NULL, "devrait renvoyer NULL");
#else
	t_missing();
#endif
}

void	suite_memmove(void)
{
#ifdef HAS_ft_memmove
	TEST("ft_memmove sans chevauchement")
	{
		char	d[8];

		memset(d, '.', 8);
		ft_memmove(d, "hello", 6);
		CHECK_MEM(d, "hello\0..", 8, "dst");
	}
	TEST("ft_memmove avec chevauchement, dst apres src : (s+2, s, 5)")
	{
		char	a[10] = "abcdefgh";
		char	b[10] = "abcdefgh";

		memmove(b + 2, b, 5);
		ft_memmove(a + 2, a, 5);
		CHECK_MEM(a, b, 10, "s");
	}
	TEST("ft_memmove avec chevauchement, dst avant src : (s, s+2, 5)")
	{
		char	a[10] = "abcdefgh";
		char	b[10] = "abcdefgh";

		memmove(b, b + 2, 5);
		ft_memmove(a, a + 2, 5);
		CHECK_MEM(a, b, 10, "s");
	}
	TEST("ft_memmove(s, s, 5) : meme pointeur")
	{
		char	a[6] = "hello";

		ft_memmove(a, a, 5);
		CHECK_MEM(a, "hello", 6, "s");
	}
	TEST("ft_memmove(dst, src, 0) ne touche a rien")
	{
		char	a[4] = "abc";

		ft_memmove(a, "xyz", 0);
		CHECK_MEM(a, "abc", 4, "dst");
	}
	TEST("ft_memmove renvoie dst")
	{
		char	a[8] = "abcdef";

		CHECK_PTR(ft_memmove(a + 1, a, 3), a + 1, a, 8, "s");
	}
	TEST_SOFT("ft_memmove(NULL, NULL, 3) ne plante pas (teste par certains correcteurs)")
		CHECK(ft_memmove(NULL, NULL, 3) == NULL, "devrait renvoyer NULL");
	fuzz_memmove();
#else
	t_missing();
#endif
}

/* ========================== strlcpy / strlcat ======================== */

void	suite_strlcpy(void)
{
#ifdef HAS_ft_strlcpy
	struct { const char *src; size_t size; } c[] = {
		{"hello", 10}, {"hello", 6}, {"hello", 5}, {"hello", 3},
		{"hello", 1}, {"hello", 0}, {"", 10}, {"", 0},
	};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_strlcpy(dst[12], %s, %zu)", t_esc(c[i].src), c[i].size)
		{
			char	a[12], b[12];

			memset(a, '.', 12);
			memset(b, '.', 12);
			size_t	exp = ref_strlcpy(b, c[i].src, c[i].size);
			CHECK_INT(ft_strlcpy(a, c[i].src, c[i].size), exp);
			CHECK_MEM(a, b, 12, "dst");
		}
	fuzz_strlcpy();
#else
	t_missing();
#endif
}

void	suite_strlcat(void)
{
#ifdef HAS_ft_strlcat
	struct { const char *dst; const char *src; size_t size; } c[] = {
		{"abc", "def", 12}, {"abc", "def", 7}, {"abc", "def", 6},
		{"abc", "def", 4}, {"abc", "def", 3}, {"abc", "def", 2},
		{"abc", "def", 0}, {"", "hello", 12}, {"", "hello", 3},
		{"abc", "", 12}, {"abcdefgh", "xy", 5},
	};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_strlcat(dst=%s, %s, %zu)", t_esc(c[i].dst), t_esc(c[i].src), c[i].size)
		{
			char	a[16], b[16];

			memset(a, '.', 16);
			memset(b, '.', 16);
			strcpy(a, c[i].dst);
			strcpy(b, c[i].dst);
			size_t	exp = ref_strlcat(b, c[i].src, c[i].size);
			CHECK_INT(ft_strlcat(a, c[i].src, c[i].size), exp);
			CHECK_MEM(a, b, 16, "dst");
		}
	fuzz_strlcat();
#else
	t_missing();
#endif
}

/* ========================= strchr / strrchr ========================== */

static void	test_chr(const char *name, char *(*ft)(const char *, int),
				char *(*ref)(const char *, int))
{
	const char	*s = "hello world";
	int			cs[] = {'h', 'l', 'o', 'd', 'z', '\0', 'e' + 256, 'w' - 256};

	for (size_t i = 0; i < sizeof(cs) / sizeof(*cs); i++)
		TEST("%s(%s, %d)", name, t_esc(s), cs[i])
			CHECK_PTR(ft(s, cs[i]), ref(s, cs[i]), s, 11, "s");
	TEST("%s(\"\", 'a') et %s(\"\", '\\0')", name, name)
	{
		const char	*e = "";

		CHECK_PTR(ft(e, 'a'), ref(e, 'a'), e, 0, "s");
		CHECK_PTR(ft(e, 0), ref(e, 0), e, 0, "s");
	}
}

void	suite_strchr(void)
{
#ifdef HAS_ft_strchr
	test_chr("ft_strchr", (char *(*)(const char *, int))ft_strchr,
		(char *(*)(const char *, int))strchr);
	fuzz_chr(0);
#else
	t_missing();
#endif
}

void	suite_strrchr(void)
{
#ifdef HAS_ft_strrchr
	test_chr("ft_strrchr", (char *(*)(const char *, int))ft_strrchr,
		(char *(*)(const char *, int))strrchr);
	fuzz_chr(1);
#else
	t_missing();
#endif
}

/* ============================== strncmp ============================== */

void	suite_strncmp(void)
{
#ifdef HAS_ft_strncmp
	struct { const char *a; const char *b; size_t n; } c[] = {
		{"abc", "abc", 3}, {"abc", "abd", 3}, {"abd", "abc", 3},
		{"abc", "abd", 2}, {"abc", "abcdef", 6}, {"abcdef", "abc", 6},
		{"abc", "xyz", 0}, {"", "", 5}, {"", "a", 1}, {"a", "", 1},
		{"\200", "\0", 1}, {"\0", "\200", 1}, {"ab\0x", "ab\0y", 4},
		{"hello", "hello", 100},
	};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_strncmp(%s, %s, %zu)", t_esc(c[i].a), t_esc(c[i].b), c[i].n)
			CHECK_SIGN(ft_strncmp(c[i].a, c[i].b, c[i].n), strncmp(c[i].a, c[i].b, c[i].n));
	fuzz_cmp(0);
#else
	t_missing();
#endif
}

/* ========================== memchr / memcmp ========================== */

void	suite_memchr(void)
{
#ifdef HAS_ft_memchr
	const char	s[] = "abc\0def\310";
	struct { int c; size_t n; } c[] = {
		{'a', 9}, {'c', 9}, {'d', 9}, {'\0', 9}, {'z', 9},
		{'d', 3}, {'a', 0}, {200, 9}, {'b' + 256, 9},
	};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_memchr(\"abc\\0def\\310\", %d, %zu)", c[i].c, c[i].n)
			CHECK_PTR(ft_memchr(s, c[i].c, c[i].n), memchr(s, c[i].c, c[i].n), s, 9, "s");
	fuzz_chr(2);
#else
	t_missing();
#endif
}

void	suite_memcmp(void)
{
#ifdef HAS_ft_memcmp
	struct { const char *a; const char *b; size_t n; } c[] = {
		{"abc", "abc", 3}, {"abc", "abd", 3}, {"abd", "abc", 3},
		{"abc", "abd", 2}, {"abc", "xyz", 0}, {"a\0b", "a\0c", 3},
		{"\200", "\0", 1}, {"\0", "\200", 1}, {"\377", "\001", 1},
	};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_memcmp(%s, %s, %zu)", t_escn(c[i].a, c[i].n), t_escn(c[i].b, c[i].n), c[i].n)
			CHECK_SIGN(ft_memcmp(c[i].a, c[i].b, c[i].n), memcmp(c[i].a, c[i].b, c[i].n));
	fuzz_cmp(1);
#else
	t_missing();
#endif
}

/* ============================== strnstr ============================== */

void	suite_strnstr(void)
{
#ifdef HAS_ft_strnstr
	const char	*big = "hello world, hello 42";
	struct { const char *little; size_t len; } c[] = {
		{"world", 30}, {"hello", 30}, {"42", 30}, {"42", 20},
		{"42", 21}, {"world", 10}, {"world", 11}, {"", 30}, {"", 0},
		{"xyz", 30}, {"hello world, hello 42 !", 100}, {"o", 5},
		{"h", 0}, {"lo w", 6}, {"world", (size_t)-1},
	};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_strnstr(%s, %s, %zu)", t_esc(big), t_esc(c[i].little), c[i].len)
			CHECK_PTR(ft_strnstr(big, c[i].little, c[i].len),
				ref_strnstr(big, c[i].little, c[i].len), big, 21, "big");
	TEST("ft_strnstr(\"\", \"\", 0) et (\"\", \"a\", 5)")
	{
		const char	*e = "";

		CHECK_PTR(ft_strnstr(e, "", 0), e, e, 0, "big");
		CHECK_PTR(ft_strnstr(e, "a", 5), NULL, e, 0, "big");
	}
	fuzz_strnstr();
#else
	t_missing();
#endif
}

/* ================================ atoi =============================== */

void	suite_atoi(void)
{
#ifdef HAS_ft_atoi
	const char	*c[] = {
		"42", "-42", "+42", "0", "-0", "00042", "   42", "\t\n\v\f\r 42",
		"--42", "+-42", "-+42", "++42", " - 42", "42abc", "4 2", "abc42", "",
		"2147483647", "-2147483648", "  +2147483647xyz", "\200 42", ("\033" "42"), "-", "+", "  -0", "-+0",
	};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_atoi(%s)", t_esc(c[i]))
			CHECK_INT(ft_atoi(c[i]), atoi(c[i]));
	fuzz_atoi();
#else
	t_missing();
#endif
}

/* =========================== calloc / strdup ========================= */

void	suite_calloc(void)
{
#ifdef HAS_ft_calloc
	TEST("ft_calloc(5, 4) : 20 octets tous a zero")
	{
		unsigned char	*p = ft_calloc(5, 4);

		CHECK(p != NULL, "renvoie NULL");
		if (p)
		{
			CHECK_ALLOC(20);
			for (int i = 0; i < 20; i++)
				if (p[i])
				{
					t_fail("octet [%d] vaut 0x%02x au lieu de 0", i, p[i]);
					break ;
				}
		}
		free(p);
	}
	TEST("ft_calloc(1, 1000) tout a zero")
	{
		unsigned char	*p = ft_calloc(1, 1000);

		CHECK(p != NULL, "renvoie NULL");
		for (int i = 0; p && i < 1000; i++)
			if (p[i])
			{
				t_fail("octet [%d] vaut 0x%02x au lieu de 0", i, p[i]);
				break ;
			}
		free(p);
	}
	TEST("ft_calloc(0, 0) : pointeur unique qu'on peut free (regle du sujet)")
	{
		void	*p = ft_calloc(0, 0);

		CHECK(p != NULL, "renvoie NULL : le sujet impose un pointeur valide qu'on peut free");
		free(p);
	}
	TEST("ft_calloc(0, 5) et ft_calloc(5, 0) : pointeurs valides")
	{
		void	*p = ft_calloc(0, 5);
		void	*q = ft_calloc(5, 0);

		CHECK(p != NULL, "ft_calloc(0, 5) renvoie NULL");
		CHECK(q != NULL, "ft_calloc(5, 0) renvoie NULL");
		free(p);
		free(q);
	}
	TEST("ft_calloc(SIZE_MAX / 2 + 2, 2) : la multiplication deborde → NULL")
	{
		void	*p = ft_calloc(SIZE_MAX / 2 + 2, 2);

		CHECK(p == NULL, "nmemb * size deborde et vaut 2 : ta fonction alloue un "
			"petit bloc au lieu de renvoyer NULL (comme le vrai calloc)");
		free(p);
	}
	TEST("ft_calloc quand malloc echoue → NULL")
	{
		hk_fail_at(1);
		void	*p = ft_calloc(4, 4);

		CHECK(p == NULL, "malloc a echoue mais la fonction ne renvoie pas NULL");
		if (p && !hk_fail_triggered())
			t_fail("n'a pas appele malloc ?");
		free(p);
	}
#else
	t_missing();
#endif
}

void	suite_strdup(void)
{
#ifdef HAS_ft_strdup
	const char	*c[] = {"hello", "", "a", "hello\nworld\t42", "\200\377"};

	for (size_t i = 0; i < sizeof(c) / sizeof(*c); i++)
		TEST("ft_strdup(%s)", t_esc(c[i]))
		{
			char	*p = ft_strdup(c[i]);

			CHECK_STR(p, c[i]);
			if (p)
				CHECK_ALLOC(strlen(c[i]) + 1);
			CHECK(p != c[i], "renvoie le meme pointeur au lieu d'une copie");
			free(p);
		}
	TEST("ft_strdup quand malloc echoue → NULL")
	{
		hk_fail_at(1);
		char	*p = ft_strdup("hello");

		CHECK(p == NULL, "malloc a echoue mais la fonction ne renvoie pas NULL");
		free(p);
	}
#else
	t_missing();
#endif
}
