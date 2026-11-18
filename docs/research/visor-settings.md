# DB1 visor position settings

`visor_settings.c` stores the requested DB1 visor offset independently from the
renderer. The runtime key is `db1.visor_offset_tiles`; the integer range is -64
through +64, in whole 8 px tiles. The renderer applies its current border clamp
to the requested value. Loading or resizing must not replace that requested
value with a temporarily clamped visible position.

Integration API:

- `dyna_visor_settings_load(settings_path, game)` reads the sibling
  `dynabrothers-display.ini`, defaults to zero, and applies the exact loaded
  request to an already bound setter for DB1.
- `dyna_visor_settings_bind(setter)` binds the live DB1 renderer setter and
  immediately applies the current request. NULL supports hub-only persistence.
- `dyna_visor_runtime_get/set` handles only the exact typed runtime key for DB1.
- `dyna_visor_mods(base)` decorates the existing widescreen package/feature.
  Game 2 receives the identical original provider pointer and no mutations or
  setter calls from this module.

The decorator appends `visor_offset_tiles`, an INTEGER option labelled
"Visor position (tiles)", to package `custom-widescreen` and feature `widescreen`.
It queries the original option counts rather than assuming an append index.
Feature status and option help explain whole 8 px tiles, automatic border
clamping and no fractional pixels. Both feature and legacy package option APIs
are supported. Other calls retain the original provider context and callbacks;
commit and prepared-commit behavior are inherited unchanged because this option
persists immediately on a successful mutation. Rewrapping its own provider does
not append twice. This is a singleton UI-thread provider, matching the existing
settings-provider ownership model.

String values accept optional leading + or -, followed by decimal digits only.
Whitespace, sign-only strings, fractions, suffixes, overflow and values outside
-64..64 are rejected. Stored values use canonical signed decimal without +.
The file contains one `db1.visor_offset_tiles=N` entry. Invalid/duplicate entries,
embedded NUL, overlong files or failed reads fall back to zero; unknown lines
are ignored. Missing settings paths cannot silently write into the working
folder. Windows UTF-8 paths are converted to UTF-16 for opening/replacement.

Writes use a same-directory exclusive temporary file, fflush, Windows _commit
(or POSIX fsync), close, and atomic replacement. A failed write/replacement
preserves the previous file, requested value and renderer state. The setter is
called only after a successful commit. Windows replacement uses the existing
system MoveFileExW API; no new third-party runtime DLL is required. The POSIX
fallback does not fsync the containing directory.

Verification: an initial no-op implementation failed the callback/default
assertion before implementation. The completed C11 -Wall -Wextra -Werror test
passes strict parsing including +2 and both boundaries, integer mutation bounds,
callback ordering, hub-only mutation followed by bind, persistence/reload,
invalid/duplicate file fallback, real Windows locked-file replacement failure
with unchanged previous bytes and callback count, UTF-8 directory roundtrip,
provider context forwarding and inherited commit, repeated wrapping, feature and
legacy option metadata, and inactive game 2 behavior. Tests create and remove
only their own PID-named directory and files under the test working directory.
No user settings profile is read or changed. Native settings UI and renderer
integration remain the root integration owner's next verification step.
