//Author: Stephen Duffy duffysd
//HW5_MPI_Tree_Broadcast
//CS523 4:00pm tr
#include <cmath>
#include <iostream>
#include <mpi.h>
#include "../HW3/mpi_infra.hpp"
#include "Timer.hpp"
#include "../HW2/MpeHelper.hpp"
#include <mpe.h>

using namespace std;

MpeTimedEvent *receiveEvent, *sendEvent, *computeEvent1, *computeEvent2;

int treeStructuredBroadcast(int in, const ProcessInfo &processInfo, bool useMPE) {
    if (useMPE) {
        computeEvent1->start();
    }
    int numberOfLayers = ceil(log2(processInfo.getNumProcesses()));
    int layer = 1;
    bool recieved = false;
    int disVal = 0;
    //create the sending buffer
    int mpiBufferSize =MPI_BSEND_OVERHEAD+sizeof(int)*2;
    char * mpiAttachedBuffer = static_cast<char *>(malloc(mpiBufferSize));
    int receiveBuffer[2]{};
    MPI_Buffer_attach(mpiAttachedBuffer, mpiBufferSize);

    if (processInfo.isMain()) {
        recieved = true;
        disVal = in;
    }
    //the sending part
    if (useMPE) {
        computeEvent1->end();
    }
    while (layer!=numberOfLayers+1) {
        if (recieved) {//if you have already received then you are to distribute the values
            if (useMPE) {
                computeEvent2->start();
            }
            int stride = static_cast<int>(pow(2, numberOfLayers)/(pow(2,layer)));//how much offset from this process the target process is
            int tosend[]{layer,disVal};//prepare the send buffer
            int destRank = processInfo.getRank()+stride;//compute the id of the destination rank
            //only send if the destination process actually exists
            if (useMPE) {
                computeEvent2->end();
            }
            if (destRank<processInfo.getNumProcesses()) {//this allows for any ammount of processes to be used and this system will still work
                if (useMPE) {
                    sendEvent->start();
                }
                MPI_Bsend(tosend,2,MPI_INT,destRank,0,MPI_COMM_WORLD);
                if (useMPE) {
                    sendEvent->end();
                }
            }
        } else {
            //listen for a value from any process,
            if (useMPE) {
                receiveEvent->start();
            }
            MPI_Recv(receiveBuffer,2,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            recieved = true;
            //extract the values
            disVal = receiveBuffer[1];
            layer = receiveBuffer[0];//this is so I do not have to calculate where this data game from and can then focus on where it is going to next
            if (useMPE) {
                receiveEvent->end();
            }
        }
        layer++;
    }
    if (useMPE) {
        computeEvent1->start();
    }
    MPI_Buffer_detach(mpiAttachedBuffer, &mpiBufferSize);

    free(mpiAttachedBuffer);
    if (useMPE) {
        computeEvent1->end();
    }
    return disVal;
}

int sequentailSend(int value, const ProcessInfo &processInfo) {
    int vout = value;
    if (processInfo.isMain()) {
        //MPI buffer things
        int mpiBufferSize =MPI_BSEND_OVERHEAD+sizeof(int);
        char * mpiAttachedBuffer = static_cast<char *>(malloc(mpiBufferSize));
        MPI_Buffer_attach(mpiAttachedBuffer, mpiBufferSize);
        //sequentially send to all processes
        for (int i=1;i<processInfo.getNumProcesses();i++) {
            MPI_Bsend(&value,1,MPI_INT,i,0,MPI_COMM_WORLD);
        }
        //more buffer things;
        MPI_Buffer_detach(mpiAttachedBuffer, &mpiBufferSize);
        free(mpiAttachedBuffer);
    } else {
        MPI_Recv(&vout,1,MPI_INT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }
    return vout;
}

int main (int argc, char* argv[]) {

    MPI_Init(&argc, &argv);
    initMPE();

    const ProcessInfo processInfo;//get the general process info
    Timer bcastTimer;
    Timer tsbTimer;
    Timer sequentialTimer;

    int value=0;
    if (processInfo.isMain()) {
        cout << "Enter value to distribute: ";
        cin >> value;
    }

    receiveEvent = new MpeTimedEvent(processInfo.getRank(),"Receive",RED);
    sendEvent = new MpeTimedEvent(processInfo.getRank(),"Send",GREEN);
    computeEvent1 = new MpeTimedEvent(processInfo.getRank(),"Compute",BLUE);
    computeEvent2 = new MpeTimedEvent(processInfo.getRank(),"Pre Send Compute",PURPLE);
    MpeTimedEvent preSyncEvent = MpeTimedEvent(processInfo.getRank(),"Sync",ORANGE);
    MpeTimedEvent PostSyncEvent = MpeTimedEvent(processInfo.getRank(),"Post Sync",YELLOW);


    //run method test with the timer
    MPI_Barrier(MPI_COMM_WORLD);//WOW, this is the leased efficient sync system I have ever seen. it just spin waits under the hood taking up lots of CPU
    startMPE();

    //warm the process up so the first measure one does not take riduclessly long
    treeStructuredBroadcast(value+48356,processInfo,false);
    treeStructuredBroadcast(value-4876532,processInfo,false);

    MPI_Barrier(MPI_COMM_WORLD);
    tsbTimer.startTimer();

    treeStructuredBroadcast(value,processInfo,false);

    MPI_Barrier(MPI_COMM_WORLD);
    tsbTimer.endTimer();
    //test the bcast send method
    bcastTimer.startTimer();
    int bcastOutput=value;
    MPI_Bcast(&bcastOutput,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    bcastTimer.endTimer();
    sequentialTimer.startTimer();

    sequentailSend(value,processInfo);

    MPI_Barrier(MPI_COMM_WORLD);
    sequentialTimer.endTimer();

    //testing with MPI wtime
    double bcastStartTime=MPI_Wtime();
    MPI_Bcast(&bcastOutput,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    double bcastEndTime=MPI_Wtime();//also TSB start time
    treeStructuredBroadcast(value,processInfo,false);
    MPI_Barrier(MPI_COMM_WORLD);
    double tsbEndTime=MPI_Wtime();//also sequential start
    sequentailSend(value,processInfo);
    preSyncEvent.start();
    MPI_Barrier(MPI_COMM_WORLD);
    double sequentialEndTime=MPI_Wtime();

    preSyncEvent.end();
    treeStructuredBroadcast(value,processInfo,true);

    PostSyncEvent.start();
    MPI_Barrier(MPI_COMM_WORLD);
    PostSyncEvent.end();


    // ReSharper disable once CppDFAConstantConditions
    if (processInfo.isMain()) {
        // ReSharper disable once CppDFAUnreachableCode
        cout << "get Time of day timers:"<<endl;
        cout << "Bcast timer:\t\t";
        bcastTimer.printTimer();
        cout << "TSB timer:\t\t";
        tsbTimer.printTimer();
        cout << "Sequential timer:\t";
        sequentialTimer.printTimer();

        cout << "W time timers:"<< endl;
        cout << "bcast:\t\t" << (bcastEndTime-bcastStartTime) << "s" << endl;
        cout << "TSB:\t\t"<< (tsbEndTime-bcastEndTime) << "s" << endl;
        cout << "sequentail:\t"<< (sequentialEndTime-tsbEndTime) << "s" << endl;
    }

    MpeTimedEvent::sync();
    MPE_Finish_log( argv[0] );

    MPI_Finalize();

    delete receiveEvent;
    delete sendEvent;
    delete computeEvent1;
    delete computeEvent2;

    return EXIT_SUCCESS;
}