#include "hook.h"
#include <stdlib.h>

#ifdef __APPLE__
# define REAL_MALLOC malloc
# define REAL_FREE free
#else
void	*__real_malloc(size_t size);
void	__real_free(void *ptr);
# define REAL_MALLOC __real_malloc
# define REAL_FREE __real_free
#endif

#define MAX_LIVE 4096

typedef struct s_live
{
	void	*ptr;
	size_t	size;
}	t_live;

static struct
{
	int		on;
	int		paused;
	int		calls;
	int		fail_at;
	int		triggered;
	size_t	nlive;
	t_live	live[MAX_LIVE];
}	g;

static int	tracking(void)
{
	return (g.on && !g.paused);
}

static void	live_add(void *p, size_t size)
{
	if (g.nlive < MAX_LIVE)
	{
		g.live[g.nlive].ptr = p;
		g.live[g.nlive].size = size;
		g.nlive++;
	}
}

static void	live_remove(void *p)
{
	size_t	i;

	i = 0;
	while (i < g.nlive)
	{
		if (g.live[i].ptr == p)
		{
			g.live[i] = g.live[g.nlive - 1];
			g.nlive--;
			return ;
		}
		i++;
	}
}

static void	*spy_malloc(size_t size)
{
	void	*p;

	if (!tracking())
		return (REAL_MALLOC(size));
	g.calls++;
	if (g.fail_at && g.calls == g.fail_at && !g.triggered)
	{
		g.triggered = 1;
		return (NULL);
	}
	p = REAL_MALLOC(size);
	if (p)
		live_add(p, size);
	return (p);
}

static void	spy_free(void *p)
{
	if (p && g.on)
		live_remove(p);
	REAL_FREE(p);
}

void	*hk_nofail_malloc(size_t size)
{
	void	*p;

	p = REAL_MALLOC(size);
	if (p && g.on)
		live_add(p, size);
	return (p);
}

void	hk_start(void)
{
	g.nlive = 0;
	g.calls = 0;
	g.fail_at = 0;
	g.triggered = 0;
	g.paused = 0;
	g.on = 1;
}

void	hk_stop(void) { g.on = 0; }
void	hk_pause(void) { g.paused++; }
void	hk_resume(void) { if (g.paused) g.paused--; }
size_t	hk_live_count(void) { return (g.nlive); }
void	hk_forget_all(void) { g.nlive = 0; }
int		hk_fail_triggered(void) { return (g.triggered); }
int		hk_malloc_calls(void) { return (g.calls); }

size_t	hk_live_bytes(void)
{
	size_t	i;
	size_t	total;

	i = 0;
	total = 0;
	while (i < g.nlive)
		total += g.live[i++].size;
	return (total);
}

void	hk_fail_at(int n)
{
	g.calls = 0;
	g.triggered = 0;
	g.fail_at = n;
}

#if defined(__APPLE__) && defined(HOOK_NO_INTERPOSE)

/* Mac + ASan : ASan remplace deja malloc, l'interposition bloquerait au demarrage */

#elif defined(__APPLE__)

typedef struct s_interpose
{
	const void	*replacement;
	const void	*original;
}	t_interpose;

__attribute__((used, section("__DATA,__interpose")))
static const t_interpose	g_interposers[] = {
{(const void *)spy_malloc, (const void *)malloc},
{(const void *)spy_free, (const void *)free},
};

#else

void	*__wrap_malloc(size_t size) { return (spy_malloc(size)); }
void	__wrap_free(void *ptr) { spy_free(ptr); }

#endif
