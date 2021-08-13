#include <iostream>
using namespace std;
#include <unistd.h>
#include <fstream>
#include <vector>
#include <sstream>
#include<queue>
#include <cstring>

#include <process.h>
#include <pager.h>

#define MAX_VPAGES 64
#define MAX_FRAMES 128
#define MAX_PROCESSES 10

queue<frame_t*> framefreelist;
frame_t frame_table[MAX_FRAMES];
vector<Process*> processes;

class MMU{
private:
    Pager* THE_PAGER;
    int num_frames;
    bool opFlag;
public:
    MMU(int frames, char pager, char* randfile, bool OFlag, bool aFlag){
        num_frames = frames;
        opFlag = OFlag;

        //Queueing all frames in the free list
        for(int i=0; i < num_frames; i++){
            memset(&frame_table[i],0,sizeof(frame_t));
            frame_table[i].frame_no = i;
            framefreelist.push(&frame_table[i]);
        }

        switch (pager){
            case 'f' :
                THE_PAGER = new FIFO_Pager(num_frames, aFlag);
                break;
            case 'r' :
                THE_PAGER = new Random_Pager(num_frames, randfile, aFlag);
                break;
            case 'c' :
                THE_PAGER = new Clock_Pager(num_frames, aFlag);
                break;
            case 'e' :
                THE_PAGER = new NRU_Pager(num_frames, aFlag);
                break;
            case 'a' :
                THE_PAGER = new Aging_Pager(num_frames, aFlag);
                break;
            case 'w' :
                THE_PAGER = new WorkingSet_Pager(num_frames, aFlag);
                break;
            default :
                THE_PAGER = new FIFO_Pager(num_frames, aFlag);
                break;
        }
    }

    frame_t* get_frame() {
        frame_t *frame = allocate_frame_from_free_list();
        if (frame == nullptr){
            frame = THE_PAGER->select_victim_frame();
            if(opFlag) cout<<" UNMAP "<<frame->procId<<":"<<frame->vpage->page_no<<endl;
            processes[frame->procId]->pstats->unmaps++;
            frame->vpage->present = 0;
            if(frame->vpage->modified){
                if(frame->vpage->file_mapped){
                    if(opFlag) cout<<" FOUT"<<endl;
                    processes[frame->procId]->pstats->fouts++;
                }
                else{
                    if(opFlag) cout<<" OUT"<<endl;
                    processes[frame->procId]->pstats->outs++;
                    frame->vpage->paged_out = 1;
                }
            }
        }
        frame->age_vector = 0;
        frame->time_last_use = inst_count-1;
        frame->vpage = nullptr;
        frame->procId = -1;
        return frame;
    }

    frame_t* allocate_frame_from_free_list(){
        frame_t* frame = nullptr;
        if(framefreelist.size() > 0){
            frame = framefreelist.front();
            framefreelist.pop();
        }
        return frame;
    }

    bool pgfault_handler(Process* process, pte_t* pte, int vpage){
        if(pte->accessible == 2)
            return false;
        else if(pte->accessible == 3)
            return true;
        else{
            for(vma* v : process->vma_specs){
                if(vpage >= v->start_vpage && vpage <= v->end_vpage){
                    pte->accessible = 3;
                    pte->file_mapped = v->file_mapped;
                    pte->write_protected = v->write_protected;
                    pte->page_no = vpage;
                    return true;
                }
            }
        }
        pte->accessible = 2;
        return false;
    }

    void update_pte(pte_t* pte, int rw){
        if(rw == 1)
            pte->modified = 1;
        else
            pte->referenced = 1;
    }

    void print_frametable(){
        cout<<"FT: ";
        for(int i=0; i < num_frames; i++){
            if(frame_table[i].vpage){
                if((frame_table[i].vpage)->present)
                    cout<<frame_table[i].procId<<":"<<(frame_table[i].vpage)->page_no<<" ";
            }
            else
                cout<<"* ";
        }
        cout<<endl;
    }
};

//Simulator begins here.

ifstream f_stream;
unsigned long process_exits=0;
unsigned long ctx_switches=0;
unsigned long inst_count=0;
unsigned long long cost=0;

bool get_next_instruction(char* operation, int* vpage){
    string line;
    while(getline(f_stream, line) && line[0] == '#'){}
    if(f_stream.eof())
        return false;
    stringstream ssin(line);
    ssin >> *operation >> *vpage;
    return true;
}

