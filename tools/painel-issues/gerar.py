#!/usr/bin/env python3
"""Gera o site estatico do painel de issues a partir de docs/issues/mapa.json."""
import argparse, datetime, json, os, shutil, subprocess, sys

AQUI = os.path.dirname(os.path.abspath(__file__))


def git(repo, *args):
    try:
        r = subprocess.run(["git", "-C", repo, *args], capture_output=True, text=True, check=True)
        return r.stdout.strip()
    except Exception:
        return ""


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo", default=os.path.abspath(os.path.join(AQUI, "..", "..")))
    ap.add_argument("--out", default=os.path.join(AQUI, "site"))
    a = ap.parse_args()

    mapa = os.path.join(a.repo, "docs", "issues", "mapa.json")
    try:
        with open(mapa, encoding="utf-8") as f:
            dados = json.load(f)
    except FileNotFoundError:
        sys.exit("ERRO: %s nao existe" % mapa)
    except (OSError, ValueError) as e:
        sys.exit("ERRO: nao consegui ler %s: %s" % (mapa, e))
    if not isinstance(dados, dict) or not isinstance(dados.get("itens"), list):
        sys.exit("ERRO: %s sem a lista 'itens'" % mapa)

    fmt = git(a.repo, "log", "-1", "--format=%h%x09%s")
    h, _, assunto = fmt.partition("\t")
    dados["meta"] = {
        "commit": h,
        "assunto": assunto,
        "branch": git(a.repo, "rev-parse", "--abbrev-ref", "HEAD"),
        "gerado_em": datetime.datetime.now().astimezone().isoformat(timespec="seconds"),
        "mapa_atualizado_em": dados.get("atualizado_em", ""),
    }

    os.makedirs(a.out, exist_ok=True)
    with open(os.path.join(a.out, "data.json"), "w", encoding="utf-8") as f:
        json.dump(dados, f, ensure_ascii=False, indent=1)
    for nome in sorted(os.listdir(AQUI)):
        if nome.endswith((".html", ".css", ".js")):
            shutil.copy2(os.path.join(AQUI, nome), os.path.join(a.out, nome))
    print("ok: %d issues -> %s (commit %s)" % (len(dados["itens"]), a.out, h or "?"))


if __name__ == "__main__":
    main()
