"""Initial DB2 Special control-flow leads, including its original F-line calls.

Capstone provides an independent decoder. This is not proof of complete coverage.
The F-line table address and handler signature are qualified to the supplied dump.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import re
import struct
import capstone

SHA256 = '1d3f08e235fa8e51b9656820fabfb5b703acaf36bc34d5378a3f9613d453bb76'


def discover(data):
    if hashlib.sha256(data).hexdigest() != SHA256:
        raise ValueError('DB2 Special dump mismatch')
    vectors = struct.unpack('>64I', data[:256])
    decoder = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN | capstone.CS_MODE_M68K_000)
    queue = collections.deque(x for x in set(vectors[1:]) if 0x200 <= x < len(data))
    seen, hooks, roots = set(), {}, set(queue)
    branches = {'bra', 'bsr', 'bhi', 'bls', 'bcc', 'bcs', 'bne', 'beq', 'bvc', 'bvs', 'bpl', 'bmi', 'bge', 'blt', 'bgt', 'ble'}
    while queue and len(seen) < 200000:
        pc = queue.popleft()
        while pc not in seen and 0x200 <= pc < 0x200000 and not pc & 1:
            seen.add(pc)
            opcode = int.from_bytes(data[pc:pc + 2], 'big')
            if opcode & 0xff00 == 0xff00 and opcode & 255 < 62:
                target = 0x6bc + struct.unpack_from('>h', data, 0x6bc + (opcode & 255) * 2)[0]
                hooks[pc] = dict(opcode=opcode, target=target)
                roots.update([pc, pc + 2, target])
                queue.extend([pc + 2, target])
                break
            ins = next(decoder.disasm(data[pc:pc + 22], pc, count=1), None)
            if not ins or ins.mnemonic in {'dc.w', 'rts', 'rte', 'rtr', 'illegal'}:
                break
            mnemonic = ins.mnemonic.split('.')[0]
            direct = re.findall(r'\$([a-f0-9]+)', ins.op_str)
            if mnemonic in branches or mnemonic.startswith('db'):
                if direct:
                    queue.append(int(direct[-1], 16))
                if mnemonic == 'bra':
                    break
            if mnemonic in {'jsr', 'jmp'}:
                if direct and '(' not in ins.op_str:
                    target = int(direct[-1], 16)
                    queue.append(target)
                    roots.add(target)
                if mnemonic == 'jmp':
                    break
            pc += ins.size
    return roots, hooks, len(seen)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('--output', type=Path, default=Path('games/db2special/flines.toml'))
    args = parser.parse_args()
    roots, hooks, count = discover(args.rom.read_bytes())
    text = '# Vector/control-flow leads; original F-line software call dispatcher at $694.\n[functions]\nextra = [\n'
    text += ''.join(f'  0x{x:06X},\n' for x in sorted(roots)) + ']\n'
    args.output.write_text(text, encoding='utf-8')
    # Engine merges function roots from discovery_files, but reads hook sites
    # only from the top-level game.toml.
    config = args.output.with_name('game.toml')
    marker = '# BEGIN GENERATED FLINE HOOKS'
    base = config.read_text(encoding='utf-8').split(marker)[0].rstrip()
    sites = ''.join(f'\n[[widescreen_site]]\naddr = 0x{pc:06X}\nkind = "game_hook"\n' for pc in sorted(hooks))
    config.write_text(base + '\n\n' + marker + '\n' + sites, encoding='utf-8')
    print(json.dumps(dict(instruction_starts=count, fline_sites=len(hooks), roots=len(roots))))


if __name__ == '__main__':
    main()
