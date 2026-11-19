#ifndef DB1_TITLE_MENU_H
#define DB1_TITLE_MENU_H
#include <stdint.h>
enum { DB1_TITLE_UP=1,DB1_TITLE_DOWN=2,DB1_TITLE_CONFIRM=4,
 DB1_TITLE_START=8,DB1_TITLE_OTHER=16 };
enum { DB1_TITLE_PENDING_ROW=16 };
typedef struct {
 unsigned enabled,entered,active,native_selection,extras,anchor;
 unsigned released,previous,modal,pending,target,button,pulse,ack,wait;
} Db1TitleMenu;
/* Reset is Original/fail-closed. Root explicitly enables Enhanced. */
void db1_title_menu_reset(Db1TitleMenu *);
void db1_title_menu_enable(Db1TitleMenu *,int enabled);
/* Per-frame live mode update. A rising enable edge may reuse an independently
 * verified mature menu, but always requires complete input release first. */
void db1_title_menu_enable_mature(Db1TitleMenu *,int enabled,int mature,
 unsigned native_selection);
/* Native selection is read-only and must be0..3; no guest state is owned. */
void db1_title_menu_before(Db1TitleMenu *,uint32_t pc,unsigned native_selection);
/* Returns one deferred host Extras-open request. When override is0, original
 * input passes through; otherwise guest_buttons is the exclusive pad mask. */
int db1_title_menu_frame(Db1TitleMenu *,unsigned input,int *override,unsigned *guest_buttons);
int db1_title_menu_active(const Db1TitleMenu *);
/* Returns native row 0..3, Extras row 4, or a native wrap destination plus
 * DB1_TITLE_PENDING_ROW while the renderer waits for the matching SAT row. */
int db1_title_menu_selection(const Db1TitleMenu *,unsigned *selected);
/* Notify after the host modal closes. Preserves Extras, requires release. */
void db1_title_menu_host_closed(Db1TitleMenu *);
/* Host handoff after canceled native navigation. Caller must independently
 * verify a mature native menu AND complete physical input release. No guest
 * observer PCs are synthesized. Missing preconditions leave host UI inactive. */
void db1_title_menu_resume_mature(Db1TitleMenu *,int enabled,int mature,
 int released,unsigned native_selection);
#endif
