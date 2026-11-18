#include "viewport.h"
#include <stdint.h>
#include <string.h>
static int dimension(const char **cursor,unsigned *value) {
    const char *p=*cursor;unsigned n=0;
    if(*p<'0'||*p>'9')return 0;
    do {
        n=n*10+(unsigned)(*p-'0');
        if(n>10000)return 0;
        ++p;
    }while(*p>='0'&&*p<='9');
    if(!n)return 0;
    *cursor=p;*value=n;return 1;
}
int dyna_viewport_configure(DynaViewport *v,const char *mode) {
    unsigned w,h;
    if(!v||!mode)return 0;
    if(!strcmp(mode,"off")){v->enabled=0;return 1;}
    if(!strcmp(mode,"fit")||!strcmp(mode,"stage")) {
        *v=(DynaViewport){1,398,!strcmp(mode,"fit")};return 1;
    }
    const char *p=mode;
    if(!dimension(&p,&w)||*p++!=':'||!dimension(&p,&h)||*p)return 0;
    int width=(int)((224u*w+h/2)/h);
    if(width<320)width=320;
    if(width>768)width=768;
    *v=(DynaViewport){1,width,0};return 1;
}
int dyna_viewport_width(const DynaViewport *v,int dw,int dh,int nh) {
    if(!v)return 0;
    if(v->adaptive&&dw>0&&dh>0&&nh>0) {
        int64_t width=((int64_t)nh*dw+dh/2)/dh;
        return width<320?320:width>768?768:(int)width;
    }
    return v->canvas;
}
