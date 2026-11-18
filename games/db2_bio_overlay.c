#include "db2_bio_overlay.h"
#include "genesis_runtime.h"
#include "video/genesis_dac.h"
#include <string.h>

/* Original resources (see evidence/db2-bio-resource/HOST-OVERLAY-CONTRACT.md):
 * menu 8x8 font = dictionary stream at ROM 010730 (5440 bytes), ASCII->tile map
 * at ROM 00FCB6, record table addresses, ink tables 05D780/05D78C. */
#define FONT_STREAM 0x10730u
#define FONT_MAP 0xfcb6u
#define FONT_BYTES 5440u
#define FONT_FNV 0x01c6cecd40ee4b49ull
#define STAT_INK 0x5d780u
#define NAME_INK 0x5d78cu

static const uint32_t record_address[6]={0x2185ac,0x2181e4,0x21878e,0x217e12,0x2183c6,0x217ffa};
static const struct{const char *name,*role,*line[3];}copy[6]={
 {"PTERA","FLYING CARNIVORE",{"PATROLS FOR FOES.","LOW HP. STRIKES,","THEN FLIES AWAY."}},
 {"ALLO","AMBUSH CARNIVORE",{"WAITS. QUICKLY","CATCHES FOES. MAY","BUNGLE."}},
 {"P-CHAN","EGG EATER",{"EGG FAN. SHOWOFF.","WEAK ARMS. QUICK","TO FLEE."}},
 {"STEGO","BREEDING HERBIVORE",{"GENTLE, CAREFREE.","GRAZES AT EASE.","LAYS EGGS."}},
 {"TYRANNO","ATTACK CARNIVORE",{"FIERCE. TOP POWER.","DIM-WITTED. KIND","AT HEART."}},
 {"TRICERA","DEFENSE HERBIVORE",{"PEACEFUL. HATES","LOSING. THRIVES IN","HARSH CONDITIONS."}}};
static const char *const stat_label[6]={"HP","ATTACK","DEFEND","SPEED","EGGRTE","P GAIN"};

static uint8_t glyph[128][8];
static uint8_t glyph_present[128];
static int font_ready;
static int pending=-1,active=-1;
static uint8_t ink_name[6]={6,5,7,9,4,8},ink_stat[6]={2,3,2,3,2,3};

static unsigned be16(const uint8_t *p){return ((unsigned)p[0]<<8)|p[1];}

/* Port of tools/db2_codec.py decode_dictionary (ROM $1080-$11B0 variant). */
static int decode_dictionary(const uint8_t *rom,size_t size,size_t address,uint8_t *out,size_t cap)
{
 if(address+5>size)return 0;
 size_t length=be16(rom+address),blocks=rom[address+3],dsize=rom[address+4]?rom[address+4]:256;
 address+=5;
 if(!length||!blocks||length>cap||address+dsize>size)return 0;
 const uint8_t *dict=rom+address;address+=dsize;
 static const unsigned gw[7]={0,3,4,5,6,7,3},gb[7]={0,1,9,25,64,128,57};
 size_t n=0;
 for(size_t b=0;b<blocks;b++){
  if(address+2>size)return 0;
  size_t blen=be16(rom+address),end=n+blen;address+=2;
  if(!blen||end>length)return 0;
  unsigned left=0,cur=0;
#define BITS(count,dst) do{unsigned v_=0;for(unsigned i_=0;i_<(count);i_++){ \
   if(!left){if(address>=size)return 0;cur=rom[address++];left=8;} \
   left--;v_=(v_<<1)|((cur>>left)&1);}(dst)=v_;}while(0)
  while(n<end){
   unsigned token;BITS(8,token);
   unsigned count=(token&128)?(token&63)+1:token+1;
   if(n+count>end)return 0;
   if(token&128){
    unsigned dist;BITS(8,dist);
    if(token&64){unsigned lo;BITS(2,lo);dist=(dist<<2)|lo;}
    dist++;if(dist>n)return 0;
    for(unsigned i=0;i<count;i++){out[n]=out[n-dist];n++;}
   }else for(unsigned i=0;i<count;i++){
    unsigned group=0,bit;
    for(;;){BITS(1,bit);if(!bit)break;if(++group>=7)return 0;}
    unsigned idx;BITS(gw[group],idx);idx+=gb[group];
    if(idx>=dsize)return 0;
    out[n++]=dict[idx];
   }
  }
#undef BITS
 }
 return n==length;
}

