#include "../games/db1_title_menu.h"
#include <assert.h>
#include <stdio.h>
static int step(Db1TitleMenu *s,unsigned input,int expected_override,unsigned expected_pad){
 int override=-1;unsigned pad=999;int open=db1_title_menu_frame(s,input,&override,&pad);
 assert(override==expected_override&&pad==expected_pad);return open;
}
static void enter(Db1TitleMenu *s,unsigned row){
 db1_title_menu_reset(s);db1_title_menu_enable(s,1);
 db1_title_menu_before(s,0x1cb30,row);assert(!db1_title_menu_active(s));
 assert(!step(s,DB1_TITLE_CONFIRM,0,DB1_TITLE_CONFIRM));
 db1_title_menu_before(s,0x1cb46,row);assert(db1_title_menu_active(s));
 assert(!step(s,DB1_TITLE_CONFIRM,1,0));assert(!step(s,0,1,0));
}
int main(void){
 Db1TitleMenu s;unsigned selected;
 db1_title_menu_reset(&s);
 db1_title_menu_before(&s,0x1cb30,3);db1_title_menu_before(&s,0x1cb46,3);
 assert(!db1_title_menu_active(&s)&&!step(&s,DB1_TITLE_DOWN,0,DB1_TITLE_DOWN));
 db1_title_menu_enable(&s,1);db1_title_menu_before(&s,0x1c9ce,0);
 db1_title_menu_before(&s,0x1cb46,0);assert(!db1_title_menu_active(&s));
 enter(&s,3);
 assert(db1_title_menu_selection(&s,&selected)&&selected==3);
 assert(!step(&s,DB1_TITLE_DOWN,1,0));
 assert(db1_title_menu_selection(&s,&selected)&&selected==4);
 assert(!step(&s,DB1_TITLE_DOWN|DB1_TITLE_CONFIRM,1,0));
 assert(!step(&s,0,1,0));
 assert(step(&s,DB1_TITLE_CONFIRM,1,0));assert(!step(&s,DB1_TITLE_CONFIRM,1,0));
 db1_title_menu_host_closed(&s);
 assert(!step(&s,DB1_TITLE_START,1,0));assert(!step(&s,0,1,0));
 assert(step(&s,DB1_TITLE_START,1,0));db1_title_menu_host_closed(&s);step(&s,0,1,0);
 assert(!step(&s,DB1_TITLE_UP,1,0)); /* Anchor3 already equals Password. */
 assert(db1_title_menu_selection(&s,&selected)&&selected==3);
 step(&s,DB1_TITLE_UP,1,0);step(&s,0,1,0);
 assert(!step(&s,DB1_TITLE_CONFIRM,0,DB1_TITLE_CONFIRM));
 db1_title_menu_before(&s,0x1cbc4,3);assert(!db1_title_menu_active(&s));
 enter(&s,0);assert(!step(&s,DB1_TITLE_UP,1,0));step(&s,0,1,0);
 assert(db1_title_menu_selection(&s,&selected)&&selected==4);
 assert(!step(&s,DB1_TITLE_DOWN,1,0)); /* Anchor0 already equals New Game. */
 step(&s,0,1,0);assert(!step(&s,DB1_TITLE_CONFIRM,0,DB1_TITLE_CONFIRM));
 enter(&s,0);step(&s,DB1_TITLE_UP,1,0);step(&s,0,1,0);
 assert(!step(&s,DB1_TITLE_UP,1,DB1_TITLE_UP)); /* Original0->3 wrap. */
 assert(db1_title_menu_selection(&s,&selected)&&selected==DB1_TITLE_PENDING_ROW+3);
 db1_title_menu_before(&s,0x1cb30,3);
 assert(db1_title_menu_selection(&s,&selected)&&selected==DB1_TITLE_PENDING_ROW+3);
 step(&s,DB1_TITLE_UP|DB1_TITLE_CONFIRM,1,DB1_TITLE_UP);
 step(&s,DB1_TITLE_UP,1,DB1_TITLE_UP);
 for(unsigned i=0;i<3;i++)step(&s,DB1_TITLE_UP,1,0);
 for(unsigned i=0;i<140;i++)step(&s,DB1_TITLE_UP,1,0);
 assert(db1_title_menu_active(&s));
 step(&s,DB1_TITLE_CONFIRM,1,0);step(&s,0,1,0);
 assert(db1_title_menu_selection(&s,&selected)&&selected==3);
 assert(!step(&s,DB1_TITLE_CONFIRM,0,DB1_TITLE_CONFIRM));
 enter(&s,3);step(&s,DB1_TITLE_DOWN,1,0);step(&s,0,1,0);
 step(&s,DB1_TITLE_DOWN,1,DB1_TITLE_DOWN);
 assert(db1_title_menu_selection(&s,&selected)&&selected==DB1_TITLE_PENDING_ROW);
 db1_title_menu_before(&s,0x1cb30,0);
 step(&s,0,1,DB1_TITLE_DOWN);step(&s,0,1,DB1_TITLE_DOWN);
 for(unsigned i=0;i<4;i++)step(&s,0,1,0);
 assert(db1_title_menu_selection(&s,&selected)&&selected==0);
 enter(&s,1);assert(!step(&s,DB1_TITLE_UP,0,DB1_TITLE_UP));
 db1_title_menu_before(&s,0x1cb30,0);assert(db1_title_menu_selection(&s,&selected)&&selected==0);
 assert(!step(&s,0,0,0));db1_title_menu_before(&s,0x1cbea,0);assert(!db1_title_menu_active(&s));
 enter(&s,3);step(&s,DB1_TITLE_DOWN,1,0);db1_title_menu_before(&s,0x1cb4a,2);
 assert(!db1_title_menu_active(&s)); /* Unexpected native anchor change. */
 enter(&s,3);db1_title_menu_before(&s,0x1cb4a,4);assert(!db1_title_menu_active(&s));
 enter(&s,3);db1_title_menu_enable(&s,0);
 assert(!db1_title_menu_selection(&s,NULL)&&!step(&s,DB1_TITLE_DOWN,0,DB1_TITLE_DOWN));
 enter(&s,3);step(&s,DB1_TITLE_DOWN,1,0);step(&s,0,1,0);step(&s,DB1_TITLE_DOWN,1,DB1_TITLE_DOWN);
 for(unsigned i=0;i<140;i++){int over;unsigned pad;assert(!db1_title_menu_frame(&s,0,&over,&pad));}
 assert(!db1_title_menu_active(&s)&&!step(&s,DB1_TITLE_CONFIRM,0,DB1_TITLE_CONFIRM));
 const uint32_t exits[]={0x1950,0x1a14,0x1e024,0x1d7b6,0x1c5a4,0x1caf6,0x1cb0c};
 for(unsigned i=0;i<sizeof exits/sizeof exits[0];i++){
  enter(&s,0);db1_title_menu_before(&s,exits[i],0);assert(!db1_title_menu_active(&s));
 }
 db1_title_menu_resume_mature(&s,1,1,1,0);
 assert(db1_title_menu_active(&s));assert(!step(&s,DB1_TITLE_UP,1,0));
 assert(db1_title_menu_selection(&s,&selected)&&selected==4);
 step(&s,0,1,0);assert(step(&s,DB1_TITLE_CONFIRM,1,0));
 db1_title_menu_resume_mature(&s,1,1,1,3);
 assert(!step(&s,DB1_TITLE_DOWN,1,0));
 assert(db1_title_menu_selection(&s,&selected)&&selected==4);
 db1_title_menu_resume_mature(&s,1,1,1,2);
 assert(db1_title_menu_selection(&s,&selected)&&selected==2);
 assert(!step(&s,DB1_TITLE_CONFIRM,0,DB1_TITLE_CONFIRM));
 enter(&s,0);step(&s,DB1_TITLE_UP,1,0);step(&s,0,1,0);
 step(&s,DB1_TITLE_UP,1,DB1_TITLE_UP); /* Pending original wrap. */
 db1_title_menu_resume_mature(&s,1,1,1,3);
 assert(!step(&s,DB1_TITLE_DOWN,1,0));
 assert(db1_title_menu_selection(&s,&selected)&&selected==4);
 const int resume_preconditions[][3]={{0,1,1},{1,0,1},{1,1,0}};
 for(unsigned i=0;i<3;i++){
  db1_title_menu_resume_mature(&s,resume_preconditions[i][0],resume_preconditions[i][1],resume_preconditions[i][2],0);
  assert(!db1_title_menu_active(&s)&&!step(&s,DB1_TITLE_UP,0,DB1_TITLE_UP));
 }
 db1_title_menu_resume_mature(&s,1,1,1,4);
 assert(!db1_title_menu_active(&s)&&!step(&s,DB1_TITLE_DOWN,0,DB1_TITLE_DOWN));
 /* A live Original -> Enhanced switch needs no synthesized observer PC. */
 db1_title_menu_reset(&s);
 db1_title_menu_enable_mature(&s,1,1,0);
 assert(db1_title_menu_active(&s));
 assert(!step(&s,DB1_TITLE_UP|DB1_TITLE_CONFIRM,1,0));
 db1_title_menu_enable_mature(&s,1,1,0);
 assert(!step(&s,DB1_TITLE_UP,1,0));
 assert(db1_title_menu_selection(&s,&selected)&&selected==0);
 assert(!step(&s,0,1,0));
 assert(!step(&s,DB1_TITLE_UP,1,0));
 assert(db1_title_menu_selection(&s,&selected)&&selected==4);
 db1_title_menu_enable_mature(&s,0,1,0);
 assert(!db1_title_menu_active(&s)&&!step(&s,DB1_TITLE_UP,0,DB1_TITLE_UP));
 db1_title_menu_enable_mature(&s,1,0,0);
 assert(!db1_title_menu_active(&s));
 db1_title_menu_enable_mature(&s,1,1,0); /* No new mode edge. */
 assert(!db1_title_menu_active(&s));
 db1_title_menu_enable_mature(&s,0,1,0);
 db1_title_menu_enable_mature(&s,1,1,4);
 assert(!db1_title_menu_active(&s));
 /* A timeout remains fail-closed while Enhanced stays enabled. */
 enter(&s,0);step(&s,DB1_TITLE_UP,1,0);step(&s,0,1,0);
 step(&s,DB1_TITLE_UP,1,DB1_TITLE_UP);
 for(unsigned i=0;i<140;i++){int over;unsigned pad;db1_title_menu_frame(&s,0,&over,&pad);}
 assert(!db1_title_menu_active(&s));
 db1_title_menu_enable_mature(&s,1,1,0);
 assert(!db1_title_menu_active(&s)&&!step(&s,DB1_TITLE_UP,0,DB1_TITLE_UP));
 puts("Title virtual Extras, Original pass-through, mature phases, held return and bounded native wrap passed");
}
