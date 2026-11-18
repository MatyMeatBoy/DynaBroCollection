#include "db2_visor.h"
#include <limits.h>
#include <stdint.h>
static int floor8(int n){int q=n/8;return n<0&&n%8?q-1:q;}
static int ceil8(int n){int q=n/8;return n>0&&n%8?q+1:q;}
int db2_visor_layout(int width,int requested,Db2VisorLayout *out){
 if(!out||width<320||width>768)return 0;
 int hud=width-104,base=(hud-192+1)/2;
 int minimum=ceil8(8-base),maximum=floor8(hud-192-8-base);
 int applied=requested;
 if(applied<minimum)applied=minimum;
 if(applied>maximum)applied=maximum;
 int x=base+8*applied;
 *out=(Db2VisorLayout){{x,16,192,192},hud,width-320,x-16,applied,minimum,maximum};
 return 1;
}
static int bounded(int64_t x){return x<INT_MIN?INT_MIN:x>INT_MAX?INT_MAX:(int)x;}
int db2_visor_canvas_x(const Db2VisorLayout *v,int x){return bounded((int64_t)x+(v?v->world_shift_x:0));}
int db2_visor_native_x(const Db2VisorLayout *v,int x){return bounded((int64_t)x-(v?v->world_shift_x:0));}
