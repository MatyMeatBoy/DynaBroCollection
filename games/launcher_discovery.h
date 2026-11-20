#ifndef LAUNCHER_DISCOVERY_H
#define LAUNCHER_DISCOVERY_H
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>
#define LD_PATH_CAP 2048
#define LD_MAX_ITEMS 128
#define LD_MAX_ROOTS 32
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { LD_UNKNOWN, LD_ROM, LD_BPS, LD_IPS } ld_kind;
typedef enum {
 LD_ROM_OK=0, LD_ROM_MISSING=3, LD_ROM_UNREADABLE=4,
 LD_ROM_INCOMPATIBLE=5, LD_ROM_WRONG_GAME=6
} ld_rom_result;
typedef struct { uint64_t size; uint32_t crc32; int game; } ld_revision;
typedef struct { wchar_t path[LD_PATH_CAP]; ld_kind kind; int game; uint64_t size; uint32_t crc32; unsigned priority; int compatible; } ld_item;
typedef struct { ld_item items[LD_MAX_ITEMS]; size_t count; unsigned examined; int truncated; } ld_inventory;
typedef struct {
 const wchar_t *base_dir; /* NULL: executable directory */
 const wchar_t *cached_rom[2]; /* validated before automatic choices */
 const wchar_t *config_file; /* NULL: base_dir/paths.ini */
 const ld_revision *revisions; size_t revision_count; /* NULL: production registry */
} ld_options;
const ld_item *ld_requested_rom(const ld_inventory *, int game, const wchar_t *path);
ld_rom_result ld_probe_rom(const wchar_t *path, int game, const ld_options *, ld_item *out);
const char *ld_rom_error(ld_rom_result result, int game);
uint32_t ld_crc32(uint32_t previous, const void *bytes, size_t length);
ld_kind ld_classify(const wchar_t *path);
int ld_identify(uint64_t size, uint32_t crc32, const ld_revision *registry, size_t count);
int ld_scan(const ld_options *options, ld_inventory *out);
const ld_item *ld_best_rom(const ld_inventory *inventory, int game);
int ld_utf8_to_wide(const char *src, wchar_t *dst, size_t capacity);
int ld_wide_to_utf8(const wchar_t *src, char *dst, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
