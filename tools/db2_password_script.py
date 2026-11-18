"""Generate a native_probe input script that types a DB2 Special password on the
password keyboard (8-column grid A-W, END at column 7 row 3 (row 3 = Y Z -> <- END)) and
screenshots the following stage briefing. Verified against KAIGAN (stage 2)."""
import sys

def pos(ch):
    i = ord(ch) - 65
    if i < 24:
        return i % 8, i // 8
    return i - 24, 3

END = (7, 3)

def moves(src, dst):
    out = []
    dx, dy = dst[0] - src[0], dst[1] - src[1]
    horiz = ['RIGHT'] * dx if dx > 0 else ['LEFT'] * -dx
    vert = ['DOWN'] * dy if dy > 0 else ['UP'] * -dy
    # Row 3 has only five keys: enter it last and leave it first.
    return horiz + vert if dst[1] == 3 else vert + horiz


def script(password, shots=True):
    lines = ['WAIT 1999', 'PRESS START 12', 'WAIT 500', 'PRESS START 3', 'WAIT 600']
    for _ in range(3):
        lines += ['PRESS DOWN 3', 'WAIT 60']
    lines += ['PRESS START 3', 'WAIT 600', 'PRESS DOWN 3', 'WAIT 60', 'PRESS C 3', 'WAIT 600']
    cur = (0, 0)
    for ch in password.upper():
        for m in moves(cur, pos(ch)):
            lines += [f'PRESS {m} 4', 'WAIT 55']
        lines += ['PRESS C 4', 'WAIT 55']
        cur = pos(ch)
    if len(password) < 6:  # six letters auto-submit; shorter words need the END key
        for m in moves(cur, END):
            lines += [f'PRESS {m} 4', 'WAIT 55']
        lines += ['PRESS C 4', 'WAIT 55']
    lines += ['WAIT 1500', 'SCREENSHOT cut.png', 'PRESS START 3', 'WAIT 1000', 'SCREENSHOT brief.png']
    return '\n'.join(lines) + '\n'

if __name__ == '__main__':
    sys.stdout.write(script(sys.argv[1]))
