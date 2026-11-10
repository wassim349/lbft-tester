#ifndef HOOK_H
# define HOOK_H

# include <stddef.h>

/*
** Espion de malloc/free.
**  - macOS : compile en libfthook.dylib, interposition dyld.
**  - Linux : compile dans le testeur, avec -Wl,--wrap=malloc,--wrap=free.
*/

void	hk_start(void);			/* vide la liste + active le suivi */
void	hk_stop(void);			/* desactive le suivi */
void	hk_pause(void);
void	hk_resume(void);
size_t	hk_live_count(void);	/* blocs alloues pendant le suivi, pas liberes */
size_t	hk_live_bytes(void);
void	hk_forget_all(void);	/* oublie les blocs suivis (sans les liberer) */
void	hk_fail_at(int n);		/* le n-ieme malloc a partir de maintenant echoue */
int		hk_fail_triggered(void);
int		hk_malloc_calls(void);	/* mallocs depuis hk_start / hk_fail_at */
void	*hk_nofail_malloc(size_t size);	/* suivi, mais n'echoue jamais */

#endif
