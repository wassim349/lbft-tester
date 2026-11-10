#!/usr/bin/env python3
"""
Verifie les traductions (make i18n) :
  - chaque texte affiche par le testeur a sa traduction dans lang/en.txt
  - la traduction a les memes %s/%d... dans le meme ordre (sinon printf plante)
  - chaque conseil de scripts/hints.txt a sa colonne anglaise
  - les cles bash n'utilisent ni $variable ni \\" (la recherche ne les trouverait pas)

Textes reperes automatiquement :
  C    : TEST, TEST_SOFT, t_fail, t_warn, t_note, FAIL_SEED, TR, N_ (1er texte),
         CHECK et child_say (texte apres le 1er argument)
  bash : msg '...' ou msg "..."

Usage : i18n_check.py [--skeleton]   (--skeleton : affiche les entrees manquantes,
                                      pretes a coller dans lang/en.txt)
"""
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CATALOG = os.path.join(ROOT, "lang", "en.txt")
HINTS = os.path.join(ROOT, "scripts", "hints.txt")
FMT = re.compile(r"%[-+ #0]*(?:\*|\d+)?(?:\.(?:\*|\d+))?(?:hh|h|ll|l|z|t|j|L)?[diouxXeEfgGcsp]")


def specs(s):
    return FMT.findall(s.replace("%%", ""))


# ------------------------------------------------------------------ C
C_CALLS = ("TEST_SOFT", "TEST", "t_fail", "t_warn", "t_note", "FAIL_SEED", "TR", "N_")


def strip_c_comments(src):
    out, i, n = [], 0, len(src)
    while i < n:
        c = src[i]
        if src.startswith("/*", i):
            j = src.find("*/", i + 2)
            out.append(" " * (j + 2 - i if j >= 0 else n - i))
            i = j + 2 if j >= 0 else n
        elif src.startswith("//", i):
            j = src.find("\n", i)
            i = j if j >= 0 else n
        elif c in "\"'":
            j = i + 1
            while j < n and src[j] != c:
                j += 2 if src[j] == "\\" else 1
            out.append(src[i:j + 1])
            i = j + 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


def read_c_strings(src, i):
    """lit des litteraux "a" "b" consecutifs a partir de i ; renvoie (texte brut, fin)"""
    n, parts = len(src), []
    while True:
        while i < n and src[i] in " \t\n\r":
            i += 1
        if i >= n or src[i] != '"':
            break
        j = i + 1
        while j < n and src[j] != '"':
            j += 2 if src[j] == "\\" else 1
        parts.append(src[i + 1:j])
        i = j + 1
    return ("".join(parts) if parts else None), i


def skip_arg(src, i):
    """saute un argument (jusqu'a la virgule de meme niveau) ; renvoie l'indice apres la virgule"""
    depth, n = 0, len(src)
    while i < n:
        c = src[i]
        if c in "\"'":
            j = i + 1
            while j < n and src[j] != c:
                j += 2 if src[j] == "\\" else 1
            i = j + 1
            continue
        if c in "([{":
            depth += 1
        elif c in ")]}":
            if depth == 0:
                return None
            depth -= 1
        elif c == "," and depth == 0:
            return i + 1
        i += 1
    return None


def c_keys():
    keys = {}
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "*.[ch]"))):
        src = strip_c_comments(open(path, encoding="utf-8").read()).replace("\\\n", " ")
        macros = {m.group(1): read_c_strings(src, m.end())[0]
                  for m in re.finditer(r"#\s*define\s+(\w+)\s+(?=\")", src)}
        rel = os.path.relpath(path, ROOT)
        for m in re.finditer(r"\b(TEST_SOFT|TEST|t_fail|t_warn|t_note|FAIL_SEED|TR|N_|CHECK|child_say)\s*\(", src):
            name, i = m.group(1), m.end()
            if name in ("CHECK", "child_say"):
                i = skip_arg(src, i)
                if i is None:
                    continue
            text, _ = read_c_strings(src, i)
            if text is None:
                w = re.match(r"\s*(\w+)", src[i:])
                text = macros.get(w.group(1)) if w else None
            if text is None or not re.search(r"[A-Za-zÀ-ÿ]", text):
                continue
            line = src.count("\n", 0, m.start()) + 1
            keys.setdefault(text, "%s:%d" % (rel, line))
    return keys


