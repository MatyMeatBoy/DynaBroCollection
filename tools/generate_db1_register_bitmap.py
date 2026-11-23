"""Qualify the exact DB1 operation bodies for the register-only executor."""

import argparse
import importlib.util
from pathlib import Path


def generate(generated, classifier, output):
    spec = importlib.util.spec_from_file_location("native_catalog_compact", classifier)
    if spec is None or spec.loader is None:
        raise ValueError(f"Cannot load descriptor classifier: {classifier}")
    catalog = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(catalog)

    lines = ["/* Generated from the operation bodies and descriptor order. */"]
    for name in ("db1step", "db1jp"):
        directory = generated / name
        effects = {}
        symbols = {}
        for index in range(32):
            path = directory / f"{name}_part{index:02d}.c"
            for symbol, _ in catalog.operations(path.read_text(encoding="utf-8"), name, effects):
                if symbol in symbols:
                    raise ValueError(f"Repeated generated operation: {symbol}")
                symbols[symbol] = symbol
        module = (directory / f"{name}_module.c").read_text(encoding="utf-8")
        descriptors = catalog.entries(module, name, symbols)
        if not descriptors:
            raise ValueError(f"Empty generated descriptor catalog: {name}")
        bits = bytearray((len(descriptors) + 7) // 8)
        for index, (_, _, _, symbol) in enumerate(descriptors):
            if effects.get(symbol) == 1:
                bits[index // 8] |= 1 << (index % 8)
        lines.append(f"static const uint8_t {name}_pure_bits[] = {{")
        for start in range(0, len(bits), 16):
            lines.append("    " + ", ".join(f"0x{value:02x}" for value in bits[start:start + 16]) + ",")
        lines.extend(("};", f"enum {{ {name}_pure_count = {len(descriptors)} }};"))
    output.write_text("\n".join(lines) + "\n", encoding="ascii")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("generated", type=Path)
    parser.add_argument("classifier", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    generate(args.generated, args.classifier, args.output)
