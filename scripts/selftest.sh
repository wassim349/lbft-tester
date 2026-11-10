#!/bin/bash
# Teste le testeur : "make selftest"
#  - traductions : make i18n (chaque texte a sa traduction anglaise)
#  - tests/ref  : une libft correcte, tous les tests doivent passer
#  - tests/bugs : une libft buguee par fichier, nommee ATTENDU__ft_xxx__description.c
#      KO   -> la fonction doit etre detectee KO
#      WARN -> la fonction doit passer, avec un avertissement
#      ASAN -> doit etre detectee KO avec --asan (saute si ASan ne marche pas ici)
#      HINT -> seul le conseil compte (fichier qui ne compile pas, prototype faux mais
#              tests qui passent...) : le moteur ne la voit pas forcement KO
#  Un fichier de bug peut commencer par  /* conseil attendu : <debut du conseil> */
#  : ce texte doit alors apparaitre dans un conseil du bilan (scripts/hints.txt).
#  - en anglais : la libft correcte et les bugs qui ont un conseil attendu sont relances
#    avec LIBFTEST_LANG=en : meme verdict, conseil en anglais, pas de francais oublie.

HERE="$(cd "$(dirname "$0")/.." && pwd)"
. "$HERE/scripts/common.sh"
TMP="$HERE/build/$(uname -s)-$(uname -m)/selftest"
rm -rf "${TMP:?}"
mkdir -p "$TMP"
pass=0; fail=0; skip=0