int db2_bio_overlay_load_font(const uint8_t *rom,size_t size)
{
 static uint8_t raw[FONT_BYTES];
 font_ready=0;memset(glyph,0,sizeof glyph);memset(glyph_present,0,sizeof glyph_present);
 if(!rom||size<0x280000u||!decode_dictionary(rom,size,FONT_STREAM,raw,sizeof raw))return 0;
 uint64_t h=0xcbf29ce484222325ull;
 for(size_t i=0;i<FONT_BYTES;i++){h^=raw[i];h*=0x100000001b3ull;}
 if(h!=FONT_FNV)return 0;
 for(unsigned ch=33;ch<127;ch++){
  unsigned tile=be16(rom+FONT_MAP+2*ch)&0x7ff,v=tile*32,off;
  if(v>=0x40&&v<0x700)off=v-0x40;
  else if(v>=0x700&&v<0xea0)off=0x6c0+v-0x700;
  else if(v>=0xf000&&v<0xf6e0)off=0xe60+v-0xf000;
  else continue;
  if(off+32>FONT_BYTES)continue;
  for(unsigned y=0;y<8;y++){uint8_t m=0;for(unsigned x=0;x<8;x++)
    if((raw[off+y*4+x/2]>>((x&1)?0:4))&15)m|=(uint8_t)(0x80>>x);
   glyph[ch][y]=m;}
  glyph_present[ch]=1;
 }
 font_ready=1;return 1;
}

void db2_bio_overlay_set_glyph(unsigned ch,const uint8_t rows[8])
{
 if(ch<128){memcpy(glyph[ch],rows,8);glyph_present[ch]=1;font_ready=1;}
}

void db2_bio_overlay_reset(void){pending=active=-1;}
void db2_bio_overlay_force(int record){active=record;}
int db2_bio_overlay_active(void){return active>=0;}

void db2_bio_overlay_observe(unsigned pc)
{
 switch(pc){
 case 0x5d3d4:pending=active=-1;break;
 case 0x5d57a:{
  pending=-1;
  uint32_t a0=g_cpu.A[0]&0xffffffu;
  for(int i=0;i<6;i++)if(record_address[i]==a0)pending=i;
  break;}
 case 0x5d674:
  if(pending>=0){
   if(!font_ready)db2_bio_overlay_load_font(g_rom,0x280000u);
   if(font_ready){
    for(int i=0;i<6;i++){ink_name[i]=g_rom[NAME_INK+2*i]&15;ink_stat[i]=g_rom[STAT_INK+2*i]&15;}
    active=pending;
   }
  }
  break;
 case 0x5d722:active=-1;break;
 default:break;
 }
}

static uint32_t cram_color(const GVDP *v,unsigned index){return genesis_dac_cram_to_argb(v->cram[index&63],0);}

static void put_text(uint32_t *out,int width,int origin_x,int x,int y,int line,const char *s,uint32_t ink,int cols)
{
 int row=line-y;
 if(row<0||row>=8)return;
 for(int c=0;c<cols&&s[c];c++){
  unsigned ch=(unsigned char)s[c];
  if(ch>='a'&&ch<='z')ch-=32;
  if(ch>=128||!glyph_present[ch])continue;
  uint8_t m=glyph[ch][row];
  for(int px=0;px<8;px++)if(m&(0x80>>px)){
   int ox=origin_x+x+c*8+px;
   if(ox>=0&&ox<width)out[ox]=ink;
  }
 }
}
static void fill(uint32_t *out,int width,int origin_x,int x0,int x1,uint32_t color)
{
 for(int x=x0;x<x1;x++){int ox=origin_x+x;if(ox>=0&&ox<width)out[ox]=color;}
}

void db2_bio_overlay_row(const GVDP *v,int line,uint32_t *out,int width,int origin_x)
{
 if(active<0||!v||!out||line<0||line>=224)return;
 int r=active;
 uint32_t bg=cram_color(v,0);
 /* Name/role share the record ink; paragraph uses bank0 index 1; stats alternate. */
 uint32_t nink=cram_color(v,ink_name[r]),pink=cram_color(v,1);
 if(line>=120&&line<136){fill(out,width,origin_x,48,128,bg);put_text(out,width,origin_x,48,124,line,copy[r].name,nink,10);}
 if(line>=16&&line<32){fill(out,width,origin_x,176,320,bg);put_text(out,width,origin_x,176,20,line,copy[r].role,nink,18);}
 for(int p=0;p<3;p++){int y=144+24*p;
  if(line>=y&&line<y+16){fill(out,width,origin_x,16,160,bg);put_text(out,width,origin_x,16,y+4,line,copy[r].line[p],pink,18);}
 }
 for(int k=0;k<6;k++){int y=40+16*k;
  if(line>=y&&line<y+16){fill(out,width,origin_x,168,216,bg);put_text(out,width,origin_x,168,y+4,line,stat_label[k],cram_color(v,ink_stat[k]),6);}
 }
}

void db2_bio_overlay_scanline(const GVDP *v,int line,uint32_t *out,int width)
{
 if(width>=320)db2_bio_overlay_row(v,line,out,width,(width-320)/2);
 else db2_bio_overlay_row(v,line,out,width,0);
}
