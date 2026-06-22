import re
import sys
import zipfile


def inspect(path: str) -> None:
    print(f"=== {path} ===")
    with zipfile.ZipFile(path) as z:
        for name in sorted(z.namelist()):
            if not name.endswith(".xml"):
                continue
            data = z.read(name).decode("utf-8", errors="replace")
            patterns = [
                (r'ref="\$A"', "ref=$A"),
                (r'sqref="[^"]*\$A[^"]*"', "sqref with $A"),
                (r'<definedName[^>]*>[^<]*\$A[^<]*</definedName>', "definedName $A"),
                (r'#REF!', "#REF!"),
                (r'bad cell coordinates', "bad cell coordinates literal"),
            ]
            hits = []
            for pattern, label in patterns:
                for match in re.finditer(pattern, data, re.IGNORECASE):
                    start = max(0, match.start() - 60)
                    end = min(len(data), match.end() + 60)
                    hits.append((label, data[start:end].replace("\n", " ")))
            if hits:
                print(f"--- {name} ({len(hits)} hits) ---")
                for label, ctx in hits[:20]:
                    print(f"  [{label}] {ctx}")


if __name__ == "__main__":
    for arg in sys.argv[1:]:
        inspect(arg)
