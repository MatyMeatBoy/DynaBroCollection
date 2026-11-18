#include "db2_bio_overlay.h"
#include "genesis_runtime.h"
#include "video/genesis_dac.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
uint8_t g_rom[0x400000],g_ram[65536];
M68KState g_cpu;
static GVDP v;
static uint32_t out[480];
static void row(int line,int width,int origin){
 for(int i=0;i<width;i++)out[i]=0x11223344u;
 db2_bio_overlay_row(&v,line,out,width,origin);
}
int main(void){
 uint8_t full[8];memset(full,0xff,sizeof full);
 for(unsigned c=33;c<127;c++)db2_bio_overlay_set_glyph(c,full);
 v.cram[0]=0x000;v.cram[1]=0xeee;v.cram[2]=0x00e;v.cram[3]=0x0e0;
 for(unsigned i=4;i<10;i++)v.cram[i]=(uint16_t)(0x020*i);
 uint32_t bg=genesis_dac_cram_to_argb(v.cram[0],0),white=genesis_dac_cram_to_argb(v.cram[1],0);
 /* Inactive: strict no-op. */
 db2_bio_overlay_reset();row(124,320,0);assert(out[48]==0x11223344u&&!db2_bio_overlay_active());
 /* Lifecycle: only a validated record address plus the enable PC activates. */
 db2_bio_overlay_observe(0x5d3d4);g_cpu.A[0]=0x123456;db2_bio_overlay_observe(0x5d57a);
 db2_bio_overlay_observe(0x5d674);assert(!db2_bio_overlay_active());
 /* Force record 5 (TRICERA). Name field: row 120 is background, glyph rows from y=124. */
 db2_bio_overlay_force(5);assert(db2_bio_overlay_active());
 row(120,320,0);assert(out[48]==bg&&out[127]==bg&&out[47]==0x11223344u&&out[128]==0x11223344u);
 row(124,320,0);assert(out[48]==genesis_dac_cram_to_argb(v.cram[8],0)); /* name ink index 8 for record 5 */
 row(124,320,0);assert(out[48+7*8+7]!=0x11223344u); /* 7 columns: TRICERA, remaining 3 cells blank-filled */
 row(135,320,0);assert(out[48]==bg);
 row(136,320,0);assert(out[48]==0x11223344u);
 /* Role field x176..319. */
 row(20,320,0);assert(out[176]!=0x11223344u&&out[319]!=0x11223344u&&out[175]==0x11223344u);
 /* Paragraph lines and stat labels use their exact rectangles. */
 row(148,320,0);assert(out[16]==white&&out[159]!=0x11223344u&&out[160]==0x11223344u);
 row(172,320,0);assert(out[16]==white);row(196,320,0);assert(out[16]==white);
 row(164,320,0);assert(out[16]==0x11223344u); /* 8px gap between lines is untouched */
 row(44,320,0);assert(out[168]==genesis_dac_cram_to_argb(v.cram[2],0)&&out[216]==0x11223344u);
 row(60,320,0);assert(out[168]==genesis_dac_cram_to_argb(v.cram[3],0));
 /* Widescreen origin shifts every column; out-of-range columns are clipped. */
 row(124,427,53);assert(out[53+48]==genesis_dac_cram_to_argb(v.cram[8],0)&&out[52+48]==0x11223344u);
 row(20,300,0);assert(out[299]!=0x11223344u);
 /* Live palette (fade) is sampled every row. */
 v.cram[8]=0x0ee;row(124,320,0);assert(out[48]==genesis_dac_cram_to_argb(v.cram[8],0));
 /* Disable PC clears; a null raster is a no-op. */
 db2_bio_overlay_observe(0x5d722);assert(!db2_bio_overlay_active());
 db2_bio_overlay_row(NULL,124,out,320,0);
 puts("DB2 biography overlay contract passed");return 0;
}
