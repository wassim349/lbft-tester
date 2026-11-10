#include "framework.h"
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TIMEOUT_SEC 3
#define MEM_LIMIT (1024UL * 1024UL * 1024UL)	/* 1 Go par test (hors ASan) */
#define FILE_LIMIT (16UL * 1024UL * 1024UL)		/* 16 Mo ecrits au max */
#define DETAILS_MAX 32768
#define MSG_MAX 4096
#define MAX_MSG_PER_TEST 4

#define RST "\033[0m"
#define BOLD "\033[1m"
#define DIM "\033[2m"
#define RED "\033[31m"
#define GRN "\033[32m"
#define YEL "\033[33m"
#define BLU "\033[34m"
#define MAG "\033[35m"

/* ---------------------------- etat global ---------------------------- */

int			g_suites_ok = 0;
int			g_suites_ko = 0;
int			g_suites_missing = 0;
int			g_tests_total = 0;
int			g_tests_ok = 0;
int			g_verbose = 0;
int			g_soft_next = 0;
char		g_ko_names[2048] = "";
char		g_missing_names[2048] = "";
FILE		*g_trace = NULL;
FILE		*g_trace_key = NULL;	/* meme trace, toujours en francais (pour les conseils) */
FILE		*g_results = NULL;
const char	*g_outdir = "build";

static const char	*g_suite;
static int			g_s_total;
static int			g_s_ko;
static int			g_s_warn;
static int			g_s_crash;
static int			g_s_leak;
static int			g_s_wrong;
static int			g_s_missing;
static int			g_s_timeouts;
static char			g_details[DETAILS_MAX];		/* dans la langue choisie */
static size_t		g_details_len;
static char			g_details_key[DETAILS_MAX];	/* en francais */
static size_t		g_details_key_len;

/* etat cote fils */
static int			g_child_fd = -1;
static int			g_child_fail;
static int			g_child_warn;
static int			g_child_nmsg;

/* ------------------------------ affichage ----------------------------- */

/* ---------------------------- traduction ---------------------------- */

/*
** Le texte francais sert de cle : TR("...") renvoie la traduction trouvee dans
** le catalogue (lang/en.txt), ou le texte tel quel. Le catalogue est lu une fois
** au demarrage, avant le premier fork.
*/
int			g_lang_en = 0;
static char	**g_tr_key;
static char	**g_tr_val;
static int	g_tr_n;

/* decode les echappements C d'une ligne du catalogue (\n, \", \310, \x80...) */
static void	tr_unescape(char *s)
{
	char	*w = s;

	while (*s)
	{
		if (*s != '\\' || !s[1])
		{
			*w++ = *s++;
			continue ;
		}
		s++;
		if (*s >= '0' && *s <= '7')
		{
			int	v = 0;

			for (int k = 0; k < 3 && *s >= '0' && *s <= '7'; k++)
				v = v * 8 + (*s++ - '0');
			*w++ = (char)v;
			continue ;
		}
		if (*s == 'x')
		{
			int	v = 0;
			int	k = 0;

			s++;
			while (k++ < 2 && isxdigit((unsigned char)*s))
			{
				v = v * 16 + (isdigit((unsigned char)*s) ? *s - '0'
						: tolower((unsigned char)*s) - 'a' + 10);
				s++;
			}
			*w++ = (char)v;
			continue ;
		}
		*w++ = *s == 'n' ? '\n' : *s == 't' ? '\t' : *s;
		s++;
	}
	*w = '\0';
}

static char	*tr_dup_line(const char *line)
{
	size_t	n = strlen(line);
	char	*d;

	while (n && (line[n - 1] == '\n' || line[n - 1] == '\r'))
		n--;
	d = malloc(n + 1);
	if (!d)
		return (NULL);
	memcpy(d, line, n);
	d[n] = '\0';
	tr_unescape(d);
	return (d);
}

