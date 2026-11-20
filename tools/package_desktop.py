"""Archive a clean CMake desktop install with executable file modes."""

import argparse
import hashlib
import os
from pathlib import Path
import stat
import tarfile
import zipfile


PROGRAMS = {"DynaBroCollection", "DynaBrothers1AOT", "DynaBrothers2AOT"}


def files_in(stage):
    for path in sorted(stage.rglob("*")):
        if path.is_file() or path.is_symlink():
            yield path


def package_macos(stage, output):
    with zipfile.ZipFile(output, "x", zipfile.ZIP_DEFLATED) as archive:
        for path in files_in(stage):
            name = path.relative_to(stage).as_posix()
            if path.is_symlink():
                info = zipfile.ZipInfo(name)
                info.create_system = 3
                info.external_attr = (stat.S_IFLNK | 0o777) << 16
                archive.writestr(info, os.readlink(path))
                continue
            executable = path.name in PROGRAMS or name.endswith("/Versions/A/SDL2")
            info = zipfile.ZipInfo.from_file(path, name)
            info.create_system = 3
            info.external_attr = (stat.S_IFREG | (0o755 if executable else 0o644)) << 16
            archive.writestr(info, path.read_bytes(), compress_type=zipfile.ZIP_DEFLATED)

        framework = "DynaBroCollection.app/Contents/Frameworks/SDL2.framework"
        if (stage / framework / "Versions/A/SDL2").is_file():
            # A Windows staging directory may have no symlinks. Restore the
            # standard framework layout in the archive consumed by macOS.
            aliases = {"Versions/Current": "A", "SDL2": "Versions/Current/SDL2",
                       "Resources": "Versions/Current/Resources", "Headers": "Versions/Current/Headers"}
            for name, destination in aliases.items():
                entry = f"{framework}/{name}"
                if entry not in archive.namelist():
                    info = zipfile.ZipInfo(entry)
                    info.create_system = 3
                    info.external_attr = (stat.S_IFLNK | 0o777) << 16
                    archive.writestr(info, destination)


def package_linux(stage, output):
    def mode(entry):
        if entry.isfile():
            entry.mode = 0o755 if Path(entry.name).name in PROGRAMS else 0o644
        elif entry.isdir():
            entry.mode = 0o755
        entry.uid = entry.gid = 0
        entry.uname = entry.gname = ""
        return entry

    with tarfile.open(output, "x:gz") as archive:
        archive.add(stage, arcname="DynaBroCollection", filter=mode)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("platform", choices=["linux", "macos"])
    parser.add_argument("stage", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    runner = args.stage if args.platform == "linux" else args.stage / "DynaBroCollection.app/Contents/MacOS"
    for name in PROGRAMS:
        if not (runner / name).is_file():
            raise SystemExit(f"Missing staged executable {runner / name}")
    if not (args.stage / "recomplauncher.json").is_file():
        raise SystemExit("The staged manifest is missing")
    if args.output.resolve().is_relative_to(args.stage.resolve()):
        raise SystemExit("Write the archive outside the staged install")
    if args.platform == "linux":
        package_linux(args.stage, args.output)
    else:
        package_macos(args.stage, args.output)
    checksum = hashlib.sha256(args.output.read_bytes()).hexdigest()
    args.output.with_name(args.output.name + ".sha256").write_text(
        f"{checksum}  {args.output.name}\n", encoding="ascii")
    print(f"{checksum}  {args.output}")


if __name__ == "__main__":
    main()
