/* Bounded descriptor lookup only. Every hit still uses the ordinary live
 * opcode/extension guards, bus helpers, exceptions and cycle accounting. */
#include "step_lookup.h"
#include <string.h>
typedef struct {const M68kStepModule *module;const M68kStepEntry *entry;uint32_t pc;} Slot;
static Slot slots[1024]; /* One native execution thread; immutable modules. */
_Static_assert(sizeof slots<=32768,"Lookup cache must remain bounded");
void dyna_step_cache_clear(void){memset(slots,0,sizeof slots);}
M68kStepResult dyna_step_cached(M68kStepContext *context,const M68kStepModule *module) {
    if(!module||!module->entries)return M68K_STEP_MISS;
    uint32_t pc=context->state.regs.PC;
    Slot *slot=&slots[((pc>>1)^((uintptr_t)module>>4))&1023u];
    if(slot->module==module&&slot->pc==pc)
        return slot->entry?m68k_step_execute(context,slot->entry):M68K_STEP_MISS;
    size_t low=0,high=module->count;
    while(low<high) {
        size_t mid=low+(high-low)/2;
        const M68kStepEntry *entry=&module->entries[mid];
        if(entry->pc<pc)low=mid+1;
        else if(entry->pc>pc)high=mid;
        else {
            slot->module=module;slot->entry=entry;slot->pc=pc;
            return m68k_step_execute(context,entry);
        }
    }
    /* Cache an absent descriptor, never a live opcode/extension guard miss. */
    *slot=(Slot){module,NULL,pc};
    return M68K_STEP_MISS;
}
