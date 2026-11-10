#!/bin/bash
# Bilan final : pour chaque fonction, tests + compilation + norminette,
# puis la liste de ce qu'il faut corriger, trie par priorite.
# Usage : summary.sh <dossier_libft> <dossier_out> [fonctions...]
# Env   : TESTER_VERBOSE=1 (tout afficher), TESTER_STATE=<fichier> (progression)
# Code de retour : 0 si tout est OK, 1 sinon.

LIBFT="$1"; OUT="$2"; shift 2
HERE="$(cd "$(dirname "$0")" && pwd)"
. "$HERE/common.sh"
RESULTS="$OUT/results.txt"
TRACE="$OUT/trace.log"
ERRS="$OUT/compile_errors.txt"
VERBOSE="${TESTER_VERBOSE:-0}"
STATE="${TESTER_STATE:-}"
NORM="$(find_norminette)"
MAXT=3; MAXN=3
[ "$VERBOSE" = 1 ] && { MAXT=99; MAXN=99; }

selected() {
	local fn="$1"; shift
	[ $# -eq 0 ] && return 0
	for a in "$@"; do [ "$a" = "$fn" ] || [ "ft_$a" = "$fn" ] && return 0; done
	return 1
}
FUNCS=()
while read -r fn; do selected "$fn" "$@" && FUNCS+=("$fn"); done < "$HERE/functions.txt"

# ------------------------------------------------------------ norminette
# un seul appel pour tous les fichiers (la norminette est lente a demarrer)
NDIR="$OUT/norm"; rm -rf "${NDIR:?}"; mkdir -p "$NDIR"
if [ -n "$NORM" ]; then
	files=()
	for fn in "${FUNCS[@]}"; do [ -s "$LIBFT/$fn.c" ] && files+=("$LIBFT/$fn.c"); done
	[ $# -eq 0 ] && [ -f "$LIBFT/libft.h" ] && files+=("$LIBFT/libft.h")
	if [ ${#files[@]} -gt 0 ]; then
		(cd "$LIBFT" && with_timeout 120 "$NORM" "${files[@]##*/}" 2>&1) | strip_colors | awk -v d="$NDIR" '
			/^[^ \t].*: (OK|Error)!/ { f=$0; sub(/: (OK|Error)!.*/, "", f); sub(/.*\//, "", f);
				file=d "/" f; printf "" > file; next }
			/Error/ && file { print >> file }'
	fi
fi
norm_count() { if [ -f "$NDIR/$1" ]; then grep -c 'Error' "$NDIR/$1"; else echo 0; fi; }
norm_lines() {	# fichier, max : "ft_x.c:12  CODE  message"
	local f="$1"
	sed -E 's/^[[:space:]]*Error: *PARSING_ERROR.*\(([0-9]+), *[0-9]+\).*/\1	PARSING_ERROR	la norminette ne comprend pas la ligne (erreur de syntaxe ?)/;
		s/^[[:space:]]*Error: *([A-Z_]+) *\(line: *([0-9]+)[^)]*\):[[:space:]]*/\2	\1	/' "$NDIR/$f" \
		| head -n "$2" | while IFS=$'\t' read -r l code msg; do
			echo "$(show_path "$LIBFT/$f"):$l  ${DIM}$code${RST}  $msg"
		done
}

# ------------------------------------------------------------ analyse
CAT=(); DETAIL=(); RISK=()	# indexes comme FUNCS (bash 3.2 sur Mac : pas de tableaux associatifs)
n_ok=0; n_total=${#FUNCS[@]}; ok_list=""
for i in "${!FUNCS[@]}"; do
	fn="${FUNCS[$i]}"
	file="$LIBFT/$fn.c"
	line="$(grep -E "^$fn " "$RESULTS" 2>/dev/null | head -n 1)"
	read -r _ st passed total crash leak wrong warns <<< "${line:-x NONE 0 0 0 0 0 0}"
	cerr="$(grep -E "^$fn " "$ERRS" 2>/dev/null | head -n 1)"
	cerr="${cerr#"$fn "}"
	nn=0; [ -s "$file" ] && nn="$(norm_count "$fn.c")"
	d=""
	if [ ! -e "$file" ]; then
		cat=5
	elif [ ! -s "$file" ]; then
		cat=5; d+="     ${RED}✘ $(show_path "$file") est vide${RST}"$'\n'
	elif [ -n "$cerr" ]; then
		cat=1
		if [[ "$cerr" =~ ^ne\ compile\ pas\ :\ (ft_[a-z_]+\.[ch]):([0-9]+):[0-9]+:\ (fatal\ )?error:\ (.*)$ ]]; then
			d+="     ${RED}✘ ne compile pas${RST}  $(show_path "$LIBFT/${BASH_REMATCH[1]}"):${BASH_REMATCH[2]}  ${BASH_REMATCH[4]}"$'\n'
		else
			d+="     ${RED}✘ $cerr${RST}"$'\n'
		fi
	elif [ "$st" = ABSENTE ]; then
		cat=1
		defs="$(grep -oE '^[a-zA-Z_].*[^a-z_](ft_[a-z_]+)[[:space:]]*\(' "$file" | grep -oE 'ft_[a-z_]+' | sort -u | tr '\n' ' ')"
		if ! echo " $defs" | grep -q " $fn "; then
			d+="     ${RED}✘ $(show_path "$file") ne contient pas la fonction $fn${RST} ${DIM}(il définit : ${defs:-rien})${RST}"$'\n'
		elif ! grep -Eq "[^a-z_]$fn[[:space:]]*\(" "$LIBFT/libft.h" 2>/dev/null; then
			d+="     ${RED}✘ $fn n'est pas déclarée dans libft.h${RST}"$'\n'
		else
			d+="     ${RED}✘ $fn absente de libft.a${RST}"$'\n'
		fi
	elif [ "$st" = NONE ]; then
		cat=1; d+="     ${RED}✘ pas testée : le testeur n'a pas pu être compilé avec ta libft (voir plus haut)${RST}"$'\n'
	elif [ "$st" = KO ]; then
		if [ "$crash" -gt 0 ] || [ "$leak" -gt 0 ]; then cat=2; else cat=3; fi
		d+="     ${RED}✘ $((total - passed)) test(s) raté(s) sur $total${RST}"$'\n'
		d+="$(awk -v f="$fn" -v max="$MAXT" '
			$0 == "[KO] " f || $0 == "[OK] " f { on=1; next }
			on && /^$/ { exit }
			on && /^     ✘/ { n++; show = (n <= max); if (show) { sub(/^ +/, ""); print "       " $0 } ; next }
			on && /^     !/ { show=0; next }
			on && show && /→/ { sub(/^ +/, ""); print "           " $0 }
			END { if (n > max) print "       … et " n - max " autre(s) (tout voir : -v, ou " "trace.log)" }' "$TRACE" 2>/dev/null \
			| sed "s|${TESTER_CLONE:-/nonexistent}|${TESTER_ORIG:-}|g; s|${ORIG_PWD:-/nonexistent}/||g")"$'\n'
	elif [ "$nn" -gt 0 ]; then
		cat=4
	else
		cat=0
	fi
	if [ "$nn" -gt 0 ] && [ -s "$file" ]; then
		d+="     ${RED}✘ norminette : $nn erreur(s)${RST}"$'\n'
		d+="$(norm_lines "$fn.c" "$MAXN" | sed 's/^/         /')"$'\n'
	fi
	# avertissements (pas bloquants)
	if [ "${warns:-0}" -gt 0 ]; then
		RISK[$i]="$(awk -v f="$fn" '
			$0 == "[KO] " f || $0 == "[OK] " f { on=1; next }
			on && /^$/ { exit }
			on && /^     !/ { show=1; sub(/^ +! /, ""); t=$0; next }
			on && /^     ✘/ { show=0; next }
			on && show && /→/ { sub(/^ +→ /, ""); print t " → " $0; show=0 }' "$TRACE" 2>/dev/null)"
	fi
	CAT[$i]=$cat; DETAIL[$i]="$d"
	[ $cat = 0 ] && { n_ok=$((n_ok + 1)); ok_list="$ok_list $fn"; }
done

HDR_KO=0
if [ $# -eq 0 ] && [ -f "$LIBFT/libft.h" ] && [ -n "$NORM" ] && [ "$(norm_count libft.h)" -gt 0 ]; then HDR_KO=1; fi

# ------------------------------------------------------------ progression
PREVF="$OUT/prev_state.txt"; : > "$PREVF"
if [ -n "$STATE" ] && [ -f "$STATE" ] && [ "$(head -n 1 "$STATE")" = "$LIBFT" ]; then
	tail -n +2 "$STATE" > "$PREVF"
fi
gained=""; lost=""
NEWF="$OUT/new_state.txt"; grep -v -E "^($(IFS='|'; echo "${FUNCS[*]}")) " "$PREVF" > "$NEWF" 2>/dev/null
for i in "${!FUNCS[@]}"; do
	fn="${FUNCS[$i]}"
	now=KO; [ "${CAT[$i]}" = 0 ] && now=OK
	prev="$(awk -v f="$fn" '$1 == f {print $2}' "$PREVF")"
	[ "$prev" = KO ] && [ $now = OK ] && gained="$gained ${fn}"
	[ "$prev" = OK ] && [ $now = KO ] && lost="$lost ${fn}"
	echo "$fn $now" >> "$NEWF"
done
if [ -n "$STATE" ]; then
	{ echo "$LIBFT"; cat "$NEWF"; } > "$STATE"
fi

# ------------------------------------------------------------ affichage
echo
echo " ${CYN}${BOLD}▌ Bilan${RST}"
echo
W=30
filled=$(( n_total > 0 ? n_ok * W / n_total : 0 ))
bar="$(printf '%*s' "$filled" '' | sed 's/ /█/g')$(printf '%*s' $((W - filled)) '' | sed 's/ /░/g')"
color=$RED; [ "$n_ok" = "$n_total" ] && color=$GRN; [ "$n_ok" -gt 0 ] && [ "$n_ok" != "$n_total" ] && color=$YEL
echo "   ${color}${bar}${RST}  ${BOLD}$n_ok/$n_total${RST} fonction(s) prête(s)"
[ -n "$gained" ] && echo "   ${GRN}▲ nouvelles réussites :${RST}$gained" | fold -s -w 100 | sed '2,$s/^/      /'
[ -n "$lost" ]   && echo "   ${RED}▼ cassées depuis la dernière fois :${RST}$lost"
if [ -n "$ok_list" ] && [ "$n_ok" != "$n_total" ]; then
	echo
	echo "   ${GRN}✅ prêtes :${RST}${DIM}$ok_list${RST}" | fold -s -w 100 | sed '2,$s/^/      /'
fi

if [ "$VERBOSE" = 1 ]; then
	echo
	printf "   ${BOLD}%-18s %s${RST}\n" fonction verdict
	for i in "${!FUNCS[@]}"; do
		fn="${FUNCS[$i]}"
		case "${CAT[$i]}" in
			0) v="${GRN}✅ OK${RST}" ;; 1) v="${RED}❌ bloque la compilation${RST}" ;;
			2) v="${RED}❌ plante / fuit${RST}" ;; 3) v="${RED}❌ résultat faux${RST}" ;;
			4) v="${YEL}❌ norme${RST}" ;; 5) v="${DIM}❌ pas écrite${RST}" ;;
		esac
		printf "   %-18s %s\n" "$fn" "$v"
	done
