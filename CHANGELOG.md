# v0.5.0 Beta 1

- Both Dyna Brothers games report newly earned achievements through the Recomp Achievements SDK.
- RecompLauncher sessions support automatic display mode and controller-specific button icons.
- The Windows release runs on SDL3 through sdl2-compat. ROM error messages, log rotation and writable profile fallback improve startup recovery.

# v0.4.3

- DB2 keeps the English menu plates consistent while the stone rises. Skipping the title clips the parked Sega Channel logo, and the upper sky follows the native palette.
- DB2 restores the intro side fade and keeps the widescreen HUD in place during pause and the opening animation of a Classic stage.
- Start + Select offers Return to Title and Quit Game in both games, with confirmation.
- The guest frame clock continues when the game window loses focus. Display synchronization resumes when the window regains focus.

# v0.4.1

- DB2 Practice/Story tutorial dialogue is now English: the ovi's opening cutscene and the full explanation (button controls, "THE EGG" rules, grass/egg/power lines, CANCEL, mid-stage surrender, "practice mode starts now") -- 25 new RAM-script catalog entries (`ram_text` can now pin the number of leading control cells with `skip`).
- DB2 title screen and main menu on a taller 16:9 canvas: the scenery is anchored to the bottom (the stone and ground reach the bottom edge instead of leaving junk tile rows), the sky above is generated from the real sky rows, and the framing is kept through the title-to-menu camera move.
- Title/menu clouds no longer pop in at the screen margins while they are still parked off-screen (margin sprites show only once they move), and the stuck SEGA CHANNEL logo after skipping the title animation is gone.
- Menu labels: English labels are applied during the title-to-menu scroll (plates ordered by nametable row), clouds now pass in front of the translated plates like they do over PASSWORD/OPTIONS, and VERSUS is a clean green with no stray yellow rows.
- DB2 gameplay: pausing and returning from the Info View keep the widened frame instead of dropping to the 4:3-style fallback for a couple of frames.
- Repo cleanup: scratch/old candidate artifacts under `local/` removed; version bumped.

# v0.4.0

- DB2 Info View (button A inside a stage): the map screen is now English -- TERRAIN MAP / CHARACTER MAP / EGG MAP titles, the TIRING / DAMAGE / DEADLY / SPECIAL AREA and BURNS / NOBURN legend, the TRIBE / EGG / GIVEUP and ALLY / FOE bars, the OK? / REALLY? confirmation, and the unit-name table (STEGO, TRICE, ALO, TYRAN, PTERA, P-CHAN, BLACK/BRAVO variants, bosses). 55 new catalog entries (DB2 Special English ROM CRC32 is now DE76C2B8).
- DB2 stage briefing (STAGE n) screens now fill 16:9: the confetti backdrop colour continues in the side and top/bottom margins while the original pattern and panels stay untouched in the centre.
- Per-game icons: the launcher and DynaBrothers.exe carry the stone "1+2" logo, DB1's executable the stone "1", DB2's the stone "2" (exe icon and in-game window icon).
- Verified on screen this pass: the lab/briefing dialogue chain and the Info View screen; in-stage Practice/Story gameplay produced no further untranslated RAM text in the traced routes.

# v0.3.0 beta

- Launcher: fixed a cartridge-reveal visual bug where the hover/focus animation slid the cartridge sideways across the box art instead of rising out of its top edge ("una parada y una cruzada" report); fixed the launcher's own diagnostic capture tool missing the Manual button from its screenshot list (the live app was unaffected); the home screen's version text now reads "Alpha"/version consistently with the rest of the UI.
- Manual viewer: long translation notes (ad pages, rule tables) could overflow the fixed-size text panel and get cut off mid-sentence; the viewer now measures each note and shrinks its font just enough to show the whole thing.
- ROM integrity: confirmed the existing CRC32 guard (Japanese-original and English-translation revisions, both games) is exercised by the packaged regression suite and rejects any other ROM cleanly.
- `tools/verify_checkpoint.py` now runs its 45 packaged regression checks in parallel (~3x faster, same results).
- DB1/DB2 string-level translation is complete at the catalog level; remaining gaps are tile-graphics text (not ROM strings) and a handful of screens whose text is already translated but unconfirmed on screen because no reliable scripted repro reaches them (QUIT dialog, results, Dyna Fight, Sega Channel, endings) — see `docs/handoff/PENDING.md` for the full honest accounting.
- Promoted to public BETA: translation and rendering fixes from v0.2.0/v0.2.1 have held up, and the remaining open items are either cosmetic, environment-dependent, or need a human with real hardware/input rather than more scripted work.

# v0.2.1 alpha

