#ifndef FRAMEWORK_H
# define FRAMEWORK_H

# include <stddef.h>
# include <string.h>
# include "hook.h"

/*
** Chaque TEST tourne dans un processus fils (fork) :
** un segfault ou une boucle infinie ne fait pas planter le testeur.
** Le fils a des limites (memoire, temps CPU, taille de fichier, pas de core dump)
** pour qu'une fonction buguee ne puisse pas faire ramer la machine.
** Les fuites memoire sont verifiees automatiquement a la fin de chaque TEST.
*/

# if defined(__SANITIZE_ADDRESS__)
#  define T_ASAN 1
# elif defined(__has_feature)
#  if __has_feature(address_sanitizer)
#   define T_ASAN 1
#  endif
# endif
# ifndef T_ASAN
#  define T_ASAN 0
# endif

extern int	g_soft_next;
extern int	g_lang_en;

/* traduction : le texte francais sert de cle (catalogue lang/en.txt) */
void		t_lang_init(void);
const char	*TR(const char *fr);
/* N_("...") : texte a traduire plus tard avec TR() (reperé par make i18n) */
# define N_(s) s

# define TEST(...) for (int _t_ = t_begin(__VA_ARGS__); _t_; _t_ = (t_end(), 0))

/* TEST_SOFT : un echec (meme un crash) ne compte que comme avertissement */
# define TEST_SOFT(...) for (int _t_ = (g_soft_next = 1, t_begin(__VA_ARGS__)); \
	_t_; _t_ = (t_end(), 0))

void		suite_begin(const char *name);
void		suite_end(void);
void		t_missing(void);
int			t_begin(const char *fmt, ...);
void		t_end(void);
void		t_fail(const char *fmt, ...);
void		t_warn(const char *fmt, ...);
void		t_note(const char *fmt, ...);
const char	*t_esc(const char *s);
const char	*t_escn(const char *s, size_t n);
const char	*t_ptr(const void *p, const void *base, size_t len, const char *name);
int			t_streq(const char *a, const char *b);
int			t_sign(long long x);
size_t		t_capture(int (*fn)(int fd, void *arg), void *arg, char *buf, size_t size);

/* tests aleatoires : generateur reproductible (graine affichee en cas d'echec) */
unsigned	t_seed(void);
unsigned	t_rand(void);
int			t_rand_range(int lo, int hi);

# define FUZZ_N 3000

# define CHECK(cond, ...) do { if (!(cond)) t_fail(__VA_ARGS__); } while (0)

# define CHECK_INT(got, exp) do { \
	long long g_ = (long long)(got); long long e_ = (long long)(exp); \
	if (g_ != e_) t_fail("retour attendu %lld, obtenu %lld", e_, g_); \
} while (0)

# define CHECK_SIGN(got, exp) do { \
	long long g_ = (long long)(got); long long e_ = (long long)(exp); \
	if (t_sign(g_) != t_sign(e_)) \
		t_fail("signe du retour faux : attendu %s (libc = %lld), obtenu %lld", \
			e_ > 0 ? "> 0" : e_ < 0 ? "< 0" : "0", e_, g_); \
} while (0)

# define CHECK_STR(got, exp) do { \
	const char *g_ = (got); const char *e_ = (exp); \
	if (!t_streq(g_, e_)) t_fail("attendu %s, obtenu %s", t_esc(e_), t_esc(g_)); \
} while (0)

# define CHECK_PTR(got, exp, base, len, name) do { \
	const void *g_ = (got); const void *e_ = (exp); \
	if (g_ != e_) t_fail("retour attendu %s, obtenu %s", \
		t_ptr(e_, base, len, name), t_ptr(g_, base, len, name)); \
} while (0)

# define CHECK_MEM(got, exp, n, label) do { \
	const unsigned char *g_ = (const unsigned char *)(got); \
	const unsigned char *e_ = (const unsigned char *)(exp); \
	for (size_t i_ = 0; i_ < (size_t)(n); i_++) \
		if (g_[i_] != e_[i_]) { \
			t_fail("%s : octet [%zu] attendu 0x%02x, obtenu 0x%02x", \
				label, i_, e_[i_], g_[i_]); break; } \
} while (0)

/* verifie qu'une allocation fait exactement la bonne taille */
# define CHECK_ALLOC(exp) do { \
	size_t g_ = hk_live_bytes(); size_t e_ = (exp); \
	if (hk_live_count() != 1) break; /* fuite : signalee a part */ \
	if (g_ < e_) t_fail("alloue %zu octet(s) seulement, il en faut %zu " \
		"(debordement memoire !)", g_, e_); \
	else if (g_ != e_) t_warn("alloue %zu octets au lieu de %zu", g_, e_); \
} while (0)

/*
** Fait echouer le 1er malloc, puis le 2e, puis le 3e... de EXPR.
** A chaque fois : EXPR doit renvoyer NULL et avoir tout libere.
** S'arrete quand EXPR fait moins de n mallocs (elle reussit : on libere
** le resultat avec FREE_FN).
*/
# define FAIL_LOOP(maxn, EXPR, FREE_FN) do { \
	for (int n_ = 1; n_ <= (maxn); n_++) { \
		size_t before_ = hk_live_count(); \
		t_note("le malloc n°%d de la fonction echoue", n_); \
		hk_fail_at(n_); \
		void *r_ = (void *)(EXPR); \
		int trig_ = hk_fail_triggered(); \
		hk_fail_at(0); \
		if (!trig_) { FREE_FN(r_); break; } \
		if (r_) { t_fail("le malloc n°%d echoue mais la fonction ne renvoie " \
			"pas NULL", n_); hk_forget_all(); break; } \
		if (hk_live_count() != before_) { t_fail("le malloc n°%d echoue : tu " \
			"renvoies bien NULL mais tu ne liberes pas ce qui etait deja " \
			"alloue (%zu bloc(s) perdu(s))", n_, hk_live_count() - before_); \
			hk_forget_all(); break; } \
	} \
} while (0)

#endif
