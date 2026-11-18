"""Repair only the five hard-ending punctuation tile references after Speech38.

The credits loader at 0x283A0 builds its second Latin bank in VRAM0500..097F:
tile29 becomes a colored digit1, while tile27 is outside the primary upload
and retains unrelated graphics. These are not the live period/bang. The font
decoded from ROM6FC00 at 0x28400 is uploaded at 0x28410..0x2842A to
VRAM0E80..16BF (tiles74..B5); later actor uploads at 0x28D82 start16C0.
Tiles99/9B are the exact original period/bang glyphs with opaque ink1; their
patterns at VRAM1320/1360 survive intact until the hard-message draw28EB4/28ED2.
The corresponding safe question/apostrophe tiles are9A/A3, but neither occurs
in this message. Existing native crops prove the three old bangs are digit1
and both old periods are opaque garbage; see local/staff-credits-audit/.
Do not change the shared bonus-message encoding or the guarded950 catalog.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import zlib

SOURCE_SHA256='f680a297595dee8037cdb4240b5935de0b183509a0f0fde92f0b2794b3899f42'
TABLE_START=0x29ce4
TABLE_LENGTH=234
TABLE_SHA256='30f986daa41a323234ee5ee2bdee7febbead2122035abfe9075843a14f139915'
TARGET_SHA256='44fe845490eadcbabe54747aea3c26b7153bef48eb343904230ec873f942f026'
TARGET_CRC32='2740C64E'
TARGET_CHECKSUM=0x7e7e
EDITS=((0x29cf8,0x29,0x9b),(0x29d15,0x29,0x9b),(0x29d4a,0x27,0x99),(0x29d61,0x29,0x9b),(0x29dae,0x27,0x99))
def repair(source:bytes)->bytes:
    """Return the qualified post-Speech image with five data-only references fixed.

    Full image, table and per-byte guards are deliberate independent boundaries.
    No font, code, animation, password or staff-attribution bytes are modified.
    """
    if not isinstance(source,(bytes,bytearray)) or len(source)!=1048576:
        raise ValueError('Expected qualified post-Speech38 source size')
    if hashlib.sha256(source).hexdigest()!=SOURCE_SHA256:
        raise ValueError('Expected qualified post-Speech38 source SHA256')
    table=source[TABLE_START:TABLE_START+TABLE_LENGTH]
    if hashlib.sha256(table).hexdigest()!=TABLE_SHA256:
        raise ValueError('Hard-ending table SHA256 guard mismatch')
    for address,before,_ in EDITS:
        if source[address]!=before:
            raise ValueError(f'Hard-ending byte guard mismatch at {address:06X}')
    output=bytearray(source)
    for address,_,after in EDITS:output[address]=after
    checksum=sum(int.from_bytes(output[i:i+2],'big') for i in range(512,len(output),2))&65535
    output[0x18e:0x190]=checksum.to_bytes(2,'big')
    result=bytes(output)
    if (checksum!=TARGET_CHECKSUM or f'{zlib.crc32(result):08X}'!=TARGET_CRC32 or
            hashlib.sha256(result).hexdigest()!=TARGET_SHA256):
        raise ValueError('Corrected output identity mismatch')
    return result

def write_derived(source_path:Path,output:Path,result:bytes)->Path:
    """Create a new project-local file exclusively; never replace any input/alias."""
    local=(Path(__file__).resolve().parents[1]/'local').resolve()
    target=Path(output).resolve();source=Path(source_path).resolve()
    if target==local or not target.is_relative_to(local):
        raise ValueError('Derived output must be a new file beneath project local/')
    if target==source or (target.exists() and source.exists() and os.path.samefile(target,source)):
        raise ValueError('Derived output aliases the source')
    if target.exists():raise FileExistsError('Derived output already exists')
    target.parent.mkdir(parents=True,exist_ok=True)
    with target.open('xb') as stream:
        if stream.write(result)!=len(result):raise OSError('Incomplete derived ROM write')
    return target

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path)
    parser.add_argument('--output',type=Path,help='Optional exclusive derived file beneath project local/; otherwise verify only')
    args=parser.parse_args()
    source=args.source.read_bytes();result=repair(source)
    written=write_derived(args.source,args.output,result) if args.output else None
    print(json.dumps(dict(source_sha256=hashlib.sha256(source).hexdigest(),
        source_crc32=f'{zlib.crc32(source):08X}',output_sha256=TARGET_SHA256,
        output_crc32=TARGET_CRC32,size=len(result),checksum=f'{TARGET_CHECKSUM:04X}',
        table_sha256=TABLE_SHA256,written=str(written) if written else None,
        edits=[dict(address=f'{a:06X}',before=f'{b:02X}',after=f'{c:02X}')for a,b,c in EDITS],
        scope='Five hard-ending punctuation references and cartridge checksum only; shared950 catalog/Speech/font/staff/code unchanged'),indent=2))
if __name__=='__main__':main()
