#include "db1_visor.h"
static int floor8(int n){int q=n/8;return n<0&&n%8?q-1:q;}
static int ceil8(int n){int q=n/8;return n>0&&n%8?q+1:q;}
int db1_visor_layout(int w,int requested,Db1VisorLayout *out){
 if(!out||w<320||w>768)return 0;
 int hud=w-104,base=(hud-208+1)/2;if(base<8)base=8;
 int minimum=ceil8(8-base),maximum=floor8(hud-208-base);
 int applied=requested;if(applied<minimum)applied=minimum;if(applied>maximum)applied=maximum;
 int x=base+8*applied;
 *out=(Db1VisorLayout){{x,8,208,208},{x+8,16,192,192},hud,w-320,x-8,applied,minimum,maximum};
 return 1;
}
int db1_visor_canvas_x(const Db1VisorLayout *v,int x){return x+(v?v->world_shift_x:0);}
int db1_visor_native_x(const Db1VisorLayout *v,int x){return x-(v?v->world_shift_x:0);}
