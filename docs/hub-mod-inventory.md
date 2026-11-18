Detected patch inventory
========================

Compile games/hub_mod_inventory.c with the games and recomp-ui/src include
directories, then wrap the final provider before opening the launcher:

    game.mods = dyna_hub_mod_inventory(existing_provider, &inventory);

The decorator copies up to LD_MAX_ITEMS (128) inventory records. It appends BPS
and IPS detections to the Mods catalog diagnostics. Each entry contains its
basename, UTF-8 path and explicit unapplied status, with a pointer to the built-in
English translation setting.
It never reads, applies, copies or modifies patch or ROM files. Even a discovery
record marked compatible is not claimed as supported by the static executable.

The SDK has no read-only/disabled feature flag: a synthetic feature would expose
an enable checkbox that cannot work. Diagnostics therefore provide the honest
inventory surface without creating activatable package/feature identities. The
SDK labels this section as catalog notices. Informational discoveries collapse
by default so the feature list remains visible; warnings/errors open by default.
Messages wrap at the available width and retain their severity color.

All original provider callbacks, ctx, options and flags are copied unchanged;
only catalog_diagnostic_count/get are decorated. Thus existing mutations retain
their own validation and no detected item is forwarded as a synthetic package.
Source callbacks and ctx must remain alive. Singleton, UI-thread use only;
reconfigure while the launcher is closed. NULL inventory clears detections;
NULL source supplies diagnostics only. Base diagnostics retain their order.
Paths and basenames truncate on UTF-8 code-point boundaries to SDK field limits
(resource: 191 bytes, basename: 255 bytes). Invalid Unicode is replaced by U+FFFD.

Standalone test (no ROMs or runtime required), using a C11 compiler:

    gcc -std=c11 -Wall -Wextra -Werror -Igames -Iupstream/recomp-ui/src games/hub_mod_inventory.c tests/hub_mod_inventory_test.c -o hub_mod_inventory_test.exe
    ./hub_mod_inventory_test.exe
