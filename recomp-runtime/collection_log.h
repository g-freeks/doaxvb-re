#ifndef DOAXBV_RECOMP_COLLECTION_LOG_H
#define DOAXBV_RECOMP_COLLECTION_LOG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Logs changes to the girls' item ownership and room inventories. Read-only. */
void recomp_collection_log_frame(void);

/* Reports the shop screen context 0x000C8840 is drawing this frame. */
void recomp_collection_note_shop(uint32_t shop);
/* Reports that an item grid with a preview (the items screen, a gift's
   choice of present) is drawing this frame; 0x000C9FF0 queues its
   preview. */
void recomp_collection_note_grid(void);

#ifdef __cplusplus
}
#endif

#endif
