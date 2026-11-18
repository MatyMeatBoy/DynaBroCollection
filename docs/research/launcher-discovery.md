# Launcher file discovery

`games/launcher_discovery.h` supplies a standalone Windows Unicode scanner. It reads files only; it does not import, modify, patch, or launch a ROM. The caller owns UI and persistence. Allocate `ld_inventory` on the heap or statically (its bounded path inventory is roughly 530 KiB).

Call `ld_scan` with an optional executable directory override, two cached explicit ROM paths, and an optional configuration filename. Null base uses `GetModuleFileNameW`, independent of the working directory. `ld_best_rom(inventory, 1 or 2)` returns only a CRC32-and-size-qualified image. All candidates remain in the inventory, including unknown ROM revisions with game zero, for review. Cached paths win, followed by executable root, `roms` (also covers `ROMs` on normal Windows filesystems), `mods`, and configured locations. Equal-priority choices use Unicode path lexical order. A truncated automatic search does not choose a ROM; a qualified explicit cached path remains eligible.

Registry values come from `generated/language/db1_language.json` and `db2_language.json`: four DB1 1 MiB revisions and two DB2 2.5 MiB revisions. Both exact size and streamed CRC32 must match. Renaming a file does not qualify it. Test callers can supply a separate registry without creating synthetic full ROMs. Accepted raw extensions are `.md`, `.bin`, `.gen`, and `.68k`, case-insensitive. Archives are ignored.

## Search paths

Default configuration is executable-adjacent `paths.ini`:

```ini
[paths]
roms=rom library;D:\Games\ROMs
mods=custom mods;D:\Games\Mods
```

Relative entries resolve against the executable directory. Semicolons separate entries; spaces within paths are preserved and surrounding whitespace is trimmed. Quoting and environment-variable expansion are not supported. Use ANSI-compatible ASCII or Windows Unicode (UTF-16) INI files; `GetPrivateProfileStringW` defines decoding. `config_file` can instead point to an existing launcher INI containing these keys. The scanner never writes configuration.

The executable root is scanned directly; named and configured directories allow two nested directory levels. There are at most 32 configured roots, 128 retained files, 4096 examined directory entries, 2048 wide characters per path, 8192 INI characters per key, and 16 MiB per file. Reparse files/directories and existing reparse ancestors are rejected. This is bounded discovery, not a security sandbox against concurrent filesystem replacement. File handles open for read with read sharing only. No full-drive recursive search occurs.

## Mods and compatibility

`.bps` files with a `BPS1` header and `.ips` files with a `PATCH` header appear as inventory only, with `compatible == 0`. A recognized container header does not prove patch integrity or compatibility. The static AOT executable cannot safely execute arbitrary patched 68000 code, so discovery never applies these files or treats a patched unknown CRC as a supported game.

The current engine `upstream/segagenesisrecomp/runner/app_config.c` exposes compiled providers for widescreen, high refresh, replay diagnostics, and audio output. It does not expose a disk manifest parser for importing arbitrary external mod packages. Accordingly this scanner does not invent or silently accept a manifest format. Built-in provider settings continue through the engine; external patch activation needs a separate explicit compatibility implementation.

## Validation

Compile `games/launcher_discovery.c` and `tests/launcher_discovery_test.c` together as C11. Windows GCC with `-Wall -Wextra -Werror` passes. The fixture verifies streamed CRC, both real registries, wrong sizes/unknown hashes, extensions/archive rejection, deterministic and cached selection, truncated selection, Unicode and spaces, relative configured paths, actual file enumeration, unknown ROM inventory, and valid/invalid patch headers. It supplies only tiny test data via an injected registry. The optional symlink fixture is skipped when the OS denies symlink creation; ancestor/reparse rejection remains implemented in the scanner. No copyrighted ROM bytes are embedded in source or tests.