void	t_lang_init(void)
{
	const char	*lang = getenv("LIBFTEST_LANG");
	const char	*path = getenv("TESTER_CATALOG");
	char		line[4096];
	char		*key = NULL;
	FILE		*f;
	int			cap = 0;

	if (!lang || strcmp(lang, "en") != 0 || !path || !(f = fopen(path, "r")))
		return ;
	g_lang_en = 1;
	while (fgets(line, sizeof(line), f))
	{
		if (strncmp(line, "fr: ", 4) == 0)
		{
			free(key);
			key = tr_dup_line(line + 4);
		}
		else if (strncmp(line, "en: ", 4) == 0 && key)
		{
			if (g_tr_n == cap)
			{
				cap = cap ? cap * 2 : 256;
				g_tr_key = realloc(g_tr_key, sizeof(char *) * (size_t)cap);
				g_tr_val = realloc(g_tr_val, sizeof(char *) * (size_t)cap);
				if (!g_tr_key || !g_tr_val)
					break ;
			}
			g_tr_key[g_tr_n] = key;
			g_tr_val[g_tr_n++] = tr_dup_line(line + 4);
			key = NULL;
		}
	}
	free(key);
	fclose(f);
}

const char	*TR(const char *s)
{
	if (!g_lang_en || !s)
		return (s);
	for (int i = 0; i < g_tr_n; i++)
		if (g_tr_val[i] && strcmp(g_tr_key[i], s) == 0)
			return (g_tr_val[i]);
	return (s);
}

/* ------------------------------ affichage ----------------------------- */

static void	vadd(char *buf, size_t *len, const char *fmt, va_list ap)
{
	int	n;

	if (*len >= DETAILS_MAX - 1)
		return ;
	n = vsnprintf(buf + *len, DETAILS_MAX - *len, fmt, ap);
	if (n > 0)
		*len += (size_t)n;
	if (*len > DETAILS_MAX - 1)
		*len = DETAILS_MAX - 1;
}

/* ajoute au detail affiche : fmt traduit */
static void	details_add(const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	vadd(g_details, &g_details_len, TR(fmt), ap);
	va_end(ap);
}

/* ajoute au detail en francais (trace.key) */
static void	details_key(const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	vadd(g_details_key, &g_details_key_len, fmt, ap);
	va_end(ap);
}

/* ecrit s sans les codes couleur (pour trace.log) */
static void	fputs_plain(const char *s, FILE *f)
{
	while (*s)
	{
		if (s[0] == '\033' && s[1] == '[')
		{
			s += 2;
			while (*s && *s != 'm')
				s++;
			if (*s)
				s++;
			continue ;
		}
		fputc(*s++, f);
	}
}

void	suite_begin(const char *name)
{
	g_suite = name;
	g_s_total = 0;
	g_s_ko = 0;
	g_s_warn = 0;
	g_s_crash = 0;
	g_s_leak = 0;
	g_s_wrong = 0;
	g_s_missing = 0;
	g_s_timeouts = 0;
	g_details_len = 0;
	g_details[0] = '\0';
	g_details_key_len = 0;
	g_details_key[0] = '\0';
	printf("  " BOLD "%-17s" RST " ", name);
	fflush(stdout);
}

void	t_missing(void)
{
	g_s_missing = 1;
}

static void	append_name(char *list, const char *name)
{
	if (strlen(list) + strlen(name) + 3 < 2048)
	{
		if (*list)
			strcat(list, ", ");
		strcat(list, name);
	}
}

void	suite_end(void)
{
	if (g_s_missing)
	{
		printf(DIM "%s" RST "\n", TR("absente"));
		g_suites_missing++;
		append_name(g_missing_names, g_suite);
		if (g_trace)
			fprintf(g_trace, "[%s] %s\n\n", TR("ABSENTE"), g_suite);
		if (g_trace_key)
			fprintf(g_trace_key, "[ABSENTE] %s\n\n", g_suite);
		if (g_results)
			fprintf(g_results, "%s ABSENTE 0 0 0 0 0 0\n", g_suite);
		return ;
	}
	if (g_s_ko)
	{
		printf("  " RED BOLD "KO" RST "\n");
		g_suites_ko++;
		append_name(g_ko_names, g_suite);
	}
	else
	{
		printf("  " GRN BOLD "OK" RST "%s%s%s\n", g_s_warn ? YEL " " : "",
			g_s_warn ? TR("(avertissements)") : "", g_s_warn ? RST : "");
		g_suites_ok++;
	}
	if (g_verbose)
		printf("%s", g_details);
	if (g_trace)
	{
		fprintf(g_trace, "[%s] %s\n", g_s_ko ? "KO" : "OK", g_suite);
		fputs_plain(g_details, g_trace);
		fputc('\n', g_trace);
	}
	if (g_trace_key)
	{
		fprintf(g_trace_key, "[%s] %s\n", g_s_ko ? "KO" : "OK", g_suite);
		fputs_plain(g_lang_en ? g_details_key : g_details, g_trace_key);
		fputc('\n', g_trace_key);
	}
	if (g_results)
		fprintf(g_results, "%s %s %d %d %d %d %d %d\n", g_suite, g_s_ko ? "KO" : "OK",
			g_s_total - g_s_ko, g_s_total, g_s_crash, g_s_leak, g_s_wrong, g_s_warn);
	fflush(stdout);
}

