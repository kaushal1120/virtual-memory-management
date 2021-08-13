using namespace std;
#include <vector>

#include <frame_t.h>

#define MAX_FRAMES 128

extern frame_t frame_table[MAX_FRAMES];
extern unsigned long inst_count;

class Pager {
public:
    bool aFlag;
    int hand;
    int num_frames;
    virtual frame_t* select_victim_frame() = 0; // virtual base class
};

class FIFO_Pager : public Pager {
public:
    FIFO_Pager(int max_frames, bool opFlag);
    frame_t* select_victim_frame();
};

class Random_Pager : public Pager {
private:
    vector<int> randvals;
    int randOffset;
    int getRandom(int seed);
public:
    Random_Pager(int max_frames, char* rand_stream, bool opFlag);
    frame_t* select_victim_frame();
};

class Clock_Pager : public Pager {
public:
    Clock_Pager(int max_frames, bool opFlag);
    frame_t* select_victim_frame();
};

class NRU_Pager : public Pager {
private:
    int last_reset;
public:
    NRU_Pager(int max_frames, bool opFlag);
    frame_t* select_victim_frame();
};

class Aging_Pager : public Pager {
public:
    Aging_Pager(int max_frames, bool opFlag);
    frame_t* select_victim_frame();
};

class WorkingSet_Pager : public Pager {
public:
    WorkingSet_Pager(int max_frames, bool opFlag);
    frame_t* select_victim_frame();
};