run()    { LIBFTEST_LANG=fr LIBFTEST_LANG_SET=1 NO_COLOR=1 bash "$HERE/run.sh" -q --seed 42 "$@" 2>&1; }
run_en() { LIBFTEST_LANG=en LIBFTEST_LANG_SET=1 NO_COLOR=1 bash "$HERE/run.sh" -q --seed 42 "$@" 2>&1; }
good()   { echo "   ${GRN}✔${RST} $*"; pass=$((pass + 1)); }
bad()    { echo "   ${RED}✘ $*${RST}"; fail=$((fail + 1)); }
flat()   { tr -s ' \n' ' '; }	# les conseils sont coupes sur plusieurs lignes
# mots francais qui ne doivent pas apparaitre dans la sortie anglaise
FRENCH='attendu|obtenu|corriger|réussi|erreur\(s\)|fonction\(s\)|Bilan|renvoie|chaine|chaîne|maillon|ne compile pas|fichier'
# conseil anglais de la regle (pour la fonction $2) dont le conseil francais contient $1
# (texte passe par l'environnement : awk -v transformerait \0)
hint_en() { H="$1" F="$2" awk -F'\t+' '!/^#/ && NF >= 4 && index($3, ENVIRON["H"]) &&
	($1 == "*" || ENVIRON["F"] ~ ("^(" $1 ")$")) { print $4; exit }' "$HERE/scripts/hints.txt"; }
french_left() { echo "$1" | sed -E 's#[^ ]*/[^ ]*##g' | grep -E "$FRENCH" | head -n 3; }

# --- traductions
if out="$(python3 "$HERE/scripts/i18n_check.py" 2>&1)"; then
	good "$(echo "$out" | tail -n 1 | sed 's/^ *//')"
else
	bad "traductions :"; echo "$out" | grep '✘' | head -n 10 | sed 's/^/     /'
fi

# --- la libft correcte
cp -r "$HERE/tests/ref" "$TMP/ref"
out="$(run "$TMP/ref")"
line="$(echo "$out" | grep -E 'tests réussis' | head -n 1)"
if echo "$line" | grep -qE ': ([0-9]+) / \1 ' && ! echo "$out" | grep -qE '^  ft_.* KO$'; then
	good "libft correcte : $(echo "$line" | grep -oE '[0-9]+ / [0-9]+')"
else
	bad "libft correcte : des tests échouent alors qu'ils ne devraient pas"
	echo "$out" | grep -E '^  ft_.* KO$' | head -n 5 | sed 's/^/       /'
fi
out="$(run_en "$TMP/ref")"
line="$(echo "$out" | grep -E 'tests passed' | head -n 1)"
left="$(french_left "$out")"
if echo "$line" | grep -qE ': ([0-9]+) / \1 ' && [ -z "$left" ]; then
	good "libft correcte, en anglais : $(echo "$line" | grep -oE '[0-9]+ / [0-9]+')"
else
	bad "libft correcte, en anglais : ${line:-pas de ligne « tests passed »}"
	[ -n "$left" ] && echo "$left" | sed 's/^/       français oublié : /'
fi

# --- les libft buguees
for bug in "$HERE"/tests/bugs/*.c; do
	name="$(basename "$bug" .c)"
	expect="${name%%__*}"
	rest="${name#*__}"
	fn="${rest%%__*}"
	what="${rest#*__}"
	dir="$TMP/$name"
	cp -r "$HERE/tests/ref" "$dir"
	cp "$bug" "$dir/$fn.c"
	opt=""; [ "$expect" = ASAN ] && opt="--asan"
	out="$(run $opt "$dir" "$fn")"
	res="$(echo "$out" | grep -E "^  $fn " | head -n 1)"
	ok=0
	case "$expect" in
		KO|ASAN) echo "$res" | grep -qE ' KO$' && ok=1 ;;
		WARN) echo "$res" | grep -q 'OK (avertissements)' && ok=1 ;;
		HINT) ok=1 ;;
	esac
	hint="$(sed -n '1s|^/\* conseil attendu : \(.*\) \*/$|\1|p' "$bug")"
	if [ "$expect" = ASAN ] && echo "$out" | grep -q "ASan ne fonctionne pas"; then
		echo "   ${DIM}- $fn ($what) : sauté, ASan ne marche pas sur cette machine${RST}"
		skip=$((skip + 1))
		continue
	elif [ "$expect" = HINT ] && [ -z "$hint" ]; then
		bad "$fn ($what) : un fichier HINT doit commencer par /* conseil attendu : ... */"
		continue
	elif [ $ok = 0 ]; then
		bad "$fn ($what) : attendu $expect, obtenu : ${res:-rien}"
		continue
	elif [ -n "$hint" ] && ! echo "$out" | flat | grep -qF "$hint"; then
		bad "$fn ($what) : bien $expect, mais le conseil « $hint » n'est pas affiché"
		continue
	fi
	good "$fn ${DIM}($what) → $expect${hint:+, conseil ok}${RST}"
	# meme chose en anglais
	[ -z "$hint" ] && continue
	out="$(run_en $opt "$dir" "$fn")"
	res="$(echo "$out" | grep -E "^  $fn " | head -n 1)"
	hen="$(hint_en "$hint" "$fn")"
	left="$(french_left "$out")"
	if [ "$expect" != HINT ] && ! echo "$res" | grep -qE ' KO$'; then
		bad "$fn ($what), en anglais : attendu KO, obtenu : ${res:-rien}"
	elif [ -z "$hen" ]; then
		bad "$fn ($what), en anglais : « $hint » n'est dans aucun conseil de hints.txt"
	elif ! echo "$out" | flat | grep -qF "$hen"; then
		bad "$fn ($what), en anglais : conseil pas affiché : $hen"
	elif [ -n "$left" ]; then
		bad "$fn ($what), en anglais : du français reste :"; echo "$left" | sed 's/^/       /'
	else
		good "$fn ${DIM}($what) → en anglais aussi${RST}"
	fi
done

rm -rf "${TMP:?}"
echo
if [ $fail = 0 ]; then
	extra=""; [ $skip -gt 0 ] && extra=", $skip sautée(s)"
	echo "   ${GRN}${BOLD}selftest OK${RST} : $pass vérifications$extra"
else
	echo "   ${RED}${BOLD}selftest KO${RST} : $fail problème(s), $pass OK"
fi
[ $fail = 0 ]
