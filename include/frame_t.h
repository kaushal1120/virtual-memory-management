#include <pte_t.h>

//a frame table entry
struct frame_t{
    pte_t* vpage;
    int procId;
    int frame_no;
    //for the aging algorithm
    unsigned long age_vector;
    //for the working set algorithm
    unsigned long time_last_use;
};