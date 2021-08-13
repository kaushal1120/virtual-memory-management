#ifndef PTE_T_H
#define PTE_T_H

//a page table entry
struct pte_t  {
     unsigned present:1;
     unsigned write_protected:1;
     unsigned modified:1;
     unsigned referenced:1;
     unsigned paged_out:1;
     unsigned frame:7;
     unsigned accessible:2;
     unsigned file_mapped:1;
     unsigned page_no:6;
     //11/20 bits left to be used
};

#endif