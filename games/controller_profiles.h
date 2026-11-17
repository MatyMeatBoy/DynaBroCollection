#pragma once
#include <stdint.h>
struct _SDL_GameController;
enum { DYNA_PAD_AUTO, DYNA_PAD_CUSTOM, DYNA_PAD_M30_DINPUT, DYNA_PAD_M30_MINI, DYNA_PAD_M30_XINPUT, DYNA_PAD_PROFILE_COUNT };
void dyna_controllers_load(const char *settings_path);
void dyna_controller_prepare(int joystick_index);
uint16_t dyna_controller_mask(unsigned player,struct _SDL_GameController *pad,uint16_t normal);
void dyna_controller_removed(unsigned player);
int dyna_controller_get(const char *key,int *value);
int dyna_controller_set(const char *key,int value);
const char *dyna_controller_name(unsigned player);
