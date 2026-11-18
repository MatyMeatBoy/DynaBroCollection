#include "db1_password_nav.h"
#include <assert.h>
#include <stdio.h>
static Db1PasswordNav s;
static unsigned frame(unsigned input){int override=-1;unsigned pad=99;db1_password_nav_frame(&s,input,&override,&pad);assert(override==1);return pad;}
static void start(unsigned native){assert(db1_password_nav_request(&s,1,native));db1_password_nav_before(&s,0x1cb46,native,DB1_PASSWORD_B);assert(frame(DB1_PASSWORD_A)==0);assert(frame(DB1_PASSWORD_A)==0);assert(frame(0)==0);}
static void move(unsigned direction,unsigned target){
 for(unsigned i=0;i<3;i++)assert(frame(0)==direction);
 db1_password_nav_before(&s,0x1cb30,target,DB1_PASSWORD_B);
 for(unsigned i=0;i<3;i++)assert(frame(0)==0);
 assert(frame(0)==0);
}
static void confirm(void){
 for(unsigned i=0;i<3;i++)assert(frame(0)==DB1_PASSWORD_B);
 for(unsigned i=0;i<3;i++)assert(frame(0)==0);
 assert(s.status==DB1_PASSWORD_WAIT);
 db1_password_nav_before(&s,0x1cbc4,3,DB1_PASSWORD_B);
 db1_password_nav_before(&s,0x1cc92,3,DB1_PASSWORD_B);
 db1_password_nav_before(&s,0x1d7b6,3,DB1_PASSWORD_B);
 assert(s.status==DB1_PASSWORD_HANDED_OFF&&!db1_password_nav_owns(&s));
}
int main(void){
 db1_password_nav_reset(&s);assert(!db1_password_nav_owns(&s));
 assert(!db1_password_nav_request(&s,0,0));assert(s.status==DB1_PASSWORD_UNAVAILABLE);
 assert(!db1_password_nav_request(&s,1,4));
 start(0);move(DB1_PASSWORD_UP,3);confirm();
 start(1);move(DB1_PASSWORD_DOWN,2);move(DB1_PASSWORD_DOWN,3);confirm();
 start(2);move(DB1_PASSWORD_DOWN,3);confirm();
 start(3);confirm();
 /* Never emit confirmation with an invalid physical remap. */
 start(3);db1_password_nav_before(&s,0x1cb54,3,3);
 for(unsigned i=0;i<12;i++)assert(frame(0)==0);
 db1_password_nav_before(&s,0x1cb54,3,DB1_PASSWORD_B);confirm();
 /* Remap changes mid-press abort, never mix physical actions. */
 start(3);assert(frame(0)==DB1_PASSWORD_B);
 db1_password_nav_before(&s,0x1cb54,3,DB1_PASSWORD_A);assert(frame(0)==0);
 assert(s.status==DB1_PASSWORD_CANCELED&&db1_password_nav_owns(&s));
 for(unsigned i=0;i<8;i++)assert(frame(DB1_PASSWORD_A)==0);
 assert(frame(0)==0&&!db1_password_nav_owns(&s));
 /* Missing acknowledgement cannot issue another direction. */
 start(1);for(unsigned i=0;i<3;i++)assert(frame(0)==DB1_PASSWORD_DOWN);
 for(unsigned i=0;i<20;i++)assert(frame(0)==0);
 assert(s.status==DB1_PASSWORD_MOVE);
 db1_password_nav_before(&s,0x1cb30,3,DB1_PASSWORD_B);
 assert(s.status==DB1_PASSWORD_CANCELED);
 /* Native acknowledgement followed by regression is a scene drift. */
 start(1);assert(frame(0)==DB1_PASSWORD_DOWN);
 db1_password_nav_before(&s,0x1cb30,2,DB1_PASSWORD_B);
 db1_password_nav_before(&s,0x1cb30,1,DB1_PASSWORD_B);
 assert(s.status==DB1_PASSWORD_CANCELED);
 /* User input after release is cancellation and consumed until released. */
 start(0);assert(frame(DB1_PASSWORD_OTHER)==0);
 assert(s.status==DB1_PASSWORD_CANCELED);
 assert(frame(DB1_PASSWORD_A)==0&&db1_password_nav_owns(&s));
 assert(frame(0)==0&&!db1_password_nav_owns(&s));
 start(3);db1_password_nav_cancel(&s);
 assert(frame(DB1_PASSWORD_A)==0&&db1_password_nav_owns(&s));assert(frame(0)==0);
 /* Idle/dispatch/scene changes cannot trigger a new native game. */
 const unsigned pcs[]={0x1950,0x1cbea,0x1cc16,0x1cc5c,0x1cc7c,0x1cbc4};
 for(unsigned i=0;i<sizeof pcs/sizeof *pcs;i++){
  start(0);db1_password_nav_before(&s,pcs[i],0,DB1_PASSWORD_B);
  assert(s.status==DB1_PASSWORD_CANCELED&&frame(DB1_PASSWORD_A)==0);
 }
 start(3);db1_password_nav_before(&s,0x1d7b6,3,DB1_PASSWORD_B);
 assert(s.status==DB1_PASSWORD_CANCELED); /* no unsolicited handoff */
 assert(db1_password_nav_request(&s,1,0));
 for(unsigned i=0;i<301;i++)assert(frame(DB1_PASSWORD_A)==0);
 assert(s.status==DB1_PASSWORD_TIMEOUT&&db1_password_nav_owns(&s));
 assert(frame(DB1_PASSWORD_A)==0);assert(frame(0)==0&&!db1_password_nav_owns(&s));
 start(1);for(unsigned i=0;i<301&&s.status!=DB1_PASSWORD_TIMEOUT;i++)assert(frame(0)==(i<3?DB1_PASSWORD_DOWN:0));
 assert(s.status==DB1_PASSWORD_TIMEOUT);assert(frame(0)==0);
 int override=9;unsigned pad=9;db1_password_nav_frame(&s,DB1_PASSWORD_A,&override,&pad);assert(!override&&!pad);
 db1_password_nav_reset(NULL);db1_password_nav_before(NULL,0,0,0);db1_password_nav_frame(NULL,0,NULL,NULL);
 assert(!db1_password_nav_request(NULL,1,0)&&!db1_password_nav_owns(NULL));
 puts("Password navigation physical pulses/ack/release/remap/handoff/cancel/timeout passed");return 0;
}
