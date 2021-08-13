#include <iostream>
#include <fstream>

#include <pager.h>

#define INSTR_CYCLE 50
#define TAU 49

FIFO_Pager::FIFO_Pager(int max_frames, bool opFlag){
    hand = 0;
    num_frames = max_frames;
    aFlag = opFlag;
}

frame_t* FIFO_Pager::select_victim_frame(){
    if(aFlag) cout<<"ASELECT "<<hand<<endl;
    frame_t* frame = &frame_table[hand++];
    hand = (hand == num_frames ? 0 : hand);
    return frame;
}

int Random_Pager::getRandom(int seed) {
    int rand = randvals[randOffset++] % seed;
    //Wrapping around
    if(randOffset == randvals.size()) randOffset=0;
    return rand;
}

Random_Pager::Random_Pager(int max_frames, char* randfile, bool opFlag){
    int number;
    ifstream rand_stream;
    rand_stream.open(randfile);

    if(!rand_stream){
        if(randfile)
            cerr<<"Not a valid random file <" << randfile << ">" <<endl;
        else
            cerr<<"randfile name not supplied"<<endl;
        exit(1);
    }

    rand_stream >> number;
    while (rand_stream >> number) randvals.push_back(number);
    rand_stream.close();

    randOffset = 0;
    num_frames = max_frames;
    aFlag = opFlag;
}

frame_t* Random_Pager::select_victim_frame(){
    return &frame_table[getRandom(num_frames)];
}

Clock_Pager::Clock_Pager(int max_frames, bool opFlag){
    hand = 0;
    num_frames = max_frames;
    aFlag = opFlag;
}

frame_t* Clock_Pager::select_victim_frame(){
    int inspections=0;
    if(aFlag) cout<<"ASELECT "<<hand<<" ";
    frame_t* frame = &frame_table[hand++];
    while(frame->vpage->referenced){
        inspections++;
        frame->vpage->referenced = 0;
        hand = (hand == num_frames ? 0 : hand);
        frame = &frame_table[hand++];
    }
    hand = (hand == num_frames ? 0 : hand);
    inspections++;
    if(aFlag) cout<<inspections<<endl;
    return frame;
}

NRU_Pager::NRU_Pager(int max_frames, bool opFlag){
    hand = 0;
    num_frames = max_frames;
    last_reset=0;
    aFlag = opFlag;
}

frame_t* NRU_Pager::select_victim_frame(){
    if(aFlag) cout<<"ASELECT: hand= "<<hand<<" "<<(inst_count-last_reset >= INSTR_CYCLE)<<" | ";
    int inspections=0;
    int nruclass;
    int lowesthand;
    frame_t* frame = nullptr;
    int lowestclass = 4;
    for(int i=0; i < num_frames; i++){
        inspections++;
        frame = &frame_table[hand];
        nruclass = 2*frame->vpage->referenced + frame->vpage->modified;
        if(nruclass < lowestclass){
            lowestclass = nruclass;
            lowesthand = hand;
        }
        hand = (hand+1 == num_frames ? 0 : hand+1);
        if(inst_count-last_reset >= INSTR_CYCLE) {
            frame->vpage->referenced = 0;
        }
        else{
            if(lowestclass == 0)
                break;
        }
    }
    last_reset = inst_count-last_reset >= INSTR_CYCLE ? inst_count : last_reset;
    hand = (lowesthand+1 == num_frames ? 0 : lowesthand+1);
    if(aFlag) cout<<lowestclass<<"  "<<frame_table[lowesthand].frame_no<<"  "<<inspections<<endl;
    return &frame_table[lowesthand];
}

Aging_Pager::Aging_Pager(int max_frames, bool opFlag){
    hand = 0;
    num_frames = max_frames;
    aFlag = opFlag;
}

frame_t* Aging_Pager::select_victim_frame(){
    if(aFlag) cout<<"ASELECT "<<hand<<"-"<<(hand-1 < 0 ? num_frames-1 : hand-1)<<" | ";
    int lowesthand = hand;
    unsigned long lowest_age = 0xffffffff;
    frame_t* frame = nullptr;
    for(int i=0 ; i < num_frames; i++){
        frame = &frame_table[hand];
        frame->age_vector = frame->age_vector >> 1;
        if(frame->vpage->referenced){
            frame->age_vector = (frame->age_vector | 0x80000000);
            frame->vpage->referenced = 0;
        }
        if(aFlag) cout<<hand<<":"<<std::hex<<frame->age_vector<<std::dec<<" ";
        if(frame->age_vector < lowest_age){
            lowest_age = frame->age_vector;
            lowesthand = hand;
        }
        hand = (hand+1 == num_frames ? 0 : hand+1);
    }
    hand = (lowesthand + 1 == num_frames ? 0 : lowesthand + 1);
    if(aFlag) cout<<"| "<<lowesthand<<endl;
    return &frame_table[lowesthand];
}

WorkingSet_Pager::WorkingSet_Pager(int max_frames, bool opFlag){
    hand = 0;
    num_frames = max_frames;
    aFlag = opFlag;
}

frame_t* WorkingSet_Pager::select_victim_frame(){
    if(aFlag) cout<<"ASELECT "<<hand<<"-"<<(hand-1 < 0 ? num_frames-1 : hand-1)<<" | ";
    int lowesthand = hand;
    unsigned long lowest_time = 0x11111111;
    frame_t* frame = nullptr;
    int inspections=0;
    for(int i=0; i < num_frames; i++){
        inspections++;
        frame = &frame_table[hand];
        if(aFlag) cout<<hand<<"("<<frame->vpage->referenced<<" "<<frame->procId<<":"<<frame->vpage->page_no<<" "<<frame->time_last_use<<") ";
        if(frame->vpage->referenced){
            frame->vpage->referenced = 0;
            frame->time_last_use = inst_count-1; //inst_cnt represents the number of instructions processes. We need the time.
        }
        else{
            if((inst_count-1)-frame->time_last_use > TAU){
                lowesthand = hand;
                if(aFlag) cout<<"STOP("<<inspections<<") ";
                break;
            }
            else{
                if(frame->time_last_use < lowest_time){
                    lowest_time = frame->time_last_use;
                    lowesthand = hand;
                }
            }
        }
        hand = (hand+1 == num_frames ? 0 : hand+1);
    }
    hand = (lowesthand + 1 == num_frames ? 0 : lowesthand + 1);
    if(aFlag) cout<<"| "<<lowesthand<<endl;
    return &frame_table[lowesthand];
}