#include "paging.h"
#define ASM 1 // enable asm


/*init_video_page
 *    DESCRIPTION: initialize the video page
 *    INPUTS: none
 *    OUTPUS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: modify the specified page status
 */
void init_video_page(void) {
    page_table[VIDEO_MEM].present = 1;
    page_table[VIDEO_MEM].read_write = 1;
    page_table[VIDEO_MEM].user_supervise = 0;
    page_table[VIDEO_MEM].write_through = 0;
    page_table[VIDEO_MEM].cache_disabled = 0;
    page_table[VIDEO_MEM].accessed = 0;
    page_table[VIDEO_MEM].dirty = 0;
    page_table[VIDEO_MEM].pat = 0;
    page_table[VIDEO_MEM].global_flag = 0;
    page_table[VIDEO_MEM].avail = 3;
    page_table[VIDEO_MEM].physical_page_addr = VIDEO_MEM;
}

/*init_frame_kb
 *    DESCRIPTION: initialize the 4KB page table
 *    INPUTS: directory_offset -- index of page directory
 *            table_offset     -- index of page table
 *    OUTPUS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: modify the specified page status
 */
void init_frame_kb(int directory_offset, int table_offset) {
    page_directories[directory_offset].page_direc_entries[table_offset].kb_page.present = 1;
    page_directories[directory_offset].page_direc_entries[table_offset].kb_page.read_write = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].kb_page.user_supervise = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].kb_page.write_through = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].kb_page.cache_disabled = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].kb_page.accessed = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].kb_page.reserved0 = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].kb_page.page_size = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].kb_page.global_flag = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].kb_page.avail = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].kb_page.PT_4kb_aligned_addr = (uint32_t)page_table >> 12; /* 20 bits for offset */
}


/*init_frame_mb
 *    DESCRIPTION: initialize the 4MB page table
 *    INPUTS: directory_offset -- index of page directory
 *            table_offset     -- index of page table
 *            aligned_addr     -- for P_4mb_aligned_addr
 *    OUTPUS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: modify the specified page status
 */
void init_frame_mb(int directory_offset, int table_offset, int aligned_addr) {

    page_directories[directory_offset].page_direc_entries[table_offset].mb_page.present = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].mb_page.read_write = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].mb_page.user_supervise = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].mb_page.write_through = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].mb_page.cache_disabled = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].mb_page.accessed = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].mb_page.dirty = 0;
    /* enable page_size to be 4mb */
    page_directories[directory_offset].page_direc_entries[table_offset].mb_page.page_size = 1;
    page_directories[directory_offset].page_direc_entries[table_offset].mb_page.global_flag = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].mb_page.avail = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].mb_page.pat = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].mb_page.reserved1 = 0;
    page_directories[directory_offset].page_direc_entries[table_offset].mb_page.P_4mb_aligned_addr = aligned_addr;
}

/*void init_paging(void):
 * DESCRIPTION:
 *      initialize the paging
 * INPUTS:
 *      NONE
 * OUTPUTS:
 *      NONE
 * SIDE_EFFECT:
 *      modify page status
 */
void init_paging() {
    //assume now only one task
    int i;
    /* initialize whole page_table */
    memset(page_table, 0, sizeof(page_table_entry_t) * NUM_ENTRIES);

    for(i = 0; i < NUM_ENTRIES; i++) {
        /* enable avail for system programmers' use */
        page_table[i].avail = 3;
        page_table[i].physical_page_addr = i;
    }

    init_video_page();

    /* set the first 4MB page */
    init_frame_kb(0, 0);

    /* set the global 4MB to 8MB page */
    init_frame_mb(0, 1, 1);

    /* mark the page present and allow read/write */
    page_directories[0].page_direc_entries[1].mb_page.present = 1;
    page_directories[0].page_direc_entries[1].mb_page.read_write = 1;

    /* raise global flag to avoid TLB flush */
    page_directories[0].page_direc_entries[1].mb_page.global_flag = 1;

    //set the rest of 4MB page not present
    for(i = 2; i < NUM_ENTRIES; i++){
        init_frame_mb(0, i, i);
    }

    /*we need to set the control register*/
    asm(
      /* move the address of page_directories  to cr3*/
	    "movl %0, %%cr3                  \n"
      /* enable 4MB paging */
	    "movl %%cr4, %%eax               \n"
	    "orl $0x00000010, %%eax          \n"
	    "movl %%eax, %%cr4               \n"
      /* enable paging mode */
	    "movl %%cr0, %%eax               \n"
	    "orl $0x80000001, %%eax 	     \n"
	    "movl %%eax, %%cr0                 "
	    : /* no output */ :"r"(page_directories) : "eax", "cc"
    );
}


