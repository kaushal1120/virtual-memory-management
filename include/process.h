using namespace std;
#include <vector>

#include <vma.h>
#include <pte_t.h>

#define MAX_VPAGES 64

struct process_stats{
    unsigned long segv;
    unsigned long segprot;
    unsigned long unmaps;
    unsigned long maps;
    unsigned long fouts;
    unsigned long fins;
    unsigned long outs;
    unsigned long ins;
    unsigned long zeros;
};

class Process{
public:
    int procId;
    vector<vma*> vma_specs;
    pte_t page_table[MAX_VPAGES];
    process_stats* pstats;
    Process(int id);
    void print_pagetable();
    void print_process_summary();
};