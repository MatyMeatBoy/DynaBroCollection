#ifndef DB1_LEVEL_CATALOG_H
#define DB1_LEVEL_CATALOG_H
#include "progress_session.h"
/* Access convenience, not completed-stage or achievement evidence. */
typedef struct {
 unsigned mode,stage;
 const char *mode_name; /* static program-lifetime string */
 char label[32];
 int has_saved_code; /* exact target in the ROM-revalidated saved store */
} Db1LevelCatalogItem;
const char *db1_level_catalog_mode_name(unsigned mode);
/* Caller supplies the qualified effective-ROM session and identity-checked
 * loaded store. Current-but-unsaved codes grant no access. No IO/mutation. */
unsigned db1_level_catalog_count(const DynaProgressSession *session);
/* Flat mode/stage order; invalid targets are omitted. Failure leaves out intact. */
int db1_level_catalog_item(const DynaProgressSession *session,unsigned index,
                          Db1LevelCatalogItem *out);
/* Rechecks access and ROM bytes; returns only a code, never queues a replay.
 * source_pc=0 denotes this ephemeral generated request. Never persist it as
 * displayed/earned progress. Failure leaves the caller-owned output intact. */
int db1_level_catalog_request_code(const DynaProgressSession *session,
                                  unsigned mode,unsigned stage,DynaProgressCode *out);
#endif
