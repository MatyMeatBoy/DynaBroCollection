#pragma once
#include "m68k_step.h"
M68kStepResult dyna_step_cached(M68kStepContext *context,const M68kStepModule *module);
void dyna_step_cache_clear(void);
