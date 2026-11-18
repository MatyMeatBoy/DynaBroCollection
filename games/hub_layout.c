#include "hub_layout.h"
#include <string.h>
static int min(int a,int b){return a<b?a:b;}
static int max(int a,int b){return a>b?a:b;}
static int px(int n,int u){return max(1,(n*u+500)/1000);}
void dyna_hub_window_bounds(DynaHubRect work,unsigned dpi,int fw,int fh,
                           int minimum,DynaHubRect *out){
 if(!out)return;
 int u=(int)(dpi?dpi:96)*1000/96;
 int margin=min(px(16,u),min(work.w,work.h)/20);
 int available_w=max(1,work.w-2*margin),available_h=max(1,work.h-2*margin);
 int w=min(available_w,px(minimum?700:1100,u)+max(0,fw));
 int h=min(available_h,px(minimum?430:620,u)+max(0,fh));
 *out=(DynaHubRect){work.x+(work.w-w)/2,work.y+(work.h-h)/2,w,h};
}
void dyna_hub_layout(int w,int h,unsigned dpi,DynaHubLayout *l){
 if(!l)return;
 memset(l,0,sizeof *l);
 if(w<=0||h<=0)return;
 int u=(int)(dpi?dpi:96)*1000/96;
 u=min(u,min(w*1000/760,h*1000/440));u=max(1,u);l->ui_scale_milli=u;
 int margin=px(24,u),gap=px(20,u),inner=max(2,w-2*margin);
 int banner_h=min(px(184,u),h*29/100);
 l->banner=(DynaHubRect){margin,px(10,u),inner,banner_h};
 l->version=(DynaHubRect){margin,l->banner.y+banner_h+px(4,u),inner,px(22,u)};
 l->status=(DynaHubRect){margin,h-px(30,u),inner,px(20,u)};
 l->note=(DynaHubRect){margin,h-px(76,u),inner,px(40,u)};
 int top=l->version.y+l->version.h+px(10,u),bottom=l->note.y-px(12,u);
 int action_h=px(38,u),action_gap=px(10,u);
 int card_h=max(1,bottom-top-action_h-action_gap),card_w=max(1,(inner-gap)/2);
 for(int i=0;i<2;i++){
  int x=margin+i*(card_w+gap);
  l->cards[i]=(DynaHubRect){x,top,card_w,card_h};
  l->settings[i]=(DynaHubRect){x,top+card_h+action_gap,card_w,action_h};
 }
}
