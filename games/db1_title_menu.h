#ifndef DB1_TITLE_MENU_H
#define DB1_TITLE_MENU_H
#include <stdint.h>
enum { DB1_TITLE_UP=1,DB1_TITLE_DOWN=2,DB1_TITLE_CONFIRM=4,
 DB1_TITLE_START=8,DB1_TITLE_OTHER=16 };
typedef struct {
 unsigned enabled,entered,active,native_selection,extras,anchor;
 unsigned released,previous,modal,pending,target,button,pulse,ack,wait;
} Db1TitleMenu;
/* Reset is Original/fail-closed. Root explicitly enables Enhanced. */
void db1_title_menu_reset(Db1TitleMenu *);
void db1_title_menu_enable(Db1TitleMenu *,int enabled);
/* Native selection is read-only and must be0..3; no guest state is owned. */
void db1_title_menu_before(Db1TitleMenu *,uint32_t pc,unsigned native_selection);
/* Returns one deferred host Extras-open request. When override is0, original
 * input passes through; otherwise guest_buttons is the exclusive pad mask. */
int db1_title_menu_frame(Db1TitleMenu *,unsigned input,int *override,unsigned *guest_buttons);
int db1_title_menu_active(const Db1TitleMenu *);
int db1_title_menu_selection(const Db1TitleMenu *,unsigned *selected);
/* Notify after the host modal closes. Preserves Extras, requires release. */
void db1_title_menu_host_closed(Db1TitleMenu *);
#endif
