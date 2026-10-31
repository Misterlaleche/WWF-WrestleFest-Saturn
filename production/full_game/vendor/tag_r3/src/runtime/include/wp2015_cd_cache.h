#ifndef WP2015_CD_CACHE_H
#define WP2015_CD_CACHE_H
#include <stddef.h>
#include <stdint.h>
#define WP2015_CD_CACHE_SECTOR_BYTES 2352u
#define WP2015_CD_CACHE_MAX_SECTORS 192u
#define WP2015_CD_CACHE_ARGUMENT (-801)
#define WP2015_CD_CACHE_NOT_READY (-802)
#define WP2015_CD_CACHE_STATE (-803)
/* Reset invalidates the old lease before clearing partition23. The existing
 * file reader owns partition0;8of200 standard buffers stay available to it.
 * Transport failures invalidate this lease and require a fresh reset.
 * Argument/state errors leave it intact. All CD operations have one owner;
 * neither an interrupt nor the other SH-2 may access the CD concurrently.
 */
int wp2015_cd_cache_reset(void);
int wp2015_cd_cache_append(const void *sector2352);
int wp2015_cd_cache_seal(uint32_t logical_bytes);
uint32_t wp2015_cd_cache_bytes(void);
/* CPU-owned scratch must be4-byte aligned, disjoint from dst, and at least
 *2352bytes.4704bytes allows two neighboring sectors in a single Get.
 * The read preserves retained sectors and attempts to restore the2048-byte
 * file mode on every transport exit. Failure includes unsuccessful cleanup.
 * On error dst may contain a completed prefix; discard the entire result.
 */
int wp2015_cd_cache_read(uint32_t offset,void *dst,size_t bytes,
                       void *scratch,size_t scratch_bytes);
#endif