void simulate(MMU* mmu, bool OFlag, bool xFlag, bool yFlag, bool fFlag){
    char instruction;
    int vpage;
    Process* current_process;
    while (get_next_instruction(&instruction, &vpage)) {
        // handle special case of “c” and “e” instruction
        // now the real instructions for read and write
        if (OFlag) cout<<inst_count<<": ==> "<<instruction<<" "<<vpage<<endl;
        inst_count++;
        if(instruction == 'c'){ 
            ctx_switches++;
            current_process = processes[vpage];
        }
        else if(instruction == 'e'){
            process_exits++;
            cout<<"EXIT current process "<<current_process->procId<<endl;
            for(int i=0; i < MAX_VPAGES; i++){
                if(current_process->page_table[i].present){
                    if(OFlag) cout<<" UNMAP "<<current_process->procId<<":"<<current_process->page_table[i].page_no<<endl;
                    current_process->pstats->unmaps++;
                    if(current_process->page_table[i].modified && current_process->page_table[i].file_mapped){
                        if(OFlag) cout<<" FOUT"<<endl;
                        current_process->pstats->fouts++;
                    }
                    frame_table[current_process->page_table[i].frame].procId = -1;
                    frame_table[current_process->page_table[i].frame].vpage = nullptr;
                    frame_table[current_process->page_table[i].frame].age_vector = 0;
                    frame_table[current_process->page_table[i].frame].time_last_use = 0;
                    framefreelist.push(&frame_table[current_process->page_table[i].frame]);
                }
                current_process->page_table[i].present = 0;
                current_process->page_table[i].modified = 0;
                current_process->page_table[i].referenced = 0;
                current_process->page_table[i].paged_out = 0;
                current_process->page_table[i].frame = 0;
            }            
            current_process = nullptr;
        }
        else{
            pte_t *pte = &current_process->page_table[vpage];
            if (!pte->present) {
                // this in reality generates the page fault exception and now you execute
                // verify this is actually a valid page in a vma if not raise error and next inst
                if(!mmu->pgfault_handler(current_process, pte, vpage)){
                    if(OFlag) cout<<" SEGV"<<endl;
                    current_process->pstats->segv++;
                    continue;
                }

                frame_t* newframe = mmu->get_frame();
                //-> figure out if/what to do with old frame if it was mapped
                // see general outline in MM-slides under Lab3 header and writeup below
                // see whether and how to bring in the content of the access page.
                newframe->vpage = pte;
                newframe->procId = current_process->procId;
                if(pte->paged_out){
                    if(OFlag) cout<<" IN"<<endl;
                    current_process->pstats->ins++;
                }
                else if(pte->file_mapped){
                    if(OFlag) cout<<" FIN"<<endl;
                    current_process->pstats->fins++;
                }
                else{
                    if(OFlag) cout<<" ZERO"<<endl;
                    current_process->pstats->zeros++;
                }
                if(OFlag) cout<<" MAP "<<newframe->frame_no<<endl;
                current_process->pstats->maps++;
                pte->modified = 0;
                pte->referenced = 0;
                pte->frame = newframe->frame_no;
                pte->present = 1;
            }
            // check write protection
            // simulate instruction execution by hardware by updating the R/M PTE bits bits based on operations.
            mmu->update_pte(pte, 0); 
            if(instruction == 'w'){
                if(pte->write_protected){
                    current_process->pstats->segprot++;
                    if(OFlag) cout<<" SEGPROT"<<endl;
                }
                else
                    mmu->update_pte(pte, 1);
            }

            if(yFlag){
                for(int i=0; i < processes.size(); i++){
                    processes[i]->print_pagetable();
                }
            }
            else if(xFlag)
                current_process->print_pagetable();

            if(fFlag) mmu->print_frametable();
        }
    }
}

void print_summary(MMU* mmu, bool PFlag, bool FFlag, bool SFlag){
    cost += ctx_switches*130;
    cost += process_exits*1250;
    cost += inst_count-ctx_switches-process_exits;
    for(int i=0; i < processes.size(); i++){
        if(PFlag) processes[i]->print_pagetable();
        cost += processes[i]->pstats->maps*300;
        cost += processes[i]->pstats->unmaps*400;
        cost += processes[i]->pstats->ins*3100;
        cost += processes[i]->pstats->outs*2700;
        cost += processes[i]->pstats->fins*2800;
        cost += processes[i]->pstats->fouts*2400;
        cost += processes[i]->pstats->zeros*140;
        cost += processes[i]->pstats->segv*340;
        cost += processes[i]->pstats->segprot*420;
    }
    if(FFlag) mmu->print_frametable();
    if(SFlag){
        for(int i=0; i < processes.size(); i++){
            processes[i]->print_process_summary();
        }
        printf("TOTALCOST %lu %lu %lu %llu %lu\n", inst_count, ctx_switches, process_exits, cost, sizeof(pte_t));
    }
}

