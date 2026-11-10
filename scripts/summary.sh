#!/bin/bash
# Bilan final : pour chaque fonction, tests + compilation + norminette,
# puis la liste de ce qu'il faut corriger, trie par priorite, avec un conseil sous chaque erreur.
# Usage : summary.sh <dossier_libft> <dossier_out> [fonctions...]
# Env   : TESTER_VERBOSE=1 (tout afficher), TESTER_STATE=<fichier> (progression),
#         TESTER_CHECK_KO=<n> (erreurs des verifications du rendu, pour « Par où commencer »)
# Code de retour : 0 si tout est OK, 1 sinon.

LIBFT="$1"; OUT="$2"; shift 2
HERE="$(cd "$(dirname "$0")" && pwd)"
. "$HERE/common.sh"
RESULTS="$OUT/results.txt"
TRACE="$OUT/trace.log"			# dans la langue choisie : affichage
TRACE_KEY="$OUT/trace.key"		# la meme, en francais : pour trouver les conseils
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
	local f="$1" perr
	perr="$(msg 'la norminette ne comprend pas la ligne (erreur de syntaxe ?)')"
	sed -E 's/^[[:space:]]*Error: *PARSING_ERROR.*\(([0-9]+), *[0-9]+\).*/\1	PARSING_ERROR	@PERR@/;
		s/^[[:space:]]*Error: *([A-Z_]+) *\(line: *([0-9]+)[^)]*\):[[:space:]]*/\2	\1	/' "$NDIR/$f" \
		| head -n "$2" | while IFS=$'\t' read -r l code m; do
			[ "$m" = @PERR@ ] && m="$perr"
			echo "$(show_path "$LIBFT/$f"):$l  ${DIM}$code${RST}  $m"
		done
}
norm_codes() {	# fichier : "NORM CODE message" par erreur (pour les conseils)
	sed -E 's/^[[:space:]]*Error: *PARSING_ERROR.*/NORM PARSING_ERROR/;
		s/^[[:space:]]*Error: *([A-Z_]+) *\([^)]*\):[[:space:]]*/NORM \1 /' "$NDIR/$1"
}

# ------------------------------------------------------------ conseils
# Pour chaque fonction on ecrit ses erreurs, une par ligne et en francais, dans $HDIR/<fn>,
# puis on cherche les conseils qui correspondent dans hints.txt (format explique dans le fichier).
HINTS="$HERE/hints.txt"
HDIR="$OUT/hints"; rm -rf "${HDIR:?}"; mkdir -p "$HDIR"
SEEN="$HDIR/.seen"; : > "$SEEN"
PROTOS_C="$OUT/protos_c.txt"
MAXH=3; [ "$VERBOSE" = 1 ] && MAXH=99
HCOL=3; [ "$LANG_CODE" = en ] && HCOL=4

# erreurs des tests d'une fonction (trace en francais) : "nom du test → message → message"
test_errors() {
	awk -v f="$1" '
		$0 == "[KO] " f || $0 == "[OK] " f { on=1; next }
		on && /^$/ { exit }
		on && /^     (✘|!)/ { if (t != "") print "TEST " t; t=""; if ($0 ~ /^     ✘/) { t=$0; sub(/^ +✘ /, "", t) }; next }
		on && t != "" && /→/ { m=$0; sub(/^ +→ /, "", m); t=t " → " m }
		END { if (t != "") print "TEST " t }' "$TRACE_KEY" 2>/dev/null
}

