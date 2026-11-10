#include "framework.h"
#include "available.h"
#include "libft.h"
#include <stdlib.h>
#include <string.h>

#ifdef HAS_ft_lstnew

/* ---- outils : construire une liste sans dependre de ta libft ---- */

static t_list	*mk(void *content)
{
	t_list	*n = hk_nofail_malloc(sizeof(t_list));

	n->content = content;
	n->next = NULL;
	return (n);
}

static t_list	*mk_list3(const char *a, const char *b, const char *c)
{
	t_list	*l = mk((void *)a);

	l->next = mk((void *)b);
	l->next->next = mk((void *)c);
	return (l);
}

static char	*dup_nofail(const char *s)
{
	char	*p = hk_nofail_malloc(strlen(s) + 1);

	strcpy(p, s);
	return (p);
}

static void	free_nodes(t_list *l)
{
	t_list	*next;

	while (l)
	{
		next = l->next;
		free(l);
		l = next;
	}
}


/* liste de n maillons, content = &g_vals[i] */
static int	g_vals[1000];

static t_list	*mk_list_n(int n)
{
	t_list	*head = NULL;
	t_list	*last = NULL;

	for (int i = 0; i < n; i++)
	{
		t_list	*node = mk(&g_vals[i]);

		g_vals[i] = i;
		if (last)
			last->next = node;
		else
			head = node;
		last = node;
	}
	return (head);
}

/* verifie que la liste contient exactement exp[0..n-1] (dans l'ordre) */
static int	list_matches(t_list *l, const int *exp, int n, const char *name)
{
	for (int i = 0; i < n; i++, l = l->next)
	{
		if (!l)
			return (t_fail("%s : la liste s'arrete au maillon %d au lieu de %d", name, i, n), 0);
		if (*(int *)l->content != exp[i])
			return (t_fail("%s : maillon %d contient %d au lieu de %d", name, i,
				*(int *)l->content, exp[i]), 0);
	}
	if (l)
		return (t_fail("%s : la liste est plus longue que %d maillons", name, n), 0);
	return (1);
}

static int	g_del_calls;

static void	del_count(void *content)
{
	g_del_calls++;
	free(content);
}

static void	del_nothing(void *content)
{
	(void)content;
	g_del_calls++;
}

#endif

void	suite_lstnew(void)
{
#ifdef HAS_ft_lstnew
	TEST("ft_lstnew(\"hello\") : content et next")
	{
		char	*s = "hello";
		t_list	*n = ft_lstnew(s);

		CHECK(n != NULL, "renvoie NULL");
		if (n)
		{
			CHECK(n->content == s, "content n'est pas le pointeur donne");
			CHECK(n->next == NULL, "next n'est pas initialise a NULL");
			CHECK_ALLOC(sizeof(t_list));
		}
		free(n);
	}
	TEST("ft_lstnew(NULL) : content NULL autorise")
	{
		t_list	*n = ft_lstnew(NULL);

		CHECK(n != NULL, "renvoie NULL");
		if (n)
		{
			CHECK(n->content == NULL, "content devrait etre NULL");
			CHECK(n->next == NULL, "next n'est pas initialise a NULL");
		}
		free(n);
	}
	TEST("ft_lstnew quand malloc echoue → NULL")
	{
		hk_fail_at(1);
		t_list	*n = ft_lstnew("x");

		CHECK(n == NULL, "malloc a echoue mais la fonction ne renvoie pas NULL");
	}
#else
	t_missing();
#endif
}

