"""Build and sign a single DynaBroCollection APK for arm64-v8a and x86_64."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
from tempfile import TemporaryDirectory
import zipfile

ROOT = Path(__file__).resolve().parent.parent
ABIS = {"arm64-v8a": "android-arm64", "x86_64": "android-x64"}
PLATFORM = ROOT / "platform/android"


def executable(directory, name):
    for suffix in (".exe", ".bat", ""):
        candidate = directory / (name + suffix)
        if candidate.is_file():
            return candidate
    raise FileNotFoundError(directory / name)


def run(*args):
    subprocess.run([str(arg) for arg in args], cwd=ROOT, check=True)


def configure_native(args, cmake, ninja, abi):
    build = args.build_root / ABIS[abi]
    run(cmake, "-S", ROOT, "-B", build, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release",
        f"-DCMAKE_MAKE_PROGRAM={ninja}",
        f"-DCMAKE_TOOLCHAIN_FILE={args.ndk / 'build/cmake/android.toolchain.cmake'}",
        f"-DANDROID_ABI={abi}", "-DANDROID_PLATFORM=android-24", "-DANDROID_STL=c++_static",
        f"-DDYNA_ANDROID_SDL_SOURCE={args.sdl2}", "-DDYNA_LAUNCHER_UI=ON")
    run(cmake, "--build", build, "--target", "DynaBrothers1AOT", "DynaBrothers2AOT",
        "-j", args.jobs)
    return build


def package(args, builds, stage):
    sdk = args.sdk / "build-tools/35.0.0"
    android_jar = args.sdk / "platforms/android-35/android.jar"
    classes = stage / "classes"
    assets = stage / "assets/assets"
    classes.mkdir(parents=True)
    assets.mkdir(parents=True)
    assets_source = next(iter(builds.values())) / "assets"
    for name in ("achievements", "recomp-achievements", "fonts", "guide", "img"):
        if (assets_source / name).is_dir():
            shutil.copytree(assets_source / name, assets / name)
    shutil.copy2(assets_source / "gamecontrollerdb.txt", assets)

    notices = assets / "licenses"
    notices.mkdir()
    shutil.copy2(ROOT / "THIRD_PARTY_NOTICES.md", notices)
    shutil.copy2(args.sdl2 / "LICENSE.txt", notices / "SDL2.txt")
    achievement_sdk = ROOT / "third_party/recomp-achievements"
    shutil.copy2(achievement_sdk / "LICENSE", notices / "RecompAchievements.txt")
    shutil.copy2(achievement_sdk / "sdk/third_party/cjson/LICENSE", notices / "RecompAchievements-cJSON.txt")
    shutil.copy2(achievement_sdk / "sdk/third_party/rcheevos/LICENSE", notices / "RecompAchievements-rcheevos.txt")

    sources = list((args.sdl2 / "android-project/app/src/main/java").rglob("*.java"))
    sources += list((PLATFORM / "java").rglob("*.java"))
    run(executable(args.jdk / "bin", "javac"), "--release", "8", "-encoding", "UTF-8",
        "-classpath", android_jar, "-d", classes, *sources)
    java_archive = stage / "app.jar"
    run(executable(args.jdk / "bin", "jar"), "cf", java_archive, "-C", classes, ".")
    os.environ["JAVA_HOME"] = str(args.jdk)
    os.environ["PATH"] = str(args.jdk / "bin") + os.pathsep + os.environ.get("PATH", "")
    run(executable(sdk, "d8"), "--lib", android_jar, "--min-api", "24",
        "--output", stage, java_archive)

    version = (ROOT / "VERSION").read_text().strip()
    major, minor, patch = (int(part) for part in version.split("."))
    code = major * 100000 + minor * 1000 + patch * 10 + args.beta
    unsigned = stage / "unsigned.apk"
    run(executable(sdk, "aapt2"), "link", "-I", android_jar, "--manifest", PLATFORM / "AndroidManifest.xml",
        "--version-name", f"{version}-beta{args.beta}", "--version-code", code, "-A", stage / "assets",
        "-o", unsigned)
    ndk_host = next((args.ndk / "toolchains/llvm/prebuilt").iterdir())
    strip = executable(ndk_host / "bin", "llvm-strip")
    libraries = ("libSDL2.so", "libDynaBrothers1AOT.so", "libDynaBrothers2AOT.so")
    with zipfile.ZipFile(unsigned, "a", zipfile.ZIP_DEFLATED) as apk:
        apk.write(stage / "classes.dex", "classes.dex")
        for abi, build in builds.items():
            native = stage / abi
            native.mkdir()
            for library in libraries:
                source = build / ("android-sdl" if library == "libSDL2.so" else "") / library
                target = native / library
                run(strip, "--strip-unneeded", "-o", target, source)
                apk.write(target, f"lib/{abi}/{library}", compress_type=zipfile.ZIP_STORED)
    aligned = stage / "aligned.apk"
    zipalign = executable(sdk, "zipalign")
    run(zipalign, "-P", "16", "-f", "4", unsigned, aligned)
    keystore = args.keystore
    if not keystore:
        # Keep this key stable across beta builds on the same machine so upgrades work.
        keystore = args.build_root / "dyna-android-debug.jks"
        if not keystore.exists():
            run(executable(args.jdk / "bin", "keytool"), "-genkeypair", "-keystore", keystore,
                "-alias", "androiddebugkey", "-storepass", "android", "-keypass", "android",
                "-keyalg", "RSA", "-keysize", "2048", "-validity", "10000",
                "-dname", "CN=Android Debug,O=Android,C=US")
        sign_options = ("--ks-key-alias", "androiddebugkey", "--ks-pass", "pass:android")
    else:
        sign_options = ("--ks-pass", "env:DYNA_ANDROID_KEYSTORE_PASSWORD")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    apksigner = executable(sdk, "apksigner")
    run(apksigner, "sign", "--ks", keystore, *sign_options, "--out", args.output, aligned)
    run(apksigner, "verify", "--verbose", args.output)
    run(zipalign, "-P", "16", "-c", "4", args.output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path, required=True)
    parser.add_argument("--ndk", type=Path, required=True)
    parser.add_argument("--jdk", type=Path, required=True)
    parser.add_argument("--sdl2", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--build-root", type=Path, default=ROOT / "build")
    parser.add_argument("--keystore", type=Path)
    parser.add_argument("--abi", choices=ABIS, action="append")
    parser.add_argument("--beta", type=int, default=1, choices=range(1, 10))
    parser.add_argument("--jobs", type=int, default=6)
    parser.add_argument("--skip-native", action="store_true")
    args = parser.parse_args()
    for name in ("sdk", "ndk", "jdk", "sdl2", "output", "build_root", "keystore"):
        value = getattr(args, name)
        if value is not None:
            setattr(args, name, value.resolve())

    cmake_bin = args.sdk / "cmake/3.22.1/bin"
    builds = {}
    for abi in args.abi or ABIS:
        builds[abi] = (args.build_root / ABIS[abi] if args.skip_native else
                       configure_native(args, executable(cmake_bin, "cmake"), executable(cmake_bin, "ninja"), abi))
    with TemporaryDirectory(prefix="android-apk-", dir=args.build_root) as stage:
        package(args, builds, Path(stage))


if __name__ == "__main__":
    main()
