//Author: Stephen Duffy duffysd
//HW5_MPI_Tree_Broadcast
//CS523 4:00pm tr
#include <cmath>
#include <iostream>
#include <mpi.h>
#include "../HW3/mpi_infra.hpp"


using namespace std;

int treeStructuredBroadcast(int in, const ProcessInfo &processInfo) {
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
    while (layer!=numberOfLayers+1) {
        if (recieved) {//if you have already received then you are to distribute the values
            int stride = static_cast<int>(pow(2, numberOfLayers)/(pow(2,layer)));//how much offset from this process the target process is
            int tosend[]{layer,disVal};//prepare the send buffer
            int destRank = processInfo.getRank()+stride;//compute the id of the destination rank
            //only send if the destination process actually exists
            if (destRank<processInfo.getNumProcesses()) {//this allows for any ammount of processes to be used and this system will still work
                MPI_Bsend(tosend,2,MPI_INT,destRank,0,MPI_COMM_WORLD);
            }
        } else {
            //listen for a value from any process,
            MPI_Recv(receiveBuffer,2,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            recieved = true;
            //extract the values
            disVal = receiveBuffer[1];
            layer = receiveBuffer[0];//this is so I do not have to calculate where this data game from and can then focus on where it is going to next
        }
        layer++;
    }
    MPI_Buffer_detach(mpiAttachedBuffer, &mpiBufferSize);

    free(mpiAttachedBuffer);
    return disVal;
}

int main (int argc, char* argv[]) {

    MPI_Init(&argc, &argv);

    const ProcessInfo processInfo;//get the general process info

    int value=0;
    if (processInfo.isMain()) {
        cout << "Enter value to distribute: ";
        cin >> value;
    }
    MPI_Barrier(MPI_COMM_WORLD);

    value = treeStructuredBroadcast(value,processInfo);
    cout << processInfo.getRank() << " | " << value << endl;

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return EXIT_SUCCESS;
}