#include "viewport.h"
#include <assert.h>
#include <string.h>
int main(void) {
    DynaViewport v=DYNA_VIEWPORT_DEFAULT,before;
    const char *bad[]={"","-16:9","16:0","0:9","16:9x","16:9:1","10001:1",
        "999999999999999999999999999999:9","16:99999999999999999999999"," 16:9","16 :9",NULL};
    assert(v.enabled && dyna_viewport_width(&v,1920,1080,224)==398);
    for(unsigned i=0;i<sizeof bad/sizeof *bad;i++) {
        before=v;assert(!dyna_viewport_configure(&v,bad[i]));assert(!memcmp(&v,&before,sizeof v));
    }
    assert(dyna_viewport_configure(&v,"off")&&!v.enabled);
    assert(dyna_viewport_configure(&v,"fit")&&v.enabled);
    assert(dyna_viewport_width(&v,1920,1080,224)==398);
    assert(dyna_viewport_width(&v,2560,1440,224)==398);
    assert(dyna_viewport_width(&v,3840,2160,224)==398);
    assert(dyna_viewport_width(&v,800,600,224)==320);
    assert(dyna_viewport_width(&v,16000,1000,224)==768);
    assert(dyna_viewport_width(&v,0,0,224)==398);
    assert(dyna_viewport_configure(&v,"00016:00009"));
    assert(dyna_viewport_width(&v,800,600,224)==398);
    assert(dyna_viewport_configure(&v,"21:9"));
    assert(dyna_viewport_width(&v,1920,1080,224)==523);
    assert(dyna_viewport_configure(&v,"1:10000")&&v.canvas==320);
    assert(dyna_viewport_configure(&v,"10000:1")&&v.canvas==768);
    assert(dyna_viewport_configure(&v,"stage")&&v.canvas==398&&!v.adaptive);
    return 0;
}
