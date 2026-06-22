import re
import sys
import zipfile


def scan(path: str) -> None:
    print(f"=== {path} ===")
    col_only = re.compile(r'^\$?[A-Z]+:\$?[A-Z]+$')
    with zipfile.ZipFile(path) as z:
        for name in sorted(z.namelist()):
            if not name.endswith(".xml"):
                continue
            data = z.read(name).decode("utf-8", errors="replace")
            for match in re.finditer(r'\bref="([^"]+)"', data):
                value = match.group(1)
                if col_only.fullmatch(value):
                    print(f"  column-only ref in {name}: {value}")
            for match in re.finditer(r'#REF!', data):
                start = max(0, match.start() - 50)
                end = min(len(data), match.end() + 50)
                print(f"  #REF! in {name}: {data[start:end].replace(chr(10),' ')}")


if __name__ == "__main__":
    for arg in sys.argv[1:]:
        scan(arg)
