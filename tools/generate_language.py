"""Generate private, reversible language data for ahead-of-time native builds."""
import json
from pathlib import Path
import re

from tools.rompatch import identity


def ranges(images):
    if not 2 <= len(images) <= 3 or not images[0] or any(len(x) != len(images[0]) for x in images):
        raise ValueError('Language variants require two or three equal-sized images')
    changed = [i for i in range(len(images[0])) if any(x[i] != images[0][i] for x in images[1:])]
    spans = []
    for index in changed:
        if spans and index - spans[-1][1] <= 8:
            spans[-1] = (spans[-1][0], index + 1)
        else:
            spans.append((index, index + 1))
    return spans


def emit_image(images, name, output):
    if not re.fullmatch(r'[a-z][a-z0-9_]*', name):
        raise ValueError('Unsafe language image identifier')
    spans = ranges(images)
    if not spans:
        raise ValueError('Language images have no differing data')
    output.mkdir(parents=True, exist_ok=True)
    lines = ['/* Generated from qualified local images. Do not distribute cartridge bytes. */',
             '#pragma once', '#include "language_patch.h"']
    for variant, image in enumerate(images):
        payload = b''.join(image[start:end] for start, end in spans)
        lines.append(f'static const uint8_t {name}_payload_{variant}[]={{')
        lines.extend(','.join(f'0x{x:02x}' for x in payload[i:i+24]) + ',' for i in range(0, len(payload), 24))
        lines.append('};')
    lines.append(f'static const DynaLanguageSpan {name}_spans[]={{')
    offset = 0
    for start, end in spans:
        pointers = ','.join(f'{name}_payload_{i}+{offset}' for i in range(len(images)))
        lines.append(f'{{0x{start:x},{end-start},{{{pointers}}}}},')
        offset += end-start
    lines.append('};')
    crcs = ','.join('0x' + identity(image)['crc32'] + 'u' for image in images)
    lines.append(f'static const DynaLanguageImage {name}_language_image={{{len(images[0])},{len(images)},{{{crcs}}},{name}_spans,{len(spans)}}};')
    (output / f'{name}_language.h').write_text('\n'.join(lines)+'\n', encoding='utf-8')
    report = dict(variants=len(images), spans=len(spans), bytes_per_variant=offset,
                  images=[identity(x) for x in images])
    (output / f'{name}_language.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    return report
