#!/bin/bash
# Mode --watch : relance les tests des que tu enregistres un fichier de la libft.
# Ne teste que les fonctions des fichiers modifies (tout si libft.h ou le Makefile change).
# Usage : watch.sh <run.sh> <dossier_libft> [fonctions...]
# Verifie toutes les secondes avec find (pas besoin d'inotify, absent des postes de 42).

RUN="$1"; LIBFT="$2"; shift 2
HERE="$(cd "$(dirname "$0")" && pwd)"
. "$HERE/common.sh"
STAMP="$HERE/../build/.watch_stamp.$$"	# un par --watch : deux en meme temps ne se genent pas
mkdir -p "$HERE/../build"

trap 'echo; echo "   ${DIM}$(msg "fin du mode watch")${RST}"; rm -f "$STAMP"; exit 0' INT TERM

clear_screen() { [ -t 1 ] && printf '\033[H\033[2J'; }

run() {
	clear_screen
	echo "   ${DIM}$(date +%H:%M:%S) · $(msg 'mode watch · Ctrl+C pour quitter')${RST}"
	bash "$RUN" -q "$LIBFT" "$@"
	echo
	echo "   ${CYN}… $(msg 'en attente de modifications dans %s' "$(show_path "$LIBFT")")${RST}"
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
	while IFS= read -r f; do	# une ligne par fichier : marche aussi avec un espace dans le chemin
		b="$(basename "$f")"
		case "$b" in
			ft_*.c) fns+=("${b%.c}") ;;
			*) all=1 ;;
		esac
	done <<< "$changed"
	if [ $all = 1 ] || [ $# -gt 0 ]; then run "$@"; else run "${fns[@]}"; fi
done