/*load_page
 * DESCRIPTION:
 *      load page
 * INPUTS:
 *      addr - starting addr of the
 *  OUTPUTS:
 *      NONE
 * SIDE_EFFECT:
 *      allocate page
 */
void map_page(uint32_t physical_addr) {
  uint32_t start_addr;

  /* make sure it is aligned */
  start_addr = physical_addr >> 22;   //22 bits as align address

  init_frame_mb(0, VIRTUAL_ENTRY, start_addr);
  /* allow user to access, mark the page to be present and user grants read/write permission */
  page_directories[0].page_direc_entries[VIRTUAL_ENTRY].mb_page.user_supervise = 1;
  page_directories[0].page_direc_entries[VIRTUAL_ENTRY].mb_page.present = 1;
  page_directories[0].page_direc_entries[VIRTUAL_ENTRY].mb_page.read_write = 1;

  /* flush TLB for new process */
  flush();
}

/*map_video_page
 * DESCRIPTION:
 *      load page
 * INPUTS:
 *      addr - starting addr of the physical memory
 *  OUTPUTS:
 *      NONE
 * SIDE_EFFECT:
 *      allocate page
 */
void map_video_page(uint32_t physical_addr, uint32_t term_id) {
  uint32_t start_addr;

  /* make sure it is aligned, shift 12 bits (4KB) to align address */
  start_addr = physical_addr >> 12;

  /* disable page_size */
  page_directories[0].page_direc_entries[VIRTUAL_VIDEO_ENTRY].kb_page.page_size = 0;

  /* allow user to access, mark the page to be present and user grants read/write permission */
  page_directories[0].page_direc_entries[VIRTUAL_VIDEO_ENTRY].kb_page.present = 1;
  page_directories[0].page_direc_entries[VIRTUAL_VIDEO_ENTRY].kb_page.user_supervise = 1;
  page_directories[0].page_direc_entries[VIRTUAL_VIDEO_ENTRY].kb_page.read_write = 1;
  page_directories[0].page_direc_entries[VIRTUAL_VIDEO_ENTRY].kb_page.PT_4kb_aligned_addr = (uint32_t)video_page_table >> 12; /* 20 bits for offset */

  /* clear video_page_table bits */
  memset(video_page_table, 0, _4 * KB);

  if (term_id == cur_active_term) {
    video_page_table[0].present = 1;
    video_page_table[0].user_supervise = 1;
    video_page_table[0].read_write = 1;
    video_page_table[0].physical_page_addr = start_addr;
  } else {
    video_page_table[0].present = 1;
    video_page_table[0].user_supervise = 1;
    video_page_table[0].read_write = 1;
    video_page_table[0].physical_page_addr = VIDEO_MEM + term_id + 1;
  }

  flush();
}

void map_terminal_page(uint32_t physical_addr){
    uint32_t pte = physical_addr / (_4 * KB);
    page_table[pte].present = 1;
    page_table[pte].read_write = 1;
    page_table[pte].user_supervise = 0;
    page_table[pte].write_through = 0;
    page_table[pte].cache_disabled = 0;
    page_table[pte].accessed = 0;
    page_table[pte].pat = 0;
    page_table[pte].dirty = 0;
    page_table[pte].global_flag = 0;
    page_table[pte].avail = 0;
    page_table[pte].physical_page_addr = (uint32_t) physical_addr >> 12; /* 20 bits for offset */
    flush();
}
