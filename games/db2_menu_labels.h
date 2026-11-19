#ifndef DB2_MENU_LABELS_H
#define DB2_MENU_LABELS_H
#include <stdint.h>
struct GVDP;
/* English labels for the four graphic main-menu plates (れんしゅう / ストーリー /
 * オリジナル / たいけつ モード). PASSWORD and OPTIONS are already English art.
 * The plates are recognised in plane A by the shared "モード" tiles 17F-184
 * (upper row) / 18F-194 (lower row); the Japanese art is covered with the
 * plate's own fill tile and the English word is drawn with the ROM menu font
 * at 2x, inked per scanline with the original label's own colours so the
 * gradient survives. Presentation only; never writes guest state. */
void db2_menu_labels_set_english(int english);
/* Once per frame (line 0): locate the plates in the current VDP state. */
void db2_menu_labels_frame(const struct GVDP *v);
/* Paint one native line; origin_x = canvas x of native column 0. */
void db2_menu_labels_row(const struct GVDP *v, int line, uint32_t *out, int width, int origin_x);
#endif
