#ifndef DB1_TITLE_MENU_SMOKE_H
#define DB1_TITLE_MENU_SMOKE_H
#include <stdint.h>
/* Explicit SDL integration diagnostic, dormant without DYNA_DB1_TITLE_UI_TEST.
 * Call init after achievement-session init; opened only after the real virtual
 * title edge opened Extras. This module never opens or draws Extras itself.
 * Render after the normal Extras compositor; frame observes its real closure.
 * DYNA_DB1_TITLE_UI_MODE=level queues Return/Down/Return for fresh-profile Normal1
 * and captures after four unchanged presentations; default remains Back/six.
 * Synthetic SDL queue coverage is not a physical keyboard/controller claim. */
void db1_title_menu_smoke_init(void);
void db1_title_menu_smoke_opened(void);
void db1_title_menu_smoke_frame(void);
void db1_title_menu_smoke_render(uint32_t *,int width,int height,int pitch_bytes);
#endif
