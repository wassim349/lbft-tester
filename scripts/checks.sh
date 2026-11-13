#!/bin/bash
# Verifications "moulinette" : fichiers, README, libft.h, Makefile, relink,
# fonctions interdites, variables globales.
# (la norminette est faite dans le bilan final, une seule fois)
# Usage : checks.sh <dossier_libft> <dossier_out>
# Ecrit <out>/checks_ko, <out>/checks_warn et <out>/checks_list.txt

LIBFT="$1"
mkdir -p "$2"
OUT="$(cd "$2" && pwd)"
HERE="$(cd "$(dirname "$0")" && pwd)"
FUNCS="$HERE/functions.txt"
. "$HERE/common.sh"
VERBOSE="${TESTER_VERBOSE:-0}"
KO=0; WARN=0; NOK=0
LIST="$OUT/checks_list.txt"
: > "$LIST"
PENDING_TITLE=""

title() { PENDING_TITLE="$*"; [ "$VERBOSE" = 1 ] && flush_title; }
flush_title() {
	[ -z "$PENDING_TITLE" ] && return
	echo; echo " ${CYN}${BOLD}▌ $PENDING_TITLE${RST}"; echo
	PENDING_TITLE=""
}
ok()    { NOK=$((NOK + 1)); [ "$VERBOSE" = 1 ] && echo "   ${GRN}✔${RST} $*"; return 0; }
ko()    { flush_title; echo "   ${RED}✘ $*${RST}"; echo "KO $*" >> "$LIST"; KO=$((KO + 1)); }
warn()  { flush_title; echo "   ${YEL}! $*${RST}"; echo "WARN $*" >> "$LIST"; WARN=$((WARN + 1)); }
info()  { echo "       ${DIM}$*${RST}"; }
tip()   { echo "$*" | fold -s -w 88 | sed "1s/^/       ${CYN}💡${RST} /; 2,\$s/^/          /"; }
indent(){ sed 's/^/       /' | head -n "${1:-12}"; }
mk()    { with_timeout 120 make "$@"; }

cd "$LIBFT" || { msg 'dossier introuvable : %s\n' "$LIBFT"; exit 1; }
HAD_LIB=0; [ -f libft.a ] && HAD_LIB=1

# ------------------------------------------------------------------ fichiers
title "$(msg 'Fichiers du rendu')"

for f in Makefile libft.h README.md; do
	if [ -f "$f" ]; then ok "$(msg '%s présent' "$f")"; else ko "$(msg '%s manquant' "$f")"; fi
done
[ -f README.md ] || tip "$(msg "il manque README.md : relis dans le sujet ce qu'il doit contenir.")"

extra=""; artifacts=""
for f in * .[!.]*; do
	[ -e "$f" ] || continue
	case "$f" in
		Makefile|libft.h|README.md|ft_*.c|.git|.gitignore) ;;
		*.o|*.a) artifacts="$artifacts $f" ;;
		.DS_Store|.vscode|.idea) ;;
		*) extra="$extra $f" ;;
	esac
done
if [ -n "$extra" ]; then
	ko "$(msg 'fichiers non autorisés à la racine (le sujet : Makefile, libft.h, ft_*.c, README.md) :')$extra"
	tip "$(msg 'supprime-les du dépôt (git rm), ou range tes tests ailleurs que dans le dossier rendu.')"
else
	ok "$(msg 'aucun fichier en trop à la racine')"
fi
[ -n "$artifacts" ] && warn "$(msg 'fichiers compilés présents (.o/.a) : ne les push pas sur le dépôt')"
ls .DS_Store >/dev/null 2>&1 && warn "$(msg '.DS_Store présent : ne le push pas (ajoute-le au .gitignore)')"

# ------------------------------------------------------------------- README
title "README.md"
if [ -f README.md ]; then
	first="$(head -n 1 README.md)"
	if echo "$first" | grep -Eq '^[*_]This project has been created as part of the 42 curriculum by [^*_]+[*_][[:space:]]*$'; then
		ok "$(msg '1re ligne en italique conforme')"
	else
		ko "$(msg 'README : 1re ligne incorrecte. Attendu (en italique) :')"
		info "*This project has been created as part of the 42 curriculum by <login>.*"
		info "$(msg 'obtenu : %s' "${first:-<vide>}")"
	fi
	for s in Description Instructions Resources; do
		if grep -Eiq "^#+[[:space:]]*$s" README.md; then ok "$(msg 'section "%s"' "$s")"
		else ko "$(msg 'README : section "%s" manquante' "$s")"; fi
	done
	if grep -Eiq '(\bAI\b|\bIA\b|intelligence artificielle|ChatGPT|Claude|LLM)' README.md; then
		ok "$(msg "usage de l'IA mentionné")"
	else
		ko "$(msg "README : le sujet impose de décrire l'usage de l'IA dans la section Resources")"
	fi
