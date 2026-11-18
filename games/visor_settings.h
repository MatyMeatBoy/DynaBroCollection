#pragma once
struct RecompLauncherCModProvider;
/* DB1-only requested tile offset; the renderer applies dynamic border clamps. */
void dyna_visor_settings_load(const char *settings_path,unsigned game);
void dyna_visor_settings_bind(void (*setter)(int));
int dyna_visor_runtime_get(const char *key,int *value);
int dyna_visor_runtime_set(const char *key,int value);
const struct RecompLauncherCModProvider *dyna_visor_mods(const struct RecompLauncherCModProvider *base);
