#include "framework.h"
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
static char			g_details[DETAILS_MAX];
static size_t		g_details_len;

/* etat cote fils */
static int			g_child_fd = -1;
static int			g_child_fail;
static int			g_child_warn;
static int			g_child_nmsg;

/* ------------------------------ affichage ----------------------------- */

static void	details_add(const char *fmt, ...)
{
	va_list	ap;
	int		n;

	if (g_details_len >= DETAILS_MAX - 1)
		return ;
	va_start(ap, fmt);
	n = vsnprintf(g_details + g_details_len, DETAILS_MAX - g_details_len, fmt, ap);
	va_end(ap);
	if (n > 0)
		g_details_len += (size_t)n;
	if (g_details_len > DETAILS_MAX - 1)
		g_details_len = DETAILS_MAX - 1;
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
		printf(DIM "absente" RST "\n");
		g_suites_missing++;
		append_name(g_missing_names, g_suite);
		if (g_trace)
			fprintf(g_trace, "[ABSENTE] %s\n\n", g_suite);
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
		printf("  " GRN BOLD "OK" RST "%s\n", g_s_warn ? YEL " (avertissements)" RST : "");
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
	if (g_results)
		fprintf(g_results, "%s %s %d %d %d %d %d %d\n", g_suite, g_s_ko ? "KO" : "OK",
			g_s_total - g_s_ko, g_s_total, g_s_crash, g_s_leak, g_s_wrong, g_s_warn);
	fflush(stdout);
}

/* ---------------------------- cote fils ------------------------------ */

static void	child_send(char tag, const char *fmt, va_list ap)
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
	{
		char	buf[256];
		int		len;

		len = snprintf(buf, sizeof(buf),
			"Ffuite memoire : %zu bloc(s) jamais libere(s), %zu octet(s)\n",
			n, bytes);
		(void)!write(g_child_fd, buf, (size_t)len);
	}
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
		return ("SEGFAULT : acces memoire invalide");
	if (sig == SIGBUS)
		return ("BUS ERROR : acces memoire invalide");
	if (sig == SIGABRT)
		return ("ABORT : souvent un double free ou un free sur un mauvais pointeur");
	if (sig == SIGALRM || sig == SIGXCPU)
		return ("TIMEOUT : plus de 3 s, boucle infinie ?");
	if (sig == SIGXFSZ)
		return ("ecrit plus de 16 Mo : boucle infinie d'ecriture ?");
	if (sig == SIGFPE)
		return ("SIGFPE : division par zero ?");
	return ("crash (signal inattendu)");
}

/* lit le rapport ASan du fils : "heap-buffer-overflow ... dans ft_xxx ft_xxx.c:12" */
static int	asan_summary(const char *path, char *out, size_t size)
{
	char	buf[16384];
	FILE	*f;
	size_t	n;
	char	*kind;
	char	*frame;
	char	*p;

	out[0] = '\0';
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
	frame = strstr(buf, " in ft_");
	{
		char	where[512];

		where[0] = '\0';
		if (frame)
		{
			size_t	i = 0;

			frame += 4;
			while (frame[i] && frame[i] != '\n' && i < sizeof(where) - 1)
			{
				where[i] = frame[i];
				i++;
			}
			where[i] = '\0';
		}
		snprintf(out, size, "ASan : %.*s%s%s", (int)(p - kind), kind,
			where[0] ? " dans " : "", where);
	}
	return (1);
}

static void	record(const char *desc, int wstatus, char *out, int soft, const char *asan)
{
	char	status;
	char	*line;
	char	*next;
	char	last_note[MSG_MAX];

	status = 0;
	last_note[0] = '\0';
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
		line = next;
	}
	if (asan[0])
		status = 'A';
	else if (WIFSIGNALED(wstatus))
	{
		int	sig = WTERMSIG(wstatus);

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
	details_add("     %s%s" RST " %s\n",
		status == 'W' ? YEL : RED, status == 'W' ? "!" : "✘", desc);
	line = out;
	while (line && *line)
	{
		if (line[0] == 'F')
			details_add("         %s→" RST " %s\n", soft ? YEL : RED, line + 1);
		else if (line[0] == 'W')
			details_add("         " YEL "→" RST " %s\n", line + 1);
		line += strlen(line) + 1;
	}
	if (asan[0])
		details_add("         " MAG "→ %s" RST "\n", asan);
	else if (WIFSIGNALED(wstatus))
		details_add("         " MAG "→ %s" RST "\n", signal_desc(WTERMSIG(wstatus)));
	else if (!strchr("OWKL", status) && !soft)
		details_add("         " RED "→ le test s'est arrete sans resultat (exit dans ta fonction ?)" RST "\n");
	if ((asan[0] || WIFSIGNALED(wstatus)) && last_note[0])
		details_add("         " MAG "→ pendant : %s" RST "\n", last_note);
}

int	t_begin(const char *fmt, ...)
{
	char	desc[1024];
	char	asan_log[1024];
	char	asan[1024];
	va_list	ap;
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
			details_add("     " RED "✘" RST " tests suivants sautes : la fonction boucle deja a l'infini\n");
		return (0);
	}
	va_start(ap, fmt);
	vsnprintf(desc, sizeof(desc), fmt, ap);
	va_end(ap);
	snprintf(asan_log, sizeof(asan_log), "%s/asan.log", g_outdir);
	asan[0] = '\0';
	fflush(stdout);
	if (g_trace)
		fflush(g_trace);
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
		asan_summary(asan_log, asan, sizeof(asan));
	record(desc, wstatus, out, soft, asan);
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
		snprintf(b, 128, "%p (en dehors de %s !)", p, name);
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
