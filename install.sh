#!/bin/bash
# Installation du testeur : ./install.sh
# - verifie les outils (cc, make, ar, nm, perl, git)
# - installe la norminette si elle manque (pipx ou pip --user, sans sudo)
# - cree la commande "libftest" dans ~/.local/bin
# Desinstaller : ./install.sh --uninstall

HERE="$(cd "$(dirname "$0")" && pwd)"
. "$HERE/scripts/common.sh"
BIN="$HOME/.local/bin"
MARK="# libft-tester"

if [ "$1" = "--uninstall" ]; then
	rm -f "$BIN/libftest"
	echo "   ${GRN}✔${RST} commande libftest supprimée (le dossier du testeur n'est pas touché)"
	exit 0
fi

echo
echo "   ${CYN}${BOLD}Installation de libft-tester${RST}"
echo

# --- 1. outils
missing=0
for t in cc make ar nm perl; do
	if command -v "$t" >/dev/null 2>&1; then echo "   ${GRN}✔${RST} $t"
	else echo "   ${RED}✘ $t introuvable${RST}"; missing=1; fi
done
command -v git >/dev/null 2>&1 && echo "   ${GRN}✔${RST} git" || echo "   ${YEL}! git introuvable : le mode --git ne marchera pas${RST}"
if [ $missing = 1 ]; then
	echo
	echo "   ${RED}Il manque des outils.${RST} Sur Ubuntu/Debian : sudo apt install build-essential perl"
	echo "   (sur les postes de 42 ils sont déjà installés : demande au staff si ce n'est pas le cas)"
	exit 1
fi

# --- 2. norminette
NORM="$(find_norminette)"
if [ -n "$NORM" ]; then
	echo "   ${GRN}✔${RST} norminette ($NORM)"
else
	echo "   ${YEL}…${RST} norminette absente : installation (sans sudo)"
	if command -v pipx >/dev/null 2>&1 && pipx install norminette >/dev/null 2>&1; then :
	elif python3 -m pip install --user --quiet norminette >/dev/null 2>&1; then :
	else
		# Ubuntu recent : pip --user est bloque (PEP 668) -> environnement isole dans ~/.local
		VENV="$HOME/.local/share/libft-tester/venv"
		if python3 -m venv "$VENV" >/dev/null 2>&1 \
			&& "$VENV/bin/pip" install --quiet norminette >/dev/null 2>&1; then
			mkdir -p "$BIN"
			ln -sf "$VENV/bin/norminette" "$BIN/norminette"
		fi
	fi
	NORM="$(find_norminette)"
	if [ -n "$NORM" ]; then
		echo "   ${GRN}✔${RST} norminette installée ($NORM)"
	else
		echo "   ${YEL}! je n'ai pas pu installer la norminette. Essaie à la main :${RST}"
		echo "       pipx install norminette"
		echo "       ou : python3 -m pip install --user norminette"
		echo "       (Ubuntu perso sans pip : sudo apt install python3-venv, puis relance ./install.sh)"
		echo "   ${DIM}(sur les postes de 42, la norminette est normalement déjà installée)${RST}"
		echo "   ${DIM}(le testeur marche sans, mais ne vérifiera pas la norme)${RST}"
	fi
fi

# --- 3. droits + commande libftest
chmod +x "$HERE/run.sh" "$HERE/install.sh" "$HERE"/scripts/*.sh
mkdir -p "$BIN"
ln -sf "$HERE/run.sh" "$BIN/libftest"
echo "   ${GRN}✔${RST} commande libftest → $HERE/run.sh"

added=""
case ":$PATH:" in
	*":$BIN:"*) ;;
	*)
		# les fichiers qui existent + celui du shell utilise (zsh sur les postes de 42)
		myrc="$HOME/.$(basename "${SHELL:-bash}")rc"
		for rc in "$HOME/.zshrc" "$HOME/.bashrc"; do
			[ -f "$rc" ] || [ "$rc" = "$myrc" ] || continue
			if ! grep -q "$MARK" "$rc" 2>/dev/null; then
				printf '\n%s\nexport PATH="$HOME/.local/bin:$PATH"\n' "$MARK" >> "$rc"
				added="$added $rc"
			fi
		done
		;;
esac

# --- 4. auto-test rapide du testeur (compile le moteur une fois)
if cc -Wall -Wextra -c "$HERE/src/framework.c" -I"$HERE/src" -o /dev/null 2>/dev/null; then
	echo "   ${GRN}✔${RST} le moteur du testeur compile"
else
	echo "   ${RED}✘ le moteur du testeur ne compile pas avec ce cc${RST}"
fi

echo
echo "   ${GRN}${BOLD}C'est installé.${RST}"
if [ -n "$added" ]; then
	rcfile="$HOME/.$(basename "${SHELL:-bash}")rc"
	echo "   ${YEL}Ouvre un nouveau terminal${RST} (ou tape : source ${rcfile/#$HOME/\~}) pour que la commande marche."
fi
echo
echo "   Ensuite, dans ton dossier libft :"
echo "     ${BOLD}libftest${RST}            tout tester"
echo "     ${BOLD}libftest atoi${RST}       une seule fonction"
echo "     ${BOLD}libftest --watch${RST}    relance à chaque sauvegarde"
echo "     ${BOLD}libftest --final${RST}    avant de rendre"
echo "     ${BOLD}libftest -h${RST}         toutes les options"
echo
