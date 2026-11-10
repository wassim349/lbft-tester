#!/bin/bash
# Teste le testeur : "make selftest"
#  - tests/ref  : une libft correcte, tous les tests doivent passer
#  - tests/bugs : une libft buguee par fichier, nommee ATTENDU__ft_xxx__description.c
#      KO   -> la fonction doit etre detectee KO
#      WARN -> la fonction doit passer, avec un avertissement
#      ASAN -> doit etre detectee KO avec --asan (saute si ASan ne marche pas ici)

HERE="$(cd "$(dirname "$0")/.." && pwd)"
. "$HERE/scripts/common.sh"
TMP="$HERE/build/selftest"
rm -rf "${HERE:?}/build/selftest"
mkdir -p "$TMP"
pass=0; fail=0; skip=0

run() { NO_COLOR=1 bash "$HERE/run.sh" -q --seed 42 "$@" 2>&1; }

# --- la libft correcte
cp -r "$HERE/tests/ref" "$TMP/ref"
out="$(run "$TMP/ref")"
line="$(echo "$out" | grep -E 'tests réussis' | head -n 1)"
if echo "$line" | grep -qE ': ([0-9]+) / \1 ' && ! echo "$out" | grep -qE '^  ft_.* KO$'; then
	echo "   ${GRN}✔${RST} libft correcte : $(echo "$line" | grep -oE '[0-9]+ / [0-9]+')"
	pass=$((pass + 1))
else
	echo "   ${RED}✘ libft correcte : des tests échouent alors qu'ils ne devraient pas${RST}"
	echo "$out" | grep -E '^  ft_.* KO$' | head -n 5 | sed 's/^/       /'
	fail=$((fail + 1))
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
	esac
	if [ "$expect" = ASAN ] && echo "$out" | grep -q "ASan ne fonctionne pas"; then
		echo "   ${DIM}- $fn ($what) : sauté, ASan ne marche pas sur cette machine${RST}"
		skip=$((skip + 1))
	elif [ $ok = 1 ]; then
		echo "   ${GRN}✔${RST} $fn ${DIM}($what) → $expect${RST}"
		pass=$((pass + 1))
	else
		echo "   ${RED}✘ $fn ($what) : attendu $expect, obtenu :${RST} ${res:-rien}"
		fail=$((fail + 1))
	fi
done

rm -rf "${HERE:?}/build/selftest"
echo
if [ $fail = 0 ]; then
	msg=""; [ $skip -gt 0 ] && msg=", $skip sautée(s)"
	echo "   ${GRN}${BOLD}selftest OK${RST} : $pass vérifications$msg"
else
	echo "   ${RED}${BOLD}selftest KO${RST} : $fail problème(s), $pass OK"
fi
[ $fail = 0 ]
