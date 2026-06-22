import re
import sys
import zipfile
from pathlib import Path


def repair_xlsx(src: Path, dest: Path) -> None:
    with zipfile.ZipFile(src) as zin, zipfile.ZipFile(dest, "w") as zout:
        for item in zin.infolist():
            data = zin.read(item.filename)
            if item.filename == "xl/workbook.xml":
                xml = data.decode("utf-8")
                xml = re.sub(
                    r"<definedNames>.*?</definedNames>",
                    "<definedNames/>",
                    xml,
                    flags=re.S | re.I,
                )
                data = xml.encode("utf-8")
            elif item.filename == "xl/worksheets/sheet1.xml":
                xml = data.decode("utf-8")
                xml = xml.replace('ref="A:J"', 'ref="A1:J552"')
                data = xml.encode("utf-8")
            zout.writestr(item, data)


if __name__ == "__main__":
    src = Path(sys.argv[1])
    dest = Path(sys.argv[2])
    dest.parent.mkdir(parents=True, exist_ok=True)
    repair_xlsx(src, dest)
    print(dest)