/* ---------------------------- cote fils ------------------------------ */

static void	send_line(char tag, const char *fmt, va_list ap)
{
	char	buf[MSG_MAX];
	int		n;

	buf[0] = tag;
	n = vsnprintf(buf + 1, sizeof(buf) - 2, fmt, ap);
	if (n < 0)
		return ;
	n = (int)strlen(buf);
	buf[n++] = '\n';
	(void)!write(g_child_fd, buf, (size_t)n);
}

/*
** Envoie le message au parent : 'F' (echec), 'W' (avertissement), 'N' (note),
** dans la langue choisie. En anglais, suivi de la meme ligne en francais avec
** le tag en minuscule ('f', 'w', 'n') : elle sert a trouver les conseils.
*/
static void	child_send(char tag, const char *fmt, va_list ap)
{
	va_list	ap2;

	va_copy(ap2, ap);
	send_line(tag, TR(fmt), ap);
	if (g_lang_en)
		send_line((char)(tag - 'A' + 'a'), fmt, ap2);
	va_end(ap2);
}

static void	child_say(char tag, const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	child_send(tag, fmt, ap);
	va_end(ap);
}

void	t_fail(const char *fmt, ...)
{
	va_list	ap;

	hk_pause();
	g_child_fail = 1;
	if (g_child_nmsg++ < MAX_MSG_PER_TEST)
	{
		va_start(ap, fmt);
		child_send('F', fmt, ap);
		va_end(ap);
	}
	hk_resume();
}

void	t_warn(const char *fmt, ...)
{
	va_list	ap;

	hk_pause();
	g_child_warn = 1;
	va_start(ap, fmt);
	child_send('W', fmt, ap);
	va_end(ap);
	hk_resume();
}

void	t_note(const char *fmt, ...)
{
	va_list	ap;

	hk_pause();
	va_start(ap, fmt);
	child_send('N', fmt, ap);
	va_end(ap);
	hk_resume();
}

void	t_end(void)
{
	size_t	n;
	size_t	bytes;
	char	status;

	hk_stop();
	n = hk_live_count();
	bytes = hk_live_bytes();
	status = 'O';
	if (n && !g_child_fail)
		status = 'L';
	if (g_child_fail)
		status = 'K';
	else if (!n && g_child_warn)
		status = 'W';
	if (n)
		child_say('F', "fuite memoire : %zu bloc(s) jamais libere(s), %zu octet(s)", n, bytes);
	{
		char	end[3];

		end[0] = 'R';
		end[1] = status;
		end[2] = '\n';
		(void)!write(g_child_fd, end, 3);
	}
	_exit(0);
}

/* limites du fils : une fonction buguee ne doit pas faire ramer la machine */
static void	child_limits(void)
{
	struct rlimit	r;

	r.rlim_cur = 0;
	r.rlim_max = 0;
	setrlimit(RLIMIT_CORE, &r);			/* pas de fichier core qui remplit le disque */
	r.rlim_cur = TIMEOUT_SEC + 1;
	r.rlim_max = TIMEOUT_SEC + 2;
	setrlimit(RLIMIT_CPU, &r);			/* filet de securite si alarm() rate */
	r.rlim_cur = FILE_LIMIT;
	r.rlim_max = FILE_LIMIT;
	setrlimit(RLIMIT_FSIZE, &r);
#if !T_ASAN
	r.rlim_cur = MEM_LIMIT;				/* malloc enorme -> NULL au lieu de swapper */
	r.rlim_max = MEM_LIMIT;
	setrlimit(RLIMIT_AS, &r);
#endif
}

/* ---------------------------- cote parent ---------------------------- */

