#!/bin/bash
# Mode --watch : relance les tests des que tu enregistres un fichier de la libft.
# Ne teste que les fonctions des fichiers modifies (tout si libft.h ou le Makefile change).
# Usage : watch.sh <run.sh> <dossier_libft> [fonctions...]
# Verifie toutes les secondes avec find (pas besoin d'inotify, absent des postes de 42).

RUN="$1"; LIBFT="$2"; shift 2
HERE="$(cd "$(dirname "$0")" && pwd)"
. "$HERE/common.sh"
STAMP="$HERE/../build/.watch_stamp"
mkdir -p "$HERE/../build"

trap 'echo; echo "   ${DIM}fin du mode watch${RST}"; rm -f "$STAMP"; exit 0' INT TERM

clear_screen() { [ -t 1 ] && printf '\033[H\033[2J'; }

run() {
	clear_screen
	echo "   ${DIM}$(date +%H:%M:%S) · mode watch · Ctrl+C pour quitter${RST}"
	bash "$RUN" -q "$LIBFT" "$@"
	echo
	echo "   ${CYN}… en attente de modifications dans $(show_path "$LIBFT")${RST}"
}

touch "$STAMP"
run "$@"
while true; do
	sleep 1
	changed="$(find "$LIBFT" -maxdepth 1 \( -name '*.c' -o -name '*.h' -o -name Makefile \) -newer "$STAMP" 2>/dev/null)"
	[ -z "$changed" ] && continue
	sleep 0.3			# laisse l'editeur finir d'ecrire
	touch "$STAMP"
	fns=()
	all=0
	for f in $changed; do
		b="$(basename "$f")"
		case "$b" in
			ft_*.c) fns+=("${b%.c}") ;;
			*) all=1 ;;
		esac
	done
	if [ $all = 1 ] || [ $# -gt 0 ]; then run "$@"; else run "${fns[@]}"; fi
done
