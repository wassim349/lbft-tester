#!/bin/bash
# Compile TOUJOURS une copie propre de la libft dans le dossier du testeur
# (jamais les .o / libft.a qui trainent dans ta libft : ils peuvent etre vieux,
# ou compiles sur une autre machine).
# Chaque ft_*.c est compile tout seul (en parallele) : un fichier casse
# n'empeche pas de tester les autres.
#
# Usage : build_lib.sh <dossier_libft> <dossier_out> [flags en plus, ex. -fsanitize=address]
# Produit : <out>/libft.a et <out>/compile_errors.txt ("ft_xxx <raison>" par fichier ecarte)

LIBFT="$1"
OUT="$2"
shift 2
EXTRA="$*"
OBJS="$OUT/objs"
ERRS="$OUT/compile_errors.txt"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
[ "$JOBS" -gt 8 ] && JOBS=8

mkdir -p "$OBJS" "$OUT/cerr"
rm -f "$OBJS"/*.o "$OUT"/cerr/*.txt "$OUT/libft.a"
: > "$ERRS"

# --- 1. compilation (meme flags que la moulinette : -Wall -Wextra -Werror)
# -fsigned-char : la moulinette tourne sur x86 ou char est signe ; sur ARM (Mac M1,
# certaines VM) char est non signe et cache les bugs de comparaison signee.
compile_one() {
	src="$1"; base="$(basename "$src" .c)"
	if ! out="$(cc -Wall -Wextra -Werror -fsigned-char $EXTRA -I"$LIBFT" -c "$src" -o "$OBJS/$base.o" 2>&1)"; then
		rm -f "$OBJS/$base.o"
		msg="$(echo "$out" | grep -m1 -E 'error' | sed -E 's/^[^ ]*\/([^/ ]+\.[ch]:)/\1/')"
		echo "$base ne compile pas : ${msg:-erreur de compilation}" > "$OUT/cerr/$base.txt"
	fi
}
export -f compile_one
export OBJS OUT LIBFT EXTRA
find "$LIBFT" -maxdepth 1 -name 'ft_*.c' -print0 2>/dev/null \
	| xargs -0 -n 1 -P "$JOBS" bash -c 'compile_one "$0"'
cat "$OUT"/cerr/*.txt >> "$ERRS" 2>/dev/null

defined_in() { nm -g "$1" 2>/dev/null | awk '$2=="T"{print $3}' | sed 's/^_//'; }

# --- 2. main() oublie
for o in "$OBJS"/*.o; do
	[ -e "$o" ] || continue
	if defined_in "$o" | grep -qx main; then
		echo "$(basename "$o" .o) contient un main() : retire-le (la moulinette a son propre main, ca ne linke pas)" >> "$ERRS"
		rm -f "$o"
	fi
done

# --- 3. fonction ft_ recopiee dans un autre fichier ("duplicate symbol" au link)
for o in "$OBJS"/*.o; do
	[ -e "$o" ] || continue
	base="$(basename "$o" .o)"
	for sym in $(defined_in "$o" | grep '^ft_'); do
		[ "$sym" = "$base" ] && continue
		if [ -f "$LIBFT/$sym.c" ]; then
			echo "$base definit aussi $sym (deja dans $sym.c) : 'duplicate symbol' au link, mets-la en static ou supprime la copie" >> "$ERRS"
			rm -f "$o"
			break
		fi
	done
done

# --- 4. un fichier qui appelle une fonction ecartee ne peut pas etre linke
changed=1
while [ $changed = 1 ]; do
	changed=0
	defined="$(for o in "$OBJS"/*.o; do [ -e "$o" ] && defined_in "$o"; done | sort -u)"
	for o in "$OBJS"/*.o; do
		[ -e "$o" ] || continue
		for sym in $(nm -u "$o" 2>/dev/null | awk '{print $NF}' | sed 's/^_//' | grep '^ft_'); do
			if ! echo "$defined" | grep -qx "$sym"; then
				echo "$(basename "$o" .o) utilise $sym, qui ne compile pas ou n'existe pas" >> "$ERRS"
				rm -f "$o"; changed=1
				break
			fi
		done
	done
done

if ls "$OBJS"/*.o >/dev/null 2>&1; then
	ar rcs "$OUT/libft.a" "$OBJS"/*.o
fi
exit 0
