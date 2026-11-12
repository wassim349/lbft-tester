#!/bin/bash
# libft-tester : lance "./run.sh -h" pour l'aide.

# --- dossier du testeur (meme si on passe par le lien "libftest" installe par install.sh)
SELF="$0"
while [ -L "$SELF" ]; do
	link="$(readlink "$SELF")"
	case "$link" in /*) SELF="$link" ;; *) SELF="$(dirname "$SELF")/$link" ;; esac
done
HERE="$(cd "$(dirname "$SELF")" && pwd)"
. "$HERE/scripts/common.sh"

usage() {
	if [ "$LANG_CODE" = en ]; then cat <<EOF
${BOLD}libftest${RST} [options] [libft_folder] [functions...]

  No folder: uses the current folder if it contains libft.h, else ../libft.

  ${BOLD}Modes${RST}
    (nothing)     submission checks + tests of every function
    -q            tests only (no Makefile, README, libft.h...)
    --asan        also catches silent memory errors (one byte overflow...)
    --git         tests what is COMMITTED in git (like the moulinette), not your folder
    --final       everything, strict, before you submit: --git + --asan + checks
    --watch       reruns the tests every time you save a file (Ctrl+C to quit)

  ${BOLD}Options${RST}
    -v            show everything (every test, full table)
    --seed N      replay the random tests with seed N
    --lang        choose the language (menu), or --lang fr / --lang en
    --no-color    no colors
    --update      update the tester (git pull)
    -h            this help

  ${BOLD}Examples${RST}
    libftest                      in your libft folder
    libftest atoi split           only ft_atoi and ft_split
    libftest --final ~/libft      before you submit
EOF
	else cat <<EOF
${BOLD}libftest${RST} [options] [dossier_libft] [fonctions...]

  Sans dossier : utilise le dossier courant s'il contient libft.h, sinon ../libft.

  ${BOLD}Modes${RST}
    (rien)        vérifications du rendu + tests de toutes les fonctions
    -q            tests seulement (sans Makefile, README, libft.h...)
    --asan        détecte aussi les erreurs mémoire silencieuses (débordement d'un octet...)
    --git         teste ce qui est COMMITÉ dans git (comme la moulinette), pas ton dossier
    --final       tout, en strict, avant le rendu : --git + --asan + vérifications
    --watch       relance les tests dès que tu enregistres un fichier (Ctrl+C pour quitter)

  ${BOLD}Options${RST}
    -v            tout afficher (détail de chaque test, tableau complet)
    --seed N      rejoue les tests aléatoires avec la graine N
    --lang        choisit la langue (menu), ou --lang fr / --lang en
    --no-color    sans couleurs
    --update      met le testeur à jour (git pull)
    -h            cette aide

  ${BOLD}Exemples${RST}
    libftest                      dans ton dossier libft
    libftest atoi split           seulement ft_atoi et ft_split
    libftest --final ~/libft      avant de rendre
EOF
	fi
}

# --- options
QUICK=0; ASAN=0; GIT=0; FINAL=0; WATCH=0; VERBOSE=0; SEED=""; LIBFT=""; FN=()
while [ $# -gt 0 ]; do
	case "$1" in
		-q|--quick) QUICK=1 ;;
		-v|--verbose) VERBOSE=1 ;;
		--asan) ASAN=1 ;;
		--git) GIT=1 ;;
		--final) FINAL=1; GIT=1; ASAN=1; QUICK=0 ;;
		--watch) WATCH=1 ;;
		--seed)
			shift
			case "$1" in ''|*[!0-9]*) echo "${RED}$(msg '--seed attend un nombre (ex. --seed 42)')${RST}"; exit 2 ;; esac
			SEED="$1" ;;
		--lang)
			case "${2:-}" in
				fr|en) lang_save "$2"; LIBFTEST_LANG="$2"; lang_load
					echo "   ${GRN}✔${RST} $(msg 'langue : français')"; exit 0 ;;
				'') lang_menu || { echo "${RED}$(msg 'pas de terminal pour le menu : libftest --lang fr (ou en)')${RST}"; exit 2; }
					exit 0 ;;
				*) echo "${RED}$(msg 'langue inconnue : %s (fr ou en)' "$2")${RST}"; exit 2 ;;
			esac ;;
		--no-color) export TESTER_COLOR=0; . "$HERE/scripts/common.sh" ;;
		--update)
			if git -C "$HERE" pull --ff-only; then echo "${GRN}✔ $(msg 'testeur à jour')${RST}"; exit 0
			else echo "${RED}✘ $(msg 'mise à jour impossible (pas de dépôt distant, ou modifications locales)')${RST}"; exit 1; fi ;;
		-h|--help) usage; exit 0 ;;
		-*) echo "${RED}$(msg 'option inconnue : %s' "$1")${RST}"; usage; exit 2 ;;
		*) if [ -z "$LIBFT" ] && [ -d "$1" ]; then LIBFT="$1"; else FN+=("$1"); fi ;;
	esac
	shift
done

# 1er lancement dans un terminal : choisir la langue (une seule fois)
if [ -z "${LIBFTEST_LANG_SET:-}" ] && [ ! -f "$LIBFTEST_CONF/lang" ] && [ -t 0 ] && [ -t 1 ] && [ $WATCH = 0 ]; then
	lang_menu && echo
fi
export LIBFTEST_LANG_SET=1

export ORIG_PWD="$PWD"
is_libft() { [ -f "$1/libft.h" ] || ls "$1"/ft_*.c >/dev/null 2>&1; }
if [ -z "$LIBFT" ]; then
	if is_libft "$PWD"; then LIBFT="$PWD"
	elif is_libft "$PWD/libft"; then LIBFT="$PWD/libft"
	elif is_libft "$HERE/../libft"; then
		LIBFT="$HERE/../libft"
		echo "   ${YEL}! $(msg 'pas de libft dans ce dossier : je teste %s' "$(cd "$LIBFT" && pwd)")${RST}"
	else
		echo "${RED}$(msg 'Je ne trouve pas ta libft.')${RST} $(msg 'Va dans ton dossier libft, ou donne son chemin :')"
		echo "   cd $(msg '~/chemin/vers/libft') && libftest"
		echo "   libftest $(msg '~/chemin/vers/libft')"
		exit 1
	fi
elif ! is_libft "$LIBFT"; then
	echo "${RED}$(msg '%s ne ressemble pas à une libft' "$LIBFT")${RST} $(msg '(pas de libft.h ni de ft_*.c dedans).')"
	exit 1
fi
LIBFT="$(cd "$LIBFT" && pwd)"

# noms de fonctions : "atoi" ou "ft_atoi" ; un nom inconnu est une erreur (sinon rien n'est teste)
bad=0
for f in "${FN[@]}"; do
	if ! grep -qx "ft_${f#ft_}" "$HERE/scripts/functions.txt"; then
		short="${f#ft_}"
		letters() { echo "$1" | fold -w1 | sort | tr -d '\n'; }
		guess="$(while read -r g; do g="${g#ft_}"; [ "$(letters "$g")" = "$(letters "$short")" ] && echo "$g"; done < "$HERE/scripts/functions.txt")"
		[ -z "$guess" ] && guess="$(grep -E "ft_.*(${short:0:3}|${short: -3})" "$HERE/scripts/functions.txt" | head -n 4 | sed 's/^ft_//')"
		guess="$(echo $guess)"
		echo "${RED}$(msg 'fonction inconnue : %s' "$f")${RST}${guess:+   $(msg 'tu voulais dire : %s' "$guess")}"
		bad=1
	fi
done
if [ $bad = 1 ]; then
	echo "${DIM}$(msg "(si c'est un dossier, il n'existe pas)")${RST}"
	exit 2
fi

if [ $WATCH = 1 ]; then
	exec bash "$HERE/scripts/watch.sh" "$HERE/run.sh" "$LIBFT" "${FN[@]}"
fi

# ne pas monopoliser le processeur : priorite basse pour tout ce qui suit
renice -n 10 -p $$ >/dev/null 2>&1

BUILD="$HERE/build/$(uname -s)-$(uname -m)"	# un dossier par systeme : pas de .o Mac utilises sous Linux
mkdir -p "$BUILD"

# un seul libftest a la fois par systeme (ex. --watch dans un terminal + libftest dans un autre) :
# ils partagent build/, le 2e attend la fin du 1er. Verrou abandonne (process mort) : repris.
LOCK="$BUILD/.lock"
waited=0
while ! mkdir "$LOCK" 2>/dev/null; do
	owner="$(cat "$LOCK/pid" 2>/dev/null)"
	if [ -n "$owner" ] && ! kill -0 "$owner" 2>/dev/null; then rm -rf "$LOCK"; continue; fi
	if [ -z "$owner" ] && [ -n "$(find "$LOCK" -maxdepth 0 -mmin +10 2>/dev/null)" ]; then rm -rf "$LOCK"; continue; fi
	[ $waited = 0 ] && echo "   ${YEL}… $(msg "un autre libftest tourne déjà : j'attends qu'il finisse")${RST}"
	waited=$((waited + 1))
	sleep 1
done
echo $$ > "$LOCK/pid"
trap 'rm -rf "$LOCK"' EXIT
trap 'rm -rf "$LOCK"; exit 130' INT TERM
if [ $ASAN = 1 ]; then
	# ASan doit compiler ET demarrer (sur certains macOS il se bloque au lancement)
	asan_ok=0
	if echo 'int main(void){return 0;}' | cc -fsanitize=address -x c - -o "$BUILD/asan_check" >/dev/null 2>&1; then
		"$BUILD/asan_check" >/dev/null 2>&1 &
		apid=$!
		for _ in 1 2 3 4 5 6 7 8 9 10; do kill -0 $apid 2>/dev/null || break; sleep 0.3; done
		if kill -0 $apid 2>/dev/null; then kill -9 $apid 2>/dev/null; wait $apid 2>/dev/null
		else wait $apid && asan_ok=1; fi
	fi
	if [ $asan_ok = 0 ]; then
		echo "   ${YEL}! $(msg 'ASan ne fonctionne pas sur cette machine (il se bloque ou ne compile pas) : mode normal.')${RST}"
		echo "   ${DIM}  $(msg 'Sur les postes Linux de 42 il marche : lance --asan / --final là-bas.')${RST}"
		ASAN=0
	fi
	rm -f "$BUILD/asan_check"
fi
OUT="$BUILD/normal"; [ $ASAN = 1 ] && OUT="$BUILD/asan"
if [ $ASAN = 1 ] && [ "$(uname -s)" = Darwin ]; then
	echo "   ${DIM}$(msg '(sur Mac, en mode ASan, les fuites et les malloc qui échouent ne sont pas testés :')"
	echo "    $(msg "le mode normal s'en charge. Sous Linux, tout est testé en même temps.)")${RST}"
fi
mkdir -p "$OUT"

mode="$(msg 'normal')"; [ $QUICK = 1 ] && mode="$(msg 'tests seuls')"; [ $ASAN = 1 ] && mode="$mode + ASan"
[ $GIT = 1 ] && mode="$mode + git"; [ $FINAL = 1 ] && mode="$(msg 'FINAL (strict)')"
echo
echo "   ${CYN}${BOLD}libft-tester${RST} ${DIM}· $(msg 'sujet v19.3') · $(uname -s)${RST}"
echo "   ${DIM}$(msg 'libft :')${RST} $(show_path "$LIBFT")   ${DIM}$(msg 'mode :')${RST} $mode"

# ---------------------------------------------------------- 0. git
GIT_KO=0
if [ $GIT = 1 ]; then
	echo
	echo " ${CYN}${BOLD}▌ $(msg 'Dépôt git')${RST}"
	echo
	if ! git -C "$LIBFT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
		echo "   ${RED}✘ $(msg "%s n'est pas un dépôt git : impossible de savoir ce que tu vas rendre" "$LIBFT")${RST}"
		GIT_KO=1
	else
		dirty="$(git -C "$LIBFT" status --porcelain -- . 2>/dev/null)"
		if [ -n "$dirty" ]; then
			echo "   ${RED}✘ $(msg 'modifications pas commitées (la moulinette ne les verra pas) :')${RST}"
			echo "$dirty" | head -n 12 | sed 's/^/       /'
			GIT_KO=1
		else
			echo "   ${GRN}✔${RST} $(msg 'tout est commité')"
		fi
		ahead="$(git -C "$LIBFT" rev-list --count '@{u}..HEAD' 2>/dev/null)"
		if [ -z "$ahead" ]; then
			echo "   ${YEL}! $(msg 'pas de branche distante suivie : vérifie que tu push bien sur le dépôt de rendu')${RST}"
		elif [ "$ahead" -gt 0 ]; then
			echo "   ${RED}✘ $(msg '%s commit(s) pas encore push : fais git push' "$ahead")${RST}"
			GIT_KO=1
		else
			echo "   ${GRN}✔${RST} $(msg 'tout est push') ${DIM}$(msg "(d'après le dernier fetch)")${RST}"
		fi
		rm -rf "${BUILD:?}/rendu"
		if git clone -q "$LIBFT" "$BUILD/rendu" 2>/dev/null; then
			echo "   ${DIM}→ $(msg 'je teste une copie propre du dernier commit (ce que la moulinette verra)')${RST}"
			export TESTER_CLONE="$BUILD/rendu" TESTER_ORIG="$LIBFT"
			LIBFT="$BUILD/rendu"
		else
			echo "   ${RED}✘ $(msg 'git clone a échoué : je teste ton dossier tel quel')${RST}"
		fi
	fi
fi

# ---------------------------------------------------------- 1. verifications
CHECK_KO="-"
if [ $QUICK = 0 ]; then
	TESTER_VERBOSE=$VERBOSE bash "$HERE/scripts/checks.sh" "$LIBFT" "$BUILD/checks"
	CHECK_KO="$(cat "$BUILD/checks/checks_ko" 2>/dev/null || echo 0)"
fi

# ---------------------------------------------------------- 2. compilation
CFLAGS_EXTRA=""; [ $ASAN = 1 ] && CFLAGS_EXTRA="-fsanitize=address -fno-omit-frame-pointer -g"
bash "$HERE/scripts/build_lib.sh" "$LIBFT" "$OUT" $CFLAGS_EXTRA
rm -f "$OUT/results.txt"
: > "$OUT/trace.log"
if [ -s "$OUT/compile_errors.txt" ]; then
	n="$(wc -l < "$OUT/compile_errors.txt" | tr -d ' ')"
	echo
	echo "   ${YEL}! $(msg '%s fichier(s) écarté(s) (ne compile pas, main() oublié...) : détail dans le bilan.' "$n")${RST}"
	echo "   ${DIM}  $(msg 'Je teste tous les autres.')${RST}"
fi

# si libft.h a change (ou une autre libft), on recompile le testeur
if ! cmp -s "$LIBFT/libft.h" "$OUT/libft.h.last" 2>/dev/null; then
	rm -rf "${OUT:?}/t"
	cp "$LIBFT/libft.h" "$OUT/libft.h.last" 2>/dev/null
fi

TEST_RC=1
if [ ! -f "$OUT/libft.a" ]; then
	echo; echo "   ${RED}${BOLD}$(msg 'Aucun fichier ne compile : tests des fonctions annulés.')${RST}"
else
	# ------------------------------------------------------------- 3. tests
	bash "$HERE/scripts/gen_available.sh" "$LIBFT" "$OUT/libft.a" "$OUT/available.h"
	# make ne supporte pas les espaces dans les chemins ("~/mon dossier/libft") : on lui donne
	# des chemins relatifs au testeur, et la libft a travers un lien sans espace.
	ROUT="${OUT#"$HERE"/}"
	rm -f "$OUT/libft_src"; ln -s "$LIBFT" "$OUT/libft_src"
	if ! make -s -C "$HERE" LIBFT="$ROUT/libft_src" OUT="$ROUT" ASAN=$ASAN > "$OUT/tester_build.log" 2>&1; then
		echo; echo "   ${RED}${BOLD}$(msg 'Le testeur ne compile pas avec ta libft :')${RST}"
		grep -E 'error|undefined|duplicate' "$OUT/tester_build.log" | head -n 10 | sed -e 's#[^ ]*/libft_src/##' -e 's/^/     /'
		echo "   ${DIM}$(msg '(souvent : un prototype de libft.h différent du sujet, voir plus haut)')${RST}"
	else
		echo
		export TESTER_OUT="$OUT" TESTER_VERBOSE=$VERBOSE
		[ -n "$SEED" ] && export TESTER_SEED="$SEED"
		export ASAN_OPTIONS="detect_leaks=0:allocator_may_return_null=1:max_allocation_size_mb=1024:print_summary=1"
		if [ -n "$RED" ]; then
			( cd "$HERE" && "$OUT/tester" "${FN[@]}" )
		else
			( cd "$HERE" && "$OUT/tester" "${FN[@]}" ) | strip_colors
		fi
		TEST_RC=${PIPESTATUS[0]}
	fi
