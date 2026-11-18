#include "db2_actors.h"
#include <string.h>
static unsigned w(const uint8_t*b){return ((unsigned)b[0]<<8)|b[1];}
static size_t l(const uint8_t*b){return ((size_t)w(b)<<16)|w(b+2);}
static int s(unsigned v){return v<128?(int)v:(int)v-256;}
static int sw(const uint8_t*b){unsigned v=w(b);return v<32768?(int)v:(int)v-65536;}
static int rr(const Db2ActorsContext*c,size_t p,size_t n){return c&&c->rom&&p<=c->rom_size&&n<=c->rom_size-p;}
static int jump(size_t*p,int d){if(d<0){if(*p<(size_t)-d)return 0;*p-=(size_t)-d;}else{if(*p>SIZE_MAX-(unsigned)d)return 0;*p+=(unsigned)d;}return 1;}
static int unpack(const Db2ActorsContext*c,size_t p,uint8_t out[512]){
 if(!rr(c,p,3)||w(c->rom+p)!=512)return 0;
 p+=3;unsigned used=0;
 while(used<512){if(!rr(c,p,1))return 0;unsigned op=c->rom[p++],n=(op&127)+1;if(n>512-used)return 0;
 if(op<128){if(!rr(c,p,n))return 0;memcpy(out+used,c->rom+p,n);used+=n;p+=n;}
 else{if(!rr(c,p,1))return 0;unsigned d=c->rom[p++]+1;if(d>used)return 0;while(n--){out[used]=out[used-d];used++;}}}return 1;}
static int pattern(const Db2ActorsContext*c,const uint8_t*a,unsigned frame,Db2ActorComponent*out){
 size_t p;unsigned type=a[0]-1;if(frame&256){if(!rr(c,0x78800,4))return 0;p=l(c->rom+0x78800);}
 else{if(!rr(c,0x15516+type,1)||!rr(c,0x78810,4))return 0;p=l(c->rom+0x78810)+4*c->rom[0x15516+type];if(!rr(c,p,4))return 0;p=l(c->rom+p);}
 p+=4*(frame&255);if(!rr(c,p,4)||!unpack(c,l(c->rom+p),out->pattern))return 0;
 /* Native 14B64..14D30 overlays the already guest-generated visual masks. */
 if(a[0x11]&0xc0){const uint8_t*m=c->ram;unsigned phase=m[0x5466],base=(a[0x11]&0x40)?0x5166:0x5066;
 if(phase>1)return 0;
 for(unsigned col=0;col<4;col++){
  unsigned start=col*128+((a[0x11]&0x40)?112:96);
  if(a[0x11]&0x40)memset(out->pattern+start,0,16);
  else {static const uint8_t mask[8]={0,15,0,15,15,0,15,0};for(unsigned j=0;j<32;j++)out->pattern[start+j]&=mask[j&7];}
  for(unsigned j=0;j<32;j++){unsigned dst=start-32+j,src=base+phase*128+col*32+j;out->pattern[dst]=(out->pattern[dst]&m[src+512])|m[src];}
 }}
 unsigned palette=0,state=a[7];
 if(state!=1){if(state<=3)palette=0x2000;
 else if(a[0]<=4){if(state==8)palette=0x2000;else goto mapped;}
 else if(a[0]==0x1d&&(state&254)==10)palette=0x6000;
 else if(a[0]==0x1e&&(state&254)==10)palette=0;
 else if(!(c->ram[0x2a89]&0x20)&&(a[0x11]&0x20)&&c->ram[0x5023]<3)palette=(c->ram[0x5023]+1)*0x2000;
 else {mapped: if(!rr(c,0x15572+type,1))return 0;palette=(c->rom[0x15572+type]&3)*0x2000;}}
 out->frame=(uint16_t)frame;out->flags=(uint16_t)((frame&0xf800)|palette);return 1;}