int main(int argc, char* argv[]){
    bool OFlag = false;
    bool PFlag = false;
    bool FFlag = false;
    bool SFlag = false;
    bool fFlag = false;
    bool aFlag = false;
    bool xFlag = false;
    bool yFlag = false;
    char algo = 'r';
    int num_frames=4;
    char c;

    opterr = 0;

    while ((c = getopt(argc, argv, "f:a:o:")) != -1){
        switch (c) {
            case 'f':
                num_frames = atoi(optarg);
                if(num_frames == 0){
                    cout << "Really funny .. you need at least one frame" << endl;
                    return 1;
                }
                else if(num_frames < 0 || num_frames > MAX_FRAMES){
                    cerr << "sorry max frames supported = " << MAX_FRAMES << endl;
                    return 1;
                }
                break;
            case 'a':
                if(!optarg || (*optarg != 'f' && *optarg != 'r' && *optarg != 'c' && *optarg != 'e' && *optarg != 'a' && *optarg != 'w')){
                    cerr<<"Unknown Replacement Algorithm: <"<<*optarg<<">"<<endl;
                    return 1;
                }
                algo = *optarg;
                break;
            case 'o':
                for(int i=0 ; i < strlen(optarg); i++){
                    switch(optarg[i]){
                        case 'O':
                            OFlag = true;
                            break;
                        case 'P':
                            PFlag = true;
                            break;
                        case 'F':
                            FFlag = true;
                            break;
                        case 'S':
                            SFlag = true;
                            break;
                        case 'x':
                            xFlag = true;
                            break;
                        case 'y':
                            yFlag = true;
                            break;
                        case 'f':
                            fFlag = true;
                            break;
                        case 'a':
                            aFlag = true;
                            break;
                        default:
                            cerr<<"Unknown output option: <"<<optarg[i]<<">"<<endl;
                            return 1;
                    }
                }
                break;
            case '?':
                if (optopt == 'o')
                    cerr<<"Unknown output option: <.>"<<endl;
                else if (isprint (optopt)){
                    cerr << argv[0] << ": invalid option -- '" << (char) optopt << "'" << endl;
                    cerr << "illegal option" << endl;
                }
                else{
                    cerr << argv[0] << ": invalid option -- '" << std::hex << optopt << "'" << endl;
                    cerr << "Usage: " << argv[0] << " [-v] inputfile randomfile" << endl;
                }
                return 1;
            default:
                abort();
        }
    }

    char* inputfile = optind < argc ? argv[optind++] : 0;
    f_stream.open(inputfile);
    if(!f_stream){
        if(inputfile)
            cerr<<"Cannot open inputfile <" << argv[optind-1] << ">" <<endl;
        else
            cerr<<"inputfile name not supplied"<<endl;
        exit(1);
    }

    //Read process and vma input
    int num_processes=0;
    int num_vma=0;
    vma* vma_spec;
    string line;
    int inst_line;
    while(getline(f_stream, line) && line[0] == '#'){}
    num_processes = stoi(line);
    if(num_processes > MAX_PROCESSES){
        cerr<<"sorry maximum number of process supported is "<<MAX_PROCESSES<<endl;
        return 1;
    }
    for(int i = 0; i < num_processes; i++){
        processes.push_back(new Process(i));
        while(getline(f_stream, line) && line[0] == '#'){}
        num_vma = stoi(line);
        for(int j = 0; j < num_vma; j++){
            while(getline(f_stream, line) && line[0] == '#'){}
            stringstream ssin(line);
            vma_spec = new vma();
            ssin >> vma_spec->start_vpage >> vma_spec->end_vpage >> vma_spec->write_protected >> vma_spec->file_mapped;
            processes[i]->vma_specs.push_back(vma_spec);
        }
    }
    inst_line = f_stream.tellg();
    while(getline(f_stream, line) && line[0] == '#')
        inst_line = f_stream.tellg();
    //Go back to the first non comment line read.
    if(f_stream) f_stream.seekg(inst_line ,std::ios_base::beg);

    MMU* mmu = new MMU(num_frames, algo, optind < argc ? argv[optind] : 0, OFlag, aFlag);
    simulate(mmu, OFlag, xFlag, yFlag, fFlag);
    print_summary(mmu, PFlag, FFlag, SFlag);
    f_stream.close();
    return 0;
}