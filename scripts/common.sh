#!/bin/bash
# Fonctions communes aux scripts (a "sourcer").

# Couleurs : desactivees si la sortie n'est pas un terminal ou si NO_COLOR est defini
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ] && [ "${TESTER_COLOR:-1}" = 1 ]; then
	RST=$'\033[0m'; BOLD=$'\033[1m'; DIM=$'\033[2m'
	RED=$'\033[31m'; GRN=$'\033[32m'; YEL=$'\033[33m'; BLU=$'\033[34m'; MAG=$'\033[35m'; CYN=$'\033[36m'
else
	RST=''; BOLD=''; DIM=''; RED=''; GRN=''; YEL=''; BLU=''; MAG=''; CYN=''
fi

# Lance une commande avec une limite de temps (timeout existe sur Linux, gtimeout sur Mac via brew)
with_timeout() {
	local t="$1"; shift
	if command -v timeout >/dev/null 2>&1; then timeout "$t" "$@"
	elif command -v gtimeout >/dev/null 2>&1; then gtimeout "$t" "$@"
	else "$@"; fi
}

# Trouve la norminette (PATH, pipx, pip --user sur Linux et Mac)
find_norminette() {
	command -v norminette 2>/dev/null && return
	for p in "$HOME/.local/bin/norminette" "$HOME"/Library/Python/*/bin/norminette; do
		[ -x "$p" ] && { echo "$p"; return; }
	done
}

# Affiche un chemin de fichier cliquable : relatif si on est dans le dossier, absolu sinon
show_path() {
	local f="$1"
	# en mode --git on teste une copie : on affiche le chemin de ton vrai fichier
	if [ -n "${TESTER_CLONE:-}" ] && [ "${f#"$TESTER_CLONE"}" != "$f" ]; then f="$TESTER_ORIG${f#"$TESTER_CLONE"}"; fi
	if [ -n "${ORIG_PWD:-}" ] && [ "$f" = "$ORIG_PWD" ]; then msg 'ce dossier'; echo
	elif [ -n "${ORIG_PWD:-}" ] && [ "${f#"$ORIG_PWD"/}" != "$f" ]; then echo "${f#"$ORIG_PWD"/}"
	else echo "$f"; fi
}

strip_colors() { perl -pe 's/\e\[[0-9;]*m//g'; }

# Langue (msg, LANG_CODE, lang_menu) : voir i18n.sh
. "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/i18n.sh"