# ------------------------------------------------------------------ bash
def bash_keys(errors):
    keys = {}
    files = [os.path.join(ROOT, f) for f in ("run.sh", "install.sh")]
    files += sorted(glob.glob(os.path.join(ROOT, "scripts", "*.sh")))
    for path in files:
        src = open(path, encoding="utf-8").read()
        rel = os.path.relpath(path, ROOT)
        for m in re.finditer(r"(?<![\w-])msg[ \t]+(['\"])", src):
            q, i = m.group(1), m.end()
            j, raw = i, []
            while j < len(src) and src[j] != q:
                if q == '"' and src[j] == "\\":
                    raw.append(src[j:j + 2])
                    j += 2
                    continue
                raw.append(src[j])
                j += 1
            text = "".join(raw)
            line = src.count("\n", 0, m.start()) + 1
            where = "%s:%d" % (rel, line)
            if src[src.rfind("\n", 0, m.start()) + 1:m.start()].lstrip().startswith("#"):
                continue
            if q == '"':
                if re.search(r"\\[\"\\$`]", text) or "$" in text:
                    errors.append("%s : cle bash entre guillemets avec $ ou \\ : %s" % (where, text))
                    continue
            keys.setdefault(text, where)
    return keys


# ------------------------------------------------------------------ catalogue
def read_catalog(errors):
    cat, order, key = {}, [], None
    for n, line in enumerate(open(CATALOG, encoding="utf-8").read().split("\n"), 1):
        if line.startswith("fr: "):
            if key is not None:
                errors.append("lang/en.txt:%d : « fr: » sans « en: » avant" % n)
            key = line[4:]
        elif line.startswith("en: "):
            if key is None:
                errors.append("lang/en.txt:%d : « en: » sans « fr: »" % n)
                continue
            if key in cat:
                errors.append("lang/en.txt:%d : cle en double : %s" % (n, key))
            cat[key] = line[4:]
            order.append(key)
            key = None
        elif line.strip() and not line.startswith("#"):
            errors.append("lang/en.txt:%d : ligne inattendue : %s" % (n, line))
    return cat, order


def main():
    skeleton = "--skeleton" in sys.argv
    errors, warnings = [], []
    used = dict(c_keys())
    for k, v in bash_keys(errors).items():
        used.setdefault(k, v)
    cat, order = read_catalog(errors)

    missing = [k for k in used if not cat.get(k)]
    for k in used:
        if cat.get(k) and specs(k) != specs(cat[k]):
            errors.append("%s : %% différents : %s  ->  %s" % (used[k], k, cat[k]))
    for k in order:
        if k not in used:
            warnings.append("lang/en.txt : entrée inutilisée : %s" % k)

    for n, line in enumerate(open(HINTS, encoding="utf-8").read().split("\n"), 1):
        if not line.strip() or line.startswith("#"):
            continue
        cols = [c for c in re.split(r"\t+", line)]
        if len(cols) < 4 or not cols[3].strip():
            errors.append("scripts/hints.txt:%d : conseil sans traduction anglaise (4e colonne)" % n)
        else:
            try:
                re.compile(cols[1])
            except re.error as e:
                errors.append("scripts/hints.txt:%d : regex invalide (%s)" % (n, e))

    if skeleton:
        for k in missing:
            print("# %s\nfr: %s\nen: \n" % (used[k], k))
        return 0
    for k in missing:
        errors.append("%s : pas de traduction : %s" % (used[k], k))
    for w in warnings:
        print("   ! " + w)
    for e in errors:
        print("   ✘ " + e)
    if errors:
        print("\n   i18n KO : %d problème(s) (textes repérés : %d)" % (len(errors), len(used)))
        return 1
    print("   i18n OK : %d textes traduits, %d conseils" % (len(used), sum(
        1 for l in open(HINTS, encoding="utf-8") if l.strip() and not l.startswith("#"))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
