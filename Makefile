# Makefile du testeur (lance par run.sh, pas besoin de l'appeler a la main).
#   LIBFT = dossier de la libft (pour libft.h)
#   OUT   = dossier de build de ce mode (build/normal ou build/asan)
#   ASAN  = 1 pour compiler avec -fsanitize=address
# LIBFT et OUT ne doivent pas contenir d'espace (make ne les supporte pas) : run.sh
# passe des chemins relatifs, et la libft a travers le lien $(OUT)/libft_src.

LIBFT	?= ../libft
OUT		?= build/normal
ASAN	?= 0
CC		= cc
CFLAGS	= -Wall -Wextra -g -Isrc -I$(OUT) -I$(LIBFT) -Wno-unused-function
LDFLAGS	=
ifeq ($(ASAN), 1)
	CFLAGS	+= -fsanitize=address -fno-omit-frame-pointer
	LDFLAGS	+= -fsanitize=address
endif

SRCS	= main.c framework.c tests_part1.c tests_part2.c tests_part3.c tests_fuzz.c
OBJS	= $(addprefix $(OUT)/t/, $(SRCS:.c=.o))
HDRS	= src/framework.h src/hook.h src/ref.h src/fuzz.h $(OUT)/available.h $(LIBFT)/libft.h

UNAME	:= $(shell uname)
HOOKFLAGS =
ifeq ($(UNAME), Darwin)
ifeq ($(ASAN), 1)
	HOOKFLAGS = -DHOOK_NO_INTERPOSE
endif
	HOOK	= $(OUT)/libfthook.dylib
	LINK	= -L$(OUT) -lfthook -Wl,-rpath,@executable_path
else
	# Linux (postes de 42) : malloc/free espionnes avec --wrap
	HOOK	= $(OUT)/t/hook.o
	LINK	= $(OUT)/t/hook.o -Wl,--wrap=malloc -Wl,--wrap=free
endif

all: $(OUT)/tester

$(OUT)/t/%.o: src/%.c $(HDRS)
	@mkdir -p $(OUT)/t
	@$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/t/hook.o: src/hook.c src/hook.h
	@mkdir -p $(OUT)/t
	@$(CC) -Wall -Wextra -g -c src/hook.c -o $@

$(OUT)/libfthook.dylib: src/hook.c src/hook.h
	@mkdir -p $(OUT)
	@$(CC) -Wall -Wextra $(HOOKFLAGS) -dynamiclib -install_name @rpath/libfthook.dylib src/hook.c -o $@

$(OUT)/tester: $(OBJS) $(OUT)/libft.a $(HOOK)
	@$(CC) $(LDFLAGS) $(OBJS) $(OUT)/libft.a $(LINK) -o $@

# verifie que le testeur detecte toujours les bugs de tests/bugs (et les traductions)
selftest:
	@bash scripts/selftest.sh

# verifie que tous les textes ont leur traduction anglaise (lang/en.txt, scripts/hints.txt)
i18n:
	@python3 scripts/i18n_check.py

clean:
	rm -rf build trace.log

.PHONY: all clean selftest i18n
