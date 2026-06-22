import re
import zipfile
from pathlib import Path

path = Path(__file__).with_name("test_gai.xlsx")
with zipfile.ZipFile(path) as z:
    wb = z.read("xl/workbook.xml").decode("utf-8")
    print("=== workbook.xml definedNames ===")
    for match in re.finditer(
        r'<definedName[^>]*name="([^"]+)"[^>]*>([^<]+)</definedName>', wb
    ):
        print(f"  {match.group(1)} = {match.group(2)}")

    s1 = z.read("xl/worksheets/sheet1.xml").decode("utf-8")
    print("=== sheet1 autoFilter ===")
    for match in re.finditer(r"<autoFilter[^>]*/>", s1):
        print(f"  {match.group(0)}")
    for match in re.finditer(r'<autoFilter[^>]*ref="([^"]+)"', s1):
        print(f"  ref={match.group(1)}")
