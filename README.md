# libft-tester

*[English below](#english)*

Un testeur pour la libft de 42. Pour chaque fonction il te dit si elle est bonne et, sinon, pourquoi.

Fait pour les postes Linux de 42, marche aussi sur Mac.

## Installer

```sh
git clone https://github.com/wassim349/lbft-tester.git ~/libft-tester
~/libft-tester/install.sh
```

L'installation demande la langue (français ou anglais). Ouvre un nouveau terminal et c'est bon.

## Utiliser

Dans ton dossier libft :

```sh
libftest              # tout
libftest atoi split   # juste quelques fonctions
libftest --watch      # relance à chaque sauvegarde
libftest --final      # avant de rendre
```

`libftest -h` pour le reste. Changer de langue : `libftest --lang` (menu), ou `libftest --lang en`.

## Ce qu'il vérifie

- les 43 fonctions, comparées à la vraie libc, plus des milliers de tests aléatoires
- les fuites, les crashs, les boucles infinies, les malloc non protégés
- le Makefile (relink, flags, règles), le README, `libft.h`, les fonctions interdites
- la norminette
- avec `--final` : que tout est commit et push, et les erreurs mémoire avec ASan

Les problèmes sont triés par priorité : d'abord ce qui empêche de compiler, ensuite les crashs, puis les résultats faux, et la norme à la fin.
Sous chaque erreur, un indice 💡 te dit où chercher (le cas qui coince, le `man` ou le sujet à relire), sans te donner la réponse : c'est à toi de trouver la correction.

## À savoir

Ça ne remplace pas tes propres tests, la moulinette peut toujours trouver autre chose.

Pour mettre à jour : `libftest --update`.

## Modifier le testeur

- `make selftest` vérifie qu'il détecte toujours les bugs de `tests/bugs/` (et que le bon conseil s'affiche), en français et en anglais.
  Il tourne aussi à chaque push (GitHub Actions : Linux avec gcc et clang, Mac). `SELFTEST_SEEDS=50 make selftest` essaie plus de graines aléatoires.
- Chaque fonction a au moins un fichier de bug dans `tests/bugs/` (`KO__ft_xxx__description.c`) : ajoutes-en un quand tu ajoutes un test.
- `make i18n` vérifie que chaque texte a sa traduction.
- Les indices sont dans `scripts/hints.txt` : une règle par ligne, indice en français puis en anglais (le format est expliqué en haut du fichier).
  Un indice dit où chercher, jamais la réponse : pas de code, pas de correction.
- Les traductions sont dans `lang/en.txt` : le texte français sert de clé. Dans le code, un texte affiché passe par `msg '...'` (bash) ou par `TEST`/`t_fail`/`TR` (C).

## English

A tester for the 42 libft. For each function it tells you whether it is right and, if not, why — with a 💡 hint under every error telling you where to look, without giving you the answer.

```sh
git clone https://github.com/wassim349/lbft-tester.git ~/libft-tester
~/libft-tester/install.sh      # asks for the language
libftest                       # in your libft folder
libftest --lang en             # switch to English at any time
```

`libftest --watch` reruns on every save, `libftest --final` checks everything (git, ASan) before you submit, `libftest -h` lists all options.
