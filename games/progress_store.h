#ifndef DYNA_PROGRESS_STORE_H
#define DYNA_PROGRESS_STORE_H
#include <stddef.h>
#include <stdint.h>
#define DYNA_PROGRESS_LIMIT 256
typedef struct {
    uint16_t mode,stage;
    uint8_t length,code[8];
    uint32_t source_pc;
} DynaProgressCode;
typedef struct {
    unsigned game,count;
    char rom_sha256[65];
    DynaProgressCode codes[DYNA_PROGRESS_LIMIT];
} DynaProgressStore;
/* Identity is an exact SHA256 of the qualified effective ROM image. */
int dyna_progress_init(DynaProgressStore *,unsigned game,const char *sha256);
/* Only call after the original game's code/identity contract is validated.
 * Duplicate identity+code is idempotent. No guest memory is written. */
int dyna_progress_add(DynaProgressStore *,const DynaProgressCode *);
/* On failure, destination is unchanged. A mismatched game/ROM is rejected. */
int dyna_progress_load(DynaProgressStore *,const char *path);
/* UTF-8 path. Absence returns0; access/encoding errors return1 to fail closed. */
int dyna_progress_file_exists(const char *path);
/* Same-directory exclusive temporary, flush, then atomic replacement.
 * A failed write/replacement never pre-deletes the prior valid file. */
int dyna_progress_save(const DynaProgressStore *,const char *path);
#endif
