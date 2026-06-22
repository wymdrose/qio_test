import re
import zipfile
from pathlib import Path

path = Path(__file__).with_name("test_gai.xlsx")
with zipfile.ZipFile(path) as z:
    s = z.read("xl/worksheets/sheet1.xml").decode("utf-8")
    refs = set(re.findall(r'ref="([^"]+)"', s))
    print("ref count", len(refs))
    for r in sorted(refs):
        if re.fullmatch(r"\$?[A-Z]{1,3}:\$?[A-Z]{1,3}", r, re.I):
            print("col-only:", r)
