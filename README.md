# libft-tester

Un testeur pour la libft de 42. Pour chaque fonction il te dit si elle est bonne et, sinon, pourquoi.

Fait pour les postes Linux de 42, marche aussi sur Mac.

## Installer

```sh
git clone <url-du-depot> ~/libft-tester
~/libft-tester/install.sh
```

Ouvre un nouveau terminal et c'est bon.

## Utiliser

Dans ton dossier libft :

```sh
libftest              # tout
libftest atoi split   # juste quelques fonctions
libftest --watch      # relance à chaque sauvegarde
libftest --final      # avant de rendre
```

`libftest -h` pour le reste.

## Ce qu'il vérifie

- les 43 fonctions, comparées à la vraie libc, plus des milliers de tests aléatoires
- les fuites, les crashs, les boucles infinies, les malloc non protégés
- le Makefile (relink, flags, règles), le README, `libft.h`, les fonctions interdites
- la norminette
- avec `--final` : que tout est commit et push, et les erreurs mémoire avec ASan

Les problèmes sont triés par priorité : d'abord ce qui empêche de compiler, ensuite les crashs, puis les résultats faux, et la norme à la fin.

## À savoir

Ça ne remplace pas tes propres tests, la moulinette peut toujours trouver autre chose.

Pour mettre à jour : `libftest --update`. Si tu modifies le testeur, lance `make selftest`
pour vérifier qu'il détecte toujours les bugs de `tests/bugs/`.