static int project(const Db2ActorsContext*c,const uint8_t*a,int dx,int dy,Db2Actor*result){
   size_t camera_address=l(c->ram+0x2a56);if(camera_address<0xff0000||camera_address>0xfffffe)return 0;
   unsigned camera=(unsigned)camera_address&65535;
   result->depth=32*a[3]+s(a[5]);
   /* ADD.B combines offsets before EXT.W in native 14E84..14EDA. */
   result->x=32*s((a[2]-c->ram[camera])&255)+s((a[4]+dx)&255)+sw(c->ram+0x2c00)+sw(c->ram+0x2c04)+sw(c->ram+0x2bfc)+sw(c->ram+0x2a60);
   result->y=32*s((a[3]-c->ram[camera+1])&255)+s((a[5]+a[6]+dy)&255)+sw(c->ram+0x2c02)+sw(c->ram+0x2c06)+sw(c->ram+0x2bfe)+4;
return 1;
}
int db2_actor_decode(const Db2ActorsContext*c,unsigned id,Db2Actor*out){
 if(!c||!c->ram||c->ram_size<65536||!out||id<1||id>254)return 0;
 const uint8_t*a=c->ram+0x5f58+48*id;unsigned type=a[0];if(!type||type>92)return 0;type--;
 if(!rr(c,0x154ba+type,1)||!rr(c,0x78814,4)||!rr(c,0x78804,4))return 0;
 size_t p;unsigned state=a[7];if(state<=3)p=l(c->rom+0x78804);else{p=l(c->rom+0x78814)+4*c->rom[0x154ba+type];if(!rr(c,p,4))return 0;p=l(c->rom+p);state-=4;}
 size_t idx=8*state+(a[1]&254);if(!rr(c,p+idx,2))return 0;p+=w(c->rom+p+idx);
 Db2Actor result;memset(&result,0,sizeof result);result.id=id;unsigned elapsed=a[8],loop=0;int dx=0,dy=0;
 for(unsigned steps=0;steps<4096;steps++){
  if(!rr(c,p,1))return 0;
  unsigned op=c->rom[p++];
  if(op<0xf0||op==0xfb){unsigned frame[2],count=1,selector=0,duration;
   if(op==0xfb){if(!rr(c,p,6))return 0;selector=c->rom[p++];frame[0]=w(c->rom+p);p+=2;frame[1]=w(c->rom+p);p+=2;duration=c->rom[p++];if(selector<1||selector>4)return 0;count=2;}
   else{if(!rr(c,p,2))return 0;frame[0]=(op<<8)|c->rom[p++];duration=c->rom[p++];}
   duration=(int8_t)duration>=1?duration:1;if(elapsed>=duration){elapsed-=duration;continue;}
   result.component_count=count;for(unsigned i=0;i<count;i++){uint8_t visual[48];memcpy(visual,a,48);if(i)visual[0x11]&=63;if(!pattern(c,visual,frame[i],result.part+i))return 0;}
   if(count==2){static const int ox[4]={0,32,0,-32},oy[4]={-32,0,32,0};result.part[1].dx=ox[selector-1];result.part[1].dy=oy[selector-1];}
   if(!project(c,a,dx,dy,&result))return 0;
   *out=result;return 1;
  }
  if(op==255){if(!rr(c,p,2))return 0;int delta=s(c->rom[p++]);unsigned n=(c->rom[p++]-1)&255;if(loop){n=(loop-1)&255;if(!n){loop=0;continue;}}loop=n;if(!jump(&p,delta))return 0;}
  else if(op==254){if(!rr(c,p,1))return 0;int d=s(c->rom[p++]);if(!jump(&p,d))return 0;}
  else if(op==253){if(!rr(c,p,1))return 0;p++;}
  else if(op==252){p=(p+1)&~(size_t)1;if(!rr(c,p,4))return 0;p=l(c->rom+p);}
  else if(op==250||op==249){if(!rr(c,p,1))return 0;if(op==250)dx=s(c->rom[p++]);else dy=s(c->rom[p++]);}
  else if(op==248)continue;else return 0;
 }return 0;
}
/* Cache membership and all identity fields must agree: actor+12 alone may be stale. */
int db2_actor_decode_cached(const Db2ActorsContext*c,unsigned id,Db2Actor*out){
 if(!c||!c->ram||c->ram_size<65536||!out||id<1||id>254)return 0;
 const uint8_t*a=c->ram+0x5f58+48*id;if(!a[0]||a[0]>92)return 0;
 unsigned off=w(c->ram+0x20b2);uint8_t seen[40]={0};
 for(unsigned n=0;n<40;n++){
  if(off==65535)break;
  if(off%32||off/32>=40||seen[off/32])return 0;
  seen[off/32]=1;const uint8_t*k=c->ram+0x1bb2+off;
  if(w(k+10)==id*256&&k[0x14]==a[0]&&k[0x18]==a[7]&&k[0x19]==a[1]&&l(k+0x1a)==0xff5f58+48*id){
   if(w(k+2)==65535||k[0x17]>4)return 0;
   Db2Actor result;memset(&result,0,sizeof result);result.id=id;result.component_count=k[0x17]?2:1;
   if(!pattern(c,a,w(k+2),result.part))return 0;
   if(result.component_count==2){uint8_t second[48];memcpy(second,a,48);second[0x11]&=63;
    if(!pattern(c,second,w(k+6),result.part+1))return 0;
    static const int ox[4]={0,32,0,-32},oy[4]={-32,0,32,0};result.part[1].dx=ox[k[0x17]-1];result.part[1].dy=oy[k[0x17]-1];}
   if(!project(c,a,s(k[0x1e]),s(k[0x1f]),&result))return 0;
   *out=result;return 1;
  }
  off=w(k);
 }
 return 0;
}
/* Native list supplies authoritative equality order for all listed actors, not just cache40. */
static void depth_ranks(const Db2ActorsContext*c,unsigned rank[255]){
 for(unsigned id=0;id<255;id++)rank[id]=72+id;
 if(!c||!c->ram||c->ram_size<65536)return;
 unsigned off=w(c->ram+0x9d3a);uint8_t seen[72]={0};
 for(unsigned n=0;n<72&&off;n++){
  if(off<4||(off-4)%8||(off-4)/8>=72||seen[(off-4)/8])break;
  seen[(off-4)/8]=1;const uint8_t*k=c->ram+0x9d3a+off;
  if(k[2]>0&&k[2]<255)rank[k[2]]=n;
  off=w(k);
 }
}
unsigned db2_actors_visible(const Db2ActorsContext*c,int width,int height,Db2Actor out[254]){
 if(!out||width<=0||height<=0)return 0;
 unsigned count=0,rank[255];Db2Actor a;depth_ranks(c,rank);
 for(unsigned id=1;id<=254;id++)if(db2_actor_decode_cached(c,id,&a)||db2_actor_decode(c,id,&a)){int visible=0;for(unsigned i=0;i<a.component_count;i++){int x=a.x+a.part[i].dx,y=a.y+a.part[i].dy;if(x+32>0&&x<width&&y+32>0&&y<height)visible=1;}if(visible)out[count++]=a;}
 for(unsigned i=0;i<count;i++)for(unsigned j=i+1;j<count;j++)if(out[i].depth<out[j].depth||(out[i].depth==out[j].depth&&rank[out[i].id]>rank[out[j].id])){a=out[i];out[i]=out[j];out[j]=a;}
 return count;
}
int db2_actor_pixel(const Db2ActorComponent*f,unsigned x,unsigned y,uint8_t*p){if(!f||!p||x>=32||y>=32)return 0;if(f->flags&0x800)x=31-x;if(f->flags&0x1000)y=31-y;unsigned off=((x/8)*4+y/8)*32+(y%8)*4+(x%8)/2;unsigned color=(f->pattern[off]>>((x&1)?0:4))&15;*p=(uint8_t)(color+((f->flags>>13)&3)*16);return 1;}
int db2_actor_native_sat_body(const Db2ActorsContext*c,unsigned attr){
 if(!c||!c->ram||c->ram_size<65536)return 0;
 unsigned off=w(c->ram+0x20b2);uint8_t seen[40]={0};
 for(unsigned n=0;n<40;n++){if(off==65535)break;if(off%32||off/32>=40||seen[off/32])return 0;seen[off/32]=1;const uint8_t*a=c->ram+0x1bb2+off;
 if(a[10]!=255&&!(a[4]&128)){if(((w(a+4)+0x37e)&0x7ff)==(attr&0x7ff))return 1;if(a[0x17]&&((w(a+8)+0x37e)&0x7ff)==(attr&0x7ff))return 1;}off=w(a);}
 return 0;
}
