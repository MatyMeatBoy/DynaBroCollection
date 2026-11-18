#include "hub_mod_inventory.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

static RecompLauncherCModProvider provider, base;
static RecompLauncherCModDiagnostic detected[LD_MAX_ITEMS];
static int detected_count;

/* Bounded UTF-16 (Windows) / UTF-32 conversion, truncating only between code
 * points. Malformed input becomes U+FFFD rather than invalid UTF-8. */
static void utf8(const wchar_t *s, size_t limit, char *out, size_t cap) {
    size_t i=0, n=0;
    while (i<limit && s[i]) {
        unsigned long cp=(unsigned long)s[i++];
#if WCHAR_MAX <= 0xffff
        if(cp>=0xd800 && cp<=0xdbff && i<limit && s[i]>=0xdc00 && s[i]<=0xdfff)
            cp=0x10000+((cp-0xd800)<<10)+(unsigned long)s[i++]-0xdc00;
#endif
        if(cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff))cp=0xfffd;
        size_t width=cp<0x80?1:cp<0x800?2:cp<0x10000?3:4;
        if(n+width>=cap)break;
        if(width==1)out[n++]=(char)cp;
        else {
            out[n++]=(char)((width==2?0xc0:width==3?0xe0:0xf0)|(cp>>(6*(width-1))));
            for(size_t j=width-1;j;j--)out[n++]=(char)(0x80|((cp>>(6*(j-1)))&63));
        }
    }
    out[n]=0;
}
static int base_count(void *ctx) {
    int n=base.catalog_diagnostic_count?base.catalog_diagnostic_count(ctx):0;
    return n<0?0:n>INT_MAX-LD_MAX_ITEMS?INT_MAX-LD_MAX_ITEMS:n;
}
static int diagnostic_count(void *ctx) { return base_count(ctx)+detected_count; }
static int diagnostic_get(void *ctx,int index,RecompLauncherCModDiagnostic *out) {
    int n;
    if(!out || index<0)return 0;
    n=base_count(ctx);
    if(index<n)return base.catalog_diagnostic_get?base.catalog_diagnostic_get(ctx,index,out):0;
    index-=n;
    if(index>=detected_count)return 0;
    *out=detected[index];return 1;
}
const RecompLauncherCModProvider *dyna_hub_mod_inventory(
    const RecompLauncherCModProvider *source,const ld_inventory *inventory) {
    /* Copy before replacing provider: repeated calls may pass its old address. */
    if(source!=&provider) {
        if(source)base=*source;else memset(&base,0,sizeof base);
    }
    provider=base;
    detected_count=0;
    if(inventory)for(size_t i=0;i<inventory->count && i<LD_MAX_ITEMS;i++) {
        const ld_item *item=&inventory->items[i];
        size_t start=0;
        char name[256];
        RecompLauncherCModDiagnostic *d;
        if(item->kind!=LD_BPS && item->kind!=LD_IPS)continue;
        for(size_t j=0;j<LD_PATH_CAP && item->path[j];j++)
            if(item->path[j]=='/' || item->path[j]=='\\')start=j+1;
        d=&detected[detected_count++];memset(d,0,sizeof *d);
        utf8(item->path,LD_PATH_CAP,d->resource,sizeof d->resource);
        utf8(item->path+start,LD_PATH_CAP-start,name,sizeof name);
        d->severity=RECOMP_MOD_DIAGNOSTIC_INFO;
        snprintf(d->message,sizeof d->message,
            "%s: external %s file found. Use the built-in English translation under Language. Extra ROM patches require a supported recompiled build; this file has not been applied.",
            name,item->kind==LD_BPS?"BPS":"IPS");
    }
    provider.catalog_diagnostic_count=diagnostic_count;
    provider.catalog_diagnostic_get=diagnostic_get;
    return &provider;
}
