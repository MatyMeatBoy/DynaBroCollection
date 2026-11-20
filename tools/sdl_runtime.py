"""Stage the official SDL2 ABI shim and SDL3 runtime for Windows."""

from hashlib import sha256
from io import BytesIO
from pathlib import Path
from urllib.request import urlopen
from zipfile import ZipFile


RELEASE = "2.32.74"
ASSET = f"sdl2-compat-{RELEASE}-win32-x64.zip"
ASSET_SHA256 = "94a0184bc2d0670151a023656fb58e8f722fc001bda3a6a424234c88c5501cc0"
DOWNLOAD = f"https://github.com/libsdl-org/sdl2-compat/releases/download/release-{RELEASE}/{ASSET}"


def stage_sdl3(destination: Path, cache: Path) -> dict:
    archive = cache / ASSET
    if archive.is_file():
        data = archive.read_bytes()
    else:
        with urlopen(DOWNLOAD, timeout=60) as response:
            data = response.read()
    if sha256(data).hexdigest() != ASSET_SHA256:
        raise RuntimeError(f"Incorrect SHA-256 for {ASSET}")
    if not archive.exists():
        cache.mkdir(parents=True, exist_ok=True)
        archive.write_bytes(data)

    licenses = destination / "licenses"
    licenses.mkdir(exist_ok=True)
    with ZipFile(BytesIO(data)) as release:
        for name in ("SDL2.dll", "SDL3.dll"):
            binary = release.read(name)
            if not binary.startswith(b"MZ"):
                raise RuntimeError(f"Invalid Windows runtime {name}")
            (destination / name).write_bytes(binary)
        (licenses / "SDL3-sdl2-compat.txt").write_bytes(release.read("LICENSE.txt"))
    return {"backend": "SDL3", "compatibility_layer": f"sdl2-compat {RELEASE}",
            "archive_sha256": ASSET_SHA256}
