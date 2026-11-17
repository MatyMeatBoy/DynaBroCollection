# Controller compatibility

User explicitly requests M30 Bluetooth/2.4GHz automatic Genesis six-button layout and broad SDL2, XInput and DirectInput support.

1. Pin SDL_GameControllerDB and stage its database/license with native targets. Keep both SDL Windows backends enabled; register a configurable generic mapping for unmapped SDL joysticks before the launcher and on hotplug.
2. Add minimal GameSpec prepare/mask/disconnect seams in the shared gamepad reader. Implement device-owned M30 physical-button profiles in `games/controller_profiles.c/.h`; do not overwrite another device's global bindings.
3. Provide Automatic, Custom SDL bindings, M30 DirectInput, M30 Mini receiver and M30 XInput choices. Persist overrides by GUID plus hashed device identity, shared between games. Six-button is the game default; preserve an explicit user's three-button choice.
4. Add launcher and in-session profile selection. Verify actual SDL virtual joysticks for M30 face buttons, Mode/Start, generic fallback, player isolation and hotplug. Physical Bluetooth/2.4GHz transport remains a separate hardware check.

Sources: 8BitDo M30 FAQ; SDL_GameControllerDB; libretro's DirectInput/SDL2 M30 profiles and issue 1291. XInput can hide the model, and Mini receivers can share IDs with Sega pads; an explicit profile is necessary when identity is ambiguous. Do not claim a unique physical identification from those shared IDs.
