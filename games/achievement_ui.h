#ifndef DYNA_ACHIEVEMENT_UI_H
#define DYNA_ACHIEVEMENT_UI_H
#include <stdint.h>
/* Host presentation only. Initialize after the achievement profile; root is
 * the assets/achievements directory, not the process working directory. */
void dyna_achievement_ui_init(const char *assets_root);
void dyna_achievement_ui_open(void);
int dyna_achievement_ui_is_open(void);
int dyna_achievement_ui_active(void); /* Includes a pending unlock toast. */
/* RecompRuntimeUiInput: toggle=0, back=1, up=2, down=3, left=4,
 * right=5, accept=6. Consume host input before forwarding to the guest. */
void dyna_achievement_ui_input(int semantic,int down,int repeat);
/* Compose on host ARGB8888 pixels after the guest pixel pipeline. */
void dyna_achievement_ui_render(uint32_t *pixels,int w,int h,int pitch_bytes);
void dyna_achievement_ui_reset(void);
#endif