void	suite_lstadd_front(void)
{
#if defined(HAS_ft_lstadd_front) && defined(HAS_ft_lstnew)
	TEST("ft_lstadd_front sur une liste vide (*lst == NULL)")
	{
		t_list	*lst = NULL;
		t_list	*n = mk("a");

		ft_lstadd_front(&lst, n);
		CHECK(lst == n, "*lst ne pointe pas sur le nouveau maillon");
		CHECK(n->next == NULL, "new->next devrait rester NULL");
		free_nodes(lst);
	}
	TEST("ft_lstadd_front sur [b, c]")
	{
		t_list	*lst = mk("b");
		t_list	*old = lst;
		t_list	*n = mk("a");

		lst->next = mk("c");
		ft_lstadd_front(&lst, n);
		CHECK(lst == n, "*lst ne pointe pas sur le nouveau maillon");
		CHECK(n->next == old, "new->next ne pointe pas sur l'ancienne tete");
		free_nodes(lst);
	}
	TEST("ft_lstadd_front 3 fois sur une liste vide : ordre inverse")
	{
		t_list		*lst = NULL;
		const int	exp[] = {2, 1, 0};

		for (int i = 0; i < 3; i++)
		{
			g_vals[i] = i;
			ft_lstadd_front(&lst, mk(&g_vals[i]));
		}
		list_matches(lst, exp, 3, "liste");
		free_nodes(lst);
	}
#else
	t_missing();
#endif
}

void	suite_lstsize(void)
{
#if defined(HAS_ft_lstsize) && defined(HAS_ft_lstnew)
	TEST("ft_lstsize(NULL) == 0")
		CHECK_INT(ft_lstsize(NULL), 0);
	TEST("ft_lstsize(liste de 1)")
	{
		t_list	*l = mk("a");

		CHECK_INT(ft_lstsize(l), 1);
		free_nodes(l);
	}
	TEST("ft_lstsize(liste de 3)")
	{
		t_list	*l = mk_list3("a", "b", "c");

		CHECK_INT(ft_lstsize(l), 3);
		free_nodes(l);
	}
	TEST("ft_lstsize(liste de 1000)")
	{
		t_list	*l = mk_list_n(1000);

		CHECK_INT(ft_lstsize(l), 1000);
		free_nodes(l);
	}
#else
	t_missing();
#endif
}

void	suite_lstlast(void)
{
#if defined(HAS_ft_lstlast) && defined(HAS_ft_lstnew)
	TEST("ft_lstlast(NULL) == NULL")
		CHECK(ft_lstlast(NULL) == NULL, "devrait renvoyer NULL");
	TEST("ft_lstlast(liste de 1) renvoie ce maillon")
	{
		t_list	*l = mk("a");

		CHECK(ft_lstlast(l) == l, "ne renvoie pas le seul maillon");
		free_nodes(l);
	}
	TEST("ft_lstlast(liste de 3) renvoie le 3e")
	{
		t_list	*l = mk_list3("a", "b", "c");

		CHECK(ft_lstlast(l) == l->next->next, "ne renvoie pas le dernier maillon");
		free_nodes(l);
	}
	TEST("ft_lstlast(liste de 1000) renvoie le 1000e")
	{
		t_list	*l = mk_list_n(1000);
		t_list	*e = l;

		while (e->next)
			e = e->next;
		CHECK(ft_lstlast(l) == e, "ne renvoie pas le dernier maillon");
		free_nodes(l);
	}
#else
	t_missing();
#endif
}