# affiche les conseils de la fonction $1 (ceux de norme deja vus plus haut : pas repetes)
show_hints() {
	local fn="$1" f="$HDIR/$1"
	[ -s "$f" ] && [ -f "$HINTS" ] || return 0
	awk -F'\t+' -v fn="$fn" -v max="$MAXH" -v seen="$SEEN" -v hints="$HINTS" -v col="$HCOL" '
		FILENAME == seen { S[$0]=1; next }
		FILENAME == hints {
			if ($0 ~ /^#/ || NF < 3) next
			n++; F[n]=$1; R[n]=$2; H[n]=(NF >= col && $col != "" ? $col : $3); next }
		{ L[++m]=$0 }
		END {	# chaque erreur prend la 1re regle qui lui correspond
			c = 0
			for (j = 1; j <= m && c < max; j++) {
				norm = (L[j] ~ /^NORM /)
				for (i = 1; i <= n; i++) {
					if (F[i] != "*" && fn !~ ("^(" F[i] ")$")) continue
					if (F[i] != "*" && L[j] !~ /^TEST / && !(L[j] ~ /^PROTO / && R[i] ~ /^\^PROTO/)) continue
					if (L[j] !~ R[i]) continue
					if (!(H[i] in D) && !(norm && (H[i] in S))) {
						D[H[i]]=1; print H[i]; c++
						if (norm) { S[H[i]]=1; print H[i] >> seen }
					}
					break
				}
			}
		}' "$SEEN" "$HINTS" - < "$f" \
	| while IFS= read -r h; do
		echo "$h" | fold -s -w 88 | sed "1s/^/       ${CYN}💡${RST} /; 2,\$s/^/          /"
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
	# fichier ecarte a la compilation (build_lib.sh) : "CODE detail"
	cerr="$(grep -E "^$fn " "$ERRS" 2>/dev/null | head -n 1)"
	cerr="${cerr#"$fn "}"
	ccode="${cerr%% *}"; cdetail=""; [ "$ccode" != "$cerr" ] && cdetail="${cerr#* }"
	hkey=""	# la meme erreur, en francais, pour les conseils
	nn=0; [ -s "$file" ] && nn="$(norm_count "$fn.c")"
	d=""
	if [ ! -e "$file" ]; then
		cat=5
	elif [ ! -s "$file" ]; then
		cat=5; d+="     ${RED}✘ $(msg '%s est vide' "$(show_path "$file")")${RST}"$'\n'
	elif [ -n "$cerr" ]; then
		cat=1
		case "$ccode" in
			COMPILE)
				hkey="ne compile pas : $cdetail"
				if [[ "$cdetail" =~ ^(ft_[a-z_]+\.[ch]):([0-9]+):[0-9]+:\ (fatal\ )?error:\ (.*)$ ]]; then
					d+="     ${RED}✘ $(msg 'ne compile pas')${RST}  $(show_path "$LIBFT/${BASH_REMATCH[1]}"):${BASH_REMATCH[2]}  ${BASH_REMATCH[4]}"$'\n'
				else
					d+="     ${RED}✘ $(msg 'ne compile pas')${RST}  $cdetail"$'\n'
				fi ;;
			MAIN)
				hkey="contient un main()"
				d+="     ${RED}✘ $(msg 'contient un main() : retire-le (la moulinette a son propre main, ça ne linke pas)')${RST}"$'\n' ;;
			DUP)
				hkey="definit aussi $cdetail"
				d+="     ${RED}✘ $(msg "définit aussi %s (déjà dans %s.c) : 'duplicate symbol' au link" "$cdetail" "$cdetail")${RST}"$'\n' ;;
			DEP)
				hkey="utilise $cdetail, qui ne compile pas"
				d+="     ${RED}✘ $(msg "utilise %s, qui ne compile pas ou n'existe pas" "$cdetail")${RST}"$'\n' ;;
			*)
				d+="     ${RED}✘ $cerr${RST}"$'\n' ;;
		esac
	elif [ "$st" = ABSENTE ]; then
		cat=1
		defs="$(grep -oE '^[a-zA-Z_].*[^a-z_](ft_[a-z_]+)[[:space:]]*\(' "$file" | grep -oE 'ft_[a-z_]+' | sort -u | tr '\n' ' ')"
		if ! echo " $defs" | grep -q " $fn "; then
			hkey="ne contient pas la fonction"
			d+="     ${RED}✘ $(msg '%s ne contient pas la fonction %s' "$(show_path "$file")" "$fn")${RST} ${DIM}$(msg '(il définit : %s)' "${defs:-$(msg 'rien')}")${RST}"$'\n'
		elif ! grep -Eq "[^a-z_]$fn[[:space:]]*\(" "$LIBFT/libft.h" 2>/dev/null; then
			hkey="n'est pas declaree dans libft.h"
			d+="     ${RED}✘ $(msg "%s n'est pas déclarée dans libft.h" "$fn")${RST}"$'\n'
		else
			d+="     ${RED}✘ $(msg '%s absente de libft.a' "$fn")${RST}"$'\n'
		fi
	elif [ "$st" = NONE ]; then
		cat=1; d+="     ${RED}✘ $(msg "pas testée : le testeur n'a pas pu être compilé avec ta libft (voir plus haut)")${RST}"$'\n'
	elif [ "$st" = KO ]; then
		if [ "$crash" -gt 0 ] || [ "$leak" -gt 0 ]; then cat=2; else cat=3; fi
		d+="     ${RED}✘ $(msg '%s test(s) raté(s) sur %s' "$((total - passed))" "$total")${RST}"$'\n'
		d+="$(awk -v f="$fn" -v max="$MAXT" -v more="$(msg '… et %s autre(s) (tout voir : -v, ou trace.log)' @N@)" '
			$0 == "[KO] " f || $0 == "[OK] " f { on=1; next }
			on && /^$/ { exit }
			on && /^     ✘/ { n++; show = (n <= max); if (show) { sub(/^ +/, ""); print "       " $0 } ; next }
			on && /^     !/ { show=0; next }
			on && show && /→/ { sub(/^ +/, ""); print "           " $0 }
			END { if (n > max) { sub(/@N@/, n - max, more); print "       " more } }' "$TRACE" 2>/dev/null \
			| sed "s|${TESTER_CLONE:-/nonexistent}|${TESTER_ORIG:-}|g; s|${ORIG_PWD:-/nonexistent}/||g")"$'\n'
	elif [ "$nn" -gt 0 ]; then
		cat=4
	else
		cat=0
	fi
	# prototype du .c different du sujet (build_lib.sh)
	pk=""; RISK[$i]=""
	pl="$(awk -F'\t' -v f="$fn" '$1 == f' "$PROTOS_C" 2>/dev/null | head -n 1)"
	if [ -n "$pl" ]; then
		pk="$(echo "$pl" | cut -f2)"; pdef="$(echo "$pl" | cut -f3-)"
		psub="$(grep -E "[^a-z_]$fn\(" "$HERE/protos.c" | tr -s ' \t' ' ')"
		if [ "$pk" = type ]; then
			{ [ $cat = 0 ] || [ $cat = 4 ]; } && cat=3
			d+="     ${RED}✘ $(msg 'prototype différent du sujet')${RST}"$'\n'
			d+="         ${DIM}$(msg 'sujet   :')${RST} $psub"$'\n'
			d+="         ${DIM}$(msg 'le tien :')${RST} $pdef"$'\n'
		elif [ $cat != 1 ]; then	# fichier ecarte : il y a plus urgent
			RISK[$i]="$(msg 'prototype : les const diffèrent du sujet : %s' "$pdef")  ${DIM}$(msg '(sujet : %s)' "$psub")${RST}"
		else
			pk=""
		fi
	fi
	if [ "$nn" -gt 0 ] && [ -s "$file" ]; then
		d+="     ${RED}✘ $(msg 'norminette : %s erreur(s)' "$nn")${RST}"$'\n'
		d+="$(norm_lines "$fn.c" "$MAXN" | sed 's/^/         /')"$'\n'
	fi
	# erreurs de la fonction, une par ligne, pour trouver les conseils (show_hints)
	{
		[ -n "$hkey" ] && echo "COMPIL $hkey"
		[ "$pk" = type ] && echo "PROTO type"
		# mauvais prototype + crash : le crash vient du prototype, pas la peine d'en dire plus
		[ "$st" = KO ] && ! { [ "$pk" = type ] && [ "${crash:-0}" -gt 0 ]; } && test_errors "$fn"
		[ "$pk" = const ] && echo "PROTO const"
		[ "$nn" -gt 0 ] && [ -s "$file" ] && norm_codes "$fn.c" | head -n "$MAXN"
	} > "$HDIR/$fn"
	# avertissements (pas bloquants) ; inutiles si la fonction plante deja
	if [ "${warns:-0}" -gt 0 ] && [ $cat != 1 ] && [ $cat != 2 ]; then
		RISK[$i]="${RISK[$i]:+${RISK[$i]}$'\n'}$(awk -v f="$fn" '
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
echo " ${CYN}${BOLD}▌ $(msg 'Bilan')${RST}"
echo
W=30
filled=$(( n_total > 0 ? n_ok * W / n_total : 0 ))
bar="$(printf '%*s' "$filled" '' | sed 's/ /█/g')$(printf '%*s' $((W - filled)) '' | sed 's/ /░/g')"
color=$RED; [ "$n_ok" = "$n_total" ] && color=$GRN; [ "$n_ok" -gt 0 ] && [ "$n_ok" != "$n_total" ] && color=$YEL
echo "   ${color}${bar}${RST}  ${BOLD}$n_ok/$n_total${RST} $(msg 'fonction(s) prête(s)')"
[ -n "$gained" ] && echo "   ${GRN}▲ $(msg 'nouvelles réussites :')${RST}$gained" | fold -s -w 100 | sed '2,$s/^/      /'
[ -n "$lost" ]   && echo "   ${RED}▼ $(msg 'cassées depuis la dernière fois :')${RST}$lost"
if [ -n "$ok_list" ] && [ "$n_ok" != "$n_total" ]; then
	echo
	echo "   ${GRN}✅ $(msg 'prêtes :')${RST}${DIM}$ok_list${RST}" | fold -s -w 100 | sed '2,$s/^/      /'
fi

if [ "$VERBOSE" = 1 ]; then
	echo
	printf "   ${BOLD}%-18s %s${RST}\n" "$(msg 'fonction')" "$(msg 'verdict')"
	for i in "${!FUNCS[@]}"; do
		fn="${FUNCS[$i]}"
		case "${CAT[$i]}" in
			0) v="${GRN}✅ OK${RST}" ;; 1) v="${RED}❌ $(msg 'bloque la compilation')${RST}" ;;
			2) v="${RED}❌ $(msg 'plante / fuit')${RST}" ;; 3) v="${RED}❌ $(msg 'résultat faux')${RST}" ;;
			4) v="${YEL}❌ $(msg 'norme')${RST}" ;; 5) v="${DIM}❌ $(msg 'pas écrite')${RST}" ;;
		esac
		printf "   %-18s %s\n" "$fn" "$v"
	done
