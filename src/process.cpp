#include <process.h>
#include <iostream>
#include <cstring>

Process::Process(int id){
    procId = id;
    pstats = new process_stats();
    //Resetting the entire page table.
    memset(page_table, 0, MAX_VPAGES*sizeof(pte_t));
    memset(pstats, 0, sizeof(process_stats));
}

void Process::print_pagetable(){
    cout<<"PT["<<procId<<"]: ";
    for(int i=0 ; i < MAX_VPAGES; i++){
        if(!page_table[i].present){
            if(page_table[i].paged_out) cout<<"# ";
            else cout<<"* ";
        }
        else
            cout<<i<<":"<<(page_table[i].referenced ? "R" : "-")<<(page_table[i].modified ? "M" : "-")<<(page_table[i].paged_out ? "S " : "- ");
    }
    cout<<endl;
}

void Process::print_process_summary(){
    printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
        procId,
        pstats->unmaps, pstats->maps, pstats->ins, pstats->outs,
        pstats->fins, pstats->fouts, pstats->zeros,
        pstats->segv, pstats->segprot);
}