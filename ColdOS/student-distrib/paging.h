#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"
#include "lib.h"
#include "systemcall.h"
#include "terminal.h"
/* function to flush TLB */
static inline void flush() {
  asm ("                          \n\
    movl %%cr3, %%eax             \n\
    movl %%eax, %%cr3             \n\
    "
    : /*no output*/ : /* no input */ : "memory", "cc", "%eax"
  );
}

/*
 *              Page_Directory_Entry (4-KByte Page)
 * 31-----------------12-11--- 9--8----7----6---5---4-----3------2-----1----0-+
 * |  Page Base Address | Avail | G | PAT | 0 | A | PCD | PWT | U/S | R/W | P |
 * +--------------------+-------+---+-----+---+---+-----+-----+------+----+---+
 */
//Page Directory Entry 4kb
typedef union Page_direc_entry_4kb_page{
     uint32_t entry_value;
     struct{
         uint32_t present: 1;
         uint32_t read_write: 1;
         uint32_t user_supervise: 1;
         uint32_t write_through: 1;
         uint32_t cache_disabled: 1;
         uint32_t accessed: 1;
         uint32_t reserved0: 1;
         uint32_t page_size: 1;
         uint32_t global_flag: 1;
         uint32_t avail: 3;
         uint32_t PT_4kb_aligned_addr: 20;
     }__attribute((packed));
}page_direc_entry_4kb_page_t;

/*
 *              Page_Directory_Entry (4-MByte Page)
 * 31-----------------22-21--------13-12------11----9-8----7-----6-----5-----4-----3------2-----1----0-+
 * |  Page Base Address |  Reserved  | pat |  avail  | G | PS |  D  |  A  | PCD | PWT |  U/S  | R/T| P |
 * +--------------------+------------+-----+---------+---+----+-----+-----+-----+-----+-------+----+---+
 */
//Page Directory Entry 4mb
typedef union Page_direc_entry_4mb_page{
     uint32_t entry_value;
     struct{
         uint32_t present: 1;
         uint32_t read_write: 1;
         uint32_t user_supervise: 1;
         uint32_t write_through: 1;
         uint32_t cache_disabled: 1;
         uint32_t accessed: 1;
         uint32_t dirty: 1;
         uint32_t page_size: 1;
         uint32_t global_flag: 1;
         uint32_t avail: 3;
         uint32_t pat: 1;
         uint32_t reserved1: 9;
         uint32_t P_4mb_aligned_addr: 10;
     }__attribute((packed));
}page_direc_entry_4mb_page_t;


/*
 *              Page_Table_Entry (4-KByte Page)
 * 31-----------------12-11--- 9--8----7----6---5---4-----3------2-----1----0-+
 * |  Page Base Address | Avail | G | PAT | D | A | PCD | PWT | U/S | R/W | P |
 * +--------------------+-------+---+-----+---+---+-----+-----+------+----+---+
 */
//Page Table Entry
typedef union Page_table_entry {
    uint32_t entry_value;
    struct{
         uint32_t present: 1;
         uint32_t read_write: 1;
         uint32_t user_supervise: 1;
         uint32_t write_through: 1;
         uint32_t cache_disabled: 1;
         uint32_t accessed: 1;
         uint32_t dirty: 1;
         uint32_t pat: 1;
         uint32_t global_flag: 1;
         uint32_t avail: 3;
         uint32_t physical_page_addr: 20;
     }__attribute((packed));
}page_table_entry_t;

//page_entry_contain 4kb and 4mb page
typedef union page_entry{
    page_direc_entry_4kb_page_t kb_page;
    page_direc_entry_4mb_page_t mb_page;
}page_entry_t;

//page directory that contained 1024 entries
typedef struct Page_direc_entries{
    page_entry_t page_direc_entries[ NUM_ENTRIES ];
}page_direc_entries_t;

page_direc_entries_t page_directories[ MAX_TASK ] __attribute__((aligned( NUM_SIZE )));
//only use for initializing the 0 to 4MB
page_table_entry_t page_table[ NUM_ENTRIES ] __attribute__((aligned( NUM_SIZE )));
//extern global array, user_page_table for checkpoint4
page_table_entry_t video_page_table[ NUM_ENTRIES ] __attribute__((aligned( NUM_SIZE )));

//functions
void init_paging();

void init_video_page(void);

void init_frame_kb(int directory_offset, int table_offset);

void init_frame_mb(int directory_offset, int table_offset, int aligned_addr);

void map_page(uint32_t addr);

void map_video_page(uint32_t addr, uint32_t term_id);

void map_terminal_page(uint32_t addr);

void unmap_page(uint32_t virtual_addr, uint32_t physical_addr);

#endif