void	suite_lstadd_back(void)
{
#if defined(HAS_ft_lstadd_back) && defined(HAS_ft_lstnew)
	TEST("ft_lstadd_back sur une liste vide (*lst == NULL)")
	{
		t_list	*lst = NULL;
		t_list	*n = mk("a");

		ft_lstadd_back(&lst, n);
		CHECK(lst == n, "*lst devrait pointer sur le nouveau maillon");
		free_nodes(lst);
	}
	TEST("ft_lstadd_back sur [a, b, c]")
	{
		t_list	*lst = mk_list3("a", "b", "c");
		t_list	*n = mk("d");

		ft_lstadd_back(&lst, n);
		CHECK(lst->next->next->next == n, "le maillon n'est pas ajoute a la fin");
		CHECK(n->next == NULL, "new->next devrait etre NULL");
		free_nodes(lst);
	}
	TEST("ft_lstadd_back 100 fois a partir d'une liste vide : ordre conserve")
	{
		t_list	*lst = NULL;
		int		exp[100];

		for (int i = 0; i < 100; i++)
		{
			g_vals[i] = i;
			exp[i] = i;
			ft_lstadd_back(&lst, mk(&g_vals[i]));
		}
		list_matches(lst, exp, 100, "liste");
		free_nodes(lst);
	}
#ifdef HAS_ft_lstadd_front
	TEST("tests aleatoires : 300 ajouts devant / derriere melanges")
	{
		t_list	*lst = NULL;
		int		model[300];
		int		n = 0;

		for (int k = 0; k < 300; k++)
		{
			g_vals[k] = k;
			if (t_rand_range(0, 1))
			{
				ft_lstadd_back(&lst, mk(&g_vals[k]));
				model[n++] = k;
			}
			else
			{
				ft_lstadd_front(&lst, mk(&g_vals[k]));
				memmove(model + 1, model, sizeof(int) * (size_t)n);
				model[0] = k;
				n++;
			}
		}
		if (!list_matches(lst, model, n, "liste"))
			t_fail("graine %u : relance avec --seed %u", t_seed(), t_seed());
		free_nodes(lst);
	}
#endif
#else
	t_missing();
#endif
}

void	suite_lstdelone(void)
{
#if defined(HAS_ft_lstdelone) && defined(HAS_ft_lstnew)
	TEST("ft_lstdelone : appelle del une fois et libere le maillon")
	{
		t_list	*n = mk(dup_nofail("hello"));

		g_del_calls = 0;
		ft_lstdelone(n, del_count);
		CHECK(g_del_calls == 1, "del appelee %d fois au lieu de 1", g_del_calls);
	}
	TEST("ft_lstdelone ne libere PAS le maillon suivant")
	{
		t_list	*a = mk(dup_nofail("a"));
		t_list	*b = mk(dup_nofail("b"));

		a->next = b;
		g_del_calls = 0;
		ft_lstdelone(a, del_count);
		CHECK(g_del_calls == 1, "del appelee %d fois au lieu de 1", g_del_calls);
		free(b->content);
		free(b);
	}
#else
	t_missing();
#endif
}

void	suite_lstclear(void)
{
#if defined(HAS_ft_lstclear) && defined(HAS_ft_lstnew)
	TEST("ft_lstclear sur 3 maillons : tout libere, *lst = NULL")
	{
		t_list	*lst = mk(dup_nofail("a"));

		lst->next = mk(dup_nofail("b"));
		lst->next->next = mk(dup_nofail("c"));
		g_del_calls = 0;
		ft_lstclear(&lst, del_count);
		CHECK(lst == NULL, "*lst n'est pas remis a NULL");
		CHECK(g_del_calls == 3, "del appelee %d fois au lieu de 3", g_del_calls);
	}
	TEST("ft_lstclear sur une liste vide ne plante pas")
	{
		t_list	*lst = NULL;

		g_del_calls = 0;
		ft_lstclear(&lst, del_nothing);
		CHECK(lst == NULL, "*lst devrait rester NULL");
		CHECK(g_del_calls == 0, "del appelee alors que la liste est vide");
	}
	TEST("ft_lstclear sur 100 maillons : del appelee 100 fois, rien ne fuit")
	{
		t_list	*lst = NULL;
		t_list	*last = NULL;

		for (int i = 0; i < 100; i++)
		{
			t_list	*node = mk(dup_nofail("x"));

			if (last)
				last->next = node;
			else
				lst = node;
			last = node;
		}
		g_del_calls = 0;
		ft_lstclear(&lst, del_count);
		CHECK(lst == NULL, "*lst n'est pas remis a NULL");
		CHECK(g_del_calls == 100, "del appelee %d fois au lieu de 100", g_del_calls);
	}
#else
	t_missing();
#endif
}

#ifdef HAS_ft_lstiter
static char	g_iter_seen[16];
static int	g_iter_n;

