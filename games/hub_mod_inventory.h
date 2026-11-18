#ifndef DYNA_HUB_MOD_INVENTORY_H
#define DYNA_HUB_MOD_INVENTORY_H
#include "launcher_discovery.h"
#include "recomp_launcher.h"
#ifdef __cplusplus
extern "C" {
#endif
/* UI-thread singleton decorator. Copies the bounded inventory; source and its
 * ctx must outlive launcher use. Call only while launcher is closed. NULL source
 * provides a diagnostics-only provider; NULL inventory clears detected entries.
 * Returned pointer is invalidated/replaced by the next call. */
const RecompLauncherCModProvider *dyna_hub_mod_inventory(
    const RecompLauncherCModProvider *source, const ld_inventory *inventory);
#ifdef __cplusplus
}
#endif
#endif