static void	print_mark(char status)
{
	if (status == 'O')
		printf(GRN "✔" RST);
	else if (status == 'W')
		printf(YEL "!" RST);
	else if (status == 'K')
		printf(RED "✘" RST);
	else if (status == 'L')
		printf(BLU "L" RST);
	else if (status == 'T')
		printf(MAG BOLD "T" RST);
	else if (status == 'A')
		printf(MAG BOLD "A" RST);
	else
		printf(MAG BOLD "S" RST);
	fflush(stdout);
}

static const char	*signal_desc(int sig)
{
	if (sig == SIGSEGV)
		return (N_("SEGFAULT : acces memoire invalide"));
	if (sig == SIGBUS)
		return (N_("BUS ERROR : acces memoire invalide"));
	if (sig == SIGABRT)
		return (N_("ABORT : souvent un double free ou un free sur un mauvais pointeur"));
	if (sig == SIGALRM || sig == SIGXCPU)
		return (N_("TIMEOUT : plus de 3 s, boucle infinie ?"));
	if (sig == SIGXFSZ)
		return (N_("ecrit plus de 16 Mo : boucle infinie d'ecriture ?"));
	if (sig == SIGFPE)
		return (N_("SIGFPE : division par zero ?"));
	return (N_("crash (signal inattendu)"));
}

/* lit le rapport ASan du fils : type ("heap-buffer-overflow") et endroit ("ft_xxx ft_xxx.c:12") */
static int	asan_summary(const char *path, char *kind_out, char *where, size_t size)
{
	char	buf[16384];
	FILE	*f;
	size_t	n;
	char	*kind;
	char	*frame;
	char	*p;

	kind_out[0] = '\0';
	where[0] = '\0';
	f = fopen(path, "r");
	if (!f)
		return (0);
	n = fread(buf, 1, sizeof(buf) - 1, f);
	fclose(f);
	buf[n] = '\0';
	kind = strstr(buf, "ERROR: AddressSanitizer: ");
	if (!kind)
		return (0);
	kind += strlen("ERROR: AddressSanitizer: ");
	p = kind;
	while (*p && *p != ' ' && *p != '\n')
		p++;
	snprintf(kind_out, size, "%.*s", (int)(p - kind), kind);
	frame = strstr(buf, " in ft_");
	if (frame)
	{
		size_t	i = 0;

		frame += 4;
		while (frame[i] && frame[i] != '\n' && i < size - 1)
		{
			where[i] = frame[i];
			i++;
		}
		where[i] = '\0';
	}
	return (1);
}

/* ajoute "→ msg" au detail affiche (msg deja traduit) et/ou a la cle */
static void	add_arrow(int to_key, const char *color, const char *msg)
{
	if (to_key)
		details_key("         %s→" RST " %s\n", color, msg);
	else
		details_add("         %s→" RST " %s\n", color, msg);
}

