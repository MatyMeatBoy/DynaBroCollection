"""Read-only DB1 translation coverage audit; emits identities and offsets, not ROM bytes."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.translate_db1 import translate
from tools.generate_language import ranges
from tools.rompatch import identity

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path, help='Qualified supplied English v0.1 image')
    parser.add_argument('current', type=Path)
    parser.add_argument('--catalog', type=Path, default=Path(__file__).resolve().parents[2] / 'translations/db1-en.json')
    args = parser.parse_args()
    source, current = args.source.read_bytes(), args.current.read_bytes()
    catalog = json.loads(args.catalog.read_text(encoding='utf-8'))
    expected = translate(source, catalog)
    if expected != current:
        raise ValueError('Current image does not equal guarded catalog result')
    edits = [dict(start='0x1B464', length=108, codec='big-endian word tile indices'),
             dict(start='0x1B510', length=40, codec='big-endian font byte offsets')]
    for table in catalog.get('credits_tables', []):
        edits.append(dict(start=hex(table['address']), length=table['width']*table['count'], codec='credits tile bytes', rows=table['count'], width=table['width']))
    for table in catalog.get('bonus_messages', []):
        edits.append(dict(start=hex(table['address']), length=table['capacity'], codec='body tiles A=0xA9; NUL row/end'))
        edits.append(dict(start=hex(table['background_address']), length=16, codec='backdrop tiles A=0x14'))
    print(json.dumps(dict(source=identity(source), current=identity(current), catalog_matches=True,
                         catalog_edits=edits, changed_spans=[dict(start=hex(a), length=b-a) for a,b in ranges([source,current])],
                         limitation='Catalog coverage only; neither text discovery nor exhaustive scene coverage'), indent=2))

if __name__ == '__main__':
    main()
