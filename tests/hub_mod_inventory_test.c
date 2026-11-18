#include "../games/hub_mod_inventory.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int token, calls;
static int count(void *ctx){assert(ctx==&token);return 1;}
static int get(void *ctx,int i,RecompLauncherCModDiagnostic *out){
    assert(ctx==&token);assert(i==0);memset(out,0,sizeof *out);
    strcpy(out->message,"base diagnostic");calls++;return 1;
}
static int enable(void *ctx,const char *p,const char *f,int value){
    assert(ctx==&token);calls++;
    return p&&f&&!strcmp(p,"real")&&!strcmp(f,"real")&&value;
}
int main(void){
    ld_inventory *inventory=calloc(1,sizeof *inventory);
    RecompLauncherCModProvider source={0};
    RecompLauncherCModDiagnostic d;
    const RecompLauncherCModProvider *p;
    assert(inventory);source.ctx=&token;source.catalog_diagnostic_count=count;
    source.catalog_diagnostic_get=get;source.feature_enable=enable;
    inventory->count=3;inventory->items[0].kind=LD_ROM;
    inventory->items[1].kind=LD_BPS;
    wcscpy(inventory->items[1].path,L"C:\\mods\\traducci\x00f3n-\xd83e\xdd96.bps");
    inventory->items[2].kind=LD_IPS;wcscpy(inventory->items[2].path,L"mods/test.ips");
    p=dyna_hub_mod_inventory(&source,inventory);
    assert(p->ctx==source.ctx && p->feature_enable==enable);
    assert(!p->feature_get && !p->package_get);
    assert(p->catalog_diagnostic_count(p->ctx)==3);
    assert(!p->catalog_diagnostic_get(p->ctx,-1,&d));
    assert(!p->catalog_diagnostic_get(p->ctx,3,&d));
    assert(!p->catalog_diagnostic_get(p->ctx,1,NULL));
    assert(p->catalog_diagnostic_get(p->ctx,0,&d));assert(calls==1);
    assert(!strcmp(d.message,"base diagnostic"));
    assert(p->catalog_diagnostic_get(p->ctx,1,&d));
    assert(strstr(d.message,"BPS file found"));
    assert(strstr(d.message,"has not been applied"));
    assert(strstr(d.resource,"traducci\xc3\xb3n-"));
#if WCHAR_MAX <= 0xffff
    assert(strstr(d.resource,"\xf0\x9f\xa6\x96"));
#endif
    assert(!d.related_package_id[0] && !d.related_feature_id[0]);
    assert(p->feature_enable(p->ctx,"real","real",1));
    assert(!p->feature_enable(p->ctx,"detected-patches","test.ips",1));
    inventory->items[1].path[0]=0;
    assert(p->catalog_diagnostic_get(p->ctx,1,&d));assert(d.resource[0]);
    inventory->count=LD_MAX_ITEMS+100;
    for(size_t i=0;i<LD_MAX_ITEMS;i++)inventory->items[i].kind=LD_IPS;
    p=dyna_hub_mod_inventory(p,inventory);
    assert(p->catalog_diagnostic_count(p->ctx)==LD_MAX_ITEMS+1);
    p=dyna_hub_mod_inventory(NULL,NULL);
    assert(!p->ctx && p->catalog_diagnostic_count(NULL)==0);
    assert(!p->catalog_diagnostic_get(NULL,0,&d));
    free(inventory);puts("hub_mod_inventory_test: passed");return 0;
}
