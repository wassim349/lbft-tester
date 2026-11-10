#!/bin/bash
# Langue de l'affichage (francais / anglais). Charge par common.sh.
#
#   msg 'format' args...   comme printf, mais traduit le format (le texte francais sert de cle)
#   LANG_CODE              fr ou en
#
# Langue choisie, par ordre de priorite :
#   1. variable LIBFTEST_LANG (fr|en)
#   2. fichier ~/.config/libftest/lang (ecrit par le menu : libftest --lang)
#   3. francais
#
# Traductions : lang/en.txt, des paires de lignes "fr: ..." / "en: ...".
# Regle pour les cles : pas de $variable dans le format (utilise %s), pas de \" ni \\.
# Verifier que tout est traduit : make i18n

I18N_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIBFTEST_CONF="${XDG_CONFIG_HOME:-$HOME/.config}/libftest"
TESTER_CATALOG="$I18N_ROOT/lang/en.txt"
export TESTER_CATALOG

lang_load() {
	LANG_CODE="${LIBFTEST_LANG:-}"
	if [ -z "$LANG_CODE" ] && [ -f "$LIBFTEST_CONF/lang" ]; then
		LANG_CODE="$(head -n 1 "$LIBFTEST_CONF/lang" 2>/dev/null | tr -d ' \r\n')"
	fi
	case "$LANG_CODE" in fr|en) ;; *) LANG_CODE=fr ;; esac
	LIBFTEST_LANG="$LANG_CODE"
	export LIBFTEST_LANG
	_tr() { R="$1"; }
	if [ "$LANG_CODE" = en ] && [ -f "$TESTER_CATALOG" ]; then
		# le catalogue devient une fonction "case" (bash 3.2 sur Mac : pas de tableaux associatifs)
		eval "$(awk '
			function q(s) { gsub(/\047/, "\047\\\047\047", s); return "\047" s "\047" }
			/^fr: / { k = substr($0, 5); next }
			/^en: / && k != "" { v = substr($0, 5); if (v != "") print "\t\t" q(k) ") R=" q(v) " ;;"; k = "" }
			BEGIN { print "_tr() {\n\tcase \"$1\" in" }
			END { print "\t\t*) R=\"$1\" ;;\n\tesac\n}" }' "$TESTER_CATALOG")"
	fi
}

msg() {
	_tr "$1"
	shift
	# shellcheck disable=SC2059
	printf -- "$R" "$@"
}

# langue enregistree pour les prochaines fois
lang_save() {
	mkdir -p "$LIBFTEST_CONF" 2>/dev/null && echo "$1" > "$LIBFTEST_CONF/lang"
}

# menu : Langue / Language. Lit le clavier (meme si stdin est redirige).
# Renvoie 1 si personne ne peut repondre (pas de terminal).
lang_menu() {
	local cur="${1:-$LANG_CODE}" ans def=1
	[ -r /dev/tty ] && { : < /dev/tty; } 2>/dev/null || return 1
	[ "$cur" = en ] && def=2
	echo
	echo "   ${CYN}${BOLD}Langue / Language${RST}"
	echo
	echo "     ${BOLD}1${RST}) Français"
	echo "     ${BOLD}2${RST}) English"
	echo
	while true; do
		printf '   [%s] > ' "$def"
		IFS= read -r ans < /dev/tty || return 1
		[ -z "$ans" ] && ans=$def
		case "$ans" in
			1|fr|FR|f|F) LIBFTEST_LANG=fr; break ;;
			2|en|EN|e|E) LIBFTEST_LANG=en; break ;;
			*) echo "   1 / 2 ?" ;;
		esac
	done
	lang_save "$LIBFTEST_LANG"
	lang_load
	echo "   ${GRN}✔${RST} $(msg 'langue : français')  ${DIM}$(msg '(pour changer : libftest --lang)')${RST}"
	return 0
}

lang_load