static void	record(const char *desc, const char *desc_key, int wstatus, char *out,
				int soft, const char *akind, const char *awhere)
{
	char	status;
	char	*line;
	char	*next;
	char	last_note[MSG_MAX];
	char	last_note_key[MSG_MAX];
	int		sig;

	status = 0;
	last_note[0] = '\0';
	last_note_key[0] = '\0';
	line = out;
	while (line && *line)
	{
		next = strchr(line, '\n');
		if (next)
			*next++ = '\0';
		if (line[0] == 'R')
			status = line[1];
		else if (line[0] == 'N')
			snprintf(last_note, sizeof(last_note), "%s", line + 1);
		else if (line[0] == 'n')
			snprintf(last_note_key, sizeof(last_note_key), "%s", line + 1);
		line = next;
	}
	if (!g_lang_en)
		memcpy(last_note_key, last_note, sizeof(last_note));
	sig = WIFSIGNALED(wstatus) ? WTERMSIG(wstatus) : 0;
	if (akind[0])
		status = 'A';
	else if (sig)
	{
		status = (sig == SIGALRM || sig == SIGXCPU) ? 'T' : 'S';
		if (status == 'T')
			g_s_timeouts++;
	}
	else if (!status)
		status = 'K';
	if (soft && status != 'O')
		status = 'W';
	g_s_total++;
	g_tests_total++;
	if (status == 'O' || status == 'W')
		g_tests_ok++;
	else
		g_s_ko++;
	if (status == 'W')
		g_s_warn++;
	else if (status == 'S' || status == 'T' || status == 'A')
		g_s_crash++;
	else if (status == 'L')
		g_s_leak++;
	else if (status == 'K')
		g_s_wrong++;
	print_mark(status);
	if (status == 'O')
		return ;
	/* 2 passes : k = 0 detail affiche, k = 1 detail en francais (seulement en anglais) */
	for (int k = 0; k <= g_lang_en; k++)
	{
		const char	*red = soft ? YEL : RED;
		const char	*mark = status == 'W' ? "!" : "✘";
		const char	*mcol = status == 'W' ? YEL : RED;
		char		tmp[MSG_MAX];
		int			kfr = k || !g_lang_en;	/* textes en francais pour cette passe */

		if (k)
			details_key("     %s%s" RST " %s\n", mcol, mark, desc_key);
		else
			details_add("     %s%s" RST " %s\n", mcol, mark, desc);
		for (line = out; line && *line; line += strlen(line) + 1)
		{
			char	want_f = k ? 'f' : 'F';
			char	want_w = k ? 'w' : 'W';

			if (line[0] == want_f)
				add_arrow(k, red, line + 1);
			else if (line[0] == want_w)
				add_arrow(k, YEL, line + 1);
		}
		if (akind[0])
		{
			if (awhere[0])
				snprintf(tmp, sizeof(tmp), kfr ? "ASan : %s dans %s" : TR("ASan : %s dans %s"),
					akind, awhere);
			else
				snprintf(tmp, sizeof(tmp), "ASan : %s", akind);
			add_arrow(k, MAG, tmp);
		}
		else if (sig)
			add_arrow(k, MAG, kfr ? signal_desc(sig) : TR(signal_desc(sig)));
		else if (!strchr("OWKL", status) && !soft)
			add_arrow(k, RED, kfr ? "le test s'est arrete sans resultat (exit dans ta fonction ?)"
				: TR("le test s'est arrete sans resultat (exit dans ta fonction ?)"));
		if ((akind[0] || sig) && (k ? last_note_key : last_note)[0])
		{
			snprintf(tmp, sizeof(tmp), kfr ? "pendant : %s" : TR("pendant : %s"),
				k ? last_note_key : last_note);
			add_arrow(k, MAG, tmp);
		}
	}
}

int	t_begin(const char *fmt, ...)
{
	char	desc[1024];
	char	desc_key[1024];
	char	asan_log[1024];
	char	akind[256];
	char	awhere[512];
	va_list	ap;
	va_list	ap2;
	int		fds[2];
	pid_t	pid;
	char	out[65536];
	size_t	len;
	ssize_t	r;
	int		wstatus;
	int		soft;

	soft = g_soft_next;
	g_soft_next = 0;
	if (g_s_timeouts >= 2)
	{
		/* la fonction boucle deja : inutile d'attendre 3 s par test */
		g_s_total++;
		g_tests_total++;
		g_s_ko++;
		g_s_crash++;
		printf(DIM "T" RST);
		if (g_s_timeouts++ == 2)
		{
			details_add("     " RED "✘" RST " %s\n", TR("tests suivants sautes : la fonction boucle deja a l'infini"));
			details_key("     " RED "✘" RST " %s\n", "tests suivants sautes : la fonction boucle deja a l'infini");
		}
		return (0);
	}
	va_start(ap, fmt);
	va_copy(ap2, ap);
	vsnprintf(desc, sizeof(desc), TR(fmt), ap);
	vsnprintf(desc_key, sizeof(desc_key), fmt, ap2);
	va_end(ap2);
	va_end(ap);
	snprintf(asan_log, sizeof(asan_log), "%s/asan.log", g_outdir);
	akind[0] = '\0';
	awhere[0] = '\0';
	fflush(stdout);
	if (g_trace)
		fflush(g_trace);
	if (g_trace_key)
		fflush(g_trace_key);
	if (g_results)
		fflush(g_results);
	if (pipe(fds) < 0)
		return (perror("pipe"), 0);
	pid = fork();
	if (pid < 0)
		return (perror("fork"), 0);
	if (pid == 0)
	{
		int	devnull;
		int	errfd;

		close(fds[0]);
		g_child_fd = fds[1];
		g_child_fail = 0;
		g_child_warn = 0;
		g_child_nmsg = 0;
		devnull = open("/dev/null", O_WRONLY);
		errfd = T_ASAN ? open(asan_log, O_WRONLY | O_CREAT | O_TRUNC, 0644) : devnull;
		dup2(devnull, 1);
		dup2(errfd >= 0 ? errfd : devnull, 2);
		close(devnull);
		if (errfd >= 0 && errfd != 2)
			close(errfd);
		signal(SIGPIPE, SIG_IGN);
		child_limits();
		alarm(TIMEOUT_SEC);
		hk_start();
		return (1);
	}
	close(fds[1]);
	len = 0;
	while (len < sizeof(out) - 1 && (r = read(fds[0], out + len, sizeof(out) - 1 - len)) > 0)
		len += (size_t)r;
	out[len] = '\0';
	close(fds[0]);
	waitpid(pid, &wstatus, 0);
	if (T_ASAN && !strstr(out, "\nR") && out[0] != 'R')
		asan_summary(asan_log, akind, awhere, sizeof(akind));
	record(desc, desc_key, wstatus, out, soft, akind, awhere);
	return (0);
}