fi

first=""
titles=("" "$(msg 'Bloque la compilation') ${DIM}→ $(msg 'à la moulinette, 0 pour tout le projet')${RST}"
	"$(msg 'Plante, fuit ou boucle')" "$(msg 'Résultat faux')"
	"$(msg 'Norme seulement') ${DIM}$(msg '(les tests passent)')${RST}" "$(msg 'Pas encore écrites')")
any=0
for c in 1 2 3 4 5; do
	list=(); idx=()
	for i in "${!FUNCS[@]}"; do [ "${CAT[$i]}" = "$c" ] && { list+=("${FUNCS[$i]}"); idx+=("$i"); }; done
	[ $c = 4 ] && [ $HDR_KO = 1 ] && list+=("libft.h")
	[ ${#list[@]} -eq 0 ] && continue
	if [ $any = 0 ]; then echo; echo " ${CYN}${BOLD}▌ $(msg 'À corriger, dans cet ordre')${RST}"; any=1; fi
	echo
	echo "   ${BOLD}$c. ${titles[$c]}${RST}"
	if [ $c = 5 ]; then
		echo "      ${DIM}${list[*]}${RST}" | fold -s -w 100 | sed '2,$s/^/      /'
		continue
	fi
	for k in "${!list[@]}"; do
		fn="${list[$k]}"
		[ -z "$first" ] && first="$fn"
		if [ "$fn" = libft.h ]; then
			echo "   ${BOLD}  libft.h${RST}"
			echo "     ${RED}✘ $(msg 'norminette : %s erreur(s)' "$(norm_count libft.h)")${RST}"
			norm_lines libft.h "$MAXN" | sed 's/^/         /'
			norm_codes libft.h | head -n "$MAXN" > "$HDIR/libft.h"; show_hints libft.h
			continue
		fi
		echo "   ${BOLD}  $fn${RST}"
		printf '%s' "${DETAIL[${idx[$k]}]}" | grep -v '^$'
		show_hints "$fn"
	done
done

if [ -n "$(printf '%s' "${RISK[@]}")" ]; then
	echo
	echo " ${YEL}${BOLD}▌ $(msg 'Risques')${RST} ${DIM}$(msg '(ne bloquent pas, mais certains correcteurs/testeurs les comptent faux)')${RST}"
	echo
	for i in "${!FUNCS[@]}"; do
		fn="${FUNCS[$i]}"
		[ -n "${RISK[$i]:-}" ] || continue
		echo "${RISK[$i]}" | head -n "$MAXT" | sed "s/^/   ${YEL}!${RST} $fn  /"
	done
fi

ck="${TESTER_CHECK_KO:--}"
if [ -n "$first" ] || { [ "$ck" != - ] && [ "$ck" != 0 ]; }; then
	echo
	echo " ${CYN}${BOLD}▌ $(msg 'Par où commencer')${RST}"
	echo
	if [ "$ck" != - ] && [ "$ck" != 0 ]; then
		echo "   $(msg 'Les vérifications du rendu (en haut) ont trouvé %s erreur(s) : un Makefile qui échoue, c’est 0 pour tout le projet.' "$ck")"
		[ -n "$first" ] && echo "   $(msg 'Ensuite :')"
	fi
	if [ "$first" = libft.h ]; then
		echo "   $(msg 'Corrige la norme de %s, puis relance %s.' "${BOLD}libft.h${RST}" "${BOLD}libftest${RST}")"
	elif [ -n "$first" ]; then
		echo "   $(msg 'Corrige %s en premier, puis relance juste elle : %s' "${BOLD}$first${RST}" "${BOLD}libftest ${first#ft_}${RST}")"
		echo "   ${DIM}$(msg 'ou laisse tourner %s : ça relance à chaque sauvegarde.' "${RST}libftest --watch ${first#ft_}${DIM}")${RST}"
	fi
fi

[ -z "$NORM" ] && { echo; echo "   ${YEL}! $(msg 'norminette pas installée : ./install.sh (ou python3 -m pip install --user norminette)')${RST}"; }
[ $any = 0 ] && [ $HDR_KO = 0 ]
