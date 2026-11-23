/* Bounded descriptor lookup only. Every hit still uses the ordinary live
 * opcode/extension guards, bus helpers, exceptions and cycle accounting. */
#include "step_lookup.h"
#include <string.h>
#if defined(DYNA_DB1_REGISTER_EXPERIMENT)
#include "db1step.h"
#include "db1jp.h"
#include "db1_register_bitmap.h"
#endif
typedef struct {
    const M68kStepModule *module;
    const M68kStepEntry *entry;
    uint32_t pc;
#if defined(DYNA_DB1_REGISTER_EXPERIMENT)
    int register_only;
#endif
} Slot;
static Slot slots[1024]; /* One native execution thread; immutable modules. */
_Static_assert(sizeof slots<=32768,"Lookup cache must remain bounded");
void dyna_step_cache_clear(void){memset(slots,0,sizeof slots);}

static M68kStepResult execute_slot(M68kStepContext *context, const Slot *slot)
{
    if (!slot->entry) return M68K_STEP_MISS;
#if defined(DYNA_DB1_REGISTER_EXPERIMENT)
    if (slot->register_only) {
        M68kStepResult result = m68k_step_execute_register(context, slot->entry);
        if (result != M68K_STEP_MISS) return result;
    }
#endif
    return m68k_step_execute(context, slot->entry);
}

#if defined(DYNA_DB1_REGISTER_EXPERIMENT)
static int register_only(const M68kStepModule *module, size_t index)
{
    const uint8_t *bits;
    if (module == &db1step_module && module->count == db1step_pure_count)
        bits = db1step_pure_bits;
    else if (module == &db1jp_module && module->count == db1jp_pure_count)
        bits = db1jp_pure_bits;
    else
        return 0;
    return (bits[index / 8] >> (index % 8)) & 1;
}
#endif

M68kStepResult dyna_step_cached(M68kStepContext *context,const M68kStepModule *module) {
    if(!module||!module->entries)return M68K_STEP_MISS;
    uint32_t pc=context->state.regs.PC;
    Slot *slot=&slots[((pc>>1)^((uintptr_t)module>>4))&1023u];
    if(slot->module==module&&slot->pc==pc)
        return execute_slot(context,slot);
    size_t low=0,high=module->count;
    while(low<high) {
        size_t mid=low+(high-low)/2;
        const M68kStepEntry *entry=&module->entries[mid];
        if(entry->pc<pc)low=mid+1;
        else if(entry->pc>pc)high=mid;
        else {
            slot->module=module;slot->entry=entry;slot->pc=pc;
#if defined(DYNA_DB1_REGISTER_EXPERIMENT)
            slot->register_only=register_only(module,mid);
#endif
            return execute_slot(context,slot);
        }
    }
    /* Cache an absent descriptor, never a live opcode/extension guard miss. */
    *slot=(Slot){.module=module,.pc=pc};
    return M68K_STEP_MISS;
}
