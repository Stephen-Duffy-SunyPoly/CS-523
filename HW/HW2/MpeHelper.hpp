//Author: Stephen Duffy duffysd
//General assignment utility
//CS523 4:00pm tr
#pragma once
#include <mpe.h>

inline void initMPE() {
    MPE_Init_log();//initialize the logging for jumpshot
}

inline void startMPE() {
    MPI_Pcontrol( 1 );//set the profiling control level to 1
    MPE_Start_log();
}

//to end logging call MPE_Finish_log( argv[0] );

//jump shot color list
enum JSColor {
    RED,
    ORANGE,
    BLUE,
    GREEN,
    WHITE,
    PURPLE,
    NAVY,
    YELLOW
};

//convert the color enum to the string representation
inline const char * jS_Color_to_str(const JSColor color) {
    switch (color) {
        case RED:
            return "red";
        case ORANGE:
            return "orange";
        case BLUE:
            return "blue";
        case GREEN:
            return "green";
        case WHITE:
            return "white";
        case PURPLE:
            return "purple";
        case NAVY:
            return "navy";
        case YELLOW:
            return "yellow";
    }
    return "";
}

class MpeTimedEvent {
    int eventStart=0;
    int eventEnd=0;
public:
    MpeTimedEvent(const int processorId, const std::string& name, const JSColor color) {
        MPE_Log_get_state_eventIDs(&eventStart, &eventEnd);
        if (processorId==0) {
            MPE_Describe_state( eventStart, eventEnd, name.c_str(), jS_Color_to_str(color) );
        }
    }

    void start() const {
        MPE_Log_event( eventStart, 0, nullptr);
    }

    void end() const {
        MPE_Log_event( eventEnd, 0, nullptr);
    }

    static void sync() {
        MPE_Log_sync_clocks();
    }
};

class MpeInstantEvent {
    int event=0;
    public:
    MpeInstantEvent(const int processorId, const std::string& name, const JSColor color) {
        MPE_Log_get_solo_eventID(&event);
        if (processorId==0) {
            MPE_Describe_event(event, name.c_str(), jS_Color_to_str(color));
        }
    }

    void mark() const {
        MPE_Log_event( event, 0, nullptr);
    }

    static void sync() {
        MPE_Log_sync_clocks();
    }
};