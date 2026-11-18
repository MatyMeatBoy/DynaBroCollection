#include "db1_password_nav.h"
#include <string.h>
int db1_password_nav_owns(const Db1PasswordNav *s){return s&&(s->drain||(s->status>=DB1_PASSWORD_RELEASE&&s->status<=DB1_PASSWORD_WAIT));}
void db1_password_nav_reset(Db1PasswordNav *s){if(s)memset(s,0,sizeof *s);}
int db1_password_nav_request(Db1PasswordNav *s,int mature,unsigned native){
 if(!s)return 0;
 db1_password_nav_reset(s);
 if(!mature||native>3){s->status=DB1_PASSWORD_UNAVAILABLE;return 0;}
 s->native_selection=s->anchor=native;s->status=DB1_PASSWORD_RELEASE;return 1;
}
static void stop(Db1PasswordNav *s,Db1PasswordNavStatus status){s->status=status;s->drain=1;}
void db1_password_nav_cancel(Db1PasswordNav *s){if(db1_password_nav_owns(s))stop(s,DB1_PASSWORD_CANCELED);}
static int confirm_valid(unsigned b){return b==DB1_PASSWORD_A||b==DB1_PASSWORD_B||b==DB1_PASSWORD_C;}
void db1_password_nav_before(Db1PasswordNav *s,uint32_t pc,unsigned native,unsigned physical){
 if(!db1_password_nav_owns(s)||s->drain)return;
 s->confirm=confirm_valid(physical)?physical:0;
 if(pc==0x1d7b6){
  s->status=(s->status==DB1_PASSWORD_CONFIRM||s->status==DB1_PASSWORD_WAIT)&&s->pulse?
   DB1_PASSWORD_HANDED_OFF:DB1_PASSWORD_CANCELED;
  if(s->status==DB1_PASSWORD_CANCELED)s->drain=1;
  return;
 }
 switch(pc){
 case 0x1cbc4:case 0x1cc92:case 0x1cc96:
  if((s->status!=DB1_PASSWORD_CONFIRM&&s->status!=DB1_PASSWORD_WAIT)||!s->pulse||native!=3)
   stop(s,DB1_PASSWORD_CANCELED);
  return;
 case 0x1950:case 0x1a14:case 0x1c5a4:case 0x1c9ce:case 0x1caf6:
 case 0x1cb0c:case 0x1cbea:case 0x1cc16:case 0x1cc5c:case 0x1cc7c:
 case 0x1cc80:case 0x1e024:stop(s,DB1_PASSWORD_CANCELED);return;
 case 0x1cb30:case 0x1cb34:case 0x1cb46:case 0x1cb4a:case 0x1cb4e:case 0x1cb54:break;
 default:return;
 }
 if(native>3){stop(s,DB1_PASSWORD_CANCELED);return;}
 if(s->status==DB1_PASSWORD_MOVE){
  if(native==s->target&&s->pulse)s->ack=1;
  else if(s->ack||native!=s->anchor){stop(s,DB1_PASSWORD_CANCELED);return;}
 }else if(native!=s->native_selection){stop(s,DB1_PASSWORD_CANCELED);return;}
 s->native_selection=native;
}
static void prepare(Db1PasswordNav *s){
 s->pulse=s->ack=0;s->anchor=s->native_selection;
 if(s->native_selection==3){s->status=DB1_PASSWORD_CONFIRM;s->button=0;}
 else {
  s->status=DB1_PASSWORD_MOVE;
  s->button=s->native_selection==0?DB1_PASSWORD_UP:DB1_PASSWORD_DOWN;
  s->target=s->native_selection==0?3:s->native_selection+1;
 }
}
void db1_password_nav_frame(Db1PasswordNav *s,unsigned input,int *override,unsigned *pad){
 if(override)*override=0;
 if(pad)*pad=0;
 if(!db1_password_nav_owns(s))return;
 if(override)*override=1;
 if(s->drain){if(!input)s->drain=0;return;}
 if(++s->elapsed>300){stop(s,DB1_PASSWORD_TIMEOUT);return;}
 if(s->status==DB1_PASSWORD_RELEASE){if(!input)prepare(s);return;}
 if(input){stop(s,DB1_PASSWORD_CANCELED);return;}
 if(s->status==DB1_PASSWORD_MOVE){
  if(pad&&s->pulse<3)*pad=s->button;
  if(s->pulse<6)++s->pulse;
  else if(s->ack)prepare(s);
  return;
 }
 if(s->status==DB1_PASSWORD_CONFIRM){
  if(!s->button){if(!s->confirm)return;s->button=s->confirm;}
  if(s->pulse<3&&s->confirm!=s->button){stop(s,DB1_PASSWORD_CANCELED);return;}
  if(pad&&s->pulse<3)*pad=s->button;
  if(++s->pulse>=6)s->status=DB1_PASSWORD_WAIT;
 }
}
Db1PasswordNavStatus db1_password_nav_status(const Db1PasswordNav *s){return s?s->status:DB1_PASSWORD_IDLE;}
const char *db1_password_nav_status_text(const Db1PasswordNav *s){
 switch(db1_password_nav_status(s)){
 case DB1_PASSWORD_UNAVAILABLE:return "Return to the title menu to load this level";
 case DB1_PASSWORD_HANDED_OFF:return "Loading selected level";
 case DB1_PASSWORD_CANCELED:return "Level loading canceled";
 case DB1_PASSWORD_TIMEOUT:return "Level loading timed out; return to the title menu";
 case DB1_PASSWORD_IDLE:return "No Password navigation requested";
 default:return "Loading selected level";
 }
}
