# Default-on language selection

Authorized by the user's request to fix/rebuild the language system and continue.

1. Implement `games/language_patch.c/.h`: bounded, all-or-nothing selection between qualified Japanese and current partial English images. Validate full CRC and every sparse span before mutation. Test roundtrip, alternate input edition, size/bounds, corrupt input, and rollback on invalid target in CTest.
2. Implement `tools/generate_language.py`: hash-qualified source images, sparse spans covering all accepted revisions, generated private byte arrays and identity metadata. Keep cartridge bytes out of Git. Both original dumps and existing translated copies must work.
3. Apply selected language once at native entry, before any guest instruction. English defaults on; Japanese remains recoverable in memory and originals on disk remain unchanged. Test both games from Japanese inputs and both language settings.
4. Pin and integrate shared `recomp-ui` launcher/in-game UI. Add one language feature, persistence and an explicit restart requirement for in-session changes. Do not swap code/resources under a running guest.
5. Rebuild, test native scene captures and settings persistence. Record partial translation status clearly. Full translation, game selection shell and true widescreen remain separate work.

The source program uses the existing EN/JP static modules for DB1 and the original static module for DB2. A changed label remains ROM data read by guest code; no interpreter fallback is introduced.
