#include "framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RST "\033[0m"
#define BOLD "\033[1m"
#define DIM "\033[2m"
#define RED "\033[31m"
#define GRN "\033[32m"
#define YEL "\033[33m"
#define BLU "\033[34m"
#define MAG "\033[35m"
#define CYN "\033[36m"

extern int	g_suites_ok, g_suites_ko, g_suites_missing, g_tests_total, g_tests_ok;
extern char	g_ko_names[], g_missing_names[];
extern FILE	*g_trace;
extern FILE	*g_trace_key;
extern FILE	*g_results;
extern int	g_verbose;
extern const char	*g_outdir;

#define S(name) void suite_##name(void);
#define LIST1 S(isalpha) S(isdigit) S(isalnum) S(isascii) S(isprint) S(strlen) \
	S(memset) S(bzero) S(memcpy) S(memmove) S(strlcpy) S(strlcat) S(toupper) \
	S(tolower) S(strchr) S(strrchr) S(strncmp) S(memchr) S(memcmp) S(strnstr) \
	S(atoi) S(calloc) S(strdup)
#define LIST2 S(substr) S(strjoin) S(strtrim) S(split) S(itoa) S(strmapi) \
	S(striteri) S(putchar_fd) S(putstr_fd) S(putendl_fd) S(putnbr_fd)
#define LIST3 S(lstnew) S(lstadd_front) S(lstsize) S(lstlast) S(lstadd_back) \
	S(lstdelone) S(lstclear) S(lstiter) S(lstmap)

LIST1 LIST2 LIST3
#undef S

typedef struct s_entry
{
	const char	*name;
	void		(*fn)(void);
	int			part;
}	t_entry;

#define S(name) {"ft_" #name, suite_##name, PART},
#define PART 1
static const t_entry	g_part1[] = {LIST1};
#undef PART
#define PART 2
static const t_entry	g_part2[] = {LIST2};
#undef PART
#define PART 3
static const t_entry	g_part3[] = {LIST3};
#undef PART

static int	selected(const char *name, int ac, char **av)
{
	if (ac <= 1)
		return (1);
	for (int i = 1; i < ac; i++)
	{
		if (strcmp(av[i], name) == 0 || strcmp(av[i], name + 3) == 0)
			return (1);
	}
	return (0);
}

static void	run_part(const char *title, const t_entry *e, size_t n, int ac, char **av)
{
	int	any = 0;

	for (size_t i = 0; i < n; i++)
		if (selected(e[i].name, ac, av))
			any = 1;
	if (!any)
		return ;
	printf("\n " CYN BOLD "▌ %s" RST "\n\n", title);
	for (size_t i = 0; i < n; i++)
	{
		if (!selected(e[i].name, ac, av))
			continue ;
		suite_begin(e[i].name);
		e[i].fn();
		suite_end();
	}
}

int	main(int ac, char **av)
{
	char	path[1024];

	setvbuf(stdout, NULL, _IOLBF, 0);
	t_lang_init();
	if (getenv("TESTER_OUT"))
		g_outdir = getenv("TESTER_OUT");
	g_verbose = getenv("TESTER_VERBOSE") && *getenv("TESTER_VERBOSE") == '1';
	snprintf(path, sizeof(path), "%s/trace.log", g_outdir);
	g_trace = fopen(path, "w");
	snprintf(path, sizeof(path), "%s/trace.key", g_outdir);
	g_trace_key = fopen(path, "w");
	snprintf(path, sizeof(path), "%s/results.txt", g_outdir);
	g_results = fopen(path, "w");
	printf(DIM "  %s " RST GRN "✔" RST " %s  " RED "✘" RST " %s  "
		BLU "L" RST " %s  " MAG BOLD "S" RST " %s  "
		MAG BOLD "T" RST " %s  " MAG BOLD "A" RST " %s  "
		YEL "!" RST " %s\n", TR("légende :"), TR("ok"), TR("faux"), TR("fuite"),
		TR("crash"), TR("boucle infinie"), TR("erreur mémoire (ASan)"), TR("avertissement"));
	run_part(TR("Partie 1 · fonctions de la libc"), g_part1,
		sizeof(g_part1) / sizeof(*g_part1), ac, av);
	run_part(TR("Partie 2 · fonctions supplémentaires"), g_part2,
		sizeof(g_part2) / sizeof(*g_part2), ac, av);
	run_part(TR("Partie 3 · listes chaînées"), g_part3,
		sizeof(g_part3) / sizeof(*g_part3), ac, av);

	printf("\n   %s %s%d / %d" RST DIM "   (%s %u)" RST "\n", TR("tests réussis :"),
		g_tests_ok == g_tests_total ? GRN : RED, g_tests_ok, g_tests_total,
		TR("graine aléatoire"), t_seed());
	if (g_trace)
		fclose(g_trace);
	if (g_trace_key)
		fclose(g_trace_key);
	if (g_results)
		fclose(g_results);
	return (g_suites_ko || g_suites_missing ? 1 : 0);
}
