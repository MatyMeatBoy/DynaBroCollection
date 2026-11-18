#pragma once
struct RecompLauncherCModProvider;
/* Game1/2 select their own db1./db2.visor_offset_tiles key in a shared sibling
 * display.ini. Other lines are preserved byte-for-byte; saves reread <=1024B,
 * reject NUL/read errors/oversize, and commit before changing RAM/callback.
 * Invalid game is inactive. Renderer applies dynamic requested-tile clamps. */
void dyna_visor_settings_load(const char *settings_path,unsigned game);
void dyna_visor_settings_bind(void (*setter)(int));
int dyna_visor_runtime_get(const char *key,int *value);
int dyna_visor_runtime_set(const char *key,int value);
const struct RecompLauncherCModProvider *dyna_visor_mods(const struct RecompLauncherCModProvider *base);