fi

# ------------------------------------------------------------------ libft.h
title "$(msg 'libft.h et prototypes')"
if [ -f libft.h ]; then
	if printf '#include "libft.h"\nint main(void){return 0;}\n' | cc -Wall -Wextra -Werror -I. -x c - -o /dev/null 2>"$OUT/h.err"; then
		ok "$(msg 'libft.h compile tout seul')"
	else
		ko "$(msg 'libft.h ne compile pas tout seul :')"; indent < "$OUT/h.err"
		grep -q 'size_t' "$OUT/h.err" && tip "$(msg "libft.h utilise size_t : quel .h le définit, et libft.h l'inclut-il ?")"
	fi
	grep -Eq '#[[:space:]]*ifndef' libft.h && grep -Eq '#[[:space:]]*define' libft.h \
		&& ok "$(msg 'protection contre la double inclusion (#ifndef)')" \
		|| { ko "$(msg 'pas de protection #ifndef / #define dans libft.h')"
			tip "$(msg 'libft.h doit être protégé contre la double inclusion : cherche « include guard ».')"; }
	HAS_TLIST=0
	if printf '#include "libft.h"\nint main(void){t_list l; l.content = 0; l.next = &l; return (l.next == 0);}\n' \
		| cc -I. -x c - -o /dev/null 2>/dev/null; then
		HAS_TLIST=1; ok "$(msg 'structure t_list (content, next) définie')"
	else
		ko "$(msg "t_list absente ou incorrecte (il faut 'void *content' et 'struct s_list *next')")"
		tip "$(msg 'relis le sujet (partie bonus) : il donne la définition exacte de t_list.')"
	fi
	missing_decl=""
	while read -r fn; do
		grep -Eq "[^a-z_]${fn}[[:space:]]*\(" libft.h || missing_decl="$missing_decl $fn"
	done < "$FUNCS"
	if [ -n "$missing_decl" ]; then
		ko "$(msg 'fonctions non déclarées dans libft.h (%s) :' "$(echo $missing_decl | wc -w | tr -d ' ')")"
		echo $missing_decl | fold -s -w 70 | indent 10
	else
		ok "$(msg 'les 43 fonctions sont déclarées')"
	fi
	extra_flag=""; [ $HAS_TLIST = 1 ] && extra_flag="-DHAS_TLIST"
	cc -I. $extra_flag -fsyntax-only "$HERE/protos.c" 2>"$OUT/protos.err"
	conflicts="$(grep -oE "conflicting types for .ft_[a-z_]+" "$OUT/protos.err" | grep -oE "ft_[a-z_]+" | sort -u)"
	for fn in $conflicts; do
		ko "$(msg 'prototype de %s différent du sujet :' "$fn")"
		info "$(msg 'sujet   :') $(grep -E "[^a-z_]$fn\(" "$HERE/protos.c" | tr -s ' \t' ' ')"
		info "$(msg 'le tien :') $(grep -oE "[^;]*[^a-z_]${fn}[[:space:]]*\([^;]*;" libft.h | head -1 | sed 's/^[[:space:]]*//' | tr -s ' \t' ' ')"
		[ "$fn" = ft_lstsize ] && info "$(msg "(le sujet v19.3 demande 'unsigned int', les anciennes versions 'int')")"
	done
	[ -z "$conflicts" ] && ok "$(msg 'les prototypes déclarés correspondent au sujet v19.3')"
	if grep -nw restrict libft.h ft_*.c 2>/dev/null | grep -q .; then
		ko "$(msg "mot-clé 'restrict' interdit par le sujet :")"; grep -nw restrict libft.h ft_*.c | indent 5
	fi
fi

# ----------------------------------------------------------------- Makefile
title "Makefile"
if [ -f Makefile ]; then
	for r in '\$\(NAME\)' all clean fclean re; do
		name="$(echo "$r" | tr -d '\\')"
		grep -Eq "^${r}[[:space:]]*:" Makefile && ok "$(msg 'règle %s' "$name")" || ko "$(msg 'règle %s manquante dans le Makefile' "$name")"
	done
	grep -Eq '^NAME[[:space:]]*:?=[[:space:]]*libft\.a' Makefile && ok "NAME = libft.a" || ko "$(msg 'NAME = libft.a introuvable dans le Makefile')"
	grep -q libtool Makefile && ko "$(msg 'libtool est interdit (utilise ar)')"
	grep -Eq '(^|[[:space:]])ar[[:space:]]' Makefile && ok "$(msg 'utilise ar')" || ko "$(msg 'ar introuvable dans le Makefile')"
	if grep -Eq '\$\(wildcard|\$\(shell|[^.]\*\.c' Makefile; then
		warn "$(msg 'Makefile : $(wildcard), $(shell) ou *.c trouvé : souvent refusé en correction, liste les fichiers à la main')"
	fi

	mk fclean >/dev/null 2>&1
	if mk > "$OUT/make.out" 2>&1; then
		ok "$(msg 'make compile sans erreur')"
	else
		ko "$(msg 'make échoue (à la moulinette : 0 pour tout le projet) :')"
		grep -E 'error|Error|\*\*\*' "$OUT/make.out" | head -n 8 | indent 8
		if grep -qE '^[^ ]*ft_[a-z_]+\.c:[0-9]+' "$OUT/make.out"; then
			tip "$(msg "c'est une erreur de compilation dans un de tes .c : elle est détaillée (avec un conseil) dans le bilan, plus bas.")"
		elif grep -qiE 'missing separator' "$OUT/make.out"; then
			tip "$(msg 'Makefile : les commandes des règles doivent commencer par une tabulation, pas par des espaces.')"
		elif grep -qiE 'No rule to make target' "$OUT/make.out"; then
			tip "$(msg "Makefile : un fichier listé dans tes sources n'existe pas (faute de frappe ou fichier pas créé).")"
		fi
	fi
	compile_lines="$(grep -E '(^|[[:space:]])-c([[:space:]]|$)' "$OUT/make.out")"
	if [ -n "$compile_lines" ]; then
		bad=0
		for flag in -Wall -Wextra -Werror; do
			echo "$compile_lines" | grep -v -- "$flag" | grep -q . && { ko "$(msg "flag %s absent d'au moins une compilation" "$flag")"; bad=1; }
		done
		[ $bad = 0 ] && ok "$(msg 'flags -Wall -Wextra -Werror partout')"
		[ $bad = 1 ] && tip "$(msg 'chaque compilation de .c doit avoir ces trois flags : regarde la règle qui fabrique les .o.')"
		echo "$compile_lines" | grep -Ev '^[[:space:]]*cc[[:space:]]' | grep -q . \
			&& warn "$(msg "le sujet impose 'cc' comme compilateur")" || ok "$(msg 'compile avec cc')"
	fi
	if [ -f libft.a ]; then
		ok "$(msg 'libft.a créé à la racine')"
		cp libft.a "$OUT/libft_make.a"
		out="$(mk 2>&1)"
		if echo "$out" | grep -Eq '(^|[[:space:]])(cc|gcc|clang|ar)[[:space:]]'; then
			ko "$(msg "relink : un 2e 'make' recompile alors que rien n'a changé")"
			echo "$out" | indent 4
			tip "$(msg 'regarde de quoi dépend la règle $(NAME), et si elle est .PHONY.')"
		else
			ok "$(msg 'pas de relink')"
		fi
		touch libft.h 2>/dev/null
		out="$(mk 2>&1)"
		if echo "$out" | grep -Eq '(^|[[:space:]])(cc|gcc|clang)[[:space:]]'; then
			ok "$(msg 'modifier libft.h relance la compilation')"
		else
			warn "$(msg 'modifier libft.h ne recompile rien : ajoute libft.h aux dépendances des .o (ex. %%.o: %%.c libft.h)')"
		fi
		mk clean >/dev/null 2>&1
		if ls ./*.o >/dev/null 2>&1; then ko "$(msg 'make clean laisse des .o')"; else ok "$(msg 'make clean supprime les .o')"; fi
		[ -f libft.a ] && ok "$(msg 'make clean garde libft.a')" || ko "$(msg 'make clean ne doit pas supprimer libft.a')"
		mk fclean >/dev/null 2>&1
		[ -f libft.a ] && ko "$(msg 'make fclean ne supprime pas libft.a')" || ok "$(msg 'make fclean supprime libft.a')"
		mk re >/dev/null 2>&1
		[ -f libft.a ] && ok "$(msg 'make re reconstruit libft.a')" || ko "$(msg 'make re ne reconstruit pas libft.a')"
		mk fclean >/dev/null 2>&1
		if mk -j4 >/dev/null 2>&1 && [ -f libft.a ]; then ok "$(msg 'make -j4 fonctionne')"
		else warn "$(msg 'make -j4 échoue (compilation en parallèle) : vérifie les dépendances de tes règles')"; fi
	else
		ko "$(msg "libft.a n'a pas été créé par make")"
	fi
fi

# ---------------------------------------------------------------- libft.a
if [ -f "$OUT/libft_make.a" ]; then
	title "$(msg 'Contenu de libft.a')"
	LIB="$OUT/libft_make.a"
	defined="$(nm -g "$LIB" 2>/dev/null | awk '$2=="T"{print $3}' | sed 's/^_//' | sort -u)"
	missing=""
	while read -r fn; do
		echo "$defined" | grep -qx "$fn" || missing="$missing $fn"
	done < "$FUNCS"
	if [ -n "$missing" ]; then
		ko "$(msg 'fonctions absentes de libft.a (%s/43) :' "$(echo $missing | wc -w | tr -d ' ')")"
		echo $missing | fold -s -w 70 | indent 10
	else
		ok "$(msg 'les 43 fonctions sont dans libft.a')"
	fi
	echo "$defined" | grep -qx main && ko "$(msg 'un main() est compilé dans libft.a (à retirer)')"

	allowed=" malloc free write __stack_chk_fail __stack_chk_guard _stack_chk_fail _stack_chk_guard dyld_stub_binder __chkstk_darwin _GLOBAL_OFFSET_TABLE_ __gmon_start__ "
	forbidden=""; compiler=""
	nm -u "$LIB" 2>/dev/null \
		| awk '/:$/{obj=$0; sub(/:$/,"",obj); sub(/.*\(/,"",obj); sub(/\)$/,"",obj); next} NF{print obj" "$NF}' \
		> "$OUT/undef.txt"
	while read -r obj sym; do
		s="${sym#_}"
		echo "$defined" | grep -qx "$s" && continue
		case "$allowed" in *" $s "*|*" $sym "*) continue ;; esac
		case "$s" in
			memset|memcpy|memmove|bzero|__bzero|__memset_chk|__memcpy_chk) compiler="$compiler $s(${obj##*/})" ;;
			*) forbidden="$forbidden $s(${obj##*/})" ;;
		esac
	done < "$OUT/undef.txt"
	if [ -n "$forbidden" ]; then
		ko "$(msg 'fonctions externes interdites (seuls malloc, free, write sont permis) :')"
		echo "$forbidden" | tr ' ' '\n' | grep . | sort -u | indent 15
		tip "$(msg 'remplace-les par tes propres ft_ (ex. strlen → ft_strlen) ; printf et les autres sont interdits dans la libft.')"
	else
		ok "$(msg 'aucune fonction externe interdite (seulement malloc, free, write)')"
	fi
	[ -n "$compiler" ] && warn "$(msg 'appels à memset/memcpy générés par le compilateur (souvent un tableau initialisé avec = {0}) :')$compiler"

	globals="$(nm "$LIB" 2>/dev/null | awk '$2 ~ /^[BDRCGS]$/ {print $3}' | grep -Ev '^_?(L|l_|\.)' | sed 's/^_//' | sort -u)"
	statics="$(nm "$LIB" 2>/dev/null | awk '$2 ~ /^[bdgs]$/ {print $3}' | grep -Ev '^_?(L|l_|\.)' | sed 's/^_//' | sort -u)"
	if [ -n "$globals" ]; then ko "$(msg 'variables globales interdites : %s' "$(echo $globals)")"; else ok "$(msg 'aucune variable globale')"; fi
	[ -n "$statics" ] && warn "$(msg 'variables static trouvées (globales de fichier) : %s' "$(echo $statics)")"

	unused=""
	members="$(ar t "$LIB" 2>/dev/null)"
	for f in ft_*.c; do
		[ -e "$f" ] || continue
		echo "$members" | grep -qx "${f%.c}.o" || unused="$unused $f"
	done
	if [ -n "$unused" ]; then
		ko "$(msg 'fichiers .c non compilés par le Makefile (inutiles ou oubliés dans SRC) :')"
		echo $unused | fold -s -w 70 | indent 10
	else
		ok "$(msg 'tous les ft_*.c sont compilés dans libft.a')"
	fi
fi
rm -f "$OUT/libft_make.a"

# ------------------------------------------------------------------- fin
[ $HAD_LIB = 0 ] && mk fclean >/dev/null 2>&1
echo
if [ $KO = 0 ] && [ $WARN = 0 ]; then
	echo "   ${GRN}✔ $(msg 'vérifications du rendu : tout est bon')${RST} ${DIM}$(msg '(%s points vérifiés)' "$NOK")${RST}"
else
	echo "   ${DIM}$(msg 'vérifications du rendu : %s OK,' "$NOK") ${RST}${RED}$(msg '%s erreur(s)' "$KO")${RST}${DIM}, ${RST}${YEL}$(msg '%s avertissement(s)' "$WARN")${RST}"
fi
echo "$KO" > "$OUT/checks_ko"
echo "$WARN" > "$OUT/checks_warn"
exit 0
