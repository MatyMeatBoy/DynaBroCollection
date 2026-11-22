"""Package and audit the Linux AArch64 handheld build."""

import argparse
import hashlib
from pathlib import Path
import re
import shutil
import stat
import struct
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parent.parent
PORT = ROOT / "platform/r36s"
FOLDER = "dynabrocollection"
LAUNCHERS = ("Dyna Brothers.sh", "Dyna Brothers 2 Special.sh")
PROGRAMS = ("DynaBrothers1AOT", "DynaBrothers2AOT")
LIBRARIES = ("lib/libSDL2-2.0.so.0", "lib/libSDL3.so.0")


def audit_elf(path, readelf):
    report = subprocess.check_output([readelf, "-h", "-l", "-d", "-V", str(path)], text=True)
    if not re.search(r"Class:\s+ELF64", report) or not re.search(r"Machine:\s+AArch64", report):
        raise ValueError(f"Expected an AArch64 ELF64 binary: {path}")
    if path.name in PROGRAMS and "[Requesting program interpreter: /lib/ld-linux-aarch64.so.1]" not in report:
        raise ValueError(f"Unexpected loader: {path}")
    versions = [tuple(map(int, version.split("."))) for version in re.findall(r"\bGLIBC_(\d+(?:\.\d+)+)", report)]
    if max(versions, default=(0,)) > (2, 31) or re.search(r"\b(?:GLIBCXX_|CXXABI_|GLIBC_PRIVATE)", report):
        raise ValueError(f"Unsupported runtime symbol requirement: {path}")
    return report


def put(archive, path, name, executable=False, text_file=False):
    info = zipfile.ZipInfo.from_file(path, name)
    info.create_system = 3
    info.external_attr = (stat.S_IFREG | (0o755 if executable else 0o644)) << 16
    info.compress_type = zipfile.ZIP_DEFLATED
    with archive.open(info, "w") as writer:
        if text_file:
            writer.write(path.read_bytes().replace(b"\r\n", b"\n"))
        else:
            with path.open("rb") as reader:
                shutil.copyfileobj(reader, writer)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("stage", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--screenshot", type=Path, required=True)
    parser.add_argument("--readelf", default="llvm-readelf")
    args = parser.parse_args()
    png = args.screenshot.read_bytes()
    if png[:8] != b"\x89PNG\r\n\x1a\n" or struct.unpack(">II", png[16:24]) != (640, 480):
        parser.error("the screenshot must be a 640x480 PNG")
    for name in (*PROGRAMS, *LIBRARIES):
        print(audit_elf(args.stage / name, args.readelf))
    required_dirs = [args.stage / name for name in ("assets", "licenses")]
    if any(not path.is_dir() for path in required_dirs):
        parser.error("install the assets and third-party licenses into the stage before packaging")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "x", zipfile.ZIP_DEFLATED) as archive:
        for name in LAUNCHERS:
            put(archive, PORT / name, name, executable=True, text_file=True)
        for name in ("launch.sh", "gameinfo.xml", "port.json", "README.md"):
            put(archive, PORT / name, f"{FOLDER}/{name}", executable=name.endswith(".sh"), text_file=True)
        put(archive, args.screenshot, f"{FOLDER}/screenshot.png")
        for name in (*PROGRAMS, *LIBRARIES, "THIRD_PARTY_NOTICES.md"):
            put(archive, args.stage / name, f"{FOLDER}/{name}", executable=name in PROGRAMS)
        for directory in required_dirs:
            for path in sorted(directory.rglob("*")):
                if path.is_file():
                    if path.is_symlink() or path.suffix.lower() in {".bin", ".md", ".smd", ".gen", ".bps"}:
                        raise ValueError(f"Unexpected input in the staged resources: {path}")
                    put(archive, path, f"{FOLDER}/{path.relative_to(args.stage).as_posix()}")
        archive.writestr(f"{FOLDER}/roms/README.txt", "Copy your own supported dumps here as db1.md and db2.md.\n")
    with args.output.open("rb") as payload:
        digest = hashlib.file_digest(payload, "sha256").hexdigest()
    args.output.with_name(args.output.name + ".sha256").write_text(f"{digest}  {args.output.name}\n", encoding="ascii")


if __name__ == "__main__":
    main()
