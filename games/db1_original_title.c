#include "db1_original_title.h"
#include "db1_title_art.h"
#include <stddef.h>
#include <limits.h>
#include <string.h>
int db1_original_title_active(const GVDP *vdp,int english,int enhanced){
    return vdp && english && !enhanced && db1_title_art_identified(vdp) &&
           db1_title_art_logo_ready();
}
int db1_original_title_render(const GVDP *vdp,uint32_t *pixels,int width,int height,
                              int pitch,int english,int enhanced){
    if(!pixels || width!=320 || height!=224 || pitch<320*4 || pitch>INT_MAX/224 || pitch%4 ||
       !db1_original_title_active(vdp,english,enhanced))return 0;
    uint32_t row[320];
    /* Original backing has no canvas shift or generated vertical bands. */
    db1_title_art_geometry(224,0,224);
    for(int line=24;line<144;line++){
        unsigned char *destination=(unsigned char *)pixels+(size_t)line*(size_t)pitch;
        memcpy(row,destination,sizeof row);
        db1_title_art_row(vdp,line,row,320,0,1);
        /* Background repairs outside the logo are intentionally local only. */
        memcpy(destination+48*4,row+48,224*sizeof *row);
    }
    return 1;
}
