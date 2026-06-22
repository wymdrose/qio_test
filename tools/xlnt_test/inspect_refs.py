import re
import sys
import zipfile

PATTERNS = [
    (r' ref="([^"]+)"', "ref attr"),
    (r' sqref="([^"]+)"', "sqref attr"),
    (r'<autoFilter[^>]*ref="([^"]+)"', "autoFilter"),
    (r'<definedName[^>]*>([^<]+)</definedName>', "definedName body"),
]


def inspect(path: str) -> None:
    print(f"=== {path} ===")
    with zipfile.ZipFile(path) as z:
        for name in sorted(z.namelist()):
            if not name.endswith(".xml"):
                continue
            data = z.read(name).decode("utf-8", errors="replace")
            bad = []
            for pattern, label in PATTERNS:
                for match in re.finditer(pattern, data, re.IGNORECASE):
                    value = match.group(1) if match.lastindex else match.group(0)
                    if re.fullmatch(r"\$?[A-Z]+\:\$?[A-Z]+", value):
                        bad.append((label, value, name))
                    elif re.fullmatch(r"\$?[A-Z]+", value):
                        bad.append((label, value, name))
                    elif value == "#REF!":
                        bad.append((label, value, name))
            if bad:
                print(f"--- {name} ---")
                for label, value, _ in bad[:30]:
                    print(f"  [{label}] {value}")


if __name__ == "__main__":
    for arg in sys.argv[1:]:
        inspect(arg)
