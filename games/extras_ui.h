#ifndef DYNA_EXTRAS_UI_H
#define DYNA_EXTRAS_UI_H
#include <stddef.h>
#include <stdint.h>
typedef struct DynaExtrasLevel {const char *title,*detail;int available;} DynaExtrasLevel;
typedef struct DynaExtrasLevelProvider {
 void *userdata;
 size_t (*count)(void *,unsigned game);
 int (*item)(void *,unsigned game,size_t index,DynaExtrasLevel *out);
 int (*launch)(void *,unsigned game,size_t index);
} DynaExtrasLevelProvider;
/* assets_root is assets/guide. Optional PNG names are <capture_key>.png.
 * Provider owns its strings. Callbacks must not re-enter this module. */
void dyna_extras_ui_set_level_provider(const DynaExtrasLevelProvider *);
void dyna_extras_ui_init(unsigned game,const char *assets_root);
void dyna_extras_ui_reset(void);
void dyna_extras_ui_open(void);
void dyna_extras_ui_notice(const char *text);
int dyna_extras_ui_is_open(void);
void dyna_extras_ui_input(int semantic,int down,int repeat);
void dyna_extras_ui_render(uint32_t *,int width,int height,int pitch_bytes);
/* Read-only navigation snapshot; screen: 0 home,1 levels,2 guide sections,3 page. */
typedef struct DynaExtrasUiState {unsigned screen,selected,section,page,scroll;int image;} DynaExtrasUiState;
DynaExtrasUiState dyna_extras_ui_state(void);
#endif
