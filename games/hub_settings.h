#pragma once
#include <stddef.h>
struct SDL_Window;struct SDL_Renderer;
#include "launcher_discovery.h"
int dyna_hub_settings_run(struct SDL_Window *,struct SDL_Renderer *,unsigned game,
 const char *config,const char *cache,const char *initial,char *picked,size_t capacity,const ld_inventory *inventory);

#include <stdint.h>
void dyna_hub_controls_start(const char *config);
uint32_t dyna_hub_controls_poll(void);