fi
cp "$OUT/trace.log" "$HERE/trace.log" 2>/dev/null
[ -f "$OUT/trace.key" ] || : > "$OUT/trace.key"

# ------------------------------------------------- 4. bilan
TESTER_CHECK_KO="$CHECK_KO" TESTER_VERBOSE=$VERBOSE TESTER_STATE="$BUILD/state.txt" \
	bash "$HERE/scripts/summary.sh" "$LIBFT" "$OUT" "${FN[@]}"
SUM_RC=$?

# ------------------------------------------------- 5. verdict
echo
if [ $SUM_RC = 0 ] && [ $TEST_RC = 0 ] && { [ "$CHECK_KO" = 0 ] || [ "$CHECK_KO" = "-" ]; } && [ $GIT_KO = 0 ]; then
	if [ $FINAL = 1 ]; then
		echo "   ${GRN}${BOLD}✅  $(msg 'PRÊT POUR LA MOULINETTE.')${RST}"
		echo "   ${DIM}   $(msg "Dernier réflexe : vérifie sur l'intra que le dépôt de rendu contient bien ton dernier push.")${RST}"
	elif [ ${#FN[@]} -gt 0 ]; then
		echo "   ${GRN}${BOLD}✅  $(msg '%s : tout est vert.' "${FN[*]}")${RST}"
	else
		echo "   ${GRN}${BOLD}✅  $(msg 'Tout est vert.')${RST} ${DIM}$(msg 'Avant de rendre : libftest --final')${RST}"
	fi
	exit 0
fi
[ "$CHECK_KO" != "-" ] && [ "$CHECK_KO" != 0 ] && echo "   ${RED}✘ $(msg 'vérifications du rendu : %s erreur(s), voir en haut' "$CHECK_KO")${RST}"
[ $GIT_KO = 1 ] && echo "   ${RED}✘ $(msg 'dépôt git : commit / push à faire, voir en haut')${RST}"
echo "   ${RED}${BOLD}❌  $(msg "Pas encore prêt : corrige dans l'ordre de la liste « À corriger ».")${RST}"
echo "   ${DIM}   $(msg 'détail complet : %s' "$(show_path "$HERE/trace.log")")${RST}"
exit 1
