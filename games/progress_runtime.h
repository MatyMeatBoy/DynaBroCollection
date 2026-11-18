#pragma once
#include <stddef.h>
#include <stdint.h>
void dyna_progress_runtime_settings(const char *path);
int dyna_progress_runtime_start(unsigned game,const uint8_t *rom,size_t size);
void dyna_progress_runtime_before(uint32_t pc,uint8_t *ram,size_t size,uint32_t d[8],const uint32_t a[8]);
int dyna_progress_runtime_action(const char *key);
const char *dyna_progress_runtime_status(void);

uint16_t dyna_progress_runtime_input(unsigned player,uint16_t mask);
