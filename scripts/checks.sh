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
indent(){ sed 's/^/       /' | head -n "${1:-12}"; }
mk()    { with_timeout 120 make "$@"; }

cd "$LIBFT" || { echo "dossier introuvable : $LIBFT"; exit 1; }
HAD_LIB=0; [ -f libft.a ] && HAD_LIB=1

# ------------------------------------------------------------------ fichiers
title "Fichiers du rendu"

for f in Makefile libft.h README.md; do
	if [ -f "$f" ]; then ok "$f présent"; else ko "$f manquant"; fi
done

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
	ko "fichiers non autorisés à la racine (le sujet : Makefile, libft.h, ft_*.c, README.md) :$extra"
else
	ok "aucun fichier en trop à la racine"
fi
[ -n "$artifacts" ] && warn "fichiers compilés présents (.o/.a) : ne les push pas sur le dépôt"
ls .DS_Store >/dev/null 2>&1 && warn ".DS_Store présent : ne le push pas (ajoute-le au .gitignore)"

# ------------------------------------------------------------------- README
title "README.md"
if [ -f README.md ]; then
	first="$(head -n 1 README.md)"
	if echo "$first" | grep -Eq '^[*_]This project has been created as part of the 42 curriculum by [^*_]+[*_][[:space:]]*$'; then
		ok "1re ligne en italique conforme"
	else
		ko "README : 1re ligne incorrecte. Attendu (en italique) :"
		info "*This project has been created as part of the 42 curriculum by <login>.*"
		info "obtenu : ${first:-<vide>}"
	fi
	for s in Description Instructions Resources; do
		if grep -Eiq "^#+[[:space:]]*$s" README.md; then ok "section \"$s\""; else ko "README : section \"$s\" manquante"; fi
	done
	if grep -Eiq '(\bAI\b|\bIA\b|intelligence artificielle|ChatGPT|Claude|LLM)' README.md; then
		ok "usage de l'IA mentionné"
	else
		ko "README : le sujet impose de décrire l'usage de l'IA dans la section Resources"
	fi
fi

