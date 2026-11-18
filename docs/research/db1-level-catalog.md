# DB1 level catalogue runtime bridge

DB1 Extras uses the pure catalogue for count, readable mode/stage labels and
launch requests. Practice/Normal/Hard have stage1 access. Valid saved targets
extend only that mode's replay frontier; Back requires a valid Back save.
Alien targets require their own exact saved code. This is selection convenience,
not completed-level evidence or achievement progress.

Selecting a target closes Extras and queues an ephemeral native password request
(source_pc0). It does not change guest PC, RAM or the saved store. From the mature title menu, bounded controller pulses navigate to the native
Password screen automatically. The native parser validates the selected code.
Fresh user input cancels navigation, held buttons drain until release, and a
300-frame timeout prevents a stuck request. Requests outside the title menu
return to Extras with guidance; they never jump the guest PC.
DB2 retains its prior saved-code count/item/launch branch and return behavior.

The provider receives the effective-ROM-qualified session and identity-checked
store from runtime startup. The catalogue revalidates target bytes against the
ROM table; it does not independently recompute the ROM hash.

SaveTest locates the expected saved target in the catalogue by mode/stage,
selects that row via Extras input and verifies exact requested code metadata and
bytes. Store count remains the saved-entry count, not catalogue access count.

Private strict runtime-object compile and focused bridge test pass with GCC
-std=c11 -Wall -Wextra -Werror -UNDEBUG. The bridge test uses inert Extras stubs;
it verifies provider labels, queued pending request, source_pc0, unchanged store,
invalid selection and DB2 empty behavior. Native compositor, automatic navigation, cancellation, timeout and SaveTest
were subsequently qualified in isolated production builds. Practice, Normal,
Hard and saved Back entry launches passed destination assertions. The separate
85-destination load matrix includes all table entries reachable through the
password flow; it does not certify campaign completion.

Integration: both game targets containing progress_runtime.c now need
 games/db1_level_catalog.c linked (the DB2 runtime object references its guarded
DB1 branch). The bridge test links catalogue, progress_session, progress_store,
progress_replay, progress_ui and bcrypt on Windows, with the runner include paths.
