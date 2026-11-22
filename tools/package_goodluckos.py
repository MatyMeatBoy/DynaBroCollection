"""Package an ARM hard-float port for the GoodLuckOS HOME partition."""

import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import zipfile

from package_r36s import PROGRAMS, ROOT, put


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("stage", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--readelf", default="llvm-readelf")
    args = parser.parse_args()
    platform = ROOT / "platform/goodluckos"
    port = "ports/dynabrocollection"
    for program in PROGRAMS:
        elf = subprocess.check_output(
            [args.readelf, "-h", "-l", "-A", str(args.stage / program)], text=True
        )
        if not all(re.search(pattern, elf) for pattern in (
            r"Class:\s+ELF32", r"Machine:\s+ARM\b", r"AAPCS VFP|VFP registers",
            r"\[Requesting program interpreter: /lib/ld-linux-armhf\.so\.3\]",
        )):
            parser.error(f"{program} must be an ARM hard-float executable")

    entries = [(args.stage / name, name, True) for name in PROGRAMS]
    entries.extend((platform / name, name, name.endswith(".sh")) for name in ("launch.sh", "README.md"))
    entries.append((args.stage / "THIRD_PARTY_NOTICES.md", "THIRD_PARTY_NOTICES.md", False))
    entries.append((ROOT / "platform/r36s/collect-debug.sh", "debug/collect.sh", True))
    for directory in ("assets", "licenses"):
        path = args.stage / directory
        if not path.is_dir():
            parser.error(f"Missing staged {directory}")
        for item in sorted(path.rglob("*")):
            if item.is_symlink() or item.suffix.lower() in {".bin", ".md", ".smd", ".gen", ".bps"}:
                parser.error(f"Unexpected staged entry {item}")
            if item.is_file():
                entries.append((item, item.relative_to(args.stage).as_posix(), False))

    with zipfile.ZipFile(args.output, "x", zipfile.ZIP_DEFLATED) as archive:
        put(archive, platform / "dynabrocollection.puppy",
            ".local/share/applications/dynabrocollection.puppy", text_file=True)
        archive.writestr(f"{port}/roms/", "")
        for path, name, executable in entries:
            put(archive, path, f"{port}/{name}", executable=executable,
                text_file=path.suffix in {".sh", ".md"})
    digest = hashlib.sha256(args.output.read_bytes()).hexdigest()
    args.output.with_suffix(args.output.suffix + ".sha256").write_text(
        f"{digest}  {args.output.name}\n", encoding="ascii"
    )


if __name__ == "__main__":
    main()
