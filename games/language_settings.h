#pragma once
struct RecompLauncherCModProvider;
void dyna_language_settings_load(const char *settings_path);
int dyna_language_english(void);
void dyna_language_started(void);
int dyna_language_runtime_get(const char *key,int *value);
int dyna_language_runtime_set(const char *key,int value);
const struct RecompLauncherCModProvider *dyna_language_mods(const struct RecompLauncherCModProvider *base);
