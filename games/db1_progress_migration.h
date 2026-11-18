#ifndef DB1_PROGRESS_MIGRATION_H
#define DB1_PROGRESS_MIGRATION_H
#include "progress_session.h"
#define DB1_PROGRESS_MIGRATION_MAX_HASHES 8u
typedef enum { DB1_MIGRATION_OK,DB1_MIGRATION_INVALID,DB1_MIGRATION_CANONICAL_CORRUPT,
 DB1_MIGRATION_CAPACITY,DB1_MIGRATION_WRITE_FAILED } Db1ProgressMigrationStatus;
typedef struct {
 Db1ProgressMigrationStatus status;
 unsigned inspected,imported,invalid_codes,corrupt_legacy,written;
} Db1ProgressMigrationReport;
/* Qualified DB1 ROM/hash allowlist supplied by caller; no hash discovery.
 * destination must be initialized for game1/canonical_sha. The UTF8 folder may
 * end in either separator. Never modifies legacy files. On failure destination
 * and canonical file are unchanged; report is an independent output.
 * Existing canonical codes must all match current ROM, otherwise fail closed.
 * A write is atomic via progress_store; RAM commits only after successful write
 * or a validated disk no-op (including loading fresh RAM). No achievement or replay state is accessed. */
int db1_progress_migration_run(DynaProgressStore *destination,const char *folder,
 const char *canonical_sha,const char *const *qualified_hashes,size_t hash_count,
 const uint8_t *effective_rom,size_t rom_size,Db1ProgressMigrationReport *report);
#endif