fi

titles=("" "Bloque la compilation ${DIM}→ à la moulinette, 0 pour tout le projet${RST}"
	"Plante, fuit ou boucle" "Résultat faux" "Norme seulement ${DIM}(les tests passent)${RST}" "Pas encore écrites")
any=0
for c in 1 2 3 4 5; do
	list=(); idx=()
	for i in "${!FUNCS[@]}"; do [ "${CAT[$i]}" = "$c" ] && { list+=("${FUNCS[$i]}"); idx+=("$i"); }; done
	[ $c = 4 ] && [ $HDR_KO = 1 ] && list+=("libft.h")
	[ ${#list[@]} -eq 0 ] && continue
	if [ $any = 0 ]; then echo; echo " ${CYN}${BOLD}▌ À corriger, dans cet ordre${RST}"; any=1; fi
	echo
	echo "   ${BOLD}$c. ${titles[$c]}${RST}"
	if [ $c = 5 ]; then
		echo "      ${DIM}${list[*]}${RST}" | fold -s -w 100 | sed '2,$s/^/      /'
		continue
	fi
	for k in "${!list[@]}"; do
		fn="${list[$k]}"
		if [ "$fn" = libft.h ]; then
			echo "   ${BOLD}  libft.h${RST}"
			echo "     ${RED}✘ norminette : $(norm_count libft.h) erreur(s)${RST}"
			norm_lines libft.h "$MAXN" | sed 's/^/         /'
			continue
		fi
		echo "   ${BOLD}  $fn${RST}"
		printf '%s' "${DETAIL[${idx[$k]}]}" | grep -v '^$'
	done
done

if [ ${#RISK[@]} -gt 0 ]; then
	echo
	echo " ${YEL}${BOLD}▌ Risques${RST} ${DIM}(ne bloquent pas, mais certains correcteurs/testeurs les comptent faux)${RST}"
	echo
	for i in "${!FUNCS[@]}"; do
		fn="${FUNCS[$i]}"
		[ -n "${RISK[$i]:-}" ] || continue
		echo "${RISK[$i]}" | head -n "$MAXT" | sed "s/^/   ${YEL}!${RST} $fn  /"
	done
fi

[ -z "$NORM" ] && { echo; echo "   ${YEL}! norminette pas installée : ./install.sh (ou python3 -m pip install --user norminette)${RST}"; }
[ $any = 0 ] && [ $HDR_KO = 0 ]
