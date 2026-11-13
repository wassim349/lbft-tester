#!/bin/bash
# Compile TOUJOURS une copie propre de la libft dans le dossier du testeur
# (jamais les .o / libft.a qui trainent dans ta libft : ils peuvent etre vieux,
# ou compiles sur une autre machine).
# Chaque ft_*.c est compile tout seul (en parallele) : un fichier casse
# n'empeche pas de tester les autres.
#
# Usage : build_lib.sh <dossier_libft> <dossier_out> [flags en plus, ex. -fsanitize=address]
# Produit :
#   <out>/libft.a
#   <out>/compile_errors.txt  un fichier ecarte par ligne : "ft_xxx CODE [detail]"
#       COMPILE <1re erreur du compilateur>   ne compile pas
#       MAIN                                  contient un main()
#       DUP <ft_yyy>                          definit aussi ft_yyy (deja dans ft_yyy.c)
#       DEP <ft_yyy>                          appelle ft_yyy, qui est ecartee ou absente
#   <out>/protos_c.txt  prototype du .c different du sujet : "ft_xxx<TAB>type|const<TAB>ta ligne"
# (les messages sont mis en forme, et traduits, par summary.sh)

LIBFT="$1"
OUT="$2"
shift 2
EXTRA="$*"
OBJS="$OUT/objs"
ERRS="$OUT/compile_errors.txt"
PERR="$OUT/protos_c.txt"
PROTOS="$(cd "$(dirname "$0")" && pwd)/protos.c"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
[ "$JOBS" -gt 8 ] && JOBS=8

mkdir -p "$OBJS" "$OUT/cerr"
rm -f "$OBJS"/*.o "$OUT"/cerr/*.txt "$OUT"/cerr/*.proto "$OUT/libft.a"
: > "$ERRS"
: > "$PERR"

# t_list definie dans libft.h ? (sinon les prototypes des fonctions de liste ne sont pas verifies)
TLIST=""
printf '#include "libft.h"\nint main(void){t_list l; l.content = 0; l.next = &l; return (l.next == 0);}\n' \
	| cc -I"$LIBFT" -x c - -o /dev/null 2>/dev/null && TLIST="-DHAS_TLIST"

# --- 1. compilation (meme flags que la moulinette : -Wall -Wextra -Werror)
# -fsigned-char : la moulinette tourne sur x86 ou char est signe ; sur ARM (Mac M1,
# certaines VM) char est non signe et cache les bugs de comparaison signee.
#
# --- puis prototype du .c compare au sujet : si le .c n'inclut pas libft.h, le
# compilateur ne voit pas que "int ft_isprint(char *s)" n'est pas le prototype du
# sujet (la moulinette l'appelle avec un int et ca plante). On recompile le fichier
# avec les prototypes du sujet inclus de force ; -Dconst= dit si seul const differe.
compile_one() {
	src="$1"; base="$(basename "$src" .c)"
	# LC_ALL=C : messages du compilateur en anglais avec des guillemets ASCII ('x' et pas ‘x’),
	# quelle que soit la langue du systeme (les conseils de hints.txt les reconnaissent)
	if ! out="$(LC_ALL=C cc -Wall -Wextra -Werror -fsigned-char $EXTRA -I"$LIBFT" -c "$src" -o "$OBJS/$base.o" 2>&1)"; then
		rm -f "$OBJS/$base.o"
		msg="$(echo "$out" | grep -m1 -E 'error' | sed -E 's/^[^ ]*\/([^/ ]+\.[ch]:)/\1/')"
		echo "$base COMPILE ${msg:-error}" > "$OUT/cerr/$base.txt"
		return
	fi
	conflict() { LC_ALL=C cc -fsyntax-only -w -I"$LIBFT" $TLIST "$@" -include "$PROTOS" "$src" 2>&1 \
		| grep -qE "^[^:]*$base\.c:[0-9]+:[0-9]+: error: conflicting types for .$base."; }
	conflict || return
	kind=type; conflict -Dconst= || kind=const
	def="$(grep -m1 -E "^[a-zA-Z_].*[^a-zA-Z0-9_]${base}[[:space:]]*\(" "$src" | tr -s ' \t' ' ')"
	printf '%s\t%s\t%s\n' "$base" "$kind" "$def" > "$OUT/cerr/$base.proto"
}
export -f compile_one
export OBJS OUT LIBFT EXTRA TLIST PROTOS
find "$LIBFT" -maxdepth 1 -name 'ft_*.c' -print0 2>/dev/null \
	| xargs -0 -n 1 -P "$JOBS" bash -c 'compile_one "$0"'
cat "$OUT"/cerr/*.txt >> "$ERRS" 2>/dev/null
cat "$OUT"/cerr/*.proto 2>/dev/null | sort >> "$PERR"

defined_in() { nm -g "$1" 2>/dev/null | awk '$2=="T"{print $3}' | sed 's/^_//'; }

# --- 2. main() oublie
for o in "$OBJS"/*.o; do
	[ -e "$o" ] || continue
	if defined_in "$o" | grep -qx main; then
		echo "$(basename "$o" .o) MAIN" >> "$ERRS"
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
			echo "$base DUP $sym" >> "$ERRS"
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
				echo "$(basename "$o" .o) DEP $sym" >> "$ERRS"
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