static void	f_iter(void *content)
{
	if (g_iter_n < 15)
		g_iter_seen[g_iter_n++] = *(char *)content;
	g_iter_seen[g_iter_n] = '\0';
}
#endif

void	suite_lstiter(void)
{
#if defined(HAS_ft_lstiter) && defined(HAS_ft_lstnew)
	TEST("ft_lstiter sur [a, b, c] : f appelee dans l'ordre")
	{
		t_list	*l = mk_list3("a", "b", "c");

		g_iter_n = 0;
		g_iter_seen[0] = '\0';
		ft_lstiter(l, f_iter);
		CHECK_STR(g_iter_seen, "abc");
		free_nodes(l);
	}
	TEST("ft_lstiter(NULL, f) ne plante pas")
	{
		g_iter_n = 0;
		ft_lstiter(NULL, f_iter);
		CHECK(g_iter_n == 0, "f appelee sur une liste vide");
	}
#else
	t_missing();
#endif
}

#ifdef HAS_ft_lstmap
static void	*f_map_upper(void *content)
{
	char	*s = dup_nofail(content);

	for (size_t i = 0; s[i]; i++)
		if (s[i] >= 'a' && s[i] <= 'z')
			s[i] -= 32;
	return (s);
}

static void	*f_map_double(void *content)
{
	int	*p = hk_nofail_malloc(sizeof(int));

	*p = *(int *)content * 2;
	return (p);
}

static void	free_mapped(void *p)
{
	t_list	*l = p;
	t_list	*next;

	while (l)
	{
		next = l->next;
		free(l->content);
		free(l);
		l = next;
	}
}
#endif

void	suite_lstmap(void)
{
#if defined(HAS_ft_lstmap) && defined(HAS_ft_lstnew)
	TEST("ft_lstmap([\"ab\", \"cd\", \"ef\"], majuscule)")
	{
		t_list	*l = mk_list3("ab", "cd", "ef");
		t_list	*m = ft_lstmap(l, f_map_upper, free);

		CHECK(m != NULL, "renvoie NULL");
		if (m)
		{
			CHECK(m != l, "renvoie la liste d'origine au lieu d'une nouvelle");
			CHECK_STR(m->content, "AB");
			if (m->next)
			{
				CHECK_STR(m->next->content, "CD");
				if (m->next->next)
				{
					CHECK_STR(m->next->next->content, "EF");
					CHECK(m->next->next->next == NULL, "la nouvelle liste est trop longue");
				}
				else
					t_fail("la nouvelle liste n'a que 2 maillons");
			}
			else
				t_fail("la nouvelle liste n'a qu'un maillon");
		}
		CHECK_STR(l->content, "ab");
		free_mapped(m);
		free_nodes(l);
	}
	TEST("ft_lstmap(NULL, f, del) renvoie NULL")
		CHECK(ft_lstmap(NULL, f_map_upper, free) == NULL, "devrait renvoyer NULL");
	TEST("ft_lstmap : malloc qui echoue → NULL, tout libere (avec del)")
	{
		t_list	*l = mk_list3("ab", "cd", "ef");

		FAIL_LOOP(6, ft_lstmap(l, f_map_upper, free), free_mapped);
		free_nodes(l);
	}
	TEST("ft_lstmap sur 50 maillons : ordre conserve, liste d'origine intacte")
	{
		t_list	*l = mk_list_n(50);
		t_list	*m;
		int		exp[50];

		for (int i = 0; i < 50; i++)
			exp[i] = i * 2;
		g_del_calls = 0;
		m = ft_lstmap(l, f_map_double, del_count);
		CHECK(m != NULL, "renvoie NULL");
		if (m)
			list_matches(m, exp, 50, "nouvelle liste");
		for (int i = 0; i < 50; i++)
			exp[i] = i;
		list_matches(l, exp, 50, "liste d'origine");
		CHECK(g_del_calls == 0, "del appelee %d fois alors que tout a reussi", g_del_calls);
		free_mapped(m);
		free_nodes(l);
	}
#else
	t_missing();
#endif
}
