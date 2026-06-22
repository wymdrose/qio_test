import re
import zipfile
from pathlib import Path

path = Path(__file__).with_name("test_gai.xlsx")
with zipfile.ZipFile(path) as z:
    for name in sorted(z.namelist()):
        if not name.endswith(".xml"):
            continue
        data = z.read(name).decode("utf-8", errors="replace")
        for match in re.finditer(r'ref="([^"]+)"', data):
            value = match.group(1)
            if re.fullmatch(r"\$?[A-Z]{1,3}:\$?[A-Z]{1,3}", value, re.I):
                print(f"{name}: {value}")
