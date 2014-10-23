#ifndef __EXPORTS_H__
#define __EXPORTS_H__

/* --- TEMPORARY ---
 * boxes relocation:
 *     - each module is relocated in 1 of the 8 subregions of the Flash/SRAM address spaces (MPU-managed)
 *     - uVisor will always be box #0
 *     - the uVisor always starts box #1 after reset
 *     - other libraries/modules get higher box #s
 */
#define    UVISOR_BOX    0
#define CLIENT_BOX    1

/* context switch */
#define CONTEXT_SWITCH_IN(f, box, a0)    (__SVC2(7, ((uint32_t) (box) << 24) | ((f) & 0xFFFFFF), a0))
#define CONTEXT_SWITCH_OUT(res)        (__SVC1(8, (uint32_t) (res)))

/* library initialization */
#define LIB_INIT(guid)            (__SVC1(9, (uint32_t) (guid)))

#define GUID_BYTE_SIZE    16
typedef uint8_t Guid[GUID_BYTE_SIZE];

/* library export table */
typedef struct export_table {
    uint16_t magic;
    uint16_t version;
    uint16_t length;
    uint16_t _reserved;
    Guid     guid;
    uint32_t fn_count;
    uint32_t *fn_table;
} ExportTable;

extern const ExportTable g_exports;

#endif/*__EXPORTS_H_*/