/* ------------------------------ aleatoire ----------------------------- */

static unsigned	g_seed = 0;
static unsigned	g_state = 0;

unsigned	t_seed(void)
{
	if (!g_seed)
	{
		const char	*e = getenv("TESTER_SEED");

		g_seed = e && *e ? (unsigned)strtoul(e, NULL, 10) : (unsigned)time(NULL);
		if (!g_seed)
			g_seed = 42;
		g_state = g_seed;
	}
	return (g_seed);
}

unsigned	t_rand(void)
{
	t_seed();
	g_state ^= g_state << 13;
	g_state ^= g_state >> 17;
	g_state ^= g_state << 5;
	return (g_state);
}

int	t_rand_range(int lo, int hi)
{
	return (lo + (int)(t_rand() % (unsigned)(hi - lo + 1)));
}

/* ------------------------------ utilitaires --------------------------- */

const char	*t_escn(const char *s, size_t n)
{
	static char	bufs[4][256];
	static int	idx = 0;
	char		*b;
	size_t		i;
	size_t		j;

	if (!s)
		return ("NULL");
	b = bufs[idx++ % 4];
	j = 0;
	b[j++] = '"';
	i = 0;
	while (i < n && s[i] && j < 240)
	{
		unsigned char	c = (unsigned char)s[i++];

		if (c == '\n')
			j += (size_t)snprintf(b + j, 256 - j, "\\n");
		else if (c == '\t')
			j += (size_t)snprintf(b + j, 256 - j, "\\t");
		else if (c == '"' || c == '\\')
			j += (size_t)snprintf(b + j, 256 - j, "\\%c", c);
		else if (c < 32 || c >= 127)
			j += (size_t)snprintf(b + j, 256 - j, "\\x%02x", c);
		else
			b[j++] = (char)c;
	}
	if (i < n && s[i])
		j += (size_t)snprintf(b + j, 256 - j, "...");
	b[j++] = '"';
	b[j] = '\0';
	return (b);
}

const char	*t_esc(const char *s)
{
	return (t_escn(s, (size_t)-1));
}

const char	*t_ptr(const void *p, const void *base, size_t len, const char *name)
{
	static char	bufs[2][128];
	static int	idx = 0;
	char		*b;

	if (!p)
		return ("NULL");
	b = bufs[idx++ % 2];
	if (base && (const char *)p >= (const char *)base
		&& (const char *)p <= (const char *)base + len)
		snprintf(b, 128, "%s+%td", name,
			(const char *)p - (const char *)base);
	else
		snprintf(b, 128, TR("%p (en dehors de %s !)"), p, name);
	return (b);
}

int	t_streq(const char *a, const char *b)
{
	if (!a || !b)
		return (a == b);
	return (strcmp(a, b) == 0);
}

int	t_sign(long long x)
{
	return ((x > 0) - (x < 0));
}

size_t	t_capture(int (*fn)(int fd, void *arg), void *arg, char *buf, size_t size)
{
	int		fds[2];
	size_t	len;
	ssize_t	r;

	if (pipe(fds) < 0)
		return (0);
	fn(fds[1], arg);
	close(fds[1]);
	len = 0;
	while (len < size - 1 && (r = read(fds[0], buf + len, size - 1 - len)) > 0)
		len += (size_t)r;
	buf[len] = '\0';
	close(fds[0]);
	return (len);
}