# ------------------------------------------------------------------ libft.h
title "libft.h et prototypes"
if [ -f libft.h ]; then
	if printf '#include "libft.h"\nint main(void){return 0;}\n' | cc -Wall -Wextra -Werror -I. -x c - -o /dev/null 2>"$OUT/h.err"; then
		ok "libft.h compile tout seul"
	else
		ko "libft.h ne compile pas tout seul :"; indent < "$OUT/h.err"
	fi
	grep -Eq '#[[:space:]]*ifndef' libft.h && grep -Eq '#[[:space:]]*define' libft.h \
		&& ok "protection contre la double inclusion (#ifndef)" \
		|| ko "pas de protection #ifndef / #define dans libft.h"
	HAS_TLIST=0
	if printf '#include "libft.h"\nint main(void){t_list l; l.content = 0; l.next = &l; return (l.next == 0);}\n' \
		| cc -I. -x c - -o /dev/null 2>/dev/null; then
		HAS_TLIST=1; ok "structure t_list (content, next) définie"
	else
		ko "t_list absente ou incorrecte (il faut 'void *content' et 'struct s_list *next')"
	fi
	missing_decl=""
	while read -r fn; do
		grep -Eq "[^a-z_]$fn[[:space:]]*\(" libft.h || missing_decl="$missing_decl $fn"
	done < "$FUNCS"
	if [ -n "$missing_decl" ]; then
		ko "fonctions non déclarées dans libft.h ($(echo $missing_decl | wc -w | tr -d ' ')) :"
		echo $missing_decl | fold -s -w 70 | indent 10
	else
		ok "les 43 fonctions sont déclarées"
	fi
	extra_flag=""; [ $HAS_TLIST = 1 ] && extra_flag="-DHAS_TLIST"
	cc -I. $extra_flag -fsyntax-only "$HERE/protos.c" 2>"$OUT/protos.err"
	conflicts="$(grep -oE "conflicting types for .ft_[a-z_]+" "$OUT/protos.err" | grep -oE "ft_[a-z_]+" | sort -u)"
	for fn in $conflicts; do
		ko "prototype de $fn différent du sujet :"
		info "sujet   : $(grep -E "[^a-z_]$fn\(" "$HERE/protos.c" | tr -s ' \t' ' ')"
		info "le tien : $(grep -oE "[^;]*[^a-z_]$fn[[:space:]]*\([^;]*;" libft.h | head -1 | sed 's/^[[:space:]]*//' | tr -s ' \t' ' ')"
		[ "$fn" = ft_lstsize ] && info "(le sujet v19.3 demande 'unsigned int', les anciennes versions 'int')"
	done
	[ -z "$conflicts" ] && ok "les prototypes déclarés correspondent au sujet v19.3"
	if grep -nw restrict libft.h ft_*.c 2>/dev/null | grep -q .; then
		ko "mot-clé 'restrict' interdit par le sujet :"; grep -nw restrict libft.h ft_*.c | indent 5
	fi
fi

# ----------------------------------------------------------------- Makefile
title "Makefile"
if [ -f Makefile ]; then
	for r in '\$\(NAME\)' all clean fclean re; do
		name="$(echo "$r" | tr -d '\\')"
		grep -Eq "^$r[[:space:]]*:" Makefile && ok "règle $name" || ko "règle $name manquante dans le Makefile"
	done
	grep -Eq '^NAME[[:space:]]*:?=[[:space:]]*libft\.a' Makefile && ok "NAME = libft.a" || ko "NAME = libft.a introuvable dans le Makefile"
	grep -q libtool Makefile && ko "libtool est interdit (utilise ar)"
	grep -Eq '(^|[[:space:]])ar[[:space:]]' Makefile && ok "utilise ar" || ko "ar introuvable dans le Makefile"
	if grep -Eq '\$\(wildcard|\$\(shell|[^.]\*\.c' Makefile; then
		warn "Makefile : \$(wildcard), \$(shell) ou *.c trouvé : souvent refusé en correction, liste les fichiers à la main"
	fi

	mk fclean >/dev/null 2>&1
	if mk > "$OUT/make.out" 2>&1; then
		ok "make compile sans erreur"
	else
		ko "make échoue (à la moulinette : 0 pour tout le projet) :"
		grep -E 'error|Error|\*\*\*' "$OUT/make.out" | head -n 8 | indent 8
	fi
	compile_lines="$(grep -E '(^|[[:space:]])-c([[:space:]]|$)' "$OUT/make.out")"
	if [ -n "$compile_lines" ]; then
		bad=0
		for flag in -Wall -Wextra -Werror; do
			echo "$compile_lines" | grep -v -- "$flag" | grep -q . && { ko "flag $flag absent d'au moins une compilation"; bad=1; }
		done
		[ $bad = 0 ] && ok "flags -Wall -Wextra -Werror partout"
		echo "$compile_lines" | grep -Ev '^[[:space:]]*cc[[:space:]]' | grep -q . \
			&& warn "le sujet impose 'cc' comme compilateur" || ok "compile avec cc"
	fi
	if [ -f libft.a ]; then
		ok "libft.a créé à la racine"
		cp libft.a "$OUT/libft_make.a"
		out="$(mk 2>&1)"
		if echo "$out" | grep -Eq '(^|[[:space:]])(cc|gcc|clang|ar)[[:space:]]'; then
			ko "relink : un 2e 'make' recompile alors que rien n'a changé"
			echo "$out" | indent 4
		else
			ok "pas de relink"
		fi
		touch libft.h 2>/dev/null
		out="$(mk 2>&1)"
		if echo "$out" | grep -Eq '(^|[[:space:]])(cc|gcc|clang)[[:space:]]'; then
			ok "modifier libft.h relance la compilation"
		else
			warn "modifier libft.h ne recompile rien : ajoute libft.h aux dépendances des .o (ex. %.o: %.c libft.h)"
		fi
		mk clean >/dev/null 2>&1
		if ls ./*.o >/dev/null 2>&1; then ko "make clean laisse des .o"; else ok "make clean supprime les .o"; fi
		[ -f libft.a ] && ok "make clean garde libft.a" || ko "make clean ne doit pas supprimer libft.a"
		mk fclean >/dev/null 2>&1
		[ -f libft.a ] && ko "make fclean ne supprime pas libft.a" || ok "make fclean supprime libft.a"
		mk re >/dev/null 2>&1
		[ -f libft.a ] && ok "make re reconstruit libft.a" || ko "make re ne reconstruit pas libft.a"
		mk fclean >/dev/null 2>&1
		if mk -j4 >/dev/null 2>&1 && [ -f libft.a ]; then ok "make -j4 fonctionne"
		else warn "make -j4 échoue (compilation en parallèle) : vérifie les dépendances de tes règles"; fi
	else
		ko "libft.a n'a pas été créé par make"
	fi
fi

# ---------------------------------------------------------------- libft.a
if [ -f "$OUT/libft_make.a" ]; then
	title "Contenu de libft.a"
	LIB="$OUT/libft_make.a"
	defined="$(nm -g "$LIB" 2>/dev/null | awk '$2=="T"{print $3}' | sed 's/^_//' | sort -u)"
	missing=""
	while read -r fn; do
		echo "$defined" | grep -qx "$fn" || missing="$missing $fn"
	done < "$FUNCS"
	if [ -n "$missing" ]; then
		ko "fonctions absentes de libft.a ($(echo $missing | wc -w | tr -d ' ')/43) :"
		echo $missing | fold -s -w 70 | indent 10
	else
		ok "les 43 fonctions sont dans libft.a"
	fi
	echo "$defined" | grep -qx main && ko "un main() est compilé dans libft.a (à retirer)"

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
		ko "fonctions externes interdites (seuls malloc, free, write sont permis) :"
		echo "$forbidden" | tr ' ' '\n' | grep . | sort -u | indent 15
	else
		ok "aucune fonction externe interdite (seulement malloc, free, write)"
	fi
	[ -n "$compiler" ] && warn "appels à memset/memcpy générés par le compilateur (souvent un tableau initialisé avec = {0}) :$compiler"

	globals="$(nm "$LIB" 2>/dev/null | awk '$2 ~ /^[BDRCGS]$/ {print $3}' | grep -Ev '^_?(L|l_|\.)' | sed 's/^_//' | sort -u)"
	statics="$(nm "$LIB" 2>/dev/null | awk '$2 ~ /^[bdgs]$/ {print $3}' | grep -Ev '^_?(L|l_|\.)' | sed 's/^_//' | sort -u)"
	if [ -n "$globals" ]; then ko "variables globales interdites : $(echo $globals)"; else ok "aucune variable globale"; fi
	[ -n "$statics" ] && warn "variables static trouvées (globales de fichier) : $(echo $statics)"

	unused=""
	members="$(ar t "$LIB" 2>/dev/null)"
	for f in ft_*.c; do
		[ -e "$f" ] || continue
		echo "$members" | grep -qx "${f%.c}.o" || unused="$unused $f"
	done
	if [ -n "$unused" ]; then
		ko "fichiers .c non compilés par le Makefile (inutiles ou oubliés dans SRC) :"
		echo $unused | fold -s -w 70 | indent 10
	else
		ok "tous les ft_*.c sont compilés dans libft.a"
	fi
fi
rm -f "$OUT/libft_make.a"

# ------------------------------------------------------------------- fin
[ $HAD_LIB = 0 ] && mk fclean >/dev/null 2>&1
echo
if [ $KO = 0 ] && [ $WARN = 0 ]; then
	echo "   ${GRN}✔ vérifications du rendu : tout est bon${RST} ${DIM}($NOK points vérifiés)${RST}"
else
	echo "   ${DIM}vérifications du rendu : $NOK OK, ${RST}${RED}$KO erreur(s)${RST}${DIM}, ${RST}${YEL}$WARN avertissement(s)${RST}"
fi
echo "$KO" > "$OUT/checks_ko"
echo "$WARN" > "$OUT/checks_warn"
exit 0
