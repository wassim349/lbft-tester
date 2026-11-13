#!/bin/bash
# Genere available.h : HAS_ft_xxx pour chaque fonction presente dans libft.a
# ET declaree dans libft.h (sinon le testeur ne compilerait pas).
# Le fichier n'est reecrit que s'il change (evite de tout recompiler).
# Usage : gen_available.sh <dossier_libft> <libft.a> <sortie.h>

LIBFT="$1"; LIB="$2"; OUT="$3"
HERE="$(cd "$(dirname "$0")" && pwd)"

defined="$(nm -g "$LIB" 2>/dev/null | awk '$2=="T"{print $3}' | sed 's/^_//' | sort -u)"
{
	echo "/* genere automatiquement par gen_available.sh */"
	while read -r fn; do
		echo "$defined" | grep -qx "$fn" || continue
		grep -Eq "[^a-z_]${fn}[[:space:]]*\(" "$LIBFT/libft.h" 2>/dev/null || continue
		echo "#define HAS_$fn 1"
	done < "$HERE/functions.txt"
} > "$OUT.tmp"
if cmp -s "$OUT.tmp" "$OUT"; then rm -f "$OUT.tmp"; else mv "$OUT.tmp" "$OUT"; fi
