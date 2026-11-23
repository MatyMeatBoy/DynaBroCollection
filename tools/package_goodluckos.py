"""Package an ARM hard-float port for the GoodLuckOS HOME partition."""

import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile
import zipfile

from package_r36s import PROGRAMS, ROOT, put


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("stage", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--readelf", default="llvm-readelf")
    parser.add_argument("--strip", default="llvm-strip", help="ARM-capable strip tool")
    diagnostics = parser.add_mutually_exclusive_group()
    diagnostics.add_argument("--device-diagnostics", action="store_true")
    diagnostics.add_argument("--device-qualification", action="store_true")
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

    entries = [(platform / name, name, name.endswith(".sh")) for name in ("launch.sh", "README.md")]
    entries.append((args.stage / "THIRD_PARTY_NOTICES.md", "THIRD_PARTY_NOTICES.md", False))
    entries.append((ROOT / "platform/r36s/collect-debug.sh", "debug/collect.sh", True))
    if args.device_diagnostics:
        entries.append((platform / "profile.sh", "profile.sh", True))
        entries.append((platform / "profile-raster.sh", "profile-raster.sh", True))
    if args.device_qualification:
        entries.append((platform / "qualify.sh", "qualify.sh", True))
    if args.device_diagnostics or args.device_qualification:
        entries.append((ROOT / "games/db1/scenes/r36s-normal1-battle.input", "debug/normal1.input", False))
    for directory in ("assets", "licenses"):
        path = args.stage / directory
        if not path.is_dir():
            parser.error(f"Missing staged {directory}")
        for item in sorted(path.rglob("*")):
            if item.is_symlink() or item.suffix.lower() in {".bin", ".md", ".smd", ".gen", ".bps"}:
                parser.error(f"Unexpected staged entry {item}")
            if item.is_file():
                entries.append((item, item.relative_to(args.stage).as_posix(), False))

    with tempfile.TemporaryDirectory() as temp:
        for name in PROGRAMS:
            binary = Path(temp) / name
            subprocess.run([args.strip, "--strip-unneeded", "-o", str(binary), str(args.stage / name)], check=True)
            entries.append((binary, name, True))
        with zipfile.ZipFile(args.output, "x", zipfile.ZIP_DEFLATED) as archive:
            put(archive, platform / "dynabrocollection.puppy",
                ".local/share/applications/dynabrocollection.puppy", text_file=True)
            if args.device_diagnostics:
                for name in ("dynabrocollection-profile.puppy", "dynabrocollection-raster.puppy"):
                    put(archive, platform / name, f".local/share/applications/{name}", text_file=True)
            if args.device_qualification:
                name = "dynabrocollection-qualify.puppy"
                put(archive, platform / name, f".local/share/applications/{name}", text_file=True)
            archive.writestr(f"{port}/roms/", "")
            for path, name, executable in entries:
                put(archive, path, f"{port}/{name}", executable=executable,
                    text_file=path.suffix in {".sh", ".md", ".input"})
    digest = hashlib.sha256(args.output.read_bytes()).hexdigest()
    args.output.with_suffix(args.output.suffix + ".sha256").write_text(
        f"{digest}  {args.output.name}\n", encoding="ascii"
    )


if __name__ == "__main__":
    main()