- New Settings > Manual viewer in the launcher: a book-sized, GDI-drawn open booklet with a staple-bound spine, PDF-reader pagination (cover alone, then real facing pairs), and a JP/EN toggle. Auto-detects whether a DB2 scan is a single page or an already-joined two-page spread and renders each correctly. Fully gamepad-reachable (D-pad to the Manual button, then LEFT/RIGHT/A/BACK inside the viewer).
- DB2 gets the same rendering fix DB1 got in v0.2.0: a single actor with a transiently unreadable record (confirmed live during ordinary battle play, not just the documented pre-battle window) no longer drops the whole frame to the native fallback image -- only that actor's own sprite is skipped.
- DB2 Special translation catalog grows from 582 to 704 entries (stage-2 RAM cutscene dialogue, password-screen fixes, stage 30 title, assorted briefing leftovers).
- Launcher: the home screen's gamepad navigation was generalized from a hardcoded 2x2 grid to a 3-row grid, with full regression coverage for the existing rows.
- Known remaining gaps unchanged from v0.2.0 (DB2 Options menu/kanji stage-name tile graphics, the DB1 title-menu wraparound timing question, a subtle moving-actor HUD-seam glitch) -- see docs/handoff/PENDING.md.

# v0.2.0 alpha

- DB2 Special translation grows from 454 to 582 catalog entries: title OPTIONS menu and sound-test names, Custom Versus settings and COM difficulty names, Practice briefings 2-6, stage-2 RAM cutscene dialogue, and a centering fix for the 3-line gameplay prompt boxes.
- DB1 native rendering: a single actor with a transiently unreadable RAM record (e.g. the frame it dies on) no longer drops the whole frame to centered-native/4:3 presentation — only that actor's own sprite is skipped, with a dedicated regression test (`manual_data_contract`/`db1_video_layers_contract`). Likely fixes the owner-reported "4:3 flash on unit death" and "glitch right after issuing an order".
- DB1 hazard weather (lightning/tsunami) clouds now fill the full widescreen canvas instead of stopping at the native 320px edge.
- Launcher: isometric box-art covers for both games with a hover-triggered cartridge reveal animation, built from real Sega Retro box scans; mouse cursor now auto-hides after 1s idle in fullscreen/borderless (both games are controller/keyboard-only).
- New in-game menu actions: Restart Level, Return to Title Screen, Quit to Desktop.
- `recomp-launcher.json`: declares `DynaBrothersLauncher.exe` as the single supported entry point for third-party recomp frontends, with both `*AOT.exe` marked as internal child processes.
- Data layer (not yet wired into a UI) for an upcoming Settings > Manuals feature: both games' Japanese manual pages plus a page-by-page English translation, tested end to end.
- Known remaining gaps: DB2 in-game Options menu text and Original mode's kanji stage names (suspected tile graphics); a subtle, low-severity moving-actor rendering glitch near the DB1 HUD/world seam; DB1 title-menu row-wrap highlight timing.

# v0.1.9 checkpoint

- DB2 Special English translation expanded from 22 to 454 catalog strings: Original/Versus/Sega Channel/Password/2P-setup menus, story-mode dialogue and stage briefings, VS character/control screens.
- New host biography overlay (`games/db2_bio_overlay.c`): creature name/stats/flavor text renders in English, decoded live from the ROM's own 8x8 menu font.
- Fixed title-menu bug: selecting Extras stayed hover-yellow after its panel opened instead of returning to the unselected white other rows use.
- Diagnostic-only groundwork for a documented DB2 widescreen SAT coherence fix (producer-epoch tracking; behavior unchanged).
- Known remaining DB2 gaps: in-game Options menu text, Original mode's kanji stage names and graphical "STAGE n" briefing cards (suspected tile graphics, not ROM text).

# v0.1.8 checkpoint

- Correct six-button terminal handshake and guest-clock expiry; real native Normal2 camera fixture.
- DB2 actual plane layout, fixed command visor, common projection and independent saved visor settings.
- Private portable development toolkit and offline restore/check commands.
- Known limitation retained: DB2 held camera pans can use native fallback; translation/campaign/hardware qualification is incomplete.

# Changelog

## 0.1.7 — checkpoint de prueba

- Extras integrado en el título de DB1, selección de niveles y navegación automática por Password con cancelación y timeout.
- Guardados independientes del idioma, importación segura de revisiones anteriores y soporte del código especial EMI.
- Ocho cuadros de diálogo gráfico traducidos, cinco signos del final reparados y logo inglés también en Original4:3.
-42 comprobaciones del paquete;50 contratos nativos y67 pruebas Python. Descarga pública completa verificada y ZIP personal independiente.
- DB2 conserva la base de0.1.6 en este archivo; su desarrollo continúa después del baseline de DB1.

