# Writes exports.txt (address name) from DungeonSiege2.exe in the current directory; dz.py and xr.py read it.
import pefile
pe = pefile.PE('DungeonSiege2.exe')
base = pe.OPTIONAL_HEADER.ImageBase
with open('exports.txt', 'w') as f:
    for s in pe.DIRECTORY_ENTRY_EXPORT.symbols:
        if s.name:
            f.write('%08x %s\n' % (base + s.address, s.name.decode('latin-1')))