## 0.1.6 — checkpoint de prueba

- Launcher verde: Settings funciona desde ambos botones reales, ventana más grande con fallback y logo1+2 aprobado.
- DB1: visor de tamaño original, desplazamiento por tiles, HUD a la derecha y alcance nativo de lluvia/daño.
- Guardado en pantallas de códigos, Continue nativo y Save del final difícil sin cambiar su temporizador; rutas Unicode protegidas.
- Sombra negra del título, suelo original hasta abajo, cielo extendido y fundido lateral del mural.
- Guía de unidades con cifras verificadas;39 comprobaciones del paquete y descarga pública verificada.
- Extras dentro del menú nativo y selección automática de niveles continúan en el desarrollo posterior al paquete.


## 0.1.5 — checkpoint de prueba

- DB1: cámara coherente, introducción de nivel y submenús extendidos, hazards continuos, fondos y logo del título integrados.
- Launcher verde único con settings en la misma ventana, DPI y navegación por mando; descubrimiento de ROMs y parches, selección explícita respetada.
- Replay de contraseña nativo verificado; guardado/Extras completo y traducciones restantes siguen pendientes. DB2 continúa pausado.

## 0.1.3 — checkpoint de prueba

- DB1: mundo ampliado a 16:9, HUD a la derecha y render independiente del buffer de 36 actores; considera todos los IDs válidos visibles y conserva las capas de cursor/selección.
- Original con presentación 4:3; Enhanced con lienzo 398×224 y escala entera por defecto. Adaptive acotado para otros aspectos de ventana.
- DB1: 76 metas locales basadas en su catálogo público y badges oficiales locked/color. DB2 Special: 93 etapas y seis hitos propios, con badges originales del segundo juego.
- Lista pausada de logros, popup con badge, cola FIFO y persistencia atómica por juego. Diagnósticos, scripts y replays excluidos del progreso.
- Test portable de DB1 con escenas, comparación de estado/audio Original/Enhanced y pruebas sintéticas de actores y prioridades.
- Continúan pendientes las traducciones completas, DB2 ampliado, campañas completas y hardware físico.

## 0.1.2 — en desarrollo

- Corregido el bloqueo de DB2 al despausar: interrupciones continuas en el stream AOT, con excepción de 44 ciclos y retorno/contexto del programa original.

- DB1: 18 nombres de créditos y mensaje secreto del final difícil en inglés.
- DB1: mensajes de completar práctica/modo alienígena y fondos de texto corregidos.
- Idiomas: se conserva la recuperación de japonés/inglés desde los dumps cualificados y las imágenes inglesas v0.1/v0.2.
- Versión compartida entre CMake, launcher y empaquetado; entregas históricas conservadas por versión/revisión.
- El gate de paquete añade las rutas preparadas de práctica, alienígenas y créditos. No certifica completar esos modos normalmente.

Traducciones completas, widescreen, modos Original/Enhanced y hardware físico siguen pendientes.

## 0.1.1 — checkpoint

### Añadido

- Launcher con dos tarjetas y ajustes compartidos por ambos juegos.
- Selección reversible japonés/inglés, con inglés activado por defecto.
- Perfiles M30 y overrides por dispositivo; fallback configurable para joysticks SDL.
- Caché de descriptores AOT con guards de instrucciones conservados.
- Empaquetado portable con DLLs, procedencia incrustada y hashes de la entrega.

### Corregido

- Seis etiquetas de mapas/rendición de DB1 que usaban índices japoneses sobre una fuente inglesa.
- Contrato de interrupción H-int, código RAM de DB2 y selección de región japonesa.
- Falsos positivos de los probes cuando no se guardaban las capturas solicitadas.
- Restauración de repeticiones que exigía mandos conectados y cambiaba sus preferencias.
- Validación de paquetes basada sólo en headers de build, en lugar del ejecutable copiado.

### Alcance

Es una entrega de desarrollo. Traducciones completas, widescreen ampliado, modos Original/Enhanced, aspecto de píxel final, campañas y hardware físico siguen pendientes. Detalles y mediciones en [notas del checkpoint](docs/releases/v0.1.1-checkpoint.md).


## 0.1.4 test checkpoint — 2026-11-18

Timer A audio correction; integer-fill canvas and DB1 scene/weather fixes; automatic SDL controller source and M30 bindings; Start+Mode menu with analog navigation; same-window full settings; themed main hub and external launch flags; Extras guide and atomic native-code progress store/replay. DB2 guarded renderer integrated, further DB2 development paused by user. See docs/releases/v0.1.4-checkpoint.md for qualification and limits.